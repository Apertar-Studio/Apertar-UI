import QtQuick

Rectangle {
    id: root

    property bool recording: false
    property bool timelapse: false
    property bool timelapseMode: false
    property bool photoMode: false
    property bool soundRecorderMode: false
    property bool photoCapturePulse: false
    property real backgroundOpacity: 1.0
    property int buttonSize: 74
    property int centerSize: 28
    property bool strongBackground: false
    property bool darkBackground: false
    property bool modeSelectionEnabled: !activeCapture
    signal clicked()
    signal pressAndHold()

    readonly property bool activeCapture: recording || timelapse
    readonly property string captureIconSource: photoMode
                                                 ? (timelapseMode ? "qrc:/qml/icons/timelapse.png" : "qrc:/qml/icons/camera.png")
                                                 : (soundRecorderMode ? "qrc:/qml/icons/microphone.png" : "")

    onTimelapseChanged: {
        if (!timelapse)
            glow.scale = 1.0
    }

    width: buttonSize
    height: buttonSize
    radius: buttonSize / 2

    opacity: enabled ? 1.0 : 0.45
    color: !enabled ? fadedColor(0x60, 0x60, 0x60, 0.35) : photoMode
           ? (timelapse
              ? fadedColor(0xfb, 0x2c, 0x36, 1.0)
              : (photoCapturePulse
              ? (root.darkBackground ? fadedBlack(0.42) : fadedWhite(root.strongBackground ? 0.24 : 0.16))
              : (root.darkBackground ? fadedBlack(0.34) : fadedWhite(root.strongBackground ? 0.14 : 0.09))))
           : (recording ? fadedColor(0xfb, 0x2c, 0x36, 1.0)
                        : (root.darkBackground ? fadedBlack(0.34) : fadedWhite(root.strongBackground ? 0.14 : 0.09)))
    border.width: 1
    border.color: photoMode
                  ? (timelapse ? fadedColor(0xff, 0x88, 0x88, 0.53)
                                : (photoCapturePulse ? fadedWhite(0.40) : fadedWhite(root.darkBackground ? 0.22 : 0.20)))
                  : (recording ? fadedColor(0xff, 0x88, 0x88, 0.53) : fadedWhite(root.darkBackground ? 0.22 : 0.20))
    layer.enabled: true
    layer.smooth: false

    scale: pressArea.pressed ? 0.93 : 1.0

    Behavior on scale {
        NumberAnimation {
            duration: 120
            easing.type: Easing.OutCubic
        }
    }

    Behavior on color {
        ColorAnimation {
            duration: 220
        }
    }

    Behavior on border.color {
        ColorAnimation {
            duration: 220
        }
    }

    function fadedWhite(baseAlpha) {
        return fadedColor(0xff, 0xff, 0xff, baseAlpha)
    }

    function fadedBlack(baseAlpha) {
        return fadedColor(0x00, 0x00, 0x00, baseAlpha)
    }

    function fadedColor(r, g, b, baseAlpha) {
        return Qt.rgba(r / 255.0,
                       g / 255.0,
                       b / 255.0,
                       Math.max(0.0, Math.min(1.0, baseAlpha * root.backgroundOpacity)))
    }

    Rectangle {
        id: glow
        anchors.centerIn: parent
        width: parent.width + 14
        height: width
        radius: width / 2
        color: (root.timelapse || root.recording) ? fadedColor(0xff, 0x44, 0x44, 0.22)
                                                   : fadedWhite(0.25)
        opacity: root.activeCapture || root.photoCapturePulse ? 0.18 : 0.0
        visible: root.activeCapture || root.photoCapturePulse
        z: -1

        Behavior on opacity {
            NumberAnimation {
                duration: 160
                easing.type: Easing.OutCubic
            }
        }

        SequentialAnimation {
            running: root.timelapse
            loops: Animation.Infinite
            NumberAnimation {
                target: glow
                property: "scale"
                from: 1.0
                to: 1.16
                duration: 640
                easing.type: Easing.InOutSine
            }
            NumberAnimation {
                target: glow
                property: "scale"
                from: 1.16
                to: 1.0
                duration: 640
                easing.type: Easing.InOutSine
            }
        }
    }

    Rectangle {
        id: centerShape
        anchors.centerIn: parent
        width: root.centerSize
        height: root.centerSize
        radius: recording ? Math.round(root.centerSize * 0.29) : root.centerSize / 2
        color: !root.enabled ? "#888888" : recording ? "white" : "#ff3b30"
        visible: !root.photoMode && !root.soundRecorderMode

        Behavior on radius {
            NumberAnimation {
                duration: 180
                easing.type: Easing.OutCubic
            }
        }

        Behavior on color {
            ColorAnimation {
                duration: 180
            }
        }
    }

    Image {
        anchors.centerIn: parent
        width: root.centerSize
        height: root.centerSize
        visible: root.captureIconSource.length > 0
        source: root.captureIconSource
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
        opacity: (root.photoCapturePulse || root.timelapse) ? 1.0 : 0.96
        scale: root.photoCapturePulse ? 1.08 : (root.timelapse ? 1.03 : 1.0)

        Behavior on scale {
            NumberAnimation {
                duration: 160
                easing.type: Easing.OutCubic
            }
        }
    }

    MouseArea {
        id: pressArea
        anchors.fill: parent
        property bool held: false
        pressAndHoldInterval: 600
        onPressed: held = false
        onPressAndHold: {
            held = true
            if (root.modeSelectionEnabled) root.pressAndHold()
        }
        onClicked: {
            if (!held) root.clicked()
        }
    }
}
