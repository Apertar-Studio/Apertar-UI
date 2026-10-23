import QtQuick

Rectangle {
    id: root

    signal clicked()
    property real backgroundOpacity: 1.0
    property int buttonSize: 64
    property int iconSize: 24
    property bool strongBackground: false
    property bool darkBackground: false

    width: buttonSize
    height: buttonSize
    radius: Math.round(buttonSize * 0.3125)

    color: root.enabled
           ? (root.darkBackground ? fadedBlack(0.34) : fadedWhite(root.strongBackground ? 0.14 : 0.09))
           : (root.darkBackground ? fadedBlack(0.24) : fadedWhite(root.strongBackground ? 0.09 : 0.05))
    border.width: 1
    border.color: root.enabled
                  ? fadedWhite(root.darkBackground ? 0.20 : 0.13)
                  : fadedWhite(root.darkBackground ? 0.11 : 0.08)
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

    function fadedWhite(baseAlpha) {
        return Qt.rgba(1.0, 1.0, 1.0,
                       Math.max(0.0, Math.min(1.0, baseAlpha * root.backgroundOpacity)))
    }

    function fadedBlack(baseAlpha) {
        return Qt.rgba(0.0, 0.0, 0.0,
                       Math.max(0.0, Math.min(1.0, baseAlpha * root.backgroundOpacity)))
    }

    Image {
        anchors.centerIn: parent
        width: root.iconSize
        height: root.iconSize
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
