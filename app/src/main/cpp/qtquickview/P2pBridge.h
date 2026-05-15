// P2pBridge.h
#pragma once
#include <QObject>
#include <QString>

#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

class P2pBridge : public QObject {
    Q_OBJECT
    public:
        explicit P2pBridge(QObject *parent = nullptr);

        Q_INVOKABLE void startP2pHost();
        Q_INVOKABLE void stopP2p();
        Q_INVOKABLE void connectToP2pClient(const QString &ssid, const QString &psk);
        Q_INVOKABLE void getP2pMacAddress();
        Q_INVOKABLE void connectToPeer(const QString &deviceAddress);
        Q_INVOKABLE void discoverAndConnect();

        // Called by JNI callbacks (static → instance via singleton)
        static P2pBridge *instance();
        void handleP2pStarted(const QString &ssid, const QString &psk,
                const QString &ip);
        void handleP2pFailed(const QString &reason);
        void handleP2pStopped();
        void handlePeerFound(const QString &name, const QString &addr);

    signals:
        void p2pStarted(QString ssid, QString psk, QString ip);
        void p2pFailed(QString reason);
        void peerFound(QString name, QString address);
        void p2pStopped();

    private:
        static P2pBridge *s_instance;
#ifdef Q_OS_ANDROID
        QJniObject m_javaObj;
#endif
};
