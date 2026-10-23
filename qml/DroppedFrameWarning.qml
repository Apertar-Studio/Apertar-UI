import QtQuick

Rectangle {
    id: root
    width: height
    height: 30
    radius: height / 2
    color: "#e63b2a13"
    border.width: 1
    border.color: "#bfaa7937"

    Accessible.role: Accessible.StaticText
    Accessible.name: "Dropped frames detected"

    FontLoader { id: interBold; source: "qrc:/qml/fonts/Inter/Inter-Bold.ttf" }

    Text {
        anchors.centerIn: parent
        text: "!"
        color: "#ffd27a"
        font.family: interBold.font.family
        font.weight: Font.Bold
        font.pixelSize: Math.round(root.height * 0.68)
        renderType: Text.NativeRendering
    }
}
