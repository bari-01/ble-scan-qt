#pragma once
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>

class TransportManager : public QObject {
    Q_OBJECT
public:
    explicit TransportManager(QObject *parent = nullptr);

    Q_INVOKABLE void startServer(quint16 port = 45678);
    Q_INVOKABLE void connectToHost(const QString &ip, quint16 port = 45678);

    Q_INVOKABLE void sendText(const QString &text);
    Q_INVOKABLE void sendFile(const QString &filePath);
    Q_INVOKABLE void sendVideoFrame(const QByteArray &frameData);

    Q_INVOKABLE bool isConnected() const;

    signals:
            void connected();
    void disconnected();
    void textReceived(QString text);
    void fileStarted(QString name, qint64 size, QString mime);
    void fileChunkReceived(QByteArray chunk);
    void fileCompleted(QString savedPath);
    void videoFrameReceived(QByteArray frameData);
    void logMessage(QString msg);
    void transferProgress(qint64 sent, qint64 total);

private slots:
            void onNewConnection();
    void onReadyRead();
    void onSocketDisconnected();

private:
    void handleIncoming(const QByteArray &payload, quint8 type);
    void writeFrame(quint8 type, const QByteArray &payload);
    void processBuffer();

    QTcpServer  *m_server  = nullptr;
    QTcpSocket  *m_socket  = nullptr;

    // Receive state
    QByteArray   m_buffer;
    qint32       m_expectedLen = -1;

    // File receive state
    QString      m_incomingFileName;
    qint64       m_incomingFileSize = 0;
    qint64       m_incomingFileReceived = 0;
    QFile       *m_incomingFile = nullptr;

    // File send state
    qint64       m_outgoingFileSize = 0;
    qint64       m_outgoingSent = 0;

    static constexpr quint8 TYPE_TEXT       = 0x01;
    static constexpr quint8 TYPE_FILE_HDR   = 0x02;
    static constexpr quint8 TYPE_FILE_CHUNK = 0x03;
    static constexpr quint8 TYPE_FILE_END   = 0x04;
    static constexpr quint8 TYPE_VIDEO      = 0x05;
};