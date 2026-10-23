import QtQuick

Rectangle {
    id: root

    property string title: ""
    property string titleIconSource: ""
    property string description: ""
    property bool enabled: false
    property bool available: true
    property bool centerToggleVertically: false
    property bool favoriteEnabled: false
    property bool favorited: false
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

    property string choice1Label: ""
    property string choice1Value: ""
    property string choice1DisplayValue: choice1Value
    property var choice1Options: []
    property var choice1OptionDisplayFormatter: null
    property bool choice1Available: true

    property string choice2Label: ""
    property string choice2Value: ""
    property string choice2DisplayValue: choice2Value
    property var choice2Options: []
    property var choice2OptionDisplayFormatter: null
    property bool choice2Available: true

    property string choice3Label: ""
    property string choice3Value: ""
    property string choice3DisplayValue: choice3Value
    property var choice3Options: []
    property var choice3OptionDisplayFormatter: null
    property bool choice3Available: true

    property string choice4Label: ""
    property string choice4Value: ""
    property string choice4DisplayValue: choice4Value
    property var choice4Options: []
    property var choice4OptionDisplayFormatter: null
    property bool choice4Available: true

    property bool secondaryToggleVisible: false
    property bool secondaryToggleChecked: false
    property string secondaryToggleLabel: ""
    property string secondaryToggleIconSource: ""
    property string secondaryToggleDescription: ""
    property bool tertiaryToggleVisible: false
    property bool tertiaryToggleChecked: false
    property bool tertiaryToggleAvailable: true
    property string tertiaryToggleLabel: ""
    property string tertiaryToggleDescription: ""
    property bool quaternaryToggleVisible: false
    property bool quaternaryToggleChecked: false
    property bool quaternaryToggleAvailable: true
    property string quaternaryToggleLabel: ""
    property string quaternaryToggleDescription: ""

    property Item popupParent: null
    property var dropdownController: null

    readonly property int choiceCount: (choice1Label.length > 0 ? 1 : 0)
                                     + (choice2Label.length > 0 ? 1 : 0)
                                     + (choice3Label.length > 0 ? 1 : 0)
                                     + (choice4Label.length > 0 ? 1 : 0)
    readonly property int dropdownRows: Math.max(1, Math.ceil(choiceCount / 2))
    readonly property int cardMargin: largeLandscape ? 26 : (standardLandscape ? 18 : (compact ? 14 : 20))
    readonly property int secondaryToggleTopMargin: largeLandscape ? 82 : (standardLandscape ? 58 : (compact ? 44 : 56))
    readonly property int secondaryToggleHeight: largeLandscape ? 76 : (standardLandscape ? 58 : (compact ? 48 : 58))
    readonly property int secondaryToggleSpacing: largeLandscape ? 22 : (standardLandscape ? 16 : (compact ? 12 : 14))
    readonly property int secondaryExtraHeight: ((secondaryToggleVisible ? 1 : 0) + (tertiaryToggleVisible ? 1 : 0) + (quaternaryToggleVisible ? 1 : 0)) * (secondaryToggleHeight + secondaryToggleSpacing)
    readonly property int dropdownTopMargin: largeLandscape ? 102 : (standardLandscape ? 74 : (compact ? 56 : 70))
    readonly property int dropdownRowSpacing: largeLandscape ? 18 : (standardLandscape ? 12 : (compact ? 8 : 10))
    readonly property int dropdownChoiceHeight: largeLandscape ? 92 : (standardLandscape ? 70 : (compact ? 58 : 72))
    readonly property int dropdownBottomPadding: largeLandscape ? 28 : (standardLandscape ? 20 : (compact ? 14 : 20))
    readonly property int dropdownContentHeight: cardMargin
                                                + dropdownTopMargin
                                                + (dropdownRows * dropdownChoiceHeight)
                                                + ((dropdownRows - 1) * dropdownRowSpacing)
                                                + secondaryExtraHeight
                                                + dropdownBottomPadding

    signal toggleRequested()
    signal choice1Selected(string value)
    signal choice2Selected(string value)
    signal choice3Selected(string value)
    signal choice4Selected(string value)
    signal secondaryToggleRequested(bool checked)
    signal tertiaryToggleRequested(bool checked)
    signal quaternaryToggleRequested(bool checked)
    signal pressAndHoldRequested()
    signal favoriteToggleRequested()

    width: parent ? parent.width : 400
    height: dropdowns.visible
            ? Math.max(root.dropdownContentHeight, root.dropdownRows > 1 ? (root.largeLandscape ? 330 : (root.standardLandscape ? 252 : (root.compact ? 214 : 264))) : (root.largeLandscape ? 226 : (root.standardLandscape ? 172 : (root.compact ? 142 : 176))))
            : (root.largeLandscape ? 140 : (root.standardLandscape ? 104 : (root.compact ? 78 : 98)))
    radius: root.largeLandscape ? 30 : (root.standardLandscape ? 22 : (root.compact ? 18 : 22))
    color: "#181818"
    opacity: root.available ? 1.0 : 0.42
    border.width: 1
    border.color: "#1affffff"

    Item {
        anchors.fill: parent
        anchors.margins: root.cardMargin

        Column {
            id: titleColumn
            anchors.left: parent.left
            anchors.right: root.favoriteEnabled ? favoriteButton.left : toggle.left
            anchors.rightMargin: root.favoriteEnabled ? (root.largeLandscape ? 16 : (root.standardLandscape ? 12 : (root.compact ? 8 : 12))) : (root.largeLandscape ? 34 : (root.standardLandscape ? 24 : (root.compact ? 16 : 24)))
            anchors.top: parent.top
            spacing: root.largeLandscape ? 7 : (root.standardLandscape ? 4 : (root.compact ? 2 : 4))

            Row {
                width: parent.width
                spacing: root.largeLandscape ? 12 : (root.standardLandscape ? 8 : (root.compact ? 6 : 8))

                Text {
                    width: Math.min(implicitWidth, parent.width - (root.titleIconSource.length > 0 ? (root.largeLandscape ? 48 : (root.standardLandscape ? 36 : (root.compact ? 26 : 32))) : 0))
                    text: root.title
                    color: "white"
                    font.family: interBold.font.family
                    font.pixelSize: root.largeLandscape ? 34 : (root.standardLandscape ? 24 : (root.compact ? 18 : 22))
                    renderType: Text.NativeRendering
                    elide: Text.ElideRight
                }

                Image {
                    visible: root.titleIconSource.length > 0
                    source: root.titleIconSource
                    width: root.largeLandscape ? 36 : (root.standardLandscape ? 26 : (root.compact ? 20 : 24))
                    height: root.largeLandscape ? 36 : (root.standardLandscape ? 26 : (root.compact ? 20 : 24))
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
                font.pixelSize: root.largeLandscape ? 19 : (root.standardLandscape ? 14 : (root.compact ? 11 : 14))
                renderType: Text.NativeRendering
                elide: Text.ElideRight
            }
        }

        MouseArea {
            anchors.fill: titleColumn
            acceptedButtons: Qt.LeftButton
            preventStealing: false
            pressAndHoldInterval: 320
            onPressAndHold: root.pressAndHoldRequested()
        }

        Rectangle {
            id: favoriteButton
            visible: root.favoriteEnabled
            width: root.largeLandscape ? 46 : (root.standardLandscape ? 34 : (root.compact ? 28 : 34))
            height: root.largeLandscape ? 46 : (root.standardLandscape ? 34 : (root.compact ? 28 : 34))
            radius: root.largeLandscape ? 16 : (root.standardLandscape ? 12 : (root.compact ? 10 : 12))
            anchors.right: toggle.left
            anchors.rightMargin: root.largeLandscape ? 14 : (root.standardLandscape ? 10 : (root.compact ? 7 : 10))
            anchors.top: root.centerToggleVertically ? undefined : parent.top
            anchors.verticalCenter: root.centerToggleVertically ? parent.verticalCenter : undefined
            color: favoriteButtonArea.containsPress
                   ? (root.favorited ? "#7a6120" : "#20ffffff")
                   : (root.favorited ? "#5f4b16" : "#141414")
            border.width: 1
            border.color: root.favorited ? "#e0b447" : "#1affffff"
            scale: favoriteButtonArea.containsPress ? 0.96 : 1.0

            Behavior on scale {
                NumberAnimation { duration: 120; easing.type: Easing.OutCubic }
            }

            Text {
                anchors.centerIn: parent
                text: root.favorited ? "★" : "☆"
                color: root.favorited ? "#ffd86f" : "#b6b8bd"
                font.family: interMedium.font.family
                font.pixelSize: root.largeLandscape ? 24 : (root.standardLandscape ? 18 : (root.compact ? 15 : 18))
                renderType: Text.NativeRendering
            }

            MouseArea {
                id: favoriteButtonArea
                anchors.fill: parent
                onClicked: root.favoriteToggleRequested()
            }
        }

        TogglePill {
            id: toggle
            anchors.right: parent.right
            anchors.top: root.centerToggleVertically ? undefined : parent.top
            anchors.verticalCenter: root.centerToggleVertically ? parent.verticalCenter : undefined
            checked: root.enabled
            compact: root.compact
            large: root.standardLandscape || root.largeLandscape
            onToggled: root.toggleRequested()
        }

        Rectangle {
            id: secondaryToggleRow
            visible: root.secondaryToggleVisible
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: dropdowns.visible ? dropdowns.bottom : parent.top
            anchors.topMargin: dropdowns.visible ? root.secondaryToggleSpacing : root.secondaryToggleTopMargin
            height: root.secondaryToggleHeight
            radius: root.largeLandscape ? 26 : (root.standardLandscape ? 19 : (root.compact ? 16 : 19))
            color: "#101010"
            border.width: 1
            border.color: root.secondaryToggleChecked ? "#55ffffff" : "#24ffffff"

            Behavior on border.color {
                ColorAnimation { duration: 160 }
            }

            Row {
                anchors.left: parent.left
                anchors.leftMargin: root.largeLandscape ? 22 : (root.standardLandscape ? 17 : (root.compact ? 13 : 15))
                anchors.verticalCenter: parent.verticalCenter
                spacing: root.largeLandscape ? 15 : (root.standardLandscape ? 12 : (root.compact ? 9 : 10))

                Image {
                    visible: root.secondaryToggleIconSource.length > 0
                    source: root.secondaryToggleIconSource
                    width: root.largeLandscape ? 38 : (root.standardLandscape ? 29 : (root.compact ? 23 : 26))
                    height: width
                    anchors.verticalCenter: parent.verticalCenter
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                    opacity: root.secondaryToggleChecked ? 1.0 : 0.74
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: root.largeLandscape ? 4 : (root.compact ? 1 : 3)

                    Text {
                        text: root.secondaryToggleLabel
                        color: "white"
                        font.family: interBold.font.family
                        font.pixelSize: root.largeLandscape ? 28 : (root.standardLandscape ? 21 : (root.compact ? 16 : 18))
                        renderType: Text.NativeRendering
                    }

                    Text {
                        visible: root.secondaryToggleDescription.length > 0
                        text: root.secondaryToggleDescription
                        color: "#70ffffff"
                        font.family: interRegular.font.family
                        font.pixelSize: root.largeLandscape ? 17 : (root.standardLandscape ? 13 : (root.compact ? 10 : 12))
                        renderType: Text.NativeRendering
                    }
                }
            }

            TogglePill {
                anchors.right: parent.right
                anchors.rightMargin: root.largeLandscape ? 20 : (root.standardLandscape ? 15 : (root.compact ? 11 : 13))
                anchors.verticalCenter: parent.verticalCenter
                checked: root.secondaryToggleChecked
                compact: root.compact
                large: root.standardLandscape || root.largeLandscape
                onToggled: function(checked) { root.secondaryToggleRequested(checked) }
            }
        }

        Rectangle {
            id: tertiaryToggleRow
            visible: root.tertiaryToggleVisible
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: root.secondaryToggleVisible ? secondaryToggleRow.bottom : dropdowns.bottom
            anchors.topMargin: root.secondaryToggleSpacing
            height: root.secondaryToggleHeight
            radius: secondaryToggleRow.radius
            color: "#101010"
            border.width: 1
            border.color: root.tertiaryToggleChecked ? "#55ffffff" : "#24ffffff"
            opacity: root.tertiaryToggleAvailable ? 1 : 0.42

            Column {
                anchors.left: parent.left
                anchors.leftMargin: root.largeLandscape ? 22 : (root.standardLandscape ? 17 : (root.compact ? 13 : 15))
                anchors.right: tertiaryToggle.left
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                spacing: root.largeLandscape ? 4 : (root.compact ? 1 : 3)
                Text {
                    width: parent.width
                    text: root.tertiaryToggleLabel
                    color: "white"
                    font.family: interBold.font.family
                    font.pixelSize: root.largeLandscape ? 28 : (root.standardLandscape ? 21 : (root.compact ? 16 : 18))
                    elide: Text.ElideRight
                    renderType: Text.NativeRendering
                }
                Text {
                    width: parent.width
                    visible: text.length > 0
                    text: root.tertiaryToggleDescription
                    color: "#70ffffff"
                    font.family: interRegular.font.family
                    font.pixelSize: root.largeLandscape ? 17 : (root.standardLandscape ? 13 : (root.compact ? 10 : 12))
                    elide: Text.ElideRight
                    renderType: Text.NativeRendering
                }
            }
            TogglePill {
                id: tertiaryToggle
                anchors.right: parent.right
                anchors.rightMargin: root.largeLandscape ? 20 : (root.standardLandscape ? 15 : (root.compact ? 11 : 13))
                anchors.verticalCenter: parent.verticalCenter
                checked: root.tertiaryToggleChecked
                enabled: root.tertiaryToggleAvailable
                compact: root.compact
                large: root.standardLandscape || root.largeLandscape
                onToggled: function(checked) { root.tertiaryToggleRequested(checked) }
            }
        }

        Rectangle {
            visible: root.quaternaryToggleVisible
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: root.tertiaryToggleVisible ? tertiaryToggleRow.bottom : root.secondaryToggleVisible ? secondaryToggleRow.bottom : dropdowns.bottom
            anchors.topMargin: root.secondaryToggleSpacing
            height: root.secondaryToggleHeight
            radius: secondaryToggleRow.radius
            color: "#101010"
            border.width: 1
            border.color: root.quaternaryToggleChecked ? "#55ffffff" : "#24ffffff"
            opacity: root.quaternaryToggleAvailable ? 1 : 0.42

            Column {
                anchors.left: parent.left
                anchors.leftMargin: root.largeLandscape ? 22 : (root.standardLandscape ? 17 : (root.compact ? 13 : 15))
                anchors.right: quaternaryToggle.left
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                spacing: root.largeLandscape ? 4 : (root.compact ? 1 : 3)
                Text {
                    width: parent.width
                    text: root.quaternaryToggleLabel
                    color: "white"
                    font.family: interBold.font.family
                    font.pixelSize: root.largeLandscape ? 28 : (root.standardLandscape ? 21 : (root.compact ? 16 : 18))
                    elide: Text.ElideRight
                    renderType: Text.NativeRendering
                }
                Text {
                    width: parent.width
                    visible: text.length > 0
                    text: root.quaternaryToggleDescription
                    color: "#70ffffff"
                    font.family: interRegular.font.family
                    font.pixelSize: root.largeLandscape ? 17 : (root.standardLandscape ? 13 : (root.compact ? 10 : 12))
                    elide: Text.ElideRight
                    renderType: Text.NativeRendering
                }
            }
            TogglePill {
                id: quaternaryToggle
                anchors.right: parent.right
                anchors.rightMargin: root.largeLandscape ? 20 : (root.standardLandscape ? 15 : (root.compact ? 11 : 13))
                anchors.verticalCenter: parent.verticalCenter
                checked: root.quaternaryToggleChecked
                enabled: root.quaternaryToggleAvailable
                compact: root.compact
                large: root.standardLandscape || root.largeLandscape
                onToggled: function(checked) { root.quaternaryToggleRequested(checked) }
            }
        }

        Grid {
            id: dropdowns
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.topMargin: root.dropdownTopMargin
            columns: 2
            rowSpacing: root.dropdownRowSpacing
            columnSpacing: root.largeLandscape ? 18 : (root.standardLandscape ? 12 : (root.compact ? 8 : 12))
            visible: root.choiceCount > 0

            SettingsChoice {
                id: choice1
                visible: root.choice1Label.length > 0
                label: root.choice1Label
                value: root.choice1Value
                displayValue: root.choice1DisplayValue
                options: root.choice1Options
                optionDisplayFormatter: root.choice1OptionDisplayFormatter
                available: root.choice1Available
                popupParent: root.popupParent
                dropdownController: root.dropdownController
                compact: root.compact
                large: root.standardLandscape || root.largeLandscape
                onValueSelected: function(v) {
                    root.choice1Selected(v)
                }
            }

            SettingsChoice {
                id: choice2
                visible: root.choice2Label.length > 0
                label: root.choice2Label
                value: root.choice2Value
                displayValue: root.choice2DisplayValue
                options: root.choice2Options
                optionDisplayFormatter: root.choice2OptionDisplayFormatter
                available: root.choice2Available
                popupParent: root.popupParent
                dropdownController: root.dropdownController
                compact: root.compact
                large: root.standardLandscape || root.largeLandscape
                onValueSelected: function(v) {
                    root.choice2Selected(v)
                }
            }

            SettingsChoice {
                id: choice3
                visible: root.choice3Label.length > 0
                label: root.choice3Label
                value: root.choice3Value
                displayValue: root.choice3DisplayValue
                options: root.choice3Options
                optionDisplayFormatter: root.choice3OptionDisplayFormatter
                available: root.choice3Available
                popupParent: root.popupParent
                dropdownController: root.dropdownController
                compact: root.compact
                large: root.standardLandscape || root.largeLandscape
                onValueSelected: function(v) {
                    root.choice3Selected(v)
                }
            }

            SettingsChoice {
                id: choice4
                visible: root.choice4Label.length > 0
                label: root.choice4Label
                value: root.choice4Value
                displayValue: root.choice4DisplayValue
                options: root.choice4Options
                optionDisplayFormatter: root.choice4OptionDisplayFormatter
                available: root.choice4Available
                popupParent: root.popupParent
                dropdownController: root.dropdownController
                compact: root.compact
                large: root.standardLandscape || root.largeLandscape
                onValueSelected: function(v) {
                    root.choice4Selected(v)
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
            if (dropdownController.activeDropdown === choice3)
                dropdownController.activeDropdown = null
            if (dropdownController.activeDropdown === choice4)
                dropdownController.activeDropdown = null
        }
    }
}
