import QtQuick

Rectangle {
    id: root

    property string title: ""
    property string titleIconSource: ""
    property string description: ""
    property bool favoriteEnabled: false
    property bool favorited: false
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
    default property alias contentData: contentColumn.data
    signal pressAndHoldRequested()
    signal favoriteToggleRequested()

    width: parent ? parent.width : 400
    height: bodyColumn.implicitHeight + (largeLandscape ? 50 : (standardLandscape ? 36 : (compact ? 28 : 40)))
    radius: largeLandscape ? 30 : (standardLandscape ? 22 : (compact ? 18 : 22))
    color: "#181818"
    border.width: 1
    border.color: "#1affffff"

    FontLoader { id: interRegular; source: "qrc:/qml/fonts/Inter/Inter-Regular.ttf" }
    FontLoader { id: interMedium; source: "qrc:/qml/fonts/Inter/Inter-Medium.ttf" }
    FontLoader { id: interBold; source: "qrc:/qml/fonts/Inter/Inter-Bold.ttf" }

    Item {
        anchors.fill: parent
        anchors.margins: root.largeLandscape ? 26 : (root.standardLandscape ? 18 : (root.compact ? 14 : 20))

        Column {
            id: bodyColumn
            width: parent.width
            spacing: root.largeLandscape ? 20 : (root.standardLandscape ? 13 : (root.compact ? 10 : 14))

            Item {
                id: titleColumn
                width: parent.width
                height: titleContent.implicitHeight

                Rectangle {
                    id: favoriteButton
                    visible: root.favoriteEnabled
                    width: root.largeLandscape ? 46 : (root.standardLandscape ? 34 : (root.compact ? 28 : 34))
                    height: root.largeLandscape ? 46 : (root.standardLandscape ? 34 : (root.compact ? 28 : 34))
                    radius: root.largeLandscape ? 16 : (root.standardLandscape ? 12 : (root.compact ? 10 : 12))
                    anchors.right: parent.right
                    anchors.top: parent.top
                    color: favoriteButtonArea.containsPress
                           ? (root.favorited ? "#7a6120" : "#20ffffff")
                           : (root.favorited ? "#5f4b16" : "#141414")
                    border.width: 1
                    border.color: root.favorited ? "#e0b447" : "#1affffff"
                    scale: favoriteButtonArea.containsPress ? 0.96 : 1.0

                    Behavior on scale {
                        NumberAnimation { duration: 120; easing.type: Easing.OutCubic }
                    }

                    Text {
                        anchors.centerIn: parent
                        text: root.favorited ? "★" : "☆"
                        color: root.favorited ? "#ffd86f" : "#b6b8bd"
                        font.family: interMedium.font.family
                        font.pixelSize: root.largeLandscape ? 24 : (root.standardLandscape ? 18 : (root.compact ? 15 : 18))
                        renderType: Text.NativeRendering
                    }

                    MouseArea {
                        id: favoriteButtonArea
                        anchors.fill: parent
                        onClicked: root.favoriteToggleRequested()
                    }
                }

                Column {
                    id: titleContent
                    width: root.favoriteEnabled ? (parent.width - favoriteButton.width - (root.largeLandscape ? 16 : (root.standardLandscape ? 12 : (root.compact ? 8 : 12)))) : parent.width
                    spacing: root.largeLandscape ? 6 : (root.standardLandscape ? 4 : (root.compact ? 2 : 4))

                    Row {
                        width: parent.width
                        spacing: root.largeLandscape ? 12 : (root.standardLandscape ? 8 : (root.compact ? 6 : 8))

                        Text {
                            width: Math.min(implicitWidth, parent.width - (root.titleIconSource.length > 0 ? (root.largeLandscape ? 48 : (root.standardLandscape ? 36 : (root.compact ? 26 : 32))) : 0))
                            text: root.title
                            color: "white"
                            font.family: interBold.font.family
                            font.pixelSize: root.largeLandscape ? 34 : (root.standardLandscape ? 24 : (root.compact ? 18 : 22))
                            renderType: Text.NativeRendering
                            elide: Text.ElideRight
                        }

                        Image {
                            visible: root.titleIconSource.length > 0
                            source: root.titleIconSource
                            width: root.largeLandscape ? 36 : (root.standardLandscape ? 26 : (root.compact ? 20 : 24))
                            height: root.largeLandscape ? 36 : (root.standardLandscape ? 26 : (root.compact ? 20 : 24))
                            anchors.verticalCenter: parent.verticalCenter
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            mipmap: true
                        }
                    }

                    Text {
                        text: root.description
                        color: "#8cffffff"
                        font.family: interRegular.font.family
                        font.pixelSize: root.largeLandscape ? 19 : (root.standardLandscape ? 14 : (root.compact ? 11 : 14))
                        wrapMode: Text.WordWrap
                        width: parent.width
                        renderType: Text.NativeRendering
                    }
                }

                MouseArea {
                    width: titleContent.width
                    height: titleContent.height
                    acceptedButtons: Qt.LeftButton
                    preventStealing: false
                    pressAndHoldInterval: 320
                    onPressAndHold: root.pressAndHoldRequested()
                }
            }

            Column {
                id: contentColumn
                width: parent.width
                spacing: root.largeLandscape ? 18 : (root.standardLandscape ? 12 : (root.compact ? 8 : 12))
            }
        }
    }
}
