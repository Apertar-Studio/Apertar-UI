import QtQuick
import "TimelapseIntervals.js" as Intervals

Rectangle {
    id: root
    objectName: "timelapseIntervalPill"
    property string currentValue: "5s"
    property Item popupParent: null
    property string fontFamily: ""
    property int fontSize: 14
    property int horizontalPadding: 24
    property int dotSize: 6
    property bool interactionAllowed: true
    property bool compact: false
    property bool large: false
    property bool menuOpen: false
    property point menuAnchor: Qt.point(0, 0)
    signal intervalSelected(string value)
    signal menuOpened()

    width: labelRow.implicitWidth + horizontalPadding
    height: 32
    radius: height / 2
    color: "#70000000"
    border.width: 1
    border.color: "#16ffffff"
    opacity: tapArea.pressed ? 0.75 : 1
    enabled: interactionAllowed
    Accessible.role: Accessible.Button
    Accessible.name: "Timelapse interval: " + currentValue
    Accessible.description: "Tap to cycle intervals; hold to select an interval or enter a custom value"
    Accessible.onPressAction: root.cycle()
    onEnabledChanged: if (!enabled) closeMenus()
    onVisibleChanged: if (!visible) closeMenus()

    function closeMenus() {
        menuOpen = false
        customDialog.close()
    }
    function cycle() {
        if (interactionAllowed)
            intervalSelected(Intervals.next(currentValue))
    }
    function choose(value) {
        if (!interactionAllowed) return
        menuOpen = false
        if (value === "Custom...") customDialog.open()
        else intervalSelected(value)
    }
    function openMenu() {
        if (!interactionAllowed || !visible) return
        menuOpened()
        menuAnchor = root.mapToItem(menuOverlay, root.width, root.height)
        menuOpen = true
        Qt.callLater(function() {
            if (!root.menuOpen) return
            var index = Intervals.options(root.currentValue).indexOf(root.currentValue)
            optionList.contentY = Math.max(0, Math.min(optionList.contentHeight - optionList.height,
                                                    index * menuFrame.optionHeight - optionList.height / 2))
        })
    }

    Row {
        id: labelRow
        anchors.centerIn: parent
        spacing: 6
        Rectangle {
            width: root.dotSize; height: width; radius: width / 2
            anchors.verticalCenter: parent.verticalCenter
            color: "#3fd0ff"
        }
        Text {
            text: "INTERVAL " + root.currentValue.toUpperCase()
            color: "white"
            font.family: root.fontFamily
            font.weight: Font.Bold
            font.pixelSize: root.fontSize
            renderType: Text.NativeRendering
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    MouseArea {
        id: tapArea
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        pressAndHoldInterval: 500
        property bool held: false
        onPressed: held = false
        onPressAndHold: { held = true; root.openMenu() }
        onClicked: if (!held) root.cycle()
    }

    Item {
        id: menuOverlay
        objectName: "timelapseIntervalMenu"
        parent: root.popupParent ? root.popupParent : root
        anchors.fill: parent
        z: 5001
        visible: root.menuOpen
        MouseArea { anchors.fill: parent; onPressed: root.menuOpen = false }
        Rectangle {
            id: menuFrame
            readonly property int optionHeight: root.large ? 46 : root.compact ? 38 : 40
            width: Math.min(menuOverlay.width - 24, root.large ? 224 : root.compact ? 180 : 192)
            height: Math.min(menuOptions.implicitHeight + 12, optionHeight * 5 + 12, menuOverlay.height - 24)
            x: Math.max(12, Math.min(menuOverlay.width - width - 12, root.menuAnchor.x - width))
            y: root.menuAnchor.y + 8 + height <= menuOverlay.height - 12 ? root.menuAnchor.y + 8 : 12
            radius: 12
            color: "#f0181818"
            border.color: "#30ffffff"
            Flickable {
                id: optionList
                anchors.fill: parent
                anchors.margins: 6
                clip: true
                contentHeight: menuOptions.implicitHeight
                boundsBehavior: Flickable.StopAtBounds
                Column {
                    id: menuOptions
                    width: parent.width
                    Repeater {
                        model: root.menuOpen ? Intervals.options(root.currentValue) : []
                        delegate: Rectangle {
                            required property string modelData
                            objectName: "timelapseIntervalOption_" + modelData
                            width: menuOptions.width
                            height: menuFrame.optionHeight
                            radius: 8
                            readonly property bool selected: root.currentValue === modelData
                            color: selected ? "#1f2c36" : optionTap.pressed ? "#16ffffff" : "transparent"
                            border.width: selected ? 1 : 0
                            border.color: "#3fd0ff"
                            Text {
                                anchors.left: parent.left; anchors.leftMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData
                                color: parent.selected ? "#3fd0ff" : "white"
                                font.family: root.fontFamily
                                font.pixelSize: root.large ? 20 : root.compact ? 16 : 17
                                renderType: Text.NativeRendering
                            }
                            MouseArea {
                                id: optionTap
                                anchors.fill: parent
                                onClicked: root.choose(modelData)
                            }
                        }
                    }
                }
            }
        }
    }
    TimelapseIntervalDialog {
        id: customDialog
        parent: root.popupParent ? root.popupParent : root
        currentValue: root.currentValue
        fontFamily: root.fontFamily
        onIntervalSelected: function(value) { if (root.interactionAllowed) root.intervalSelected(value) }
    }
}
