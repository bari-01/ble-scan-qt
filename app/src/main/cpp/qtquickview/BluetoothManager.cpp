#include "BluetoothManager.h"
#include <QTimer>

const QBluetoothUuid BluetoothManager::SERVICE_UUID =
QBluetoothUuid(QString("12345678-1234-1234-1234-123456789abc"));
const QBluetoothUuid BluetoothManager::NONCE_CHAR_UUID =
QBluetoothUuid(QString("12345678-1234-1234-1234-123456789ab1"));
const QBluetoothUuid BluetoothManager::HOTSPOT_CHAR_UUID =
QBluetoothUuid(QString("12345678-1234-1234-1234-123456789ab2"));

BluetoothManager::BluetoothManager(QObject *parent) : QObject(parent)
{
    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);

    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &BluetoothManager::onDeviceDiscovered);

    connect(m_discoveryAgent,&QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this, [=](QBluetoothDeviceDiscoveryAgent::Error error)
            { emit logMessage(QString("Scan error: %1").arg(error)); });

    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, [=]() { emit logMessage("Scan finished"); });
}

void BluetoothManager::onDeviceDiscovered(const QBluetoothDeviceInfo &device)
{
    if (!(device.coreConfigurations() &
                QBluetoothDeviceInfo::LowEnergyCoreConfiguration))
        return;

    if (!device.serviceUuids().contains(BluetoothManager::SERVICE_UUID))
        return;

    QString address = device.address().toString();

    if (m_seenAddresses.contains(address)) return;

    m_seenAddresses.insert(address);
    m_deviceMap.insert(address, device);

    QString name = device.name();
    if (name.isEmpty()) name = "Unknown";

    emit deviceFound(name, address);
}

void BluetoothManager::startScan()
{
    m_seenAddresses.clear();
    m_devices.clear();

    emit logMessage("Starting BLE scan...");

    m_discoveryAgent->start( QBluetoothDeviceDiscoveryAgent::LowEnergyMethod );
}

void BluetoothManager::connectToDevice(const QString &address)
{
    if (!m_deviceMap.contains(address)) return;
    QBluetoothDeviceInfo device = m_deviceMap[address];

    emit logMessage("Connecting...");
    m_centralController = QLowEnergyController::createCentral(device, this);

    connect(m_centralController, &QLowEnergyController::connected, this,
            [=]() {
            emit logMessage("Connected. Discovering services...");
            m_centralController->discoverServices();
            });

    //connect(controller, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error), this,
    //        [this, controller](QLowEnergyController::Error error)
    //        {
    //            emit logMessage("Controller Error: " + QString::number(error));
    //            controller->deleteLater();
    //        }
    //);

    connect(m_centralController, &QLowEnergyController::disconnected, this,
            [=]() { emit logMessage("Disconnected"); });

    connect(m_centralController, &QLowEnergyController::serviceDiscovered, this,
            [=](const QBluetoothUuid &uuid) {
            emit logMessage("Service: " + uuid.toString());
            if (uuid != SERVICE_UUID) return;
            auto *svc = m_centralController->createServiceObject(uuid, this);
            svc->discoverDetails();

            connect(svc, &QLowEnergyService::stateChanged, this,
                    [=](QLowEnergyService::ServiceState state) {
                    if (state != QLowEnergyService::RemoteServiceDiscovered) return;

                    connect(svc, &QLowEnergyService::characteristicChanged, this,
                            [=](const QLowEnergyCharacteristic &ch, const QByteArray &val) {
                                if (ch.uuid() != HOTSPOT_CHAR_UUID) return;
                                if (val.isEmpty()) return;

                                QStringList parts = QString::fromUtf8(val).split("::");
                                if (parts[0] != "READY" || parts.size() < 2) return;

                                quint16 port = parts[1].toUInt();
                                emit logMessage("Host ready (notification)");

                                emit p2pNegotiationComplete(m_isHost, port);
                            });

                    connect(svc, &QLowEnergyService::characteristicRead, this,
                            [=](const QLowEnergyCharacteristic &ch, const QByteArray &val) {
                                if (ch.uuid() != HOTSPOT_CHAR_UUID) return;
                                if (val.isEmpty()) return;

                                QStringList parts = QString::fromUtf8(val).split("::");
                                if (parts[0] != "READY" || parts.size() < 2) return;

                                // Stop polling!
                                auto timers = this->findChildren<QTimer*>();
                                for (auto t : timers) {
                                    if (t->interval() == 500) {
                                        t->stop();
                                        t->deleteLater();
                                    }
                                }

                                quint16 port = parts[1].toUInt();
                                emit logMessage("Host ready");

                                emit p2pNegotiationComplete(m_isHost, port);
                            });

                    // ── Polling fallback (avoids Android CCCD bugs) ───────────────
                    auto hotspotChar = svc->characteristic(HOTSPOT_CHAR_UUID);
                    auto *pollTimer = new QTimer(this);
                    pollTimer->setInterval(500);
                    connect(pollTimer, &QTimer::timeout, this, [=]() {
                        svc->readCharacteristic(hotspotChar);
                    });
                    pollTimer->start();
                    emit logMessage("Polling host for READY signal...");
                    // ─────────────────────────────────────────────────────────────

                    auto nonceChar = svc->characteristic(NONCE_CHAR_UUID);
                    QByteArray val = nonceChar.value();
                    uint32_t remoteNonce = qFromBigEndian<uint32_t>(
                            reinterpret_cast<const uchar*>(val.constData())
                            );
                    electHost(remoteNonce, svc);

                    // Issue initial read
                    svc->readCharacteristic(hotspotChar);
                    });
            });


    m_centralController->connectToDevice();
}

void BluetoothManager::disconnectBle()
{
    if (m_centralController && m_centralController->state() != QLowEnergyController::UnconnectedState) {
        emit logMessage("Disconnecting Central BLE");
        m_centralController->disconnectFromDevice();
    }
    if (m_peripheralController && m_peripheralController->state() != QLowEnergyController::UnconnectedState) {
        emit logMessage("Disconnecting Peripheral BLE");
        m_peripheralController->disconnectFromDevice();
    }
}

void BluetoothManager::startPeripheral()
{
    if (m_peripheralController) {
        m_peripheralController->stopAdvertising();
        m_peripheralController->deleteLater();
        m_peripheralController = nullptr;
    }

    // Generate nonce
    m_localNonce = QRandomGenerator::global()->generate(); // uint32

    // Build GATT service definition
    QLowEnergyCharacteristicData nonceChar;
    nonceChar.setUuid(NONCE_CHAR_UUID);
    nonceChar.setProperties(
            QLowEnergyCharacteristic::Read | QLowEnergyCharacteristic::Write
            );
    // Pack nonce as 4 bytes
    QByteArray nonceBytes(4, 0);
    qToBigEndian(m_localNonce, nonceBytes.data());
    nonceChar.setValue(nonceBytes);

    //QLowEnergyCharacteristicData hotspotChar;
    //hotspotChar.setUuid(HOTSPOT_CHAR_UUID);
    //hotspotChar.setProperties(QLowEnergyCharacteristic::Read
    //        | QLowEnergyCharacteristic::Write);
    //hotspotChar.setValue(QByteArray()); // filled in later by host
                                        //
    QLowEnergyCharacteristicData hotspotChar;
    hotspotChar.setUuid(HOTSPOT_CHAR_UUID);
    hotspotChar.setProperties(
            QLowEnergyCharacteristic::Read  |
            QLowEnergyCharacteristic::Write |
            QLowEnergyCharacteristic::Notify   // ← must be here
            );

    QLowEnergyDescriptorData cccd(
            QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration,
            QByteArray(2, 0)
            );
    //cccd.setReadConstraints(QBluetooth::AttAccessConstraint(0));   // open read
    //cccd.setWriteConstraints(QBluetooth::AttAccessConstraint(0));  // open write
    hotspotChar.addDescriptor(cccd);     // ← must be here
    hotspotChar.setValue(QByteArray("WAIT"));

    QLowEnergyServiceData serviceData;
    serviceData.setType(QLowEnergyServiceData::ServiceTypePrimary);
    serviceData.setUuid(SERVICE_UUID);
    serviceData.addCharacteristic(nonceChar);
    serviceData.addCharacteristic(hotspotChar);

    m_peripheralController = QLowEnergyController::createPeripheral(this);
    m_negotiationService = m_peripheralController->addService(serviceData);

    // Watch for writes from the remote peer
    connect(m_negotiationService, &QLowEnergyService::characteristicChanged,
            this, &BluetoothManager::onCharacteristicWritten);

    // Advertise with your service UUID so peers can filter on it
    QLowEnergyAdvertisingData advData;
    advData.setDiscoverability(QLowEnergyAdvertisingData::DiscoverabilityGeneral);
    advData.setServices({SERVICE_UUID});
    advData.setLocalName("P2PNode");

    m_peripheralController->startAdvertising(QLowEnergyAdvertisingParameters(), advData);
    emit logMessage("Advertising started, nonce: " + QString::number(m_localNonce));
}

void BluetoothManager::onCharacteristicWritten(
    const QLowEnergyCharacteristic &ch, const QByteArray &val)
{
    emit logMessage(QString("Char written: uuid=%1 val=%2")
        .arg(ch.uuid().toString())
        .arg(QString::fromUtf8(val.toHex())));
    if (ch.uuid() == NONCE_CHAR_UUID) {
        // Remote updated their nonce after a collision re-roll
        if (val.size() < 4) return;
        uint32_t remoteNonce = qFromBigEndian<uint32_t>(
            reinterpret_cast<const uchar*>(val.constData())
        );
        emit logMessage("Peer re-rolled nonce: " + QString::number(remoteNonce));
        // Re-elect — but we need the remoteSvc for this.
        // Problem: onCharacteristicWritten doesn't have remoteSvc in scope.
        // Solution: cache it (see note below)
        electHost(remoteNonce, m_remoteSvc);
        return;
    }

    if (ch.uuid() == HOTSPOT_CHAR_UUID) {
        // Shouldn't happen on host (host writes this, not reads it),
        // but guard anyway
        emit logMessage("Unexpected HOTSPOT_CHAR write on peripheral");
    }
}

void BluetoothManager::electHost(uint32_t remoteNonce, QLowEnergyService *remoteSvc)
{
    emit logMessage(QString("electHost called: local=%1 remote=%2")//done=%3")
        .arg(m_localNonce).arg(remoteNonce));//.arg(m_electionDone));

    if (remoteSvc) m_remoteSvc = remoteSvc; // cache it for onCharacteristicWritten

    if (m_localNonce == remoteNonce) {
        emit logMessage("Nonce collision — re-rolling");
        // re-advertise with new nonce, retry
        m_localNonce = QRandomGenerator::global()->generate();
        // update local characteristic value
        auto ch = m_negotiationService->characteristic(NONCE_CHAR_UUID);
        QByteArray nb(4, 0);
        qToBigEndian(m_localNonce, nb.data());
        m_negotiationService->writeCharacteristic(ch, nb);
        return;
    }

    m_isHost = (m_localNonce > remoteNonce);
    emit logMessage(m_isHost ? "I am HOST → will create hotspot"
            : "I am CLIENT → will connect to hotspot");

    if (m_isHost) {
        // We just need to emit p2pNegotiationComplete so the TransportManager can start the backend.
        // We will start the backend at the requested port (45678).
        // Before emitting, write READY to BLE for client.
        QString payload = "READY::45678";
        auto ch = m_negotiationService->characteristic(HOTSPOT_CHAR_UUID);
        m_negotiationService->writeCharacteristic(ch, payload.toUtf8());
        
        emit p2pNegotiationComplete(true, 45678);
    } else {
        connect(remoteSvc, &QLowEnergyService::characteristicChanged, this,
                [=](const QLowEnergyCharacteristic &ch, const QByteArray &val) {
                    if (ch.uuid() != HOTSPOT_CHAR_UUID) return;
                    QStringList parts = QString::fromUtf8(val).split("::");
                    if (parts[0] != "READY" || parts.size() < 2) return;

                    quint16 port = parts[1].toUInt();
                    emit logMessage("Host ready — negotiation complete");

                    emit p2pNegotiationComplete(false, port);
                });

                return;
            }


            //// Full creds payload (optional fallback)
            //if (parts.size() >= 4) {
            //    QString ip   = parts[2];
            //    QString port = parts[3];
            //    emit readyToConnect(ip, port.toUInt());
            //}
        //});
    //}
}
