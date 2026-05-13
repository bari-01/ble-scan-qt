// HotspotBridge.h
#pragma once
#include <QObject>
#include <QString>

#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

class HotspotBridge : public QObject {
    Q_OBJECT
public:
    explicit HotspotBridge(QObject *parent = nullptr);

    Q_INVOKABLE void startHotspot();
    Q_INVOKABLE void stopHotspot();
    Q_INVOKABLE void connectToHotspot(const QString &ssid, const QString &psk);

    // Called by JNI callbacks (static → instance via singleton)
    static HotspotBridge *instance();
    void handleHotspotStarted(const QString &ssid,
                              const QString &psk,
                              const QString &ip);
    void handleHotspotFailed(const QString &reason);
    void handleHotspotStopped();

    signals:
            void hotspotStarted(QString ssid, QString psk, QString ip);
    void hotspotFailed(QString reason);
    void hotspotStopped();

private:
    static HotspotBridge *s_instance;
#ifdef Q_OS_ANDROID
    QJniObject m_javaObj;
#endif
};