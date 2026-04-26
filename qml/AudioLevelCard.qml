import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property string title: ""
    property string description: ""
    property string iconSource: ""
    property string mutedIconSource: ""
    property int level: 50
    readonly property bool muted: Math.round(levelSlider.value) <= 0

    signal levelAdjusted(int value)

    width: parent ? parent.width : 400
    height: 128
    radius: 18
    color: "#171717"
    border.width: 1
    border.color: "#1affffff"

    FontLoader { id: interRegular; source: "qrc:/qml/fonts/Inter/Inter-Regular.ttf" }
    FontLoader { id: interMedium; source: "qrc:/qml/fonts/Inter/Inter-Medium.ttf" }
    FontLoader { id: interBold; source: "qrc:/qml/fonts/Inter/Inter-Bold.ttf" }

    onLevelChanged: {
        if (Math.round(levelSlider.value) !== root.level)
            levelSlider.value = root.level
    }

    Item {
        anchors.fill: parent
        anchors.margins: 16

        Column {
            anchors.fill: parent
            spacing: 14

            Item {
                id: headerRow
                width: parent.width
                height: 44

                Rectangle {
                    id: iconBadge
                    width: 44
                    height: 44
                    radius: 14
                    color: "#18ffffff"
                    border.width: 1
                    border.color: "#1affffff"
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter

                    Image {
                        anchors.centerIn: parent
                        source: root.muted && root.mutedIconSource.length > 0
                                ? root.mutedIconSource
                                : root.iconSource
                        width: 22
                        height: 22
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: true
                    }
                }

                Rectangle {
                    id: levelChip
                    width: 68
                    height: 36
                    radius: 14
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    color: "#111111"
                    border.width: 1
                    border.color: "#1affffff"

                    Text {
                        anchors.centerIn: parent
                        text: Math.round(levelSlider.value) + "%"
                        color: root.muted ? "#ff6b6b" : "white"
                        font.family: interBold.font.family
                        font.weight: Font.Bold
                        font.pixelSize: 15
                        renderType: Text.NativeRendering
                    }
                }

                Column {
                    anchors.left: iconBadge.right
                    anchors.leftMargin: 12
                    anchors.right: levelChip.left
                    anchors.rightMargin: 18
                    spacing: 4
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        text: root.title
                        color: "white"
                        font.family: interBold.font.family
                        font.pixelSize: 20
                        elide: Text.ElideRight
                        width: parent.width
                        renderType: Text.NativeRendering
                    }

                    Text {
                        text: root.description
                        color: "#8cffffff"
                        font.family: interRegular.font.family
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        width: parent.width
                        renderType: Text.NativeRendering
                    }
                }
            }

            Slider {
                id: levelSlider
                width: parent.width
                from: 0
                to: 100
                stepSize: 1
                value: root.level

                onMoved: root.levelAdjusted(Math.round(value))
                onPressedChanged: {
                    if (!pressed)
                        root.levelAdjusted(Math.round(value))
                }

                background: Item {
                    implicitWidth: levelSlider.width
                    implicitHeight: 28

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width
                        height: 10
                        radius: 5
                        color: "#101010"
                        border.width: 1
                        border.color: "#1affffff"
                    }

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: levelSlider.visualPosition * parent.width
                        height: 10
                        radius: 5
                        color: "#d7d9de"
                    }
                }

                handle: Rectangle {
                    x: levelSlider.leftPadding + levelSlider.visualPosition * (levelSlider.availableWidth - width)
                    y: levelSlider.topPadding + levelSlider.availableHeight / 2 - height / 2
                    width: 24
                    height: 24
                    radius: 12
                    color: levelSlider.pressed ? "#ffffff" : "#f3f5f8"
                    border.width: 1
                    border.color: "#96a0ad"
                    scale: levelSlider.pressed ? 0.94 : 1.0

                    Behavior on scale {
                        NumberAnimation { duration: 120; easing.type: Easing.OutCubic }
                    }
                }
            }
        }
    }
}
