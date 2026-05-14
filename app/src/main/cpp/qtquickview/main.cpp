#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <QPermission>
#include <QBluetoothPermission>
#include <QLocationPermission>

#include "BluetoothManager.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    BluetoothManager manager;

    qmlRegisterSingletonInstance(
            "qmlModule",
            1, 0,
            "Bluetooth",
            &manager
    );

    // Request Bluetooth permissions (Scan & Connect)
    QBluetoothPermission btPermission;
    btPermission.setCommunicationModes(QBluetoothPermission::Access);
    app.requestPermission(btPermission, [](const QPermission &p) {
        if (p.status() == Qt::PermissionStatus::Granted)
            qDebug() << "Bluetooth permission granted";
        else
            qDebug() << "Bluetooth permission denied";
    });

    // Request Location permissions (often needed for BLE scan and WiFi)
    QLocationPermission locPermission;
    locPermission.setAccuracy(QLocationPermission::Precise);
    locPermission.setAvailability(QLocationPermission::WhenInUse);
    app.requestPermission(locPermission, [](const QPermission &p) {
        if (p.status() == Qt::PermissionStatus::Granted)
            qDebug() << "Location permission granted";
        else
            qDebug() << "Location permission denied";
    });

    // NOTE: NEARBY_WIFI_DEVICES (Android 13+) and CHANGE_NETWORK_STATE
    // are currently not available via the typed QPermission API.
    // They should be requested via JNI or the internal QtAndroidPrivate API if needed.

    return app.exec();
}
