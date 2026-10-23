import QtQuick

Item {
    id: root

    FontLoader { id: interRegular; source: "qrc:/qml/fonts/Inter/Inter-Regular.ttf" }

    property string value: ""
    property bool isOpen: false
    property var options: []
    property real maxPopupHeight: 340
    property int valueFontSize: 25
    property real backgroundOpacity: 1.0
    property int popupMargin: 8
    property int popupSpacing: 6
    property int popupOptionHeight: 36
    property int popupOptionRadius: 12
    property int popupOptionFontSize: 34
    property int popupRadius: 18
    property int popupTopMargin: 8

    signal toggleRequested()
    signal optionSelected(string value)

    property bool popupVisible: isOpen || openAnim.running || closeAnim.running

    z: popupVisible ? 1000 : 0

    Text {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.bottomMargin: 11
        text: root.value
        color: "white"
        font.family: interRegular.font.family
        font.pixelSize: root.valueFontSize
        renderType: Text.NativeRendering
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.toggleRequested()
    }

    Item {
        id: popupWrap
        anchors.top: parent.bottom
        anchors.topMargin: root.popupTopMargin
        anchors.left: parent.left
        width: parent.width
        height: popup.height

        visible: root.popupVisible
        opacity: 0.0
        y: -6
        scale: 0.985
        z: 2000

        transformOrigin: Item.Top

        Rectangle {
            id: popup
            width: root.width
            radius: root.popupRadius
            color: root.fadedControlColor("#c0141414")
            border.width: 1
            border.color: root.fadedControlColor("#20ffffff")
            clip: true

            height: Math.min(flick.contentHeight + (root.popupMargin * 2), root.maxPopupHeight)

            Flickable {
                id: flick
                anchors.fill: parent
                anchors.margins: root.popupMargin
                contentWidth: width
                contentHeight: optionsColumn.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.VerticalFlick
                interactive: root.isOpen

                Column {
                    id: optionsColumn
                    width: flick.width
                    spacing: root.popupSpacing

                    Repeater {
                        // Keep closed dropdown delegates out of the startup
                        // object tree. popupVisible remains true through the
                        // close animation, so existing motion is preserved.
                        model: root.popupVisible ? root.options : []

                        delegate: Rectangle {
                            width: optionsColumn.width
                            height: root.popupOptionHeight
                            radius: root.popupOptionRadius
                            color: root.fadedControlColor(modelData === root.value ? "#22ffffff" : "transparent")

                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: "white"
                                font.family: interRegular.font.family
                                font.pixelSize: root.popupOptionFontSize
                                renderType: Text.NativeRendering
                            }

                            MouseArea {
                                anchors.fill: parent
                                enabled: root.isOpen
                                onClicked: root.optionSelected(modelData)
                            }
                        }
                    }
                }
            }

            Rectangle {
                visible: flick.contentHeight > flick.height
                width: 4
                radius: 2
                color: root.fadedControlColor("#80ffffff")
                anchors.right: parent.right
                anchors.rightMargin: 4

                property real travel: flick.height - height
                property real denom: Math.max(1, flick.contentHeight - flick.height)

                height: Math.max(24, (flick.height / flick.contentHeight) * flick.height)
                y: 8 + (flick.contentY / denom) * travel
            }
        }
    }

    SequentialAnimation {
        id: openAnim
        running: root.isOpen && !closeAnim.running

        ParallelAnimation {
            NumberAnimation {
                target: popupWrap
                property: "opacity"
                from: popupWrap.opacity
                to: 1.0
                duration: 270
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                target: popupWrap
                property: "y"
                from: popupWrap.y
                to: 0
                duration: 300
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                target: popupWrap
                property: "scale"
                from: popupWrap.scale
                to: 1.0
                duration: 300
                easing.type: Easing.OutCubic
            }
        }
    }

    SequentialAnimation {
        id: closeAnim
        running: !root.isOpen && popupWrap.visible && !openAnim.running

        ParallelAnimation {
            NumberAnimation {
                target: popupWrap
                property: "opacity"
                from: popupWrap.opacity
                to: 0.0
                duration: 280
                easing.type: Easing.InCubic
            }
            NumberAnimation {
                target: popupWrap
                property: "y"
                from: popupWrap.y
                to: -4
                duration: 290
                easing.type: Easing.InCubic
            }
            NumberAnimation {
                target: popupWrap
                property: "scale"
                from: popupWrap.scale
                to: 0.90
                duration: 290
                easing.type: Easing.InCubic
            }
        }
    }

    function fadedControlColor(colorValue) {
        var text = String(colorValue)
        if (text === "transparent")
            return "transparent"

        if (text.length > 0 && text[0] === "#") {
            var hex = text.slice(1)
            var alpha = 1.0
            var offset = 0

            if (hex.length === 8) {
                alpha = parseInt(hex.slice(0, 2), 16) / 255.0
                offset = 2
            } else if (hex.length !== 6) {
                return colorValue
            }

            var r = parseInt(hex.slice(offset, offset + 2), 16) / 255.0
            var g = parseInt(hex.slice(offset + 2, offset + 4), 16) / 255.0
            var b = parseInt(hex.slice(offset + 4, offset + 6), 16) / 255.0
            return Qt.rgba(r, g, b, Math.max(0.0, Math.min(1.0, alpha * root.backgroundOpacity)))
        }

        return colorValue
    }

    onIsOpenChanged: {
        if (isOpen) {
            if (closeAnim.running)
                closeAnim.stop()
            if (!popupWrap.visible) {
                popupWrap.opacity = 0.0
                popupWrap.y = -6
                popupWrap.scale = 0.7
            }
        } else {
            if (openAnim.running)
                openAnim.stop()
        }
    }
}
