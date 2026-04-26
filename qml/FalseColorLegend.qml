import QtQuick

Item {
    id: root

    property string mode: "Exposure Based"
    readonly property var entries: mode === "Skin Tone"
                                   ? [
                                       { label: "75", color: "#ff3b30" },
                                       { label: "65", color: "#ffe24a" },
                                       { label: "45", color: "#34d15f" },
                                       { label: "30", color: "#36a8ff" },
                                       { label: "0", color: "#2350ff" }
                                     ]
                                   : mode === "Highlight Priority"
                                     ? [
                                         { label: "98", color: "#ff3b30" },
                                         { label: "92", color: "#ff6a30" },
                                         { label: "85", color: "#ffb030" },
                                         { label: "70", color: "#ffe24a" },
                                         { label: "0", color: "#4a4a4a" }
                                       ]
                                     : mode === "Shadow Priority"
                                       ? [
                                           { label: "20", color: "#36f3ff" },
                                           { label: "10", color: "#36a8ff" },
                                           { label: "5", color: "#2350ff" },
                                           { label: "2", color: "#8b2dff" },
                                           { label: "0", color: "#232323" }
                                         ]
                                       : [
                                           { label: "95", color: "#ff3b30" },
                                           { label: "85", color: "#ff8a1f" },
                                           { label: "70", color: "#ffe24a" },
                                           { label: "55", color: "#96f59a" },
                                           { label: "45", color: "#5a5a5a" },
                                           { label: "35", color: "#34d15f" },
                                           { label: "20", color: "#36a8ff" },
                                           { label: "10", color: "#2350ff" },
                                           { label: "0", color: "#b13dff" }
                                         ]

    visible: opacity > 0.0
    width: 50
    height: legendCard.height
    opacity: 1.0
    scale: 1.0

    FontLoader { id: interMedium; source: "qrc:/qml/fonts/Inter/Inter-Medium.ttf" }
    FontLoader { id: interBold; source: "qrc:/qml/fonts/Inter/Inter-Bold.ttf" }

    Rectangle {
        id: legendCard
        width: root.width
        height: legendColumn.implicitHeight + 22
        radius: 14
        color: "#9c000000"
        border.width: 1
        border.color: "#16ffffff"

        Item {
            anchors.fill: parent
            anchors.margins: 11

            Column {
                id: legendColumn
                width: parent.width
                spacing: 3

                Row {
                    width: parent.width
                    spacing: 4

                    Column {
                        width: 16
                        spacing: 2

                        Repeater {
                            model: root.entries

                            delegate: Item {
                                width: parent.width
                                height: 16

                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: modelData.label
                                    color: "white"
                                    font.family: interBold.font.family
                                    font.weight: Font.Bold
                                    font.pixelSize: 9
                                    renderType: Text.NativeRendering
                                }
                            }
                        }
                    }

                    Rectangle {
                        width: 12
                        height: (root.entries.length * 16) + ((root.entries.length - 1) * 2)
                        radius: 6
                        color: "#16000000"
                        border.width: 1
                        border.color: "#14ffffff"

                        Item {
                            anchors.fill: parent
                            anchors.margins: 1

                            Column {
                                anchors.fill: parent
                                spacing: 2

                                Repeater {
                                    model: root.entries

                                    delegate: Rectangle {
                                        width: parent.width
                                        height: 16
                                        radius: 2
                                        color: modelData.color
                                        opacity: 0.98
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Behavior on opacity {
        NumberAnimation {
            duration: 180
            easing.type: Easing.OutCubic
        }
    }

    Behavior on scale {
        NumberAnimation {
            duration: 220
            easing.type: Easing.OutCubic
        }
    }
}
