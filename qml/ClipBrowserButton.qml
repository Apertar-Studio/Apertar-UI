import QtQuick

Rectangle {
    id: root

    signal clicked()

    width: 64
    height: 64
    radius: 20

    color: root.enabled ? "#18ffffff" : "#0dffffff"
    border.width: 1
    border.color: root.enabled ? "#22ffffff" : "#14ffffff"
    layer.enabled: true
    layer.smooth: false
    opacity: root.enabled ? 1.0 : 0.42

    scale: root.enabled && pressArea.pressed ? 0.94 : 1.0

    Behavior on scale {
        NumberAnimation {
            duration: 120
            easing.type: Easing.OutCubic
        }
    }

    Behavior on opacity {
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

    Image {
        anchors.centerIn: parent
        width: 24
        height: 24
        source: "qrc:/qml/icons/folder.png"
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
        opacity: root.enabled ? 0.88 : 0.38
    }

    MouseArea {
        id: pressArea
        anchors.fill: parent
        enabled: root.enabled
        onClicked: root.clicked()
    }
}
