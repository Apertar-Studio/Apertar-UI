import QtQuick

Item {
    id: root

    property string label: ""
    property string value: ""
    property string displayValue: value
    property var options: []
    property var optionDisplayFormatter: null
    property bool available: true
    property bool isOpen: false
    property real maxPopupHeight: 220
    property Item popupParent: null
    property var dropdownController: null
    property bool openUpwards: false
    property bool landscapeCompact: typeof displayConfigBridge !== "undefined"
                                    && displayConfigBridge.uiLayout === "landscape_compact"
    property bool mediumLandscape: typeof displayConfigBridge !== "undefined"
                                   && displayConfigBridge.uiLayout === "landscape_medium"
    property bool largeLandscape: typeof displayConfigBridge !== "undefined"
                                  && displayConfigBridge.uiLayout === "landscape_large"
    property bool standardLandscape: typeof displayConfigBridge !== "undefined"
                                     && displayConfigBridge.uiLayout === "landscape"
    property bool regularLandscape: mediumLandscape || standardLandscape || largeLandscape
    property bool compact: landscapeCompact || regularLandscape
    property bool large: standardLandscape || largeLandscape

    property real popupOffsetY: isOpen ? 0 : (openUpwards ? 6 : -6)
    property real popupX: 0
    property real popupY: 0

    signal valueSelected(string value)

    function displayTextForValue(v) {
        if (root.optionDisplayFormatter)
            return root.optionDisplayFormatter(v)
        return v
    }

    width: largeLandscape ? 246 : (large ? 182 : (compact ? 150 : 164))
    height: largeLandscape ? 92 : (large ? 70 : (compact ? 58 : 72))
    opacity: root.available ? 1.0 : 0.38
    z: 1

    function updatePopupPosition() {
        var popupHeight = Math.min(optionsColumn.implicitHeight + 12, root.maxPopupHeight)

        if (root.popupParent) {
            var p = root.mapToItem(root.popupParent, 0, 0)
            var belowY = p.y + root.height + 8
            var aboveY = p.y - popupHeight - 8
            var fitsBelow = belowY + popupHeight <= root.popupParent.height
            var fitsAbove = aboveY >= 0

            root.openUpwards = !fitsBelow && fitsAbove
            popupX = p.x
            popupY = (root.openUpwards ? aboveY : belowY) + root.popupOffsetY
        } else {
            root.openUpwards = false
            popupX = 0
            popupY = root.height + 8 + root.popupOffsetY
        }
    }

    Rectangle {
        id: closedBox
        anchors.fill: parent
        radius: root.largeLandscape ? 24 : (root.large ? 18 : (root.compact ? 14 : 18))
        color: "#171717"
        border.width: 1
        border.color: "#1affffff"

        Item {
            anchors.fill: parent
            anchors.leftMargin: root.largeLandscape ? 21 : (root.large ? 15 : (root.compact ? 11 : 14))
            anchors.rightMargin: root.largeLandscape ? 21 : (root.large ? 15 : (root.compact ? 11 : 14))
            anchors.topMargin: root.largeLandscape ? 14 : (root.large ? 10 : (root.compact ? 8 : 10))
            anchors.bottomMargin: root.largeLandscape ? 14 : (root.large ? 10 : (root.compact ? 8 : 10))

            Column {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                spacing: root.largeLandscape ? 6 : (root.large ? 4 : (root.compact ? 2 : 4))

                Text {
                    text: root.label
                    color: "#66ffffff"
                    font.family: interMedium.font.family
                    font.pixelSize: root.largeLandscape ? 15 : (root.large ? 11 : (root.compact ? 9 : 11))
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: root.largeLandscape ? 3.0 : (root.large ? 2.2 : (root.compact ? 1.8 : 2.2))
                    renderType: Text.NativeRendering
                }

                Text {
                    text: root.displayValue
                    color: "white"
                    font.family: interMedium.font.family
                    font.pixelSize: root.largeLandscape ? 24 : (root.large ? 17 : (root.compact ? 13 : 16))
                    renderType: Text.NativeRendering
                }
            }

            Text {
                anchors.right: parent.right
                anchors.top: parent.top
                text: "⌄"
                color: "#80ffffff"
                font.family: interRegular.font.family
                font.pixelSize: root.largeLandscape ? 21 : (root.large ? 15 : (root.compact ? 12 : 14))
                renderType: Text.NativeRendering
            }
        }

        MouseArea {
            anchors.fill: parent
            enabled: root.available
            onClicked: {
                if (root.isOpen) {
                    root.isOpen = false
                    if (root.dropdownController && root.dropdownController.activeDropdown === root)
                        root.dropdownController.activeDropdown = null
                } else {
                    if (root.dropdownController)
                        root.dropdownController.activeDropdown = root
                    root.updatePopupPosition()
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

        transformOrigin: root.openUpwards ? Item.BottomLeft : Item.TopLeft

        Rectangle {
            id: popup
            width: root.width
            height: Math.min(optionsColumn.implicitHeight + (root.largeLandscape ? 18 : (root.large ? 12 : (root.compact ? 8 : 12))), root.maxPopupHeight)
            radius: root.largeLandscape ? 24 : (root.large ? 18 : (root.compact ? 14 : 18))
            color: "#171717"
            border.width: 1
            border.color: "#22ffffff"
            clip: true

            Flickable {
                id: flick
                anchors.fill: parent
                anchors.margins: root.largeLandscape ? 9 : (root.large ? 6 : (root.compact ? 4 : 6))
                contentWidth: width
                contentHeight: optionsColumn.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.VerticalFlick
                interactive: root.isOpen

                Column {
                    id: optionsColumn
                    width: flick.width
                    spacing: root.largeLandscape ? 6 : (root.large ? 4 : (root.compact ? 3 : 4))

                    Repeater {
                        model: root.options

                        delegate: Rectangle {
                            width: optionsColumn.width
                            height: root.largeLandscape ? 58 : (root.large ? 44 : (root.compact ? 34 : 42))
                            radius: root.largeLandscape ? 18 : (root.large ? 14 : (root.compact ? 10 : 12))
                            color: modelData === root.value ? "#22ffffff" : "transparent"

                            Text {
                                anchors.centerIn: parent
                                text: root.displayTextForValue(modelData)
                                color: "white"
                                font.family: interMedium.font.family
                                font.pixelSize: root.largeLandscape ? 24 : (root.large ? 17 : (root.compact ? 13 : 16))
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

            Rectangle {
                visible: flick.contentHeight > flick.height
                width: 4
                radius: 2
                color: "#80ffffff"
                anchors.right: parent.right
                anchors.rightMargin: 4

                property real travel: flick.height - height
                property real denom: Math.max(1, flick.contentHeight - flick.height)

                height: Math.max(24, (flick.height / flick.contentHeight) * flick.height)
                y: 6 + (flick.contentY / denom) * travel
            }
        }
    }

    Timer {
        interval: 5
        repeat: true
        running: root.isOpen
        onTriggered: root.updatePopupPosition()
    }

    onAvailableChanged: {
        if (!available) {
            root.isOpen = false
            if (root.dropdownController && root.dropdownController.activeDropdown === root)
                root.dropdownController.activeDropdown = null
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
        target: root.dropdownController ? root.dropdownController : null
        ignoreUnknownSignals: true

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
            popupOffsetY = -6
            if (!popupWrap.visible) {
                popupWrap.opacity = 0.0
                popupWrap.scale = 0.96
            }
        } else {
            if (openAnim.running)
                openAnim.stop()
            popupOffsetY = -4
        }
    }
}
