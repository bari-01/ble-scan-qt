#include "BluetoothManager.h"
#include "HotspotBridge.h"

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
    QLowEnergyController *controller = QLowEnergyController::createCentral(device, this);

    connect(controller, &QLowEnergyController::connected, this,
            [=]() {
            emit logMessage("Connected. Discovering services...");
            controller->discoverServices();
            });

    //connect(controller, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error), this,
    //        [this, controller](QLowEnergyController::Error error)
    //        {
    //            emit logMessage("Controller Error: " + QString::number(error));
    //            controller->deleteLater();
    //        }
    //);

    connect(controller, &QLowEnergyController::disconnected, this,
            [=]() { emit logMessage("Disconnected"); });

    connect(controller, &QLowEnergyController::serviceDiscovered, this,
            [=](const QBluetoothUuid &uuid) {
            emit logMessage("Service: " + uuid.toString());
            if (uuid != SERVICE_UUID) return;
            auto *svc = controller->createServiceObject(uuid, this);
            svc->discoverDetails();

            connect(svc, &QLowEnergyService::stateChanged, this,
                    [=](QLowEnergyService::ServiceState state) {
                    if (state != QLowEnergyService::RemoteServiceDiscovered) return;

            // ── Enable notifications on HOTSPOT_CHAR ──────────────────────
            auto hotspotChar = svc->characteristic(HOTSPOT_CHAR_UUID);

            // Find the CCCD descriptor
            auto cccd = hotspotChar.descriptor(
                QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration
            );
            if (cccd.isValid()) {
                svc->writeDescriptor(cccd,
                    QByteArray::fromHex("0100")  // enable notifications
                );
                emit logMessage("Subscribed to HOTSPOT_CHAR notifications");
            } else {
                emit logMessage("WARNING: No CCCD on HOTSPOT_CHAR — polling instead");
            }
            // ─────────────────────────────────────────────────────────────

                    auto nonceChar = svc->characteristic(NONCE_CHAR_UUID);
                    QByteArray val = nonceChar.value();
                    uint32_t remoteNonce = qFromBigEndian<uint32_t>(
                            reinterpret_cast<const uchar*>(val.constData())
                            );
                    electHost(remoteNonce, svc);
                    });
            });


    controller->connectToDevice();
}

void BluetoothManager::startPeripheral()
{
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
    hotspotChar.setValue(QByteArray());

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
        auto *bridge = new HotspotBridge(this);
        connect(bridge, &HotspotBridge::hotspotStarted, this,
                [=](QString ssid, QString psk, QString ip) {
                    if (!m_transport) {
                        // Write "READY" to BLE — client just needs to know
                        // the host's group is up and which port to use
                        QString payload = "READY::45678";
                        auto ch = m_negotiationService->characteristic(HOTSPOT_CHAR_UUID);
                        m_negotiationService->writeCharacteristic(ch, payload.toUtf8());
                        emit logMessage("P2P group up: " + ssid);

                        m_transport = new TransportManager(this);
                        connect(m_transport, &TransportManager::logMessage,
                                this, &BluetoothManager::logMessage);
                        connect(m_transport, &TransportManager::connected, this,
                                [=]() { emit logMessage("TCP ready — peer connected"); });
                        connect(m_transport, &TransportManager::textReceived,
                                this, &BluetoothManager::textReceived);
                        m_transport->startServer(45678);
                    }
                });
        bridge->startHotspot();
    } else {
        connect(remoteSvc, &QLowEnergyService::characteristicChanged, this,
                [=](const QLowEnergyCharacteristic &ch, const QByteArray &val) {
                    if (ch.uuid() != HOTSPOT_CHAR_UUID) return;
                    QStringList parts = QString::fromUtf8(val).split("::");
                    if (parts[0] != "READY" || parts.size() < 2) return;

                    quint16 port = parts[1].toUInt();
                    emit logMessage("Host ready — starting P2P discovery");

                    auto *bridge = new HotspotBridge(this);

                    // When a peer is found, connect to it
                    connect(bridge, &HotspotBridge::peerFound, this,
                            [=](QString name, QString addr) {
                                emit logMessage("P2P peer found: " + name);
                                // Match by the BLE advertised name
                                if (name == "P2PNode" || !name.isEmpty()) {
                                    bridge->connectToPeer(addr);
                                }
                            });

                    // When P2P is connected, open TCP
                    connect(bridge, &HotspotBridge::hotspotStarted, this,
                            [=](QString, QString, QString ownerIp) {
                                m_transport = new TransportManager(this);
                                connect(m_transport, &TransportManager::logMessage,
                                        this, &BluetoothManager::logMessage);
                                connect(m_transport, &TransportManager::connected,
                                        this, [=]() {
                                            emit logMessage("TCP ready — connected to host");
                                        });
                                connect(m_transport, &TransportManager::textReceived,
                                        this, &BluetoothManager::textReceived);
                                m_transport->connectToHost(ownerIp, port);
                            });

                    bridge->discoverAndConnect();
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
