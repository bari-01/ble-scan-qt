import QtQuick
//import QtQuick.Controls
//import qmlModule // Must import the module where the singleton is registered

Rectangle {
    id: root
    color: "white"
    visible: true // Ensure it's visible

    Text {
        text: Bluetooth.bluetoothStatus
        anchors.centerIn: parent
        font.pointSize: 24
        color: "black"
    }
}
