#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickView>

#include <QPermission>
#include <QBluetoothPermission>

#include "BluetoothManager.h"
#include "TransportManager.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    BluetoothManager manager;
    TransportManager transportManager;

    QObject::connect(&manager, &BluetoothManager::p2pNegotiationComplete,
                     &transportManager, &TransportManager::onP2pNegotiated);
    
    QObject::connect(&transportManager, &TransportManager::connected,
                     &manager, &BluetoothManager::disconnectBle);

    qmlRegisterSingletonInstance(
            "qmlModule",
            1, 0,
            "Bluetooth",
            &manager
    );
    
    qmlRegisterSingletonInstance(
            "qmlModule",
            1, 0,
            "Transport",
            &transportManager
    );

#ifndef Q_OS_ANDROID
    QQuickView view;
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl("qrc:/qt/qml/qmlModule/Main.qml"));
    view.resize(400, 800);
    view.show();
#endif

    return app.exec();
}
