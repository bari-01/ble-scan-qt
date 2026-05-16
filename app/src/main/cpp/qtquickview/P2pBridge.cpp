// P2pBridge.cpp
#include "P2pBridge.h"
#include <QDebug>

#ifdef Q_OS_ANDROID
#include <QJniEnvironment>
#endif

P2pBridge *P2pBridge::s_instance = nullptr;

P2pBridge::P2pBridge(QObject *parent) : QObject(parent) {
    s_instance = this;

#ifdef Q_OS_ANDROID
    // Get Android application context
    QJniObject activity = QJniObject::callStaticObjectMethod(
        "org/qtproject/qt/android/QtNative",
        "activity",

        "()Landroid/app/Activity;"
    );

    m_javaObj = QJniObject(
        "io/bari/qshare/P2pManager",
        "(Landroid/content/Context;)V",
        activity.object<jobject>()
    );
#endif
}

P2pBridge *P2pBridge::instance() { return s_instance; }

void P2pBridge::startP2pHost() {
#ifdef Q_OS_ANDROID
    m_javaObj.callMethod<void>("startP2pHost");
#elif defined(Q_OS_LINUX)
    system("pkexec wpa_cli p2p_group_add");
    system("wpa_cli p2p_group_add");
    emit p2pStarted("DIRECT-linux", "password", "192.168.4.1");
#else
    emit p2pFailed("Not on Android");
#endif
}

void P2pBridge::stopP2p() {
#ifdef Q_OS_ANDROID
    m_javaObj.callMethod<void>("stopP2p");
#endif
}

void P2pBridge::connectToP2pClient(const QString &ssid, const QString &psk) {
#ifdef Q_OS_ANDROID
    m_javaObj.callMethod<void>(
        "connectToP2pClient",
        "(Ljava/lang/String;Ljava/lang/String;)V",
        QJniObject::fromString(ssid).object<jstring>(),
        QJniObject::fromString(psk).object<jstring>()
    );
#elif defined(Q_OS_LINUX)
    system("wpa_cli p2p_connect 00:00:00:00:00:00 pbc");
#else
    Q_UNUSED(ssid); Q_UNUSED(psk);
    emit p2pFailed("Not on Android");
#endif
}

void P2pBridge::getP2pMacAddress()
{
#ifdef Q_OS_ANDROID
    m_javaObj.callMethod<void>("getP2pMacAddress");
#else
    emit p2pFailed("Not on Android");
#endif
}

// ── JNI callbacks (Java → C++) ───────────────────────────────────────────────

void P2pBridge::handleP2pStarted(const QString &ssid,
                                         const QString &psk,
                                         const QString &ip) {
    emit p2pStarted(ssid, psk, ip);
}
void P2pBridge::handleP2pFailed(const QString &reason) {
    emit p2pFailed(reason);
}
void P2pBridge::handleP2pStopped() {
    emit p2pStopped();
}

#ifdef Q_OS_ANDROID
// These are declared `native` in Java — JNI glue
extern "C" {

JNIEXPORT void JNICALL
Java_io_bari_qshare_P2pManager_onP2pStarted(
        JNIEnv *env, jobject /*thiz*/,
jstring ssid, jstring psk, jstring ip)
{
auto toQt = [&](jstring js) -> QString {
    const char *c = env->GetStringUTFChars(js, nullptr);
    QString s = QString::fromUtf8(c);
    env->ReleaseStringUTFChars(js, c);
    return s;
};
if (P2pBridge::instance())
P2pBridge::instance()->handleP2pStarted(
        toQt(ssid), toQt(psk), toQt(ip));
}

JNIEXPORT void JNICALL
Java_io_bari_qshare_P2pManager_onP2pFailed(
        JNIEnv *env, jobject /*thiz*/, jstring reason)
{
const char *c = env->GetStringUTFChars(reason, nullptr);
QString s = QString::fromUtf8(c);
env->ReleaseStringUTFChars(reason, c);
if (P2pBridge::instance())
P2pBridge::instance()->handleP2pFailed(s);
}

JNIEXPORT void JNICALL
Java_io_bari_qshare_P2pManager_onP2pStopped(
        JNIEnv *env, jobject /*thiz*/)
{
Q_UNUSED(env);
if (P2pBridge::instance())
P2pBridge::instance()->handleP2pStopped();
}

} // extern "C"
  //
void P2pBridge::handlePeerFound(const QString &name, const QString &addr) {
    emit peerFound(name, addr);
}
extern "C" JNIEXPORT void JNICALL
Java_io_bari_qshare_P2pManager_onPeerFound(
    JNIEnv *env, jobject, jstring name, jstring addr)
{
    auto toQt = [&](jstring js) {
        const char *c = env->GetStringUTFChars(js, nullptr);
        QString s = QString::fromUtf8(c);
        env->ReleaseStringUTFChars(js, c);
        return s;
    };
    if (P2pBridge::instance())
        P2pBridge::instance()->handlePeerFound(toQt(name), toQt(addr));
}
#endif

void P2pBridge::connectToPeer(const QString &deviceAddress)
{
#ifdef Q_OS_ANDROID
    m_javaObj.callMethod<void>(
        "connectToPeer",
        "(Ljava/lang/String;)V",
        QJniObject::fromString(deviceAddress).object<jstring>()
    );
#elif defined(Q_OS_LINUX)
    system(QString("pkexec wpa_cli p2p_connect %1 pbc").arg(deviceAddress).toStdString().c_str());
#else
    emit p2pFailed("Not on Android");
#endif
}

void P2pBridge::discoverAndConnect()
{
#ifdef Q_OS_ANDROID
    m_javaObj.callMethod<void>("discoverAndConnect");
#elif defined(Q_OS_LINUX)
    system("pkexec wpa_cli p2p_find");
    system("pkexec wpa_cli p2p_connect 00:00:00:00:00:00 pbc");
    emit p2pStarted("CLIENT_CONNECTED", "", "192.168.49.1");
#else
    emit p2pFailed("Not on Android");
#endif
}
