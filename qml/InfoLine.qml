import QtQuick

Item {
    id: root

    property string label: ""
    property string value: ""
    property string sub: ""
    property string iconSource: ""
    property bool alignRight: false

    width: 180
    height: sub.length > 0 ? 56 : 46

    Row {
        anchors.fill: parent
        layoutDirection: root.alignRight ? Qt.RightToLeft : Qt.LeftToRight
        spacing: 8

        // ICON
        Image {
            width: 18
            height: 18
            source: root.iconSource
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: true
            opacity: 0.9
            y: 2   // aligns with label
        }

        Column {
            width: parent.width - 20
            spacing: 1

            Text {
                text: root.label
                color: "#99ffffff"
                font.pixelSize: 13
                font.letterSpacing: 1.5
                font.capitalization: Font.AllUppercase
                horizontalAlignment: root.alignRight ? Text.AlignRight : Text.AlignLeft
                width: parent.width
                renderType: Text.NativeRendering
            }

            Text {
                text: root.value
                color: "white"
                font.pixelSize: 18
                horizontalAlignment: root.alignRight ? Text.AlignRight : Text.AlignLeft
                width: parent.width
                renderType: Text.NativeRendering
            }

            Text {
                visible: root.sub.length > 0
                text: root.sub
                color: "#80ffffff"
                font.pixelSize: 13
                horizontalAlignment: root.alignRight ? Text.AlignRight : Text.AlignLeft
                width: parent.width
                renderType: Text.NativeRendering
            }
        }
    }
}