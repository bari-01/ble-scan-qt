import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import qmlModule

Rectangle {
    visible: true
    width: 400
    height: 800
    color: "#121212"

    Material.theme: Material.Dark
    Material.accent: Material.Blue

    readonly property real u: Screen.pixelDensity * 1.75
    readonly property real fontSmall: u * 1.0
    readonly property real fontMid:   u * 1.4
    readonly property real fontTitle: u * 2.2

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: scanPage
    }

    // ── Page 1: Scan ──────────────────────────────────────────────────────────
    Component {
        id: scanPage

        Page {
            id: scanPageRoot
            background: Rectangle { color: "#121212" }

            property bool isConnected: false

            header: ToolBar {
                background: Rectangle { color: "#1E1E1E" }
                Label {
                    anchors.centerIn: parent
                    text: "Qshare"
                    font.pixelSize: fontTitle
                    font.bold: true
                    color: "white"
                }
            }

            // ── Log panel anchored to bottom ──────────────────────────────
            ColumnLayout {
                id: logPanel
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                spacing: 0

                property bool expanded: false

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: u * 3
                    color: "#1A1A1A"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: u
                        anchors.rightMargin: u
                        Label {
                            text: "Log"
                            color: "#666666"
                            font.pixelSize: fontSmall
                            Layout.fillWidth: true
                        }
                        Label {
                            text: logPanel.expanded ? "▼" : "▲"
                            color: "#666666"
                            font.pixelSize: fontSmall
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: logPanel.expanded = !logPanel.expanded
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: logPanel.expanded ? u * 14 : 0
                    clip: true
                    color: "#111111"
                    Behavior on Layout.preferredHeight { NumberAnimation { duration: 200 } }

                    ListView {
                        id: logListView
                        anchors.fill: parent
                        anchors.margins: u * 0.8
                        clip: true
                        model: logModel
                        spacing: u * 0.3
                        
                        onCountChanged: Qt.callLater(positionViewAtEnd)

                        delegate: Label {
                            required property string msg
                            required property string msgColor
                            text: msg
                            color: msgColor
                            font.pixelSize: fontSmall
                            wrapMode: Text.Wrap
                            width: ListView.view.width
                        }
                    }
                }
            }

            // ── Main content above log ────────────────────────────────────
            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: logPanel.top
                anchors.margins: u * 1.5
                spacing: u * 1.2

                // Status pill
                Rectangle {
                    id: statusPill
                    Layout.fillWidth: true
                    Layout.preferredHeight: u * 3.5
                    radius: height / 2
                    color: "#1A1A1A"
                    border.color: "#333333"
                    border.width: 1
                    Behavior on color { ColorAnimation { duration: 300 } }

                    Label {
                        id: statusLabel
                        anchors.centerIn: parent
                        text: "Idle"
                        color: "white"
                        font.pixelSize: fontSmall
                        font.bold: true
                    }
                }

                // Buttons — swap between discover and connected actions
                Loader {
                    Layout.fillWidth: true
                    Layout.preferredHeight: u * 5
                    sourceComponent: scanPageRoot.isConnected
                                     ? connectedButtons : discoverButton
                }

                Component {
                    id: discoverButton
                    Button {
                        text: "Start Discovering"
                        font.pixelSize: fontMid
                        onClicked: {
                            deviceModel.clear()
                            Bluetooth.startPeripheral()
                            Bluetooth.startScan()
                            statusLabel.text = "Discovering…"
                            statusPill.color = "#1A3A5C"
                        }
                    }
                }

                Component {
                    id: connectedButtons
                    RowLayout {
                        spacing: u * 0.8
                        Button {
                            text: "File"
                            Layout.fillWidth: true
                            font.pixelSize: fontSmall
                            onClicked: fileDialog.open()
                        }
                        Button {
                            text: "Chat"
                            Layout.fillWidth: true
                            font.pixelSize: fontSmall
                            onClicked: stack.push(chatPage)
                        }
                        Button {
                            text: "✕"
                            Layout.preferredWidth: u * 5
                            font.pixelSize: fontSmall
                            onClicked: {
                                Transport.disconnectAll()
                                Bluetooth.disconnectBle()
                                scanPageRoot.isConnected = false
                                statusLabel.text = "Idle"
                                statusPill.color = "#1A1A1A"
                            }
                        }
                    }
                }

                // Device list
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "transparent"

                    Label {
                        anchors.centerIn: parent
                        text: "No devices found yet"
                        color: "#444444"
                        font.pixelSize: fontSmall
                        visible: deviceModel.count === 0
                    }

                    ListView {
                        anchors.fill: parent
                        spacing: u * 0.6
                        clip: true
                        model: deviceModel

                        delegate: Rectangle {
                            width: ListView.view.width
                            height: u * 7
                            radius: u
                            color: "#1E1E1E"
                            border.color: "#333333"
                            property bool tapped: false

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: u
                                spacing: u

                                Column {
                                    Layout.fillWidth: true
                                    spacing: u * 0.3
                                    Text {
                                        text: name
                                        color: "white"
                                        font.pixelSize: fontMid
                                        elide: Text.ElideRight
                                        width: parent.width
                                    }
                                    Text {
                                        text: address
                                        color: "#888888"
                                        font.pixelSize: fontSmall
                                    }
                                }

                                Button {
                                    text: tapped ? "…" : "Connect"
                                    font.pixelSize: fontSmall
                                    Layout.preferredHeight: u * 4
                                    enabled: !tapped
                                    opacity: enabled ? 1.0 : 0.5
                                    onClicked: {
                                        tapped = true
                                        Bluetooth.connectToDevice(address)
                                        statusLabel.text = "Connecting…"
                                        statusPill.color = "#2A2A1A"
                                    }
                                }
                            }
                        }
                    }
                }
            }

            ListModel { id: deviceModel }
            ListModel { id: logModel }

            FileDialog {
                id: fileDialog
                title: "Choose a file to send"
                onAccepted: {
                    var path = selectedFile.toString().replace("file://", "")
                    Transport.sendFile(path)
                    appendLog("Sending: " + path.split("/").pop(), "#FFFFBB")
                }
            }

            function appendLog(msg, color) {
                logModel.append({ "msg": msg, "msgColor": color })
                if (logModel.count > 100) logModel.remove(0)
            }

            // ── Bluetooth signals ─────────────────────────────────────────
            Connections {
                target: Bluetooth

                function onDeviceFound(name, address) {
                    deviceModel.append({ "name": name, "address": address })
                    appendLog("Found: " + name, "#88CCFF")
                }

                function onLogMessage(message) {
                    if (message.indexOf("HOST") !== -1) {
                        statusLabel.text = "HOST — setting up"
                        statusPill.color = "#1A4A1A"
                    } else if (message.indexOf("CLIENT") !== -1) {
                        statusLabel.text = "CLIENT — connecting"
                        statusPill.color = "#4A3A1A"
                    } else if (message.indexOf("Polling") !== -1) {
                        statusLabel.text = "Waiting for host…"
                        statusPill.color = "#3A2A1A"
                    }
                    appendLog(message, "#BBBBBB")
                }

                function onTcpConnected() {
                    scanPageRoot.isConnected = true
                    statusLabel.text = "Connected ✓"
                    statusPill.color = "#0A6A0A"
                    appendLog("TCP up ✓", "#88FF88")
                }

                function onTextReceived(text) {
                    appendLog("← " + text, "#AAFFAA")
                }
            }

            // ── Transport signals ─────────────────────────────────────────
            Connections {
                target: Transport

                function onLogMessage(message) {
                    if (message.indexOf("P2P group up") !== -1
                            || message.indexOf("TCP server listening") !== -1) {
                        statusLabel.text = "P2P up ✓"
                        statusPill.color = "#1A5A1A"
                    }
                    appendLog(message, "#DDDDDD")
                }

                function onP2pStatus(status) {
                    if (status === "Failed") {
                        statusLabel.text = "P2P failed ✗"
                        statusPill.color = "#8A1A1A"
                        appendLog("P2P failed", "#FF8888")
                    }
                }

                function onConnected() {
                    scanPageRoot.isConnected = true
                    statusLabel.text = "Connected ✓"
                    statusPill.color = "#0A6A0A"
                    appendLog("TCP up ✓", "#88FF88")
                }

                function onDisconnected() {
                    scanPageRoot.isConnected = false
                    statusLabel.text = "Idle"
                    statusPill.color = "#1A1A1A"
                    appendLog("Disconnected", "#FF8888")
                    if (stack.depth > 1) stack.pop()
                }

                function onTextReceived(text) {
                    appendLog("← " + text, "#AAFFAA")
                }

                function onFileCompleted(path) {
                    appendLog("📁 Saved: " + path.split("/").pop(), "#88CCFF")
                }

                function onTransferProgress(sent, total) {
                    // no progress bar on scan page — handled in chat page
                }
            }
        }
    }

    // ── Page 2: Chat ──────────────────────────────────────────────────────────
    Component {
        id: chatPage

        Page {
            background: Rectangle { color: "#121212" }

            header: ToolBar {
                background: Rectangle { color: "#1E1E1E" }
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: u

                    ToolButton {
                        text: "←"
                        font.pixelSize: fontMid
                        onClicked: stack.pop()
                    }
                    Label {
                        text: "Chat"
                        font.pixelSize: fontMid
                        font.bold: true
                        color: "white"
                        Layout.fillWidth: true
                    }
                    Rectangle {
                        width: u * 1.2
                        height: u * 1.2
                        radius: width / 2
                        color: "#00CC44"
                    }
                }
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: u * 1.5
                spacing: u

                ListView {
                    id: messageList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: messageModel
                    spacing: u * 0.8
                    onCountChanged: Qt.callLater(positionViewAtEnd)

                    delegate: Item {
                        width: ListView.view.width
                        height: bubble.height + u * 0.5

                        Rectangle {
                            id: bubble
                            width: Math.min(msgText.implicitWidth + u * 2.5,
                                           parent.width * 0.78)
                            height: msgText.implicitHeight + u * 1.8
                            radius: u * 0.8
                            color: fromMe ? "#1A4A8A" : "#2A2A2A"
                            anchors.right: fromMe ? parent.right : undefined
                            anchors.left:  fromMe ? undefined   : parent.left

                            Text {
                                id: msgText
                                anchors.centerIn: parent
                                text: content
                                color: "white"
                                font.pixelSize: fontMid
                                wrapMode: Text.Wrap
                                width: Math.min(implicitWidth,
                                               parent.width - u * 2.5)
                            }
                        }
                    }
                }

                ProgressBar {
                    id: progressBar
                    Layout.fillWidth: true
                    visible: value > 0 && value < 1
                    value: 0
                    from: 0; to: 1
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: u

                    TextField {
                        id: messageInput
                        Layout.fillWidth: true
                        placeholderText: "Message…"
                        font.pixelSize: fontMid
                        color: "white"
                        background: Rectangle {
                            color: "#2A2A2A"
                            radius: u * 0.5
                        }
                        onAccepted: sendMessage()
                    }

                    Button {
                        text: "Send"
                        font.pixelSize: fontMid
                        implicitHeight: u * 5
                        onClicked: sendMessage()
                    }
                }
            }

            ListModel { id: messageModel }

            function sendMessage() {
                var text = messageInput.text.trim()
                if (!text) return
                Transport.sendText(text)
                messageModel.append({ "content": text, "fromMe": true })
                messageInput.text = ""
            }

            Connections {
                target: Transport

                function onTextReceived(text) {
                    messageModel.append({ "content": text, "fromMe": false })
                }

                function onTransferProgress(sent, total) {
                    progressBar.value = total > 0 ? sent / total : 0
                }

                function onFileCompleted(path) {
                    progressBar.value = 0
                    messageModel.append({
                        "content": "📁 " + path.split("/").pop(),
                        "fromMe": false
                    })
                }

                function onDisconnected() {
                    if (stack.depth > 1) stack.pop()
                }
            }

            Connections {
                target: Bluetooth
                function onTextReceived(text) {
                    messageModel.append({ "content": text, "fromMe": false })
                }
            }
        }
    }
}