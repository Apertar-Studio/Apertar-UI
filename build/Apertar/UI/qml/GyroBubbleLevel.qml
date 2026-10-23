import QtQuick

Rectangle {
    id: root

    property string orientation: "XYZ"
    property bool liveAvailable: false
    property string liveStatusText: "Live gyro idle"
    property real accelXG: 0
    property real accelYG: 0
    property real accelZG: 0
    property bool levelReferenceSet: false
    property string levelReferenceText: "Not set"
    property real levelAccelXG: 0
    property real levelAccelYG: 0
    property real levelAccelZG: 0
    property bool landscapeCompact: typeof displayConfigBridge !== "undefined"
                                    && displayConfigBridge.uiLayout === "landscape_compact"
    property bool mediumLandscape: typeof displayConfigBridge !== "undefined"
                                   && displayConfigBridge.uiLayout === "landscape_medium"
    property bool largeLandscape: typeof displayConfigBridge !== "undefined"
                                  && displayConfigBridge.uiLayout === "landscape_large"
    property bool standardLandscape: typeof displayConfigBridge !== "undefined"
                                     && displayConfigBridge.uiLayout === "landscape"
    property bool regularLandscape: mediumLandscape || standardLandscape || largeLandscape
    property bool compact: landscapeCompact || regularLandscape
    property real mappedXG: liveAvailable ? mappedAxisValue(0) : 0
    property real mappedYG: liveAvailable ? mappedAxisValue(1) : 0
    property real mappedZG: liveAvailable ? mappedAxisValue(2) : 0
    property real lateralXG: levelReferenceSet ? mappedXG - levelAccelXG : mappedXG
    property real verticalYG: levelReferenceSet ? mappedYG - levelAccelYG : mappedYG
    property real forwardZG: levelReferenceSet ? mappedZG - levelAccelZG : mappedZG
    property real rollDegrees: liveAvailable ? degreesFromG(lateralXG) : 0
    property real pitchDegrees: liveAvailable ? degreesFromG(forwardZG) : 0
    property real levelToleranceDegrees: 0.6
    property bool isLevel: liveAvailable
                           && Math.abs(rollDegrees) <= levelToleranceDegrees
                           && Math.abs(pitchDegrees) <= levelToleranceDegrees

    width: parent ? parent.width : 360
    height: largeLandscape ? 250 : (standardLandscape ? 194 : (compact ? 158 : 184))
    radius: largeLandscape ? 28 : (standardLandscape ? 22 : (compact ? 17 : 20))
    color: "#101010"
    border.width: 1
    border.color: root.isLevel ? "#663fd0ff" : "#1affffff"
    clip: true

    FontLoader { id: interRegular; source: "qrc:/qml/fonts/Inter/Inter-Regular.ttf" }
    FontLoader { id: interMedium; source: "qrc:/qml/fonts/Inter/Inter-Medium.ttf" }
    FontLoader { id: interBold; source: "qrc:/qml/fonts/Inter/Inter-Bold.ttf" }

    Behavior on lateralXG { NumberAnimation { duration: 90; easing.type: Easing.OutCubic } }
    Behavior on forwardZG { NumberAnimation { duration: 90; easing.type: Easing.OutCubic } }
    Behavior on rollDegrees { NumberAnimation { duration: 90; easing.type: Easing.OutCubic } }
    Behavior on pitchDegrees { NumberAnimation { duration: 90; easing.type: Easing.OutCubic } }

    function normalizedOrientation() {
        var value = String(root.orientation || "XYZ").replace(/\s/g, "")
        if (value.length !== 3)
            return "XYZ"

        var seen = {}
        for (var i = 0; i < value.length; ++i) {
            var axis = value.charAt(i).toUpperCase()
            if (axis !== "X" && axis !== "Y" && axis !== "Z")
                return "XYZ"
            if (seen[axis])
                return "XYZ"
            seen[axis] = true
        }
        return value
    }

    function sourceAxisValue(axis) {
        if (axis === "X")
            return root.accelXG
        if (axis === "Y")
            return root.accelYG
        return root.accelZG
    }

    function mappedAxisValue(index) {
        var value = root.normalizedOrientation()
        var token = value.charAt(index)
        var axis = token.toUpperCase()
        var sign = token === axis ? 1 : -1
        return sign * root.sourceAxisValue(axis)
    }

    function clamp(value, minimum, maximum) {
        return Math.max(minimum, Math.min(maximum, value))
    }

    function degreesFromG(value) {
        return Math.asin(root.clamp(value, -1, 1)) * 180 / Math.PI
    }

    function signedDegrees(value) {
        return (value >= 0 ? "+" : "") + value.toFixed(1) + "°"
    }

    Canvas {
        id: bubbleCanvas
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: root.largeLandscape ? 250 : (root.standardLandscape ? 194 : (root.compact ? 158 : 184))
        antialiasing: true

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            var centerX = width * 0.5
            var centerY = height * 0.54
            var radius = Math.min(width, height) * 0.34
            var ring1 = radius * 0.36
            var ring2 = radius * 0.68
            var edgeDegrees = 8.0
            var limit = radius * 0.86
            var dotX = centerX + root.clamp(root.rollDegrees / edgeDegrees, -1, 1) * limit
            var dotY = centerY + root.clamp(root.pitchDegrees / edgeDegrees, -1, 1) * limit
            var bubbleRadius = root.largeLandscape ? 15 : (root.standardLandscape ? 11 : (root.compact ? 8 : 10))

            ctx.save()
            ctx.lineCap = "round"
            ctx.lineJoin = "round"

            var grd = ctx.createRadialGradient(centerX, centerY, radius * 0.1, centerX, centerY, radius * 1.08)
            grd.addColorStop(0, "#1c2d35")
            grd.addColorStop(1, "#0c0c0c")
            ctx.fillStyle = grd
            ctx.beginPath()
            ctx.arc(centerX, centerY, radius * 1.08, 0, Math.PI * 2)
            ctx.fill()

            ctx.strokeStyle = "#1affffff"
            ctx.lineWidth = 1
            ctx.beginPath()
            ctx.arc(centerX, centerY, radius * 1.08, 0, Math.PI * 2)
            ctx.stroke()

            ctx.strokeStyle = root.isLevel ? "#803fd0ff" : "#26ffffff"
            ctx.lineWidth = root.largeLandscape ? 2.4 : 1.5
            ctx.beginPath()
            ctx.arc(centerX, centerY, ring2, 0, Math.PI * 2)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(centerX, centerY, ring1, 0, Math.PI * 2)
            ctx.stroke()

            ctx.strokeStyle = "#34ffffff"
            ctx.lineWidth = 1
            ctx.beginPath()
            ctx.moveTo(centerX - radius, centerY)
            ctx.lineTo(centerX + radius, centerY)
            ctx.moveTo(centerX, centerY - radius)
            ctx.lineTo(centerX, centerY + radius)
            ctx.stroke()

            ctx.fillStyle = root.isLevel ? "#3fd0ff" : "#f6f8fb"
            ctx.shadowColor = root.isLevel ? "#803fd0ff" : "#45ffffff"
            ctx.shadowBlur = root.largeLandscape ? 18 : 12
            ctx.beginPath()
            ctx.arc(dotX, dotY, bubbleRadius, 0, Math.PI * 2)
            ctx.fill()

            ctx.shadowBlur = 0
            ctx.strokeStyle = root.isLevel ? "#e7fbff" : "#ffffff"
            ctx.lineWidth = root.largeLandscape ? 2.4 : 1.6
            ctx.beginPath()
            ctx.arc(dotX, dotY, bubbleRadius, 0, Math.PI * 2)
            ctx.stroke()

            ctx.fillStyle = "#f2ffffff"
            ctx.beginPath()
            ctx.arc(centerX, centerY, root.largeLandscape ? 4.8 : (root.compact ? 3.2 : 4), 0, Math.PI * 2)
            ctx.fill()

            if (!root.liveAvailable) {
                ctx.fillStyle = "#70ffffff"
                ctx.font = "700 " + (root.largeLandscape ? 19 : (root.compact ? 12 : 15)) + "px " + interBold.font.family
                ctx.textAlign = "center"
                ctx.textBaseline = "middle"
                ctx.fillText("WAIT", centerX, centerY)
            }

            ctx.restore()
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    Column {
        anchors.left: bubbleCanvas.right
        anchors.leftMargin: root.largeLandscape ? 16 : (root.standardLandscape ? 12 : (root.compact ? 8 : 11))
        anchors.right: parent.right
        anchors.rightMargin: root.largeLandscape ? 20 : (root.standardLandscape ? 14 : (root.compact ? 10 : 14))
        anchors.verticalCenter: parent.verticalCenter
        spacing: root.largeLandscape ? 10 : (root.standardLandscape ? 7 : (root.compact ? 4 : 6))

        Text {
            width: parent.width
            text: "Digital Level"
            color: "white"
            font.family: interBold.font.family
            font.pixelSize: root.largeLandscape ? 29 : (root.standardLandscape ? 21 : (root.compact ? 16 : 19))
            renderType: Text.NativeRendering
            elide: Text.ElideRight
        }

        Row {
            width: parent.width
            spacing: root.largeLandscape ? 8 : (root.compact ? 5 : 6)

            Rectangle {
                width: root.largeLandscape ? 9 : (root.compact ? 6 : 7)
                height: width
                radius: width / 2
                color: root.liveAvailable ? (root.isLevel ? "#3fd0ff" : "#17d975") : "#e0b447"
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                width: parent.width - (root.largeLandscape ? 9 : (root.compact ? 6 : 7)) - parent.spacing
                text: root.liveAvailable
                      ? (root.isLevel ? "Level" : "Tilt active")
                      : root.liveStatusText
                color: root.liveAvailable ? (root.isLevel ? "#dffbff" : "#dfffee") : "#d8c173"
                font.family: interBold.font.family
                font.pixelSize: root.largeLandscape ? 18 : (root.standardLandscape ? 13 : (root.compact ? 10 : 12))
                renderType: Text.NativeRendering
                elide: Text.ElideRight
            }
        }

        Text {
            width: parent.width
            text: "Roll " + root.signedDegrees(root.rollDegrees)
            color: "#cfffffff"
            font.family: interBold.font.family
            font.pixelSize: root.largeLandscape ? 22 : (root.standardLandscape ? 16 : (root.compact ? 12 : 14))
            renderType: Text.NativeRendering
            elide: Text.ElideRight
        }

        Text {
            width: parent.width
            text: "Pitch " + root.signedDegrees(root.pitchDegrees)
            color: "#cfffffff"
            font.family: interBold.font.family
            font.pixelSize: root.largeLandscape ? 22 : (root.standardLandscape ? 16 : (root.compact ? 12 : 14))
            renderType: Text.NativeRendering
            elide: Text.ElideRight
        }

        Text {
            width: parent.width
            text: root.levelReferenceSet ? "Zeroed: " + root.levelReferenceText : "Use Set Level to zero this tripod angle."
            color: "#8cffffff"
            font.family: interRegular.font.family
            font.pixelSize: root.largeLandscape ? 16 : (root.standardLandscape ? 12 : (root.compact ? 9 : 11))
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
            renderType: Text.NativeRendering
        }
    }

    Component.onCompleted: bubbleCanvas.requestPaint()
    onLiveAvailableChanged: bubbleCanvas.requestPaint()
    onAccelXGChanged: bubbleCanvas.requestPaint()
    onAccelYGChanged: bubbleCanvas.requestPaint()
    onAccelZGChanged: bubbleCanvas.requestPaint()
    onLateralXGChanged: bubbleCanvas.requestPaint()
    onForwardZGChanged: bubbleCanvas.requestPaint()
    onRollDegreesChanged: bubbleCanvas.requestPaint()
    onPitchDegreesChanged: bubbleCanvas.requestPaint()
    onLevelReferenceSetChanged: bubbleCanvas.requestPaint()
}
