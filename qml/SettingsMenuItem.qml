import QtQuick

Rectangle {
    id: root

    property string label: ""
    property string iconSource: ""
    property bool active: false
    property bool inputEnabled: true
    property bool compact: false
    property bool large: false
    property bool oversized: false

    signal clicked()
    signal pressAndHoldRequested()

    width: parent ? parent.width : 180
    height: oversized ? 88 : (large ? 64 : (compact ? 40 : 56))
    radius: oversized ? 26 : (large ? 20 : (compact ? 12 : 16))

    color: active ? "#1affffff" : "transparent"
    border.width: active ? 1 : 0
    border.color: "#22ffffff"

    Row {
        anchors.fill: parent
        anchors.leftMargin: root.oversized ? 22 : (root.large ? 15 : (root.compact ? 9 : 14))
        anchors.rightMargin: root.oversized ? 22 : (root.large ? 15 : (root.compact ? 9 : 14))
        spacing: root.oversized ? 18 : (root.large ? 12 : (root.compact ? 7 : 12))

        Image {
            source: root.iconSource
            width: root.oversized ? 36 : (root.large ? 26 : (root.compact ? 17 : 20))
            height: root.oversized ? 36 : (root.large ? 26 : (root.compact ? 17 : 20))
            anchors.verticalCenter: parent.verticalCenter
            fillMode: Image.PreserveAspectFit
            opacity: root.active ? 1.0 : 0.7
        }

        Text {
            text: root.label
            color: "white"
            font.pixelSize: root.oversized ? 31 : (root.large ? 24 : (root.compact ? 16 : 20))
            font.family: interMedium.font.family
            anchors.verticalCenter: parent.verticalCenter
            opacity: root.active ? 1.0 : 0.85
            elide: Text.ElideRight
            width: parent.width - x
            renderType: Text.NativeRendering
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.inputEnabled
        pressAndHoldInterval: 260
        onClicked: root.clicked()
        onPressAndHold: root.pressAndHoldRequested()
    }
}
