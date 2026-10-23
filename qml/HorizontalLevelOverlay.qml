import QtQuick

Item {
    id: root

    property bool levelEnabled: false
    property bool smpteEnabled: false
    property real previewAspectRatio: 16 / 9
    property string orientation: "XYZ"
    property bool liveAvailable: false
    property string liveStatusText: "Live gyro idle"
    property real accelXG: 0
    property real accelYG: 0
    property real accelZG: 0
    property bool levelReferenceSet: false
    property real levelAccelXG: 0
    property real levelAccelYG: 0
    property real levelAccelZG: 0
    property string levelColor: "Blue"
    property bool large: false
    property bool compact: false

    readonly property real activeAspectRatio: previewAspectRatio > 0.0 ? previewAspectRatio : (16.0 / 9.0)
    readonly property real activeImageWidth: (width / Math.max(1, height)) > activeAspectRatio
                                           ? height * activeAspectRatio
                                           : width
    readonly property real activeImageHeight: (width / Math.max(1, height)) > activeAspectRatio
                                            ? height
                                            : width / activeAspectRatio
    readonly property real activeImageX: Math.round((width - activeImageWidth) / 2)
    readonly property real activeImageY: Math.round((height - activeImageHeight) / 2)
    readonly property real mappedXG: liveAvailable ? mappedAxisValue(0) : 0
    readonly property real mappedYG: liveAvailable ? mappedAxisValue(1) : 1
    readonly property real mappedZG: liveAvailable ? mappedAxisValue(2) : 0
    readonly property real liveRollDegrees: liveAvailable ? rollFromXY(mappedXG, mappedYG) : 0
    readonly property real referenceRollDegrees: levelReferenceSet ? rollFromXY(levelAccelXG, levelAccelYG) : 0
    readonly property real rollDegrees: normalizeDegrees(liveRollDegrees - referenceRollDegrees)
    readonly property real livePitchDegrees: liveAvailable ? pitchFromXYZ(mappedXG, mappedYG, mappedZG) : 0
    readonly property real referencePitchDegrees: levelReferenceSet ? pitchFromXYZ(levelAccelXG, levelAccelYG, levelAccelZG) : 0
    readonly property real pitchDegrees: livePitchDegrees - referencePitchDegrees
    readonly property real displayBaseDegrees: Math.abs(rollDegrees) >= 45 && Math.abs(rollDegrees) <= 135
                                              ? (rollDegrees < 0 ? -90 : 90)
                                              : 0
    readonly property bool isLevel: liveAvailable
                                   && Math.abs(normalizeDegrees(rollDegrees - displayBaseDegrees)) <= 2.0
                                   && Math.abs(pitchDegrees) <= 2.0
    readonly property real pitchOffset: -clamp(pitchDegrees / 45, -1, 1)
                                        * Math.min(activeImageWidth, activeImageHeight) * 0.22
    readonly property real pitchOffsetX: -Math.sin(displayedBaseDegrees * Math.PI / 180) * pitchOffset
    readonly property real pitchOffsetY: Math.cos(displayedBaseDegrees * Math.PI / 180) * pitchOffset
    readonly property real stageWidth: Math.min(activeImageWidth * 0.42, large ? 500 : (compact ? 210 : 320))
    readonly property real stageHeight: large ? 72 : (compact ? 36 : 50)
    readonly property real lineThickness: large ? 5 : (compact ? 2.5 : 3.5)
    readonly property real referenceLineWidth: stageWidth * 0.11
    readonly property real movingLineWidth: stageWidth * 0.17
    readonly property real movingInnerGap: stageWidth * 0.14
    readonly property real referenceGap: stageWidth * 0.055
    readonly property real movingLeftX: (stageWidth / 2) - movingInnerGap - movingLineWidth
    readonly property real movingRightX: (stageWidth / 2) + movingInnerGap
    readonly property color activeLevelColor: colorForName(levelColor)
    readonly property color referenceColor: liveAvailable ? (isLevel ? activeLevelColor : "#80ffffff") : "#40ffffff"
    readonly property color sideColor: liveAvailable ? (isLevel ? activeLevelColor : "#f6f8fb") : "#55ffffff"
    readonly property real displayedRollDegrees: rollDegrees
    readonly property real displayedBaseDegrees: displayBaseDegrees

    anchors.fill: parent
    visible: levelEnabled && !smpteEnabled

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

    function colorForName(name) {
        var value = String(name || "Blue").toLowerCase()
        if (value === "red")
            return "#ff1a1a"
        if (value === "green")
            return "#1aff1a"
        if (value === "yellow")
            return "#ffff1a"
        if (value === "pink")
            return "#ff33b3"
        return "#3fd0ff"
    }

    function rollFromXY(x, y) {
        return Math.atan2(x, y) * 180 / Math.PI
    }

    function pitchFromXYZ(x, y, z) {
        return Math.atan2(z, Math.sqrt(x * x + y * y)) * 180 / Math.PI
    }

    function normalizeDegrees(value) {
        var out = value
        while (out > 180)
            out -= 360
        while (out < -180)
            out += 360
        return out
    }

    Item {
        id: stage
        width: root.stageWidth
        height: root.stageHeight
        x: root.activeImageX + (root.activeImageWidth - width) / 2
        y: root.activeImageY + (root.activeImageHeight - height) / 2
        opacity: root.liveAvailable ? 1.0 : 0.55

        Item {
            anchors.fill: parent
            rotation: root.displayedBaseDegrees
            transformOrigin: Item.Center

            Rectangle {
                width: root.referenceLineWidth
                height: root.lineThickness
                radius: height / 2
                x: root.movingLeftX - root.referenceGap - width
                y: (parent.height - height) / 2
                color: root.referenceColor
                antialiasing: false
            }

            Rectangle {
                width: root.referenceLineWidth
                height: root.lineThickness
                radius: height / 2
                x: root.movingRightX + root.movingLineWidth + root.referenceGap
                y: (parent.height - height) / 2
                color: root.referenceColor
                antialiasing: false
            }
        }

        Item {
            width: parent.width
            height: parent.height
            // Counter-move the pitch indicator; preserve this in portrait.
            x: root.pitchOffsetX
            y: root.pitchOffsetY
            // atan2(X, Y) already counter-rotates in Qt's clockwise screen axes.
            rotation: root.displayedRollDegrees
            transformOrigin: Item.Center

            Rectangle {
                width: root.movingLineWidth
                height: root.lineThickness
                radius: height / 2
                x: root.movingLeftX
                y: (parent.height - height) / 2
                color: root.sideColor
                antialiasing: false
            }

            Rectangle {
                width: root.movingLineWidth
                height: root.lineThickness
                radius: height / 2
                x: root.movingRightX
                y: (parent.height - height) / 2
                color: root.sideColor
                antialiasing: false
            }
        }
    }
}
