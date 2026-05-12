// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <simpleble/SimpleBLE.h>

#ifdef __ANDROID__
#include <jni.h>
#include <QJniEnvironment>
#include <simplejni/VM.hpp>

// Dummy implementation to satisfy the linker for SimpleBLE's internal dependency.
extern "C" JNIEXPORT jint JNICALL JNI_GetCreatedJavaVMs(JavaVM **vmBuf, jsize bufLen, jsize *nVMs) {
    if (nVMs) *nVMs = 0;
    return JNI_OK;
}
#endif

class BluetoothProvider : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString bluetoothStatus READ bluetoothStatus CONSTANT)
public:
    explicit BluetoothProvider(QObject *parent = nullptr) : QObject(parent) {
        m_status = "Bluetooth is enabled";
        try {
            if (!SimpleBLE::Adapter::bluetooth_enabled()) {
                m_status = "Bluetooth is disabled. Please turn it on.";
            }
        } catch (...) {
            m_status = "Error checking Bluetooth status.";
        }
    }
    QString bluetoothStatus() const { return m_status; }
    QString m_status;
};

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

#ifdef __ANDROID__
    JavaVM* qjvm = QJniEnvironment::javaVM();
    if (qjvm) {
        SimpleJNI::VM::jvm(qjvm);
    }
#endif

    // Register the singleton BEFORE any QML is loaded.
    // This makes it available to all engines in the process,
    // including the one created by QtQuickView in Kotlin.
    qmlRegisterSingletonInstance("qmlModule", 1, 0, "Bluetooth", new BluetoothProvider(&app));

    return app.exec();
}

#include "main.moc"
