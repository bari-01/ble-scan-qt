#include "TransportManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDataStream>
#include <QHostAddress>
#include <QCoreApplication>

TransportManager::TransportManager(QObject *parent) : QObject(parent) {}

// ── Server (HOST) ─────────────────────────────────────────────────────────────

void TransportManager::startServer(quint16 port) {
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection,
            this, &TransportManager::onNewConnection);

    if (!m_server->listen(QHostAddress::Any, port)) {
        emit logMessage("TCP listen failed: " + m_server->errorString());
        return;
    }
    emit logMessage("TCP server listening on :" + QString::number(port));
}

void TransportManager::onNewConnection() {
    if (m_socket) {
        // Only one peer for now — reject extras
        m_server->nextPendingConnection()->disconnectFromHost();
        return;
    }
    m_socket = m_server->nextPendingConnection();
    connect(m_socket, &QTcpSocket::readyRead,
            this, &TransportManager::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &TransportManager::onSocketDisconnected);

    emit logMessage("Peer connected from "
                    + m_socket->peerAddress().toString());
    emit connected();
}

// ── Client ────────────────────────────────────────────────────────────────────

void TransportManager::connectToHost(const QString &ip, quint16 port) {
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, [=]() {
        emit logMessage("TCP connected to " + ip);
        emit connected();
    });
    connect(m_socket, &QTcpSocket::readyRead,
            this, &TransportManager::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &TransportManager::onSocketDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this, [=]() {
        emit logMessage("TCP error: " + m_socket->errorString());
    });

    emit logMessage("Connecting TCP to " + ip + ":" + QString::number(port));
    m_socket->connectToHost(ip, port);
}

// ── Send ──────────────────────────────────────────────────────────────────────

void TransportManager::writeFrame(quint8 type, const QByteArray &payload) {
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState)
        return;

    QByteArray frame;
    frame.resize(5);
    qint32 len = payload.size();
    frame[0] = (len >> 24) & 0xFF;
    frame[1] = (len >> 16) & 0xFF;
    frame[2] = (len >>  8) & 0xFF;
    frame[3] = (len      ) & 0xFF;
    frame[4] = type;
    frame.append(payload);
    m_socket->write(frame);
}

void TransportManager::sendText(const QString &text) {
    writeFrame(TYPE_TEXT, text.toUtf8());
}

void TransportManager::sendVideoFrame(const QByteArray &frameData) {
    writeFrame(TYPE_VIDEO, frameData);
}

void TransportManager::sendFile(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit logMessage("Cannot open file: " + filePath);
        return;
    }

    QFileInfo info(filePath);
    m_outgoingFileSize = info.size();
    m_outgoingSent     = 0;

    // Send header
    QJsonObject hdr;
    hdr["name"] = info.fileName();
    hdr["size"] = info.size();
    hdr["mime"] = "application/octet-stream"; // extend later
    writeFrame(TYPE_FILE_HDR, QJsonDocument(hdr).toJson(QJsonDocument::Compact));

    // Send chunks
    constexpr int CHUNK = 64 * 1024; // 64KB chunks
    while (!file.atEnd()) {
        QByteArray chunk = file.read(CHUNK);
        writeFrame(TYPE_FILE_CHUNK, chunk);
        m_outgoingSent += chunk.size();
        emit transferProgress(m_outgoingSent, m_outgoingFileSize);
        // Let event loop breathe for large files
        QCoreApplication::processEvents();
    }

    writeFrame(TYPE_FILE_END, QByteArray());
    emit logMessage("File sent: " + info.fileName());
}

// ── Receive ───────────────────────────────────────────────────────────────────

void TransportManager::onReadyRead() {
    m_buffer.append(m_socket->readAll());
    processBuffer();
}

void TransportManager::processBuffer() {
    while (true) {
        // Need at least 5 bytes for header (4 len + 1 type)
        if (m_buffer.size() < 5) return;

        qint32 payloadLen =
                ((quint8)m_buffer[0] << 24) |
                ((quint8)m_buffer[1] << 16) |
                ((quint8)m_buffer[2] <<  8) |
                ((quint8)m_buffer[3]);

        // Sanity check — reject absurd sizes (> 50MB per frame)
        if (payloadLen < 0 || payloadLen > 50 * 1024 * 1024) {
            emit logMessage("Bad frame length, resetting buffer");
            m_buffer.clear();
            return;
        }

        if (m_buffer.size() < 5 + payloadLen) return; // wait for more data

        quint8     type    = (quint8)m_buffer[4];
        QByteArray payload = m_buffer.mid(5, payloadLen);
        m_buffer.remove(0, 5 + payloadLen);

        handleIncoming(payload, type);
    }
}

void TransportManager::handleIncoming(const QByteArray &payload, quint8 type) {
    switch (type) {

        case TYPE_TEXT:
            emit textReceived(QString::fromUtf8(payload));
            break;

        case TYPE_FILE_HDR: {
            QJsonObject hdr = QJsonDocument::fromJson(payload).object();
            m_incomingFileName     = hdr["name"].toString();
            m_incomingFileSize     = hdr["size"].toVariant().toLongLong();
            m_incomingFileReceived = 0;

            // Open output file in Downloads
            QString dir  = QStandardPaths::writableLocation(
                    QStandardPaths::DownloadLocation);
            QString path = dir + "/" + m_incomingFileName;
            if (m_incomingFile) { m_incomingFile->close(); delete m_incomingFile; }
            m_incomingFile = new QFile(path, this);
            m_incomingFile->open(QIODevice::WriteOnly);

            emit fileStarted(m_incomingFileName, m_incomingFileSize, "");
            emit logMessage("Receiving: " + m_incomingFileName
                            + " (" + QString::number(m_incomingFileSize) + " bytes)");
            break;
        }

        case TYPE_FILE_CHUNK:
            if (m_incomingFile && m_incomingFile->isOpen()) {
                m_incomingFile->write(payload);
                m_incomingFileReceived += payload.size();
                emit transferProgress(m_incomingFileReceived, m_incomingFileSize);
                emit fileChunkReceived(payload);
            }
            break;

        case TYPE_FILE_END:
            if (m_incomingFile) {
                QString path = m_incomingFile->fileName();
                m_incomingFile->close();
                delete m_incomingFile;
                m_incomingFile = nullptr;
                emit fileCompleted(path);
                emit logMessage("File saved: " + path);
            }
            break;

        case TYPE_VIDEO:
            emit videoFrameReceived(payload);
            break;

        default:
            emit logMessage("Unknown frame type: " + QString::number(type));
            break;
    }
}

void TransportManager::onSocketDisconnected() {
    emit logMessage("TCP peer disconnected");
    emit disconnected();
    m_socket->deleteLater();
    m_socket = nullptr;
    m_buffer.clear();
}

bool TransportManager::isConnected() const {
    return m_socket &&
           m_socket->state() == QAbstractSocket::ConnectedState;
}
