import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import qmlModule

Rectangle {
    color: "#121212"
    Material.theme: Material.Dark
    Material.accent: Material.Blue

    readonly property real u: Screen.pixelDensity * 1.75
    readonly property real fontSmall: u * 1.0
    readonly property real fontMid:   u * 1.4
    readonly property real fontTitle: u * 2.4

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
            id: statusPill
            Layout.fillWidth: true
            height: u * 4
            radius: height / 2
            color: "#333333"
            Behavior on color { ColorAnimation { duration: 300 } }

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
                    statusPill.color = "#1A3A5C"
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
                    statusPill.color = "#1A3A5C"
                }
            }
        }

        // Transfer progress bar — hidden until a transfer is active
        ProgressBar {
            id: progressBar
            Layout.fillWidth: true
            visible: value > 0 && value < 1
            value: 0
            from: 0
            to: 1
        }

        // Log box
        Rectangle {
            Layout.fillWidth: true
            height: u * 12
            color: "#1E1E1E"
            radius: u

            ScrollView {
                id: logScroll
                anchors.fill: parent
                anchors.margins: u * 0.8
                clip: true

                Column {
                    id: logColumn
                    spacing: u * 0.3
                    width: logScroll.width

                    Repeater {
                        model: logModel
                        Label {
                            text: msg
                            color: msgColor
                            font.pixelSize: fontSmall
                            wrapMode: Text.Wrap
                            width: logColumn.width
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
                        statusPill.color = "#2A2A1A"
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

        Button {
            text: "Send Test"
            Layout.fillWidth: true
            font.pixelSize: fontMid
            implicitHeight: u * 5
            onClicked: Bluetooth.sendText("ping from device")
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
            if (message.indexOf("HOST") !== -1) {
                statusLabel.text = "HOST — creating hotspot"
                statusPill.color = "#1A4A1A"
            } else if (message.indexOf("CLIENT") !== -1) {
                statusLabel.text = "CLIENT — waiting for creds"
                statusPill.color = "#4A3A1A"
            } else if (message.indexOf("P2P group up") !== -1
                    || message.indexOf("Hotspot up")   !== -1) {
                statusLabel.text = "P2P up ✓ — " + message.split(": ")[1]
                statusPill.color = "#1A5A1A"
            } else if (message.indexOf("collision") !== -1) {
                statusLabel.text = "Nonce collision — retrying"
                statusPill.color = "#5A1A1A"
            }
            appendLog(message, "#BBBBBB")
        }

        function onTcpConnected() {
            statusLabel.text = "TCP connected ✓"
            statusPill.color = "#0A6A0A"
            appendLog("TCP up ✓", "#88FF88")
        }

        function onTextReceived(text) {
            appendLog("← " + text, "#AAFFAA")
        }

        function onFileCompleted(path) {
            progressBar.value = 0
            appendLog("Saved: " + path, "#88CCFF")
        }

        function onTransferProgress(sent, total) {
            progressBar.value = total > 0 ? sent / total : 0
        }

        function onReadyToConnect(ip, port) {
            statusLabel.text = "TCP ready → " + ip + ":" + port
            statusPill.color = "#1A5A1A"
            appendLog("✓ Ready: " + ip + ":" + port, "#88FF88")
        }
    }

    function appendLog(msg, color) {
        logModel.append({ "msg": msg, "msgColor": color })
        if (logModel.count > 20) logModel.remove(0)
    }
}