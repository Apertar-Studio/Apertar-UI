import QtQuick

Rectangle {
    id: root

    property bool recording: false
    property bool photoMode: false
    property bool photoCapturePulse: false
    signal clicked()

    width: 74
    height: 74
    radius: 37

    color: photoMode
           ? (photoCapturePulse ? "#2affffff" : "#18ffffff")
           : (recording ? "#fb2c36" : "#18ffffff")
    border.width: 1
    border.color: photoMode
                  ? (photoCapturePulse ? "#66ffffff" : "#33ffffff")
                  : (recording ? "#88ff8888" : "#33ffffff")
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

    Rectangle {
        id: glow
        anchors.centerIn: parent
        width: parent.width + 14
        height: width
        radius: width / 2
        color: root.photoMode ? "#40ffffff" : "#30ff4444"
        opacity: root.recording || root.photoCapturePulse ? 0.18 : 0.0
        visible: root.recording || root.photoCapturePulse
        z: -1

        Behavior on opacity {
            NumberAnimation {
                duration: 160
                easing.type: Easing.OutCubic
            }
        }
    }

    Rectangle {
        id: centerShape
        anchors.centerIn: parent
        width: recording ? 28 : 28
        height: 28
        radius: recording ? 8 : 14
        color: recording ? "white" : "#ff3b30"
        visible: !root.photoMode

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
        width: 28
        height: 28
        visible: root.photoMode
        source: "qrc:/qml/icons/camera.png"
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
        opacity: root.photoCapturePulse ? 1.0 : 0.96
        scale: root.photoCapturePulse ? 1.08 : 1.0

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
        onClicked: root.clicked()
    }
}
