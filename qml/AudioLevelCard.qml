import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property string title: ""
    property string description: ""
    property string iconSource: ""
    property string mutedIconSource: ""
    property int level: 50
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
    readonly property bool muted: Math.round(levelSlider.value) <= 0

    signal levelAdjusted(int value)

    width: parent ? parent.width : 400
    height: largeLandscape ? 176 : (standardLandscape ? 132 : (compact ? 104 : 128))
    radius: largeLandscape ? 24 : (standardLandscape ? 18 : (compact ? 15 : 18))
    color: "#171717"
    border.width: 1
    border.color: "#1affffff"

    FontLoader { id: interRegular; source: "qrc:/qml/fonts/Inter/Inter-Regular.ttf" }
    FontLoader { id: interBold; source: "qrc:/qml/fonts/Inter/Inter-Bold.ttf" }

    onLevelChanged: {
        if (Math.round(levelSlider.value) !== root.level)
            levelSlider.value = root.level
    }

    Item {
        anchors.fill: parent
        anchors.margins: root.largeLandscape ? 22 : (root.standardLandscape ? 16 : (root.compact ? 12 : 16))

        Column {
            anchors.fill: parent
            spacing: root.largeLandscape ? 20 : (root.standardLandscape ? 14 : (root.compact ? 10 : 14))

            Item {
                id: headerRow
                width: parent.width
                height: root.largeLandscape ? 62 : (root.standardLandscape ? 46 : (root.compact ? 36 : 44))

                Rectangle {
                    id: iconBadge
                    width: root.largeLandscape ? 62 : (root.standardLandscape ? 46 : (root.compact ? 36 : 44))
                    height: root.largeLandscape ? 62 : (root.standardLandscape ? 46 : (root.compact ? 36 : 44))
                    radius: root.largeLandscape ? 20 : (root.standardLandscape ? 15 : (root.compact ? 12 : 14))
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
                            width: root.largeLandscape ? 34 : (root.standardLandscape ? 24 : (root.compact ? 18 : 22))
                            height: root.largeLandscape ? 34 : (root.standardLandscape ? 24 : (root.compact ? 18 : 22))
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: true
                    }
                }

                Rectangle {
                    id: levelChip
                    width: root.largeLandscape ? 96 : (root.standardLandscape ? 72 : (root.compact ? 58 : 68))
                    height: root.largeLandscape ? 50 : (root.standardLandscape ? 38 : (root.compact ? 30 : 36))
                    radius: root.largeLandscape ? 20 : (root.standardLandscape ? 15 : (root.compact ? 12 : 14))
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
                        font.pixelSize: root.largeLandscape ? 22 : (root.standardLandscape ? 16 : (root.compact ? 12 : 15))
                        renderType: Text.NativeRendering
                    }
                }

                Column {
                    anchors.left: iconBadge.right
                    anchors.leftMargin: root.largeLandscape ? 18 : (root.standardLandscape ? 13 : (root.compact ? 9 : 12))
                    anchors.right: levelChip.left
                    anchors.rightMargin: root.largeLandscape ? 24 : (root.standardLandscape ? 18 : (root.compact ? 12 : 18))
                    spacing: root.largeLandscape ? 6 : (root.standardLandscape ? 4 : (root.compact ? 2 : 4))
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        text: root.title
                        color: "white"
                        font.family: interBold.font.family
                        font.pixelSize: root.largeLandscape ? 31 : (root.standardLandscape ? 22 : (root.compact ? 16 : 20))
                        elide: Text.ElideRight
                        width: parent.width
                        renderType: Text.NativeRendering
                    }

                    Text {
                        text: root.description
                        color: "#8cffffff"
                        font.family: interRegular.font.family
                        font.pixelSize: root.largeLandscape ? 19 : (root.standardLandscape ? 14 : (root.compact ? 11 : 13))
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
                        height: root.largeLandscape ? 14 : (root.standardLandscape ? 10 : (root.compact ? 8 : 10))
                        radius: height / 2
                        color: "#101010"
                        border.width: 1
                        border.color: "#1affffff"
                    }

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: levelSlider.visualPosition * parent.width
                        height: root.largeLandscape ? 14 : (root.standardLandscape ? 10 : (root.compact ? 8 : 10))
                        radius: height / 2
                        color: "#d7d9de"
                    }
                }

                handle: Rectangle {
                    x: levelSlider.leftPadding + levelSlider.visualPosition * (levelSlider.availableWidth - width)
                    y: levelSlider.topPadding + levelSlider.availableHeight / 2 - height / 2
                    width: root.largeLandscape ? 36 : (root.standardLandscape ? 26 : (root.compact ? 20 : 24))
                    height: root.largeLandscape ? 36 : (root.standardLandscape ? 26 : (root.compact ? 20 : 24))
                    radius: width / 2
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
