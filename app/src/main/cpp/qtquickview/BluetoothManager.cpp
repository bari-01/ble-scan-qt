#include "BluetoothManager.h"

BluetoothManager::BluetoothManager(QObject *parent)
        : QObject(parent)
{
    m_discoveryAgent =
            new QBluetoothDeviceDiscoveryAgent(this);

    connect(
            m_discoveryAgent,
            &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this,
            [this](const QBluetoothDeviceInfo &device)
            {
                if (!(device.coreConfigurations() &
                      QBluetoothDeviceInfo::LowEnergyCoreConfiguration))
                {
                    return;
                }

                QString address = device.address().toString();

                if (m_seenAddresses.contains(address))
                    return;

                m_seenAddresses.insert(address);

                m_deviceMap.insert(address, device);
                QString name = device.name();

                if (name.isEmpty())
                    name = "Unknown";

                emit deviceFound(name, address);
            });

    connect(
            m_discoveryAgent,
            &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this,
            [this](QBluetoothDeviceDiscoveryAgent::Error error)
            {
                emit logMessage(
                        QString("Scan error: %1")
                                .arg(error)
                );
            });

    connect(
            m_discoveryAgent,
            &QBluetoothDeviceDiscoveryAgent::finished,
            this,
            [this]()
            {
                emit logMessage("Scan finished");
            });
}

void BluetoothManager::startScan()
{
    m_seenAddresses.clear();
    m_devices.clear();

    emit logMessage("Starting BLE scan...");

    m_discoveryAgent->start(
            QBluetoothDeviceDiscoveryAgent::LowEnergyMethod
    );
}

void BluetoothManager::connectToDevice(const QString &address)
{
    if (!m_deviceMap.contains(address))
        return;

    auto device = m_deviceMap[address];

    emit logMessage("Connecting...");

    auto *controller =
            QLowEnergyController::createCentral(device, this);

    connect(controller, &QLowEnergyController::connected, this, [=]() {
        emit logMessage("Connected. Discovering services...");
        controller->discoverServices();
    });

    connect(controller, &QLowEnergyController::serviceDiscovered,
            this, [=](const QBluetoothUuid &uuid) {
                emit logMessage("Service: " + uuid.toString());
            });

    connect(controller, &QLowEnergyController::disconnected, this, [=]() {
        emit logMessage("Disconnected");
    });

    controller->connectToDevice();
}