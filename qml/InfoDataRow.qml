import QtQuick

Rectangle {
    id: root

    property string label: ""
    property string value: ""
    property string detail: ""

    width: parent ? parent.width : 400
    height: detail.length > 0 ? 84 : 68
    radius: 18
    color: "#171717"
    border.width: 1
    border.color: "#1affffff"

    FontLoader { id: interRegular; source: "qrc:/qml/fonts/Inter/Inter-Regular.ttf" }
    FontLoader { id: interMedium; source: "qrc:/qml/fonts/Inter/Inter-Medium.ttf" }
    FontLoader { id: interBold; source: "qrc:/qml/fonts/Inter/Inter-Bold.ttf" }

    Item {
        anchors.fill: parent
        anchors.margins: 14

        Column {
            anchors.fill: parent
            spacing: 4

            Text {
                text: root.label
                color: "#66ffffff"
                font.family: interMedium.font.family
                font.pixelSize: 11
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 2.0
                renderType: Text.NativeRendering
            }

            Text {
                text: root.value
                color: "white"
                font.family: interBold.font.family
                font.pixelSize: 18
                width: parent.width
                elide: Text.ElideRight
                renderType: Text.NativeRendering
            }

            Text {
                visible: root.detail.length > 0
                text: root.detail
                color: "#8cffffff"
                font.family: interRegular.font.family
                font.pixelSize: 13
                width: parent.width
                wrapMode: Text.WordWrap
                renderType: Text.NativeRendering
            }
        }
    }
}
