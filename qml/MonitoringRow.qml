import QtQuick

Rectangle {
    id: root

    property string title: ""
    property string titleIconSource: ""
    property string description: ""
    property bool enabled: false
    property bool available: true

    property string choice1Label: ""
    property string choice1Value: ""
    property var choice1Options: []

    property string choice2Label: ""
    property string choice2Value: ""
    property var choice2Options: []

    property Item popupParent: null
    property var dropdownController: null

    signal toggleRequested()
    signal choice1Selected(string value)
    signal choice2Selected(string value)

    width: parent ? parent.width : 400
    height: dropdowns.visible ? 176 : 98
    radius: 22
    color: "#181818"
    opacity: root.available ? 1.0 : 0.42
    border.width: 1
    border.color: "#1affffff"

    Item {
        anchors.fill: parent
        anchors.margins: 20

        Column {
            anchors.left: parent.left
            anchors.right: toggle.left
            anchors.rightMargin: 24
            anchors.top: parent.top
            spacing: 4

            Row {
                width: parent.width
                spacing: 8

                Text {
                    width: Math.min(implicitWidth, parent.width - (root.titleIconSource.length > 0 ? 32 : 0))
                    text: root.title
                    color: "white"
                    font.family: interBold.font.family
                    font.pixelSize: 22
                    renderType: Text.NativeRendering
                    elide: Text.ElideRight
                }

                Image {
                    visible: root.titleIconSource.length > 0
                    source: root.titleIconSource
                    width: 24
                    height: 24
                    anchors.verticalCenter: parent.verticalCenter
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                }
            }

            Text {
                text: root.description
                color: "#8cffffff"
                font.family: interRegular.font.family
                font.pixelSize: 14
                renderType: Text.NativeRendering
                elide: Text.ElideRight
            }
        }

        TogglePill {
            id: toggle
            anchors.right: parent.right
            anchors.top: parent.top
            checked: root.enabled
            onToggled: root.toggleRequested()
        }

        Grid {
            id: dropdowns
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.topMargin: 70
            columns: 2
            rowSpacing: 0
            columnSpacing: 12
            visible: root.choice1Label.length > 0 || root.choice2Label.length > 0

            SettingsChoice {
                id: choice1
                visible: root.choice1Label.length > 0
                label: root.choice1Label
                value: root.choice1Value
                options: root.choice1Options
                popupParent: root.popupParent
                dropdownController: root.dropdownController
                onValueSelected: function(v) {
                    root.choice1Selected(v)
                }
            }

            SettingsChoice {
                id: choice2
                visible: root.choice2Label.length > 0
                label: root.choice2Label
                value: root.choice2Value
                options: root.choice2Options
                popupParent: root.popupParent
                dropdownController: root.dropdownController
                onValueSelected: function(v) {
                    root.choice2Selected(v)
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        visible: !root.available
        enabled: visible
        z: 100
    }

    onAvailableChanged: {
        if (!available && dropdownController) {
            if (dropdownController.activeDropdown === choice1)
                dropdownController.activeDropdown = null
            if (dropdownController.activeDropdown === choice2)
                dropdownController.activeDropdown = null
        }
    }
}
