import QtQuick

Rectangle {
    id: root

    property string label: ""
    property string value: ""
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
    signal decrementRequested()
    signal incrementRequested()

    width: 200
    height: largeLandscape ? 94 : (standardLandscape ? 72 : (compact ? 56 : 72))
    radius: largeLandscape ? 24 : (standardLandscape ? 18 : (compact ? 15 : 18))
    color: "#4d000000"
    border.width: 1
    border.color: "#1affffff"

    FontLoader { id: interMedium; source: "qrc:/qml/fonts/Inter/Inter-Medium.ttf" }
    FontLoader { id: interBold; source: "qrc:/qml/fonts/Inter/Inter-Bold.ttf" }

    Item {
        anchors.fill: parent
        anchors.margins: root.largeLandscape ? 20 : (root.standardLandscape ? 14 : (root.compact ? 10 : 14))

        Row {
            anchors.fill: parent
            spacing: root.largeLandscape ? 18 : (root.standardLandscape ? 12 : (root.compact ? 8 : 12))

            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: root.largeLandscape ? 138 : (root.standardLandscape ? 100 : (root.compact ? 78 : 94))
                text: root.label
                color: "#66ffffff"
                font.family: interMedium.font.family
                font.pixelSize: root.largeLandscape ? 17 : (root.standardLandscape ? 13 : (root.compact ? 10 : 13))
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 2.0
                renderType: Text.NativeRendering
            }

            Item {
                width: parent.width - (root.largeLandscape ? 156 : (root.standardLandscape ? 112 : (root.compact ? 86 : 106)))
                height: parent.height

                Row {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: root.largeLandscape ? 18 : (root.standardLandscape ? 12 : (root.compact ? 8 : 12))

                    Rectangle {
                        width: root.largeLandscape ? 62 : (root.standardLandscape ? 46 : (root.compact ? 36 : 46))
                        height: root.largeLandscape ? 62 : (root.standardLandscape ? 46 : (root.compact ? 36 : 46))
                        radius: root.largeLandscape ? 20 : (root.standardLandscape ? 14 : (root.compact ? 12 : 14))
                        color: decrementArea.containsPress ? "#24ffffff" : "#14ffffff"
                        border.width: 1
                        border.color: "#1affffff"
                        scale: decrementArea.containsPress ? 0.95 : 1.0

                        Behavior on scale {
                            NumberAnimation { duration: 120; easing.type: Easing.OutCubic }
                        }

                        Text {
                            anchors.centerIn: parent
                            text: "−"
                            color: "white"
                            font.family: interMedium.font.family
                            font.pixelSize: root.largeLandscape ? 30 : (root.standardLandscape ? 22 : (root.compact ? 18 : 22))
                            renderType: Text.NativeRendering
                        }

                        MouseArea {
                            id: decrementArea
                            anchors.fill: parent
                            onClicked: root.decrementRequested()
                        }
                    }

                    Text {
                        width: root.largeLandscape ? 104 : (root.standardLandscape ? 76 : (root.compact ? 54 : 72))
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.value
                        color: "white"
                        font.family: interBold.font.family
                        font.pixelSize: root.largeLandscape ? 32 : (root.standardLandscape ? 24 : (root.compact ? 18 : 24))
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        renderType: Text.NativeRendering
                    }

                    Rectangle {
                        width: root.largeLandscape ? 62 : (root.standardLandscape ? 46 : (root.compact ? 36 : 46))
                        height: root.largeLandscape ? 62 : (root.standardLandscape ? 46 : (root.compact ? 36 : 46))
                        radius: root.largeLandscape ? 20 : (root.standardLandscape ? 14 : (root.compact ? 12 : 14))
                        color: incrementArea.containsPress ? "#24ffffff" : "#14ffffff"
                        border.width: 1
                        border.color: "#1affffff"
                        scale: incrementArea.containsPress ? 0.95 : 1.0

                        Behavior on scale {
                            NumberAnimation { duration: 120; easing.type: Easing.OutCubic }
                        }

                        Text {
                            anchors.centerIn: parent
                            text: "+"
                            color: "white"
                            font.family: interMedium.font.family
                            font.pixelSize: root.largeLandscape ? 30 : (root.standardLandscape ? 22 : (root.compact ? 18 : 22))
                            renderType: Text.NativeRendering
                        }

                        MouseArea {
                            id: incrementArea
                            anchors.fill: parent
                            onClicked: root.incrementRequested()
                        }
                    }
                }
            }
        }
    }
}
