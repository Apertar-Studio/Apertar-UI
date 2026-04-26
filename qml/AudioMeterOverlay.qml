import QtQuick

Rectangle {
    id: root

    property int inputLevel: 0
    readonly property real normalizedLevel: Math.max(0, Math.min(1, inputLevel / 100))
    readonly property bool muted: inputLevel <= 0
    readonly property int segmentCount: 14

    width: 220
    height: 34
    radius: 17
    color: "#000000"
    border.width: 1
    border.color: root.muted ? "#3a2a2a" : "#20ffffff"

    Item {
        anchors.fill: parent
        anchors.margins: 6

        Row {
            anchors.fill: parent
            spacing: 8

            Rectangle {
                width: 22
                height: 22
                radius: 11
                anchors.verticalCenter: parent.verticalCenter
                color: root.muted ? "#4b1f1f" : "#18ffffff"
                border.width: 1
                border.color: root.muted ? "#8f3d3d" : "#1affffff"

                Image {
                    anchors.centerIn: parent
                    width: 12
                    height: 12
                    source: root.muted
                            ? "qrc:/qml/icons/microphone-muted.png"
                            : "qrc:/qml/icons/microphone.png"
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                }
            }

            Rectangle {
                width: parent.width - 30
                height: 12
                anchors.verticalCenter: parent.verticalCenter
                radius: 6
                color: "#101113"
                border.width: 1
                border.color: "#1affffff"

                Item {
                    anchors.fill: parent
                    anchors.margins: 3

                    Row {
                        anchors.fill: parent
                        spacing: 2

                        Repeater {
                            model: root.segmentCount

                            delegate: Rectangle {
                                readonly property real threshold: (index + 1) / root.segmentCount

                                width: (parent.width - ((root.segmentCount - 1) * 2)) / root.segmentCount
                                height: parent.height
                                radius: 3
                                color: root.normalizedLevel >= threshold
                                       ? (threshold > 0.8 ? "#ff6262"
                                          : threshold > 0.55 ? "#ffd55a"
                                          : "#42df86")
                                       : "#1d2327"

                                Behavior on color {
                                    ColorAnimation {
                                        duration: 110
                                        easing.type: Easing.OutCubic
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
