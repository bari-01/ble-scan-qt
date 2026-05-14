#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <QPermission>
#include <QBluetoothPermission>

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

    QBluetoothPermission permission;

    app.requestPermission(permission,
                          [](const QPermission &p)
                          {
                              if (p.status() == Qt::PermissionStatus::Granted)
                                  qDebug() << "Bluetooth permission granted";
                          });
    //engine.rootContext()->setContextProperty("Transport", transportManager);

    return app.exec();
}
