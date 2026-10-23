import QtQuick

Item {
    id: root

    property string label: ""
    property string value: ""
    property string displayValue: value
    property var options: []
    property bool isOpen: false
    property real maxPopupHeight: 220
    property Item popupParent: null
    property var dropdownController: null
    property bool openUpwards: false
    property bool fieldEnabled: true
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

    property real popupOffsetY: openUpwards ? 6 : -6
    property real popupX: 0
    property real popupY: 0

    signal valueSelected(string value)

    width: 200
    height: largeLandscape ? 92 : (large ? 72 : (compact ? 54 : 66))
    z: 1

    FontLoader { id: interRegular; source: "qrc:/qml/fonts/Inter/Inter-Regular.ttf" }
    FontLoader { id: interMedium; source: "qrc:/qml/fonts/Inter/Inter-Medium.ttf" }

    function optionLabel(option) {
        if (option && typeof option === "object") {
            if (option.label !== undefined)
                return String(option.label)
            if (option.text !== undefined)
                return String(option.text)
            if (option.value !== undefined)
                return String(option.value)
        }
        return String(option)
    }

    function optionValue(option) {
        if (option && typeof option === "object" && option.value !== undefined)
            return String(option.value)
        return String(option)
    }

    function optionEnabled(option) {
        if (option && typeof option === "object" && option.enabled !== undefined)
            return !!option.enabled
        return true
    }

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
        radius: root.largeLandscape ? 24 : (root.large ? 18 : (root.compact ? 14 : 18))
        color: root.fieldEnabled
               ? (openArea.containsPress ? "#18ffffff" : "#40000000")
               : "#1f000000"
        border.width: 1
        border.color: root.fieldEnabled ? "#1affffff" : "#10ffffff"

        Item {
            anchors.fill: parent
            anchors.leftMargin: root.largeLandscape ? 22 : (root.large ? 16 : (root.compact ? 12 : 16))
            anchors.rightMargin: root.largeLandscape ? 22 : (root.large ? 16 : (root.compact ? 12 : 16))
            anchors.topMargin: root.largeLandscape ? 14 : (root.large ? 11 : (root.compact ? 8 : 12))
            anchors.bottomMargin: root.largeLandscape ? 14 : (root.large ? 11 : (root.compact ? 8 : 12))

            Column {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                spacing: root.largeLandscape ? 6 : (root.large ? 4 : (root.compact ? 2 : 4))

                Text {
                    text: root.label
                    color: root.fieldEnabled ? "#66ffffff" : "#42ffffff"
                    font.family: interMedium.font.family
                    font.pixelSize: root.largeLandscape ? 15 : (root.large ? 11 : (root.compact ? 9 : 11))
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: root.largeLandscape ? 3.0 : (root.large ? 2.2 : (root.compact ? 1.8 : 2.2))
                    renderType: Text.NativeRendering
                }

                Text {
                    text: root.displayValue
                    color: root.fieldEnabled ? "white" : "#66ffffff"
                    font.family: interMedium.font.family
                    font.pixelSize: root.largeLandscape ? 24 : (root.large ? 18 : (root.compact ? 13 : 16))
                    renderType: Text.NativeRendering
                }
            }

            Text {
                anchors.right: parent.right
                anchors.top: parent.top
                text: "⌄"
                color: root.fieldEnabled ? "#80ffffff" : "#40ffffff"
                font.family: interRegular.font.family
                font.pixelSize: root.largeLandscape ? 22 : (root.large ? 16 : (root.compact ? 12 : 14))
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
                            radius: root.largeLandscape ? 18 : (root.large ? 14 : (root.compact ? 11 : 14))
                            readonly property string itemValue: root.optionValue(modelData)
                            readonly property string itemLabel: root.optionLabel(modelData)
                            readonly property bool itemEnabled: root.optionEnabled(modelData)
                            color: itemValue === root.value ? "#24ffffff" : "transparent"

                            Text {
                                anchors.centerIn: parent
                                text: parent.itemLabel
                                color: parent.itemEnabled ? "white" : "#52ffffff"
                                font.family: interMedium.font.family
                                font.pixelSize: root.largeLandscape ? 24 : (root.large ? 17 : (root.compact ? 13 : 16))
                                renderType: Text.NativeRendering
                            }

                            MouseArea {
                                anchors.fill: parent
                                enabled: parent.itemEnabled
                                onClicked: {
                                    root.valueSelected(parent.itemValue)
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
