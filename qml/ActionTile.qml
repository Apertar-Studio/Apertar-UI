import QtQuick

Rectangle {
    id: root

    property string label: ""
    property string tone: "default"
    property bool actionEnabled: true
    property bool holdToActivate: false
    property int holdDuration: 900
    property string holdLabel: "Hold..."
    property real holdProgress: 0.0
    property bool holdTriggered: false
    signal clicked()

    width: 200
    height: 70
    radius: 18
    clip: true
    color: root.tone === "danger"
           ? (actionArea.pressed && root.actionEnabled ? "#a32828" : (root.actionEnabled ? "#8d2020" : "#2b1414"))
           : (actionArea.pressed && root.actionEnabled ? "#24ffffff" : "#171717")
    border.width: 1
    border.color: root.tone === "danger"
                  ? (root.actionEnabled ? "#ba4a4a" : "#5f2c2c")
                  : "#1affffff"
    opacity: root.actionEnabled ? 1.0 : 0.45

    FontLoader { id: interMedium; source: "qrc:/qml/fonts/Inter/Inter-Medium.ttf" }

    Item {
        anchors.fill: parent
        anchors.margins: root.border.width
        visible: root.holdToActivate && root.actionEnabled

        Canvas {
            id: holdCanvas
            anchors.fill: parent
            opacity: root.holdToActivate ? 0.55 : 0.0
            antialiasing: true

            function roundedRectPath(ctx, x, y, w, h, r) {
                const radius = Math.max(0, Math.min(r, w / 2, h / 2))
                ctx.beginPath()
                ctx.moveTo(x + radius, y)
                ctx.lineTo(x + w - radius, y)
                ctx.arcTo(x + w, y, x + w, y + radius, radius)
                ctx.lineTo(x + w, y + h - radius)
                ctx.arcTo(x + w, y + h, x + w - radius, y + h, radius)
                ctx.lineTo(x + radius, y + h)
                ctx.arcTo(x, y + h, x, y + h - radius, radius)
                ctx.lineTo(x, y + radius)
                ctx.arcTo(x, y, x + radius, y, radius)
                ctx.closePath()
            }

            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)

                const progressWidth = width * root.holdProgress
                if (progressWidth <= 0.5)
                    return

                ctx.save()
                ctx.beginPath()
                ctx.rect(0, 0, progressWidth, height)
                ctx.clip()
                roundedRectPath(ctx, 0, 0, width, height, Math.max(0, root.radius - root.border.width))
                ctx.fillStyle = root.tone === "danger" ? "#c84242" : "#2affffff"
                ctx.fill()
                ctx.restore()
            }

            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
        }
    }

    onHoldProgressChanged: holdCanvas.requestPaint()
    onToneChanged: holdCanvas.requestPaint()
    onActionEnabledChanged: holdCanvas.requestPaint()

    Text {
        anchors.centerIn: parent
        text: root.holdToActivate && actionArea.pressed && !root.holdTriggered ? root.holdLabel : root.label
        color: root.tone === "danger" ? "#ffd6d6" : "white"
        font.family: interMedium.font.family
        font.pixelSize: 16
        renderType: Text.NativeRendering
    }

    MouseArea {
        id: actionArea
        anchors.fill: parent
        enabled: root.actionEnabled
        pressAndHoldInterval: root.holdDuration

        onPressed: {
            if (!root.holdToActivate)
                return

            root.holdTriggered = false
            root.holdProgress = 0.0
            holdFillAnim.restart()
        }

        onReleased: {
            if (!root.holdToActivate)
                return

            if (!root.holdTriggered) {
                holdFillAnim.stop()
                holdResetAnim.restart()
            } else {
                holdProgress = 0.0
                root.holdTriggered = false
            }
        }

        onCanceled: {
            if (!root.holdToActivate)
                return

            holdFillAnim.stop()
            holdResetAnim.restart()
            root.holdTriggered = false
        }

        onClicked: {
            if (!root.holdToActivate)
                root.clicked()
        }

        onPressAndHold: {
            if (!root.holdToActivate)
                return

            holdFillAnim.stop()
            root.holdProgress = 1.0
            root.holdTriggered = true
            root.clicked()
            holdCompleteReset.restart()
        }
    }

    NumberAnimation {
        id: holdFillAnim
        target: root
        property: "holdProgress"
        from: 0.0
        to: 1.0
        duration: root.holdDuration
        easing.type: Easing.Linear
    }

    NumberAnimation {
        id: holdResetAnim
        target: root
        property: "holdProgress"
        to: 0.0
        duration: 140
        easing.type: Easing.OutCubic
    }

    Timer {
        id: holdCompleteReset
        interval: 160
        repeat: false
        onTriggered: {
            root.holdProgress = 0.0
            root.holdTriggered = false
        }
    }
}
