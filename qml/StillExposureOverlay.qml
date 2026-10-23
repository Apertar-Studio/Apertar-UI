import QtQuick

Rectangle {
    id: root
    property var controlBridge: null
    property string fontFamily: ""
    property real secondsRemaining: 0
    visible: controlBridge ? controlBridge.stillExposureActive : false
    color: "black"

    function updateCountdown() {
        var seconds = controlBridge ? Math.max(0, controlBridge.stillExposureRemainingSeconds) : 0
        secondsRemaining = seconds < 1 ? Math.ceil(seconds * 10) / 10 : Math.ceil(seconds)
    }
    onVisibleChanged: updateCountdown()
    Connections {
        target: root.controlBridge
        function onStillExposureChanged() { root.updateCountdown() }
    }
    Timer {
        interval: 250
        running: root.visible
        repeat: true
        onTriggered: root.updateCountdown()
    }
    Column {
        anchors.centerIn: parent
        spacing: 6
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.secondsRemaining > 0 ? root.secondsRemaining + "s" : "Finishing..."
            color: "white"
            font.family: root.fontFamily
            font.pixelSize: 24
            renderType: Text.NativeRendering
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "EXPOSING"
            color: "#99ffffff"
            font.family: root.fontFamily
            font.pixelSize: 11
            font.letterSpacing: 1.5
            renderType: Text.NativeRendering
        }
    }
}
