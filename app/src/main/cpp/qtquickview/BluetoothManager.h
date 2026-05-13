#pragma once

#include "TransportManager.h"
#include <QObject>
#include <QSet>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QRandomGenerator>

#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QLowEnergyCharacteristic>
#include <QLowEnergyCharacteristicData>
#include <QLowEnergyServiceData>
#include <QLowEnergyDescriptorData>
#include <QLowEnergyAdvertisingParameters>

class BluetoothManager : public QObject
{
    Q_OBJECT

    public:
        explicit BluetoothManager(QObject *parent = nullptr);
        QBluetoothDeviceDiscoveryAgent *m_discoveryAgent;
        QSet<QString> m_seenAddresses;
        QMap<QString, QBluetoothDeviceInfo> m_deviceMap;
        QList<QBluetoothDeviceInfo> m_devices;
        quint32 m_localNonce;
        bool m_isHost = false;
        TransportManager *m_transport = nullptr;

        QLowEnergyAdvertisingData m_advData;
        QLowEnergyController *m_peripheralController = nullptr;
        QLowEnergyService *m_negotiationService = nullptr;
        QLowEnergyService *m_remoteSvc = nullptr;

        static const QBluetoothUuid SERVICE_UUID;
        static const QBluetoothUuid NONCE_CHAR_UUID;   // for host election
        static const QBluetoothUuid HOTSPOT_CHAR_UUID; // for SSID/PSK exchange

        Q_INVOKABLE void startScan();

        Q_INVOKABLE void onDeviceDiscovered(const QBluetoothDeviceInfo &device);
        Q_INVOKABLE void connectToDevice(const QString &address);
        //Q_INVOKABLE void onDeviceConnected(QLowEnergyController *controller);

        Q_INVOKABLE void startPeripheral();

        Q_INVOKABLE void onCharacteristicWritten(const QLowEnergyCharacteristic &ch, const QByteArray &val0);

        Q_INVOKABLE void sendText(const QString &text) {
            if (m_transport) m_transport->sendText(text);
            else emit logMessage("sendText: no transport yet");
        }

        Q_INVOKABLE void sendFile(const QString &path) {
            if (m_transport) m_transport->sendFile(path);
            else emit logMessage("sendFile: no transport yet");
        }

        void electHost(uint32_t remoteNonce, QLowEnergyService *remoteSvc);

    signals:
        void deviceFound(QString name, QString address);
        void logMessage(QString message);
        void readyToConnect(QString ip, quint16 port); //
 
        void textReceived(QString text);
        void fileCompleted(QString path);
        void transferProgress(qint64 sent, qint64 total);
        void tcpConnected();
        //private:

};
