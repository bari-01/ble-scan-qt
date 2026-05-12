import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import qmlModule

Rectangle {
    //width: 400
    //height: 700
    color: "#121212"

    Material.theme: Material.Dark
    Material.accent: Material.Blue

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // Header
        Label {
            text: "BLE Scanner"
            font.pointSize: 72          // reasonable size for Android
            font.bold: true
            color: "white"
            Layout.alignment: Qt.AlignHCenter
        }

        // Scan button
        Button {
            text: "Scan Devices"
            Layout.fillWidth: true
            font.pointSize: 60
            height: 144                   // comfortable height for tapping
            onClicked: {
                deviceModel.clear()
                Bluetooth.startScan()
            }
        }

        // Log text
        Label {
            id: logText
            text: "Ready"
            color: "#BBBBBB"
            font.pointSize: 60
            wrapMode: Text.Wrap
            width: 600
            Layout.fillWidth: true
        }

        // Device list
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8
            model: deviceModel

            delegate: Rectangle {
                width: ListView.view.width
                height: 144                 // good size for readability
                radius: 30
                color: "#1E1E1E"
                border.color: "#333333"

                MouseArea {
                    anchors.fill: parent
                    anchors.margins: 12

                    onClicked: {
                        Bluetooth.connectToDevice(address)
                    }
                }

                Column {
                    anchors.centerIn: parent

                    Text { text: name; color: "white"; font.pixelSize: 55 }
                    Text { text: address; color: "gray"; font.pixelSize: 55 }
                }
            }
        }

        // Device model
        ListModel {
            id: deviceModel
        }
    }

    // Bluetooth signals
    Connections {
        target: Bluetooth

        function onDeviceFound(name, address) {
            deviceModel.append({
                "name": name,
                "address": address
            })
        }

        function onLogMessage(message) {
            logText.text = message
        }
    }
}