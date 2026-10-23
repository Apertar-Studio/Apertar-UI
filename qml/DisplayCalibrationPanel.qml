import QtQuick

Column {
    id: root
    property var calibration: null
    property var popupParent: null
    property var dropdownController: null
    readonly property var values: calibration ? calibration.profile : ({enabled: false, red: 1, green: 1, blue: 1, gamma: 1, black: 0, white: 1})
    width: parent ? parent.width : 400
    spacing: 12

    FontLoader { id: regular; source: "qrc:/qml/fonts/Inter/Inter-Regular.ttf" }

    InlineDropdownField {
        width: parent.width
        label: "Display"
        value: root.calibration ? root.calibration.target : "Built-in"
        options: ["Built-in", "HDMI"]
        popupParent: root.popupParent
        dropdownController: root.dropdownController
        onValueSelected: function(v) { if (root.calibration) root.calibration.target = v }
    }

    Item {
        width: parent.width
        height: Math.max(36, enabledToggle.height)
        Text {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            text: "Apply calibration"
            color: "white"
            font.family: regular.font.family
            font.pixelSize: 14
        }
        TogglePill {
            id: enabledToggle
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            checked: root.values.enabled
            onToggled: function(v) { if (root.calibration) root.calibration.setValue("enabled", v) }
        }
    }

    Column {
        width: parent.width
        spacing: 6
        Row {
            width: parent.width
            height: 36
            Repeater {
                model: 16
                Rectangle { width: parent.width / 16; height: 36; color: Qt.rgba(index / 15, index / 15, index / 15, 1) }
            }
        }
        Row {
            width: parent.width
            height: 24
            Repeater {
                model: ["#ffffff", "#ffff00", "#00ffff", "#00ff00", "#ff00ff", "#ff0000", "#0000ff", "#000000"]
                Rectangle { width: parent.width / 8; height: 24; color: modelData }
            }
        }
    }

    Repeater {
        model: [
            {key: "red", label: "Red", step: 0.01},
            {key: "green", label: "Green", step: 0.01},
            {key: "blue", label: "Blue", step: 0.01},
            {key: "gamma", label: "Gamma", step: 0.05},
            {key: "black", label: "Black Level", step: 0.005},
            {key: "white", label: "White Level", step: 0.01}
        ]
        AdjustNumberTile {
            width: root.width
            label: modelData.label
            value: modelData.key === "gamma" ? Number(root.values[modelData.key]).toFixed(2)
                  : (Number(root.values[modelData.key]) * 100).toFixed(modelData.key === "black" ? 1 : 0) + "%"
            onDecrementRequested: if (root.calibration) root.calibration.setValue(modelData.key, Number(root.values[modelData.key]) - modelData.step)
            onIncrementRequested: if (root.calibration) root.calibration.setValue(modelData.key, Number(root.values[modelData.key]) + modelData.step)
        }
    }

    Rectangle {
        width: parent.width
        height: 44
        radius: 12
        color: resetArea.containsPress ? "#301a80ff" : "#181a80ff"
        border.color: "#661a80ff"
        Text {
            anchors.centerIn: parent
            text: "Reset This Display"
            color: "white"
            font.family: regular.font.family
            font.pixelSize: 14
        }
        MouseArea { id: resetArea; anchors.fill: parent; onClicked: if (root.calibration) root.calibration.resetSelected() }
    }

    Text {
        width: parent.width
        text: "Display only. Recordings and camera white balance are unchanged. Gamma 1.00 is neutral."
        color: "#8cffffff"
        font.family: regular.font.family
        font.pixelSize: 12
        wrapMode: Text.WordWrap
    }
    Text {
        width: parent.width
        text: root.calibration ? root.calibration.status : "Display calibration unavailable"
        color: "#8cffffff"
        font.family: regular.font.family
        font.pixelSize: 12
        wrapMode: Text.WordWrap
    }
}
