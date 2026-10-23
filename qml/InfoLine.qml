import QtQuick

Item {
    id: root

    property string label: ""
    property string value: ""
    property string sub: ""
    property string iconSource: ""
    property bool alignRight: false
    property int iconSize: 18
    property int rowSpacing: 8
    property int textSpacing: 1
    property int labelFontSize: 13
    property int valueFontSize: 18
    property int subFontSize: 13
    property int heightWithSub: 56
    property int heightWithoutSub: 46

    width: 180
    height: sub.length > 0 ? heightWithSub : heightWithoutSub

    Row {
        anchors.fill: parent
        layoutDirection: root.alignRight ? Qt.RightToLeft : Qt.LeftToRight
        spacing: root.rowSpacing

        // ICON
        Image {
            width: root.iconSize
            height: root.iconSize
            source: root.iconSource
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: true
            opacity: 0.9
            y: 2   // aligns with label
        }

        Column {
            width: parent.width - root.iconSize - root.rowSpacing
            spacing: root.textSpacing

            Text {
                text: root.label
                color: "#99ffffff"
                font.pixelSize: root.labelFontSize
                font.letterSpacing: 1.5
                font.capitalization: Font.AllUppercase
                horizontalAlignment: root.alignRight ? Text.AlignRight : Text.AlignLeft
                width: parent.width
                renderType: Text.NativeRendering
            }

            Text {
                text: root.value
                color: "white"
                font.pixelSize: root.valueFontSize
                horizontalAlignment: root.alignRight ? Text.AlignRight : Text.AlignLeft
                width: parent.width
                renderType: Text.NativeRendering
            }

            Text {
                visible: root.sub.length > 0
                text: root.sub
                color: "#80ffffff"
                font.pixelSize: root.subFontSize
                horizontalAlignment: root.alignRight ? Text.AlignRight : Text.AlignLeft
                width: parent.width
                renderType: Text.NativeRendering
            }
        }
    }
}
