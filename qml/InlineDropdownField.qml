import QtQuick

Item {
    id: root

    property string label: ""
    property string value: ""
    property var options: []
    property bool isOpen: false
    property real maxPopupHeight: 220
    property Item popupParent: null
    property var dropdownController: null
    property bool openUpwards: false
    property bool fieldEnabled: true

    property real popupOffsetY: openUpwards ? 6 : -6
    property real popupX: 0
    property real popupY: 0

    signal valueSelected(string value)

    width: 200
    height: 66
    z: 1

    FontLoader { id: interRegular; source: "qrc:/qml/fonts/Inter/Inter-Regular.ttf" }
    FontLoader { id: interMedium; source: "qrc:/qml/fonts/Inter/Inter-Medium.ttf" }

    function updatePopupPosition() {
        var anchorY = openUpwards ? (-popup.height - 8) : (root.height + 8)

        if (root.popupParent) {
            var p = root.mapToItem(root.popupParent, 0, anchorY)
            popupX = p.x
            popupY = p.y + root.popupOffsetY
        } else {
            popupX = 0
            popupY = anchorY + root.popupOffsetY
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: 18
        color: root.fieldEnabled
               ? (openArea.containsPress ? "#18ffffff" : "#40000000")
               : "#1f000000"
        border.width: 1
        border.color: root.fieldEnabled ? "#1affffff" : "#10ffffff"

        Item {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            anchors.topMargin: 12
            anchors.bottomMargin: 12

            Column {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4

                Text {
                    text: root.label
                    color: root.fieldEnabled ? "#66ffffff" : "#42ffffff"
                    font.family: interMedium.font.family
                    font.pixelSize: 11
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 2.2
                    renderType: Text.NativeRendering
                }

                Text {
                    text: root.value
                    color: root.fieldEnabled ? "white" : "#66ffffff"
                    font.family: interMedium.font.family
                    font.pixelSize: 16
                    renderType: Text.NativeRendering
                }
            }

            Text {
                anchors.right: parent.right
                anchors.top: parent.top
                text: "⌄"
                color: root.fieldEnabled ? "#80ffffff" : "#40ffffff"
                font.family: interRegular.font.family
                font.pixelSize: 14
                rotation: root.isOpen ? 180 : 0
                renderType: Text.NativeRendering

                Behavior on rotation {
                    NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
                }
            }
        }

        MouseArea {
            id: openArea
            anchors.fill: parent
            enabled: root.fieldEnabled
            onClicked: {
                if (root.isOpen) {
                    root.isOpen = false
                    if (root.dropdownController && root.dropdownController.activeDropdown === root)
                        root.dropdownController.activeDropdown = null
                } else {
                    if (root.dropdownController)
                        root.dropdownController.activeDropdown = root
                    root.isOpen = true
                }
            }
        }
    }

    Item {
        id: popupWrap

        parent: root.popupParent ? root.popupParent : root
        width: root.width
        height: popup.height
        x: root.popupX
        y: root.popupY
        visible: root.isOpen || openAnim.running || closeAnim.running
        opacity: 0.0
        scale: 0.96
        z: 6000
        transformOrigin: Item.TopLeft

        Rectangle {
            id: popup
            width: root.width
            height: Math.min(optionsColumn.implicitHeight + 12, root.maxPopupHeight)
            radius: 18
            color: "#171717"
            border.width: 1
            border.color: "#22ffffff"
            clip: true

            Flickable {
                id: flick
                anchors.fill: parent
                anchors.margins: 6
                contentWidth: width
                contentHeight: optionsColumn.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.VerticalFlick
                interactive: root.isOpen

                Column {
                    id: optionsColumn
                    width: flick.width
                    spacing: 4

                    Repeater {
                        model: root.options

                        delegate: Rectangle {
                            width: optionsColumn.width
                            height: 42
                            radius: 14
                            color: modelData === root.value ? "#24ffffff" : "transparent"

                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: "white"
                                font.family: interMedium.font.family
                                font.pixelSize: 16
                                renderType: Text.NativeRendering
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    root.valueSelected(modelData)
                                    root.isOpen = false
                                    if (root.dropdownController && root.dropdownController.activeDropdown === root)
                                        root.dropdownController.activeDropdown = null
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Timer {
        interval: 16
        repeat: true
        running: root.isOpen
        onTriggered: root.updatePopupPosition()
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
                duration: 180
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                target: root
                property: "popupOffsetY"
                from: root.openUpwards ? 6 : -6
                to: 0
                duration: 220
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                target: popupWrap
                property: "scale"
                from: popupWrap.scale
                to: 1.0
                duration: 220
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
                duration: 160
                easing.type: Easing.InCubic
            }
            NumberAnimation {
                target: root
                property: "popupOffsetY"
                from: 0
                to: root.openUpwards ? 4 : -4
                duration: 180
                easing.type: Easing.InCubic
            }
            NumberAnimation {
                target: popupWrap
                property: "scale"
                from: popupWrap.scale
                to: 0.94
                duration: 180
                easing.type: Easing.InCubic
            }
        }
    }

    Connections {
        target: root.dropdownController

        function onActiveDropdownChanged() {
            if (root.dropdownController && root.dropdownController.activeDropdown !== root && root.isOpen)
                root.isOpen = false
        }
    }

    onXChanged: updatePopupPosition()
    onYChanged: updatePopupPosition()
    onWidthChanged: updatePopupPosition()
    onHeightChanged: updatePopupPosition()
    onPopupOffsetYChanged: updatePopupPosition()
    onPopupParentChanged: updatePopupPosition()
    onFieldEnabledChanged: {
        if (!root.fieldEnabled && root.isOpen) {
            root.isOpen = false
            if (root.dropdownController && root.dropdownController.activeDropdown === root)
                root.dropdownController.activeDropdown = null
        }
    }

    Component.onCompleted: updatePopupPosition()

    Component.onDestruction: {
        if (root.dropdownController && root.dropdownController.activeDropdown === root)
            root.dropdownController.activeDropdown = null
    }

    onIsOpenChanged: {
        updatePopupPosition()

        if (isOpen) {
            if (closeAnim.running)
                closeAnim.stop()
            popupOffsetY = root.openUpwards ? 6 : -6
            if (!popupWrap.visible) {
                popupWrap.opacity = 0.0
                popupWrap.scale = 0.96
            }
        } else {
            if (openAnim.running)
                openAnim.stop()
            popupOffsetY = root.openUpwards ? 4 : -4
        }
    }
}
