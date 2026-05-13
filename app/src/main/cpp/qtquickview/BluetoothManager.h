#pragma once

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

        void electHost(uint32_t remoteNonce, QLowEnergyService *remoteSvc);

signals:
        void deviceFound(QString name, QString address);
        void logMessage(QString message);
        void readyToConnect(QString ip, quint16 port); //
        //private:

};
