import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import qmlModule

Rectangle {
    color: "#121212"
    Material.theme: Material.Dark
    Material.accent: Material.Blue

    // ── One place to tune sizes ───────────────────────────────────────────────
    readonly property real u: Screen.pixelDensity * 1.75   // ~1 "unit" ≈ 3.5mm
    readonly property real fontSmall:  u * 1.0
    readonly property real fontMid:    u * 1.4
    readonly property real fontLarge:  u * 1.8
    readonly property real fontTitle:  u * 2.4

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: u * 2
        spacing: u

        Label {
            text: "P2P Test"
            font.pixelSize: fontTitle
            font.bold: true
            color: "white"
            Layout.alignment: Qt.AlignHCenter
        }

        // Status pill
        Rectangle {
            Layout.fillWidth: true
            height: u * 4
            radius: height / 2
            color: statusColor
            Behavior on color { ColorAnimation { duration: 300 } }
            property string statusColor: "#333333"

            Label {
                id: statusLabel
                anchors.centerIn: parent
                text: "Idle"
                color: "white"
                font.pixelSize: fontMid
                font.bold: true
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: u

            Button {
                text: "Advertise"
                Layout.fillWidth: true
                font.pixelSize: fontMid
                implicitHeight: u * 5
                onClicked: {
                    Bluetooth.startPeripheral()
                    statusLabel.text = "Advertising…"
                    statusLabel.parent.statusColor = "#1A3A5C"
                }
            }

            Button {
                text: "Scan"
                Layout.fillWidth: true
                font.pixelSize: fontMid
                implicitHeight: u * 5
                onClicked: {
                    deviceModel.clear()
                    Bluetooth.startScan()
                    statusLabel.text = "Scanning…"
                    statusLabel.parent.statusColor = "#1A3A5C"
                }
            }
        }

        // Log box
        Rectangle {
            Layout.fillWidth: true
            height: u * 12
            color: "#1E1E1E"
            radius: u

            ScrollView {
                anchors.fill: parent
                anchors.margins: u * 0.8
                clip: true

                Column {
                    spacing: u * 0.3
                    width: parent.width

                    Repeater {
                        model: logModel
                        Label {
                            text: msg
                            color: msgColor
                            font.pixelSize: fontSmall
                            wrapMode: Text.Wrap
                            width: parent.width
                        }
                    }
                }
            }
        }

        // Device list
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: u * 0.6
            clip: true
            model: deviceModel

            delegate: Rectangle {
                width: ListView.view.width
                height: u * 6
                radius: u
                color: "#1E1E1E"
                border.color: "#444444"

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        Bluetooth.connectToDevice(address)
                        statusLabel.text = "Connecting to " + name + "…"
                        statusLabel.parent.statusColor = "#2A2A1A"
                    }
                }

                Column {
                    anchors.centerIn: parent
                    spacing: u * 0.3
                    Text { text: name;    color: "white"; font.pixelSize: fontMid }
                    Text { text: address; color: "gray";  font.pixelSize: fontSmall }
                }
            }
        }

        ListModel { id: deviceModel }
        ListModel { id: logModel }
    }

    Connections {
        target: Bluetooth

        function onDeviceFound(name, address) {
            deviceModel.append({ "name": name, "address": address })
            appendLog("Found: " + name + " [" + address + "]", "#88CCFF")
        }

        function onLogMessage(message) {
            if      (message.indexOf("HOST")       !== -1) {
                statusLabel.text = "HOST — creating hotspot"
                statusLabel.parent.statusColor = "#1A4A1A"
            } else if (message.indexOf("CLIENT")   !== -1) {
                statusLabel.text = "CLIENT — waiting for creds"
                statusLabel.parent.statusColor = "#4A3A1A"
            } else if (message.indexOf("P2P group up") !== -1      // ← add
                    || message.indexOf("Hotspot up")  !== -1) {
                statusLabel.text = "P2P up ✓ — " + message.split(": ")[1]
                statusLabel.parent.statusColor = "#1A5A1A"
            } else if (message.indexOf("collision") !== -1) {
                statusLabel.text = "Nonce collision — retrying"
                statusLabel.parent.statusColor = "#5A1A1A"
            }
            appendLog(message, "#BBBBBB")
        }
        function onReadyToConnect(ip, port) {
            statusLabel.text = "TCP ready → " + ip + ":" + port
            statusLabel.parent.statusColor = "#1A5A1A"
            appendLog("✓ Ready: " + ip + ":" + port, "#88FF88")
        }
    }

    function appendLog(msg, color) {
        logModel.append({ "msg": msg, "msgColor": color })
        if (logModel.count > 20) logModel.remove(0)
    }
}