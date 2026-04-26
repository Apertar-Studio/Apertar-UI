import QtQuick

Rectangle {
    id: root

    property string value: "1.0x"
    signal clicked()

    width: 110
    height: 40
    radius: 14
    color: "#18ffffff"
    border.width: 1
    border.color: "#22ffffff"

    Text {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 14
        text: root.value
        color: "white"
        font.pixelSize: 16
        renderType: Text.NativeRendering
    }

    Text {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: 12
        text: "⌄"
        color: "#ccffffff"
        font.pixelSize: 16
        renderType: Text.NativeRendering
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.clicked()
    }
}