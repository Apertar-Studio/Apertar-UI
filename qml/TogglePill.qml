import QtQuick

Rectangle {
    id: root

    property bool checked: false
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
    signal toggled(bool value)

    width: largeLandscape ? 82 : (large ? 62 : (regularLandscape ? 52 : (compact ? 44 : 52)))
    height: largeLandscape ? 42 : (large ? 32 : (regularLandscape ? 28 : (compact ? 24 : 28)))
    radius: height / 2

    // Background
    color: checked ? "white" : "#40ffffff"
    border.width: 1
    border.color: checked ? "#ffffff" : "#33ffffff"

    // smooth transition
    Behavior on color {
        ColorAnimation { duration: 180 }
    }

    Rectangle {
        id: knob
        width: root.largeLandscape ? 34 : (root.large ? 26 : (root.regularLandscape ? 22 : (root.compact ? 18 : 22)))
        height: root.largeLandscape ? 34 : (root.large ? 26 : (root.regularLandscape ? 22 : (root.compact ? 18 : 22)))
        radius: width / 2
        anchors.verticalCenter: parent.verticalCenter

        // Knob color
        color: root.checked ? "#000000" : "white"

        // position animation
        x: root.checked ? (parent.width - width - 3) : 3

        Behavior on x {
            NumberAnimation {
                duration: 200
                easing.type: Easing.OutCubic
            }
        }

        Behavior on color {
            ColorAnimation { duration: 180 }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            root.toggled(!root.checked)
        }
    }
}
