#pragma once

#include <QObject>
#include <QSet>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>

#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QLowEnergyCharacteristic>

class BluetoothManager : public QObject
{
    Q_OBJECT

public:
    explicit BluetoothManager(QObject *parent = nullptr);
    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent;
    QSet<QString> m_seenAddresses;
    QMap<QString, QBluetoothDeviceInfo> m_deviceMap;
    QList<QBluetoothDeviceInfo> m_devices;

    Q_INVOKABLE void startScan();
    Q_INVOKABLE void connectToDevice(const QString &address);

signals:
    void deviceFound(QString name, QString address);
    void logMessage(QString message);

private:

};