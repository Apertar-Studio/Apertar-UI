import QtQuick

Rectangle {
    id: root

    property string label: ""
    property string value: ""
    signal decrementRequested()
    signal incrementRequested()

    width: 200
    height: 72
    radius: 18
    color: "#4d000000"
    border.width: 1
    border.color: "#1affffff"

    FontLoader { id: interMedium; source: "qrc:/qml/fonts/Inter/Inter-Medium.ttf" }
    FontLoader { id: interBold; source: "qrc:/qml/fonts/Inter/Inter-Bold.ttf" }

    Item {
        anchors.fill: parent
        anchors.margins: 14

        Row {
            anchors.fill: parent
            spacing: 12

            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: 94
                text: root.label
                color: "#66ffffff"
                font.family: interMedium.font.family
                font.pixelSize: 13
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 2.0
                renderType: Text.NativeRendering
            }

            Item {
                width: parent.width - 106
                height: parent.height

                Row {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 12

                    Rectangle {
                        width: 46
                        height: 46
                        radius: 14
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
                            font.pixelSize: 22
                            renderType: Text.NativeRendering
                        }

                        MouseArea {
                            id: decrementArea
                            anchors.fill: parent
                            onClicked: root.decrementRequested()
                        }
                    }

                    Text {
                        width: 72
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.value
                        color: "white"
                        font.family: interBold.font.family
                        font.pixelSize: 24
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        renderType: Text.NativeRendering
                    }

                    Rectangle {
                        width: 46
                        height: 46
                        radius: 14
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
                            font.pixelSize: 22
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
