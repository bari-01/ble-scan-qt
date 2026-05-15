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

    Material.theme: Material.Dark
    Material.accent: Material.Blue

    readonly property real u: Screen.pixelDensity * 1.75
    readonly property real fontSmall: u * 1.0
    readonly property real fontMid:   u * 1.4
    readonly property real fontTitle: u * 2.2

    // ── Navigation stack ─────────────────────────────────────────────────────
    StackView {
        id: stack
        anchors.fill: parent
        initialItem: scanPage
    }

    // ── Page 1: Scan ─────────────────────────────────────────────────────────
    Component {
        id: scanPage
        Page {
            background: Rectangle { color: "#121212" }

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

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: u * 1.5
                spacing: u * 1.2

                // Status pill
                Rectangle {
                    id: statusPill
                    Layout.fillWidth: true
                    height: u * 3.5
                    radius: height / 2
                    color: "#1A1A1A"
                    border.color: "#333333"
                    border.width: 1

                    Label {
                        id: statusLabel
                        anchors.centerIn: parent
                        text: "Idle"
                        color: "white"
                        font.pixelSize: fontSmall
                        font.bold: true
                    }
                }

                property bool isConnectedState: false

                // Action buttons
                StackLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: u * 5.5
                    currentIndex: isConnectedState ? 1 : 0

                    Button {
                        text: "Start Discovering"
                        Layout.fillWidth: true
                        font.pixelSize: fontMid
                        implicitHeight: u * 5
                        onClicked: {
                            deviceModel.clear()
                            Bluetooth.startPeripheral()
                            Bluetooth.startScan()
                            statusLabel.text = "Discovering…"
                            statusPill.color = "#1A3A5C"
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: u

                        Button {
                            text: "Send File"
                            Layout.fillWidth: true
                            font.pixelSize: fontMid
                            implicitHeight: u * 5
                            onClicked: fileDialog.open()
                        }
                        Button {
                            text: "Chat"
                            Layout.fillWidth: true
                            font.pixelSize: fontMid
                            implicitHeight: u * 5
                            onClicked: stack.push(chatPage)
                        }
                        Button {
                            text: "Disconnect"
                            Layout.fillWidth: true
                            font.pixelSize: fontMid
                            implicitHeight: u * 5
                            onClicked: Transport.disconnectAll()
                        }
                    }
                }

                // Device list
                ListView {
                    id: deviceList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: u * 0.6
                    clip: true
                    model: deviceModel

                    // Show a hint if empty
                    Label {
                        anchors.centerIn: parent
                        text: "No devices discovered yet"
                        color: "#444444"
                        font.pixelSize: fontSmall
                        visible: deviceModel.count === 0
                    }

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
                                }
                                Text {
                                    text: address
                                    color: "#888888"
                                    font.pixelSize: fontSmall
                                }
                            }

                            Button {
                                text: "Connect"
                                font.pixelSize: fontSmall
                                implicitHeight: u * 4
                                enabled: !parent.parent.tapped
                                opacity: enabled ? 1.0 : 0.4
                                onClicked: {
                                    parent.parent.tapped = true
                                    Bluetooth.connectToDevice(address)
                                    statusLabel.text = "Connecting…"
                                    statusPill.color = "#2A2A1A"
                                }
                            }
                        }
                    }
                }

                // Collapsible log
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        height: u * 3
                        color: "#1A1A1A"
                        radius: u * 0.5

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: u * 0.5

                            Label {
                                text: "Log"
                                color: "#888888"
                                font.pixelSize: fontSmall
                                Layout.fillWidth: true
                            }
                            Label {
                                text: logExpanded ? "▼" : "▲"
                                color: "#888888"
                                font.pixelSize: fontSmall
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: logExpanded = !logExpanded
                        }
                    }

                    property bool logExpanded: false

                    Rectangle {
                        Layout.fillWidth: true
                        height: logExpanded ? u * 14 : 0
                        clip: true
                        color: "#111111"
                        Behavior on height { NumberAnimation { duration: 200 } }

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
                                        text: model.msg !== undefined ? model.msg : msg
                                        color: model.msgColor !== undefined ? model.msgColor : msgColor
                                        font.pixelSize: fontSmall
                                        wrapMode: Text.Wrap
                                        width: parent.width
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
                    var path = selectedFile.toString()
                    if (path.startsWith("file://")) {
                        path = path.substring(7)
                    }
                    Transport.sendFile(path)
                    appendLog("Sending file: " + path, "#FFFFBB")
                }
            }

            function appendLog(msg, color) {
                logModel.append({ "msg": msg, "msgColor": color })
                if (logModel.count > 30) logModel.remove(0)
            }

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
                    }
                    appendLog(message, "#BBBBBB")
                }
            }

            Connections {
                target: Transport

                function onP2pStatus(status) {
                    if (status === "Started") {
                        statusLabel.text = "P2P up ✓"
                        statusPill.color = "#1A5A1A"
                    } else if (status === "Failed") {
                        statusLabel.text = "P2P failed ✗"
                        statusPill.color = "#8A1A1A"
                    }
                }

                function onLogMessage(message) {
                    appendLog(message, "#DDDDDD")
                }

                function onConnected() {
                    isConnectedState = true
                    statusLabel.text = "Connected ✓"
                    statusPill.color = "#0A6A0A"
                    appendLog("TCP up ✓", "#88FF88")
                }

                function onTextReceived(text) {
                    appendLog("← " + text, "#AAFFAA")
                }
            }
        }
    }

    // ── Page 2: Connected / Chat ──────────────────────────────────────────────
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
                        onClicked: {
                            Transport.disconnectAll()
                            if (stack.depth > 1) stack.pop()
                        }
                    }
                    Label {
                        text: "Connected"
                        font.pixelSize: fontMid
                        font.bold: true
                        color: "white"
                        Layout.fillWidth: true
                    }
                    // Connection indicator
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

                // Message history
                ListView {
                    id: messageList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: messageModel
                    spacing: u * 0.5

                    // Auto-scroll to bottom
                    onCountChanged: positionViewAtEnd()

                    delegate: Row {
                        width: ListView.view.width
                        layoutDirection: fromMe ? Qt.RightToLeft : Qt.LeftToRight
                        spacing: u

                        Rectangle {
                            width: Math.min(msgText.implicitWidth + u * 2,
                                           parent.width * 0.75)
                            height: msgText.implicitHeight + u * 1.5
                            radius: u
                            color: fromMe ? "#1A4A8A" : "#2A2A2A"

                            Text {
                                id: msgText
                                anchors.centerIn: parent
                                anchors.margins: u
                                text: content
                                color: "white"
                                font.pixelSize: fontMid
                                wrapMode: Text.Wrap
                                width: parent.width - u * 2
                            }
                        }
                    }
                }

                // Progress bar for file transfers
                ProgressBar {
                    id: progressBar
                    Layout.fillWidth: true
                    visible: value > 0 && value < 1
                    value: 0
                    from: 0; to: 1
                }

                // Input row
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
                if (text.length === 0) return
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
                        "content": "📁 File saved: " + path,
                        "fromMe": false
                    })
                }
                
                function onDisconnected() {
                    isConnectedState = false
                    statusLabel.text = "Idle"
                    statusPill.color = "#333333"
                    if (stack.depth > 1) stack.pop()
                }
            }
        }
    }
}