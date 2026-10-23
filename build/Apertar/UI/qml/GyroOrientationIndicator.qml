import QtQuick

Rectangle {
    id: root

    property string orientation: "XYZ"
    property string syncOffsetText: "0 ms"
    property int sampleRateHz: 500
    property bool liveAvailable: false
    property string liveStatusText: "Live gyro idle"
    property real accelXG: 0
    property real accelYG: 0
    property real accelZG: 0
    property real gyroXDps: 0
    property real gyroYDps: 0
    property real gyroZDps: 0
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
    property real mappedGyroXDps: liveAvailable ? mappedGyroAxisValue(0) : 0
    property real mappedGyroYDps: liveAvailable ? mappedGyroAxisValue(1) : 0
    property real mappedGyroZDps: liveAvailable ? mappedGyroAxisValue(2) : 0
    property real displayXG: levelReferenceSet ? mappedXG - levelAccelXG : mappedXG
    property real displayYG: levelReferenceSet ? mappedYG - levelAccelYG : mappedYG
    property real displayZG: levelReferenceSet ? mappedZG - levelAccelZG : mappedZG

    width: parent ? parent.width : 360
    height: largeLandscape ? 210 : (standardLandscape ? 164 : (compact ? 132 : 154))
    radius: largeLandscape ? 24 : (standardLandscape ? 18 : (compact ? 15 : 18))
    color: "#101010"
    border.width: 1
    border.color: "#1affffff"

    FontLoader { id: interRegular; source: "qrc:/qml/fonts/Inter/Inter-Regular.ttf" }
    FontLoader { id: interBold; source: "qrc:/qml/fonts/Inter/Inter-Bold.ttf" }

    Behavior on mappedXG { NumberAnimation { duration: 95; easing.type: Easing.OutCubic } }
    Behavior on mappedYG { NumberAnimation { duration: 95; easing.type: Easing.OutCubic } }
    Behavior on mappedZG { NumberAnimation { duration: 95; easing.type: Easing.OutCubic } }
    Behavior on displayXG { NumberAnimation { duration: 95; easing.type: Easing.OutCubic } }
    Behavior on displayYG { NumberAnimation { duration: 95; easing.type: Easing.OutCubic } }
    Behavior on displayZG { NumberAnimation { duration: 95; easing.type: Easing.OutCubic } }

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

    function sourceGyroAxisValue(axis) {
        if (axis === "X")
            return root.gyroXDps
        if (axis === "Y")
            return root.gyroYDps
        return root.gyroZDps
    }

    function mappedGyroAxisValue(index) {
        var value = root.normalizedOrientation()
        var token = value.charAt(index)
        var axis = token.toUpperCase()
        var sign = token === axis ? 1 : -1
        return sign * root.sourceGyroAxisValue(axis)
    }

    function axisColor(axis) {
        if (axis === "X")
            return "#ff4b4b"
        if (axis === "Y")
            return "#42d879"
        return "#4d9fff"
    }

    function axisVector(axis, positive) {
        if (axis === "X")
            return { x: positive ? 1 : -1, y: 0 }
        if (axis === "Y")
            return { x: 0, y: positive ? 1 : -1 }
        return { x: positive ? 0.72 : -0.72, y: positive ? -0.72 : 0.72 }
    }

    function signed(value) {
        return (value >= 0 ? "+" : "") + value.toFixed(2)
    }

    Canvas {
        id: axisCanvas
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: root.largeLandscape ? 232 : (root.standardLandscape ? 182 : (root.compact ? 148 : 172))
        antialiasing: true

        function clampMagnitude(value) {
            return Math.max(0, Math.min(1.25, Math.abs(value)))
        }

        function drawArrow(ctx, centerX, centerY, baseVec, axis, signedValue, radius, lineWidth, labelSize) {
            var magnitude = clampMagnitude(signedValue)
            var sign = signedValue >= 0 ? 1 : -1
            var minLength = root.liveAvailable ? radius * 0.16 : radius * 0.7
            var length = root.liveAvailable ? Math.max(minLength, radius * Math.min(1, magnitude)) : radius * 0.78
            var vecX = baseVec.x * sign
            var vecY = baseVec.y * sign
            var endX = centerX + vecX * length
            var endY = centerY + vecY * length
            var head = root.largeLandscape ? 12 : (root.standardLandscape ? 9 : (root.compact ? 7 : 8))
            var angle = Math.atan2(endY - centerY, endX - centerX)

            ctx.strokeStyle = root.axisColor(axis)
            ctx.fillStyle = root.axisColor(axis)
            ctx.lineWidth = lineWidth
            ctx.globalAlpha = root.liveAvailable ? (0.34 + Math.min(0.62, magnitude * 0.56)) : 0.38
            ctx.beginPath()
            ctx.moveTo(centerX, centerY)
            ctx.lineTo(endX, endY)
            ctx.stroke()

            ctx.globalAlpha = root.liveAvailable ? 0.94 : 0.48
            ctx.beginPath()
            ctx.moveTo(endX, endY)
            ctx.lineTo(endX - Math.cos(angle - 0.52) * head, endY - Math.sin(angle - 0.52) * head)
            ctx.lineTo(endX - Math.cos(angle + 0.52) * head, endY - Math.sin(angle + 0.52) * head)
            ctx.closePath()
            ctx.fill()

            ctx.globalAlpha = 1.0
            ctx.font = "700 " + labelSize + "px " + interBold.font.family
            ctx.textAlign = "center"
            ctx.textBaseline = "middle"
            ctx.fillText(axis, endX + vecX * (labelSize * 0.82), endY + vecY * (labelSize * 0.82))
        }

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            var centerX = width * 0.50
            var centerY = height * 0.54
            var radius = Math.min(width, height) * 0.30
            var lineWidth = root.largeLandscape ? 6 : (root.standardLandscape ? 4.6 : (root.compact ? 3.8 : 4.4))
            var labelSize = root.largeLandscape ? 24 : (root.standardLandscape ? 18 : (root.compact ? 14 : 16))
            var value = root.normalizedOrientation()

            ctx.lineCap = "round"
            ctx.lineJoin = "round"

            ctx.strokeStyle = "#20ffffff"
            ctx.lineWidth = 1
            ctx.globalAlpha = 1
            ctx.beginPath()
            ctx.arc(centerX, centerY, radius * 1.04, 0, Math.PI * 2)
            ctx.stroke()

            for (var i = 0; i < value.length; ++i) {
                var token = value.charAt(i)
                var axis = token.toUpperCase()
                var positive = token === axis
                var baseVec = root.axisVector(axis, positive)
                var mapped = i === 0 ? root.displayXG : (i === 1 ? root.displayYG : root.displayZG)
                drawArrow(ctx, centerX, centerY, baseVec, axis, mapped, radius, lineWidth, labelSize)
            }

            if (root.liveAvailable) {
                var dotLimit = radius * 0.82
                var dotX = centerX + Math.max(-1, Math.min(1, root.displayXG)) * dotLimit
                var dotY = centerY + Math.max(-1, Math.min(1, root.displayYG)) * dotLimit
                ctx.strokeStyle = "#baffffff"
                ctx.fillStyle = "#f8ffffff"
                ctx.lineWidth = root.largeLandscape ? 2.2 : 1.4
                ctx.beginPath()
                ctx.arc(dotX, dotY, root.largeLandscape ? 7 : (root.compact ? 4.4 : 5.5), 0, Math.PI * 2)
                ctx.fill()
                ctx.stroke()
            } else {
                ctx.fillStyle = "#60ffffff"
                ctx.font = "700 " + Math.max(10, labelSize * 0.62) + "px " + interBold.font.family
                ctx.textAlign = "center"
                ctx.textBaseline = "middle"
                ctx.fillText("WAIT", centerX, centerY)
            }

            ctx.fillStyle = "#f2ffffff"
            ctx.globalAlpha = 1.0
            ctx.beginPath()
            ctx.arc(centerX, centerY, root.largeLandscape ? 5 : (root.compact ? 3.4 : 4), 0, Math.PI * 2)
            ctx.fill()
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    Column {
        anchors.left: axisCanvas.right
        anchors.leftMargin: root.largeLandscape ? 16 : (root.standardLandscape ? 11 : (root.compact ? 8 : 11))
        anchors.right: parent.right
        anchors.rightMargin: root.largeLandscape ? 20 : (root.standardLandscape ? 14 : (root.compact ? 10 : 14))
        anchors.verticalCenter: parent.verticalCenter
        spacing: root.largeLandscape ? 10 : (root.standardLandscape ? 7 : (root.compact ? 4 : 6))

        Text {
            width: parent.width
            text: "Live Gyro"
            color: "white"
            font.family: interBold.font.family
            font.pixelSize: root.largeLandscape ? 28 : (root.standardLandscape ? 20 : (root.compact ? 15 : 18))
            renderType: Text.NativeRendering
            elide: Text.ElideRight
        }

        Row {
            spacing: root.largeLandscape ? 8 : (root.compact ? 5 : 6)
            width: parent.width

            Rectangle {
                width: root.largeLandscape ? 9 : (root.compact ? 6 : 7)
                height: width
                radius: width / 2
                color: root.liveAvailable ? "#17d975" : "#e0b447"
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                width: parent.width - (root.largeLandscape ? 9 : (root.compact ? 6 : 7)) - parent.spacing
                text: root.liveAvailable ? (root.levelReferenceSet ? "Live motion | Level set" : "Live motion") : root.liveStatusText
                color: root.liveAvailable ? "#dfffee" : "#d8c173"
                font.family: interBold.font.family
                font.pixelSize: root.largeLandscape ? 18 : (root.standardLandscape ? 13 : (root.compact ? 10 : 12))
                renderType: Text.NativeRendering
                elide: Text.ElideRight
            }
        }

        Text {
            width: parent.width
            text: "Orientation " + root.normalizedOrientation() + "  |  " + root.sampleRateHz + " Hz"
            color: "#baffffff"
            font.family: interBold.font.family
            font.pixelSize: root.largeLandscape ? 20 : (root.standardLandscape ? 14 : (root.compact ? 11 : 13))
            renderType: Text.NativeRendering
            elide: Text.ElideRight
        }

        Text {
            width: parent.width
            text: (root.levelReferenceSet ? "Level " : "A ") + root.signed(root.displayXG) + " / " + root.signed(root.displayYG) + " / " + root.signed(root.displayZG) + " g"
            color: "#9dffffff"
            font.family: interRegular.font.family
            font.pixelSize: root.largeLandscape ? 17 : (root.standardLandscape ? 12 : (root.compact ? 9 : 11))
            renderType: Text.NativeRendering
            elide: Text.ElideRight
        }

        Text {
            width: parent.width
            text: "G " + root.signed(root.mappedGyroXDps) + " / " + root.signed(root.mappedGyroYDps) + " / " + root.signed(root.mappedGyroZDps) + " d/s"
            color: "#78ffffff"
            font.family: interRegular.font.family
            font.pixelSize: root.largeLandscape ? 17 : (root.standardLandscape ? 12 : (root.compact ? 9 : 11))
            renderType: Text.NativeRendering
            elide: Text.ElideRight
        }

        Text {
            width: parent.width
            text: "Sync " + root.syncOffsetText
            color: "#6dffffff"
            font.family: interRegular.font.family
            font.pixelSize: root.largeLandscape ? 16 : (root.standardLandscape ? 11 : (root.compact ? 8 : 10))
            renderType: Text.NativeRendering
            elide: Text.ElideRight
        }
    }

    Component.onCompleted: {
        axisCanvas.requestPaint()
    }

    onOrientationChanged: axisCanvas.requestPaint()
    onLiveAvailableChanged: axisCanvas.requestPaint()
    onAccelXGChanged: axisCanvas.requestPaint()
    onAccelYGChanged: axisCanvas.requestPaint()
    onAccelZGChanged: axisCanvas.requestPaint()
    onMappedXGChanged: axisCanvas.requestPaint()
    onMappedYGChanged: axisCanvas.requestPaint()
    onMappedZGChanged: axisCanvas.requestPaint()
    onDisplayXGChanged: axisCanvas.requestPaint()
    onDisplayYGChanged: axisCanvas.requestPaint()
    onDisplayZGChanged: axisCanvas.requestPaint()
    onLevelReferenceSetChanged: axisCanvas.requestPaint()
}
