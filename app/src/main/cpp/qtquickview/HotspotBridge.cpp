// HotspotBridge.cpp
#include "HotspotBridge.h"
#include <QDebug>

#ifdef Q_OS_ANDROID
#include <QJniEnvironment>
#endif

HotspotBridge *HotspotBridge::s_instance = nullptr;

HotspotBridge::HotspotBridge(QObject *parent) : QObject(parent) {
    s_instance = this;

#ifdef Q_OS_ANDROID
    // Get Android application context
    QJniObject activity = QJniObject::callStaticObjectMethod(
        "org/qtproject/qt/android/QtNative",
        "activity",

        "()Landroid/app/Activity;"
    );

    m_javaObj = QJniObject(
        "io/bari/qshare/HotspotManager",
        "(Landroid/content/Context;)V",
        activity.object<jobject>()
    );
#endif
}

HotspotBridge *HotspotBridge::instance() { return s_instance; }

void HotspotBridge::startHotspot() {
#ifdef Q_OS_ANDROID
    m_javaObj.callMethod<void>("startHotspot");
#else
    emit hotspotFailed("Not on Android");
#endif
}

void HotspotBridge::stopHotspot() {
#ifdef Q_OS_ANDROID
    m_javaObj.callMethod<void>("stopHotspot");
#endif
}

void HotspotBridge::connectToHotspot(const QString &ssid, const QString &psk) {
#ifdef Q_OS_ANDROID
    m_javaObj.callMethod<void>(
        "connectToHotspot",
        "(Ljava/lang/String;Ljava/lang/String;)V",
        QJniObject::fromString(ssid).object<jstring>(),
        QJniObject::fromString(psk).object<jstring>()
    );
#else
    Q_UNUSED(ssid); Q_UNUSED(psk);
    emit hotspotFailed("Not on Android");
#endif
}

void HotspotBridge::getP2pMacAddress()
{
#ifdef Q_OS_ANDROID
    m_javaObj.callMethod<void>("getP2pMacAddress");
#else
    emit hotspotFailed("Not on Android");
#endif
}

// ── JNI callbacks (Java → C++) ───────────────────────────────────────────────

void HotspotBridge::handleHotspotStarted(const QString &ssid,
                                         const QString &psk,
                                         const QString &ip) {
    emit hotspotStarted(ssid, psk, ip);
}
void HotspotBridge::handleHotspotFailed(const QString &reason) {
    emit hotspotFailed(reason);
}
void HotspotBridge::handleHotspotStopped() {
    emit hotspotStopped();
}

// These are declared `native` in Java — JNI glue
extern "C" {

JNIEXPORT void JNICALL
Java_io_bari_qshare_HotspotManager_onHotspotStarted(
        JNIEnv *env, jobject /*thiz*/,
jstring ssid, jstring psk, jstring ip)
{
auto toQt = [&](jstring js) -> QString {
    const char *c = env->GetStringUTFChars(js, nullptr);
    QString s = QString::fromUtf8(c);
    env->ReleaseStringUTFChars(js, c);
    return s;
};
if (HotspotBridge::instance())
HotspotBridge::instance()->handleHotspotStarted(
        toQt(ssid), toQt(psk), toQt(ip));
}

JNIEXPORT void JNICALL
Java_io_bari_qshare_HotspotManager_onHotspotFailed(
        JNIEnv *env, jobject /*thiz*/, jstring reason)
{
const char *c = env->GetStringUTFChars(reason, nullptr);
QString s = QString::fromUtf8(c);
env->ReleaseStringUTFChars(reason, c);
if (HotspotBridge::instance())
HotspotBridge::instance()->handleHotspotFailed(s);
}

JNIEXPORT void JNICALL
Java_io_bari_qshare_HotspotManager_onHotspotStopped(
        JNIEnv *env, jobject /*thiz*/)
{
Q_UNUSED(env);
if (HotspotBridge::instance())
HotspotBridge::instance()->handleHotspotStopped();
}

} // extern "C"
  //
void HotspotBridge::handlePeerFound(const QString &name, const QString &addr) {
    emit peerFound(name, addr);
}
extern "C" JNIEXPORT void JNICALL
Java_io_bari_qshare_HotspotManager_onPeerFound(
    JNIEnv *env, jobject, jstring name, jstring addr)
{
    auto toQt = [&](jstring js) {
        const char *c = env->GetStringUTFChars(js, nullptr);
        QString s = QString::fromUtf8(c);
        env->ReleaseStringUTFChars(js, c);
        return s;
    };
    if (HotspotBridge::instance())
        HotspotBridge::instance()->handlePeerFound(toQt(name), toQt(addr));
}

void HotspotBridge::connectToPeer(const QString &deviceAddress)
{
#ifdef Q_OS_ANDROID
    m_javaObj.callMethod<void>(
        "connectToPeer",
        "(Ljava/lang/String;)V",
        QJniObject::fromString(deviceAddress).object<jstring>()
    );
#else
    emit hotspotFailed("Not on Android");
#endif
}

void HotspotBridge::discoverAndConnect()
{
#ifdef Q_OS_ANDROID
    m_javaObj.callMethod<void>("discoverAndConnect");
#else
    emit hotspotFailed("Not on Android");
#endif
}
