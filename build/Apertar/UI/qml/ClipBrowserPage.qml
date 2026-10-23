import QtQuick
import QtQuick.Controls

Item {
    id: root

    property bool stillMode: false
    property var settingsState
    property string selectedClipPath: ""
    property var selectedClipPaths: []
    property bool deleteConfirmOpen: false
    property string displayLayout: "square"
    readonly property bool selectionMode: selectedClipPaths.length > 0
    readonly property bool landscapeCompactLayout: displayLayout === "landscape_compact"
    readonly property bool mediumLandscapeLayout: displayLayout === "landscape_medium"
    readonly property bool largeLandscapeLayout: displayLayout === "landscape_large"
    readonly property bool standardLandscapeLayout: displayLayout === "landscape"
                                                   || largeLandscapeLayout
    readonly property bool regularLandscapeLayout: mediumLandscapeLayout
                                                   || standardLandscapeLayout
    readonly property bool compactLandscapeLayout: landscapeCompactLayout || regularLandscapeLayout
    readonly property int gridColumnCount: standardLandscapeLayout ? 4 : (regularLandscapeLayout ? 3 : 2)
    readonly property int gridColumnGap: largeLandscapeLayout ? 16 : (compactLandscapeLayout ? 10 : 14)
    readonly property int clipCellHeight: largeLandscapeLayout ? 354 : (compactLandscapeLayout ? 218 : 246)
    readonly property int clipCardMargin: largeLandscapeLayout ? 7 : (compactLandscapeLayout ? 4 : 5)
    readonly property int clipCardRadius: largeLandscapeLayout ? 30 : (compactLandscapeLayout ? 22 : 24)
    readonly property int clipThumbnailInset: largeLandscapeLayout ? 16 : 12
    readonly property int clipThumbnailTopMargin: largeLandscapeLayout ? 16 : (compactLandscapeLayout ? 10 : 12)
    readonly property int clipThumbnailHeight: largeLandscapeLayout ? 206 : (compactLandscapeLayout ? 112 : 122)
    readonly property int clipThumbnailFooterGap: largeLandscapeLayout ? 18 : (compactLandscapeLayout ? 12 : 14)
    readonly property int clipThumbnailRadius: largeLandscapeLayout ? 23 : (compactLandscapeLayout ? 16 : 18)
    readonly property int clipFooterSideMargin: largeLandscapeLayout ? 18 : 14
    readonly property int clipFooterBottomMargin: largeLandscapeLayout ? 28 : (compactLandscapeLayout ? 18 : 24)
    readonly property int clipFooterHeight: largeLandscapeLayout ? 76 : (compactLandscapeLayout ? 54 : 58)
    readonly property int clipNameFontSize: largeLandscapeLayout ? 22 : (compactLandscapeLayout ? 15 : 17)
    readonly property int clipTypeFontSize: largeLandscapeLayout ? 16 : (compactLandscapeLayout ? 11 : 13)
    readonly property int clipDateFontSize: largeLandscapeLayout ? 14 : (compactLandscapeLayout ? 10 : 11)
    readonly property int clipDurationHeight: largeLandscapeLayout ? 34 : (compactLandscapeLayout ? 24 : 28)
    readonly property int clipDurationRadius: largeLandscapeLayout ? 17 : (compactLandscapeLayout ? 12 : 14)
    readonly property int clipDurationFontSize: largeLandscapeLayout ? 13 : (compactLandscapeLayout ? 10 : 11)
    readonly property int clipSelectionBadgeSize: largeLandscapeLayout ? 34 : (compactLandscapeLayout ? 24 : 28)
    readonly property int clipSelectionBadgeFontSize: largeLandscapeLayout ? 18 : (compactLandscapeLayout ? 13 : 15)
    readonly property int browserTopButtonHeight: largeLandscapeLayout ? 72 : (standardLandscapeLayout ? 60 : (mediumLandscapeLayout ? 52 : (compactLandscapeLayout ? 40 : 54)))
    readonly property int browserTopButtonRadius: largeLandscapeLayout ? 24 : (standardLandscapeLayout ? 20 : (mediumLandscapeLayout ? 18 : (compactLandscapeLayout ? 15 : 18)))
    readonly property int browserTopButtonMargin: largeLandscapeLayout ? 30 : (standardLandscapeLayout ? 22 : (compactLandscapeLayout ? 22 : 22))
    readonly property int browserTopButtonGap: largeLandscapeLayout ? 16 : (standardLandscapeLayout ? 12 : (mediumLandscapeLayout ? 9 : (compactLandscapeLayout ? 8 : 12)))
    readonly property int browserBackButtonWidth: largeLandscapeLayout ? 158 : (standardLandscapeLayout ? 132 : (mediumLandscapeLayout ? 116 : (compactLandscapeLayout ? 90 : 112)))
    readonly property int browserModeButtonWidth: largeLandscapeLayout ? 178 : (standardLandscapeLayout ? 148 : (mediumLandscapeLayout ? 128 : (compactLandscapeLayout ? 94 : 136)))
    readonly property int browserRefreshButtonWidth: largeLandscapeLayout ? 254 : (standardLandscapeLayout ? 204 : (mediumLandscapeLayout ? 176 : (compactLandscapeLayout ? 136 : 166)))

    signal backRequested()
    signal clipOpened(string clipPath, string clipName, int frameCount, int clipIndex)

    property bool browserReady: false

    Component.onCompleted: {
        browserReady = true
        root.updateBrowserActive()
    }
    Component.onDestruction: clipModel.browserOpen = false
    onVisibleChanged: root.updateBrowserActive()

    onStillModeChanged: {
        root.clearSelection()
        if (cdngThumbProvider)
            cdngThumbProvider.clearCache()
    }

    onEnabledChanged: root.updateBrowserActive()

    function updateBrowserActive() {
        if (!browserReady)
            return
        thumbnailCacheReleaseTimer.stop()
        if (cdngThumbProvider)
            cdngThumbProvider.clearCache()
        root.clearSelection()
        clipModel.browserOpen = root.visible && root.enabled
        if (!clipModel.browserOpen)
            thumbnailCacheReleaseTimer.restart()
    }

    function isClipSelected(path) {
        return selectedClipPaths.indexOf(path) >= 0
    }

    function selectOnlyClip(path) {
        selectedClipPaths = [path]
    }

    function toggleClipSelection(path) {
        var updated = selectedClipPaths.slice(0)
        var existingIndex = updated.indexOf(path)
        if (existingIndex >= 0)
            updated.splice(existingIndex, 1)
        else
            updated.push(path)
        selectedClipPaths = updated
        if (updated.length === 0)
            deleteConfirmOpen = false
    }

    function clearSelection() {
        selectedClipPaths = []
        deleteConfirmOpen = false
    }

    function removeSelectedClips() {
        var pathsToRemove = selectedClipPaths.slice(0)
        for (var i = 0; i < pathsToRemove.length; ++i)
            clipModel.removeClip(pathsToRemove[i])
        clearSelection()
    }

    function toggleCaptureMode() {
        root.clearSelection()
        if (settingsState)
            settingsState.photoModeEnabled = !settingsState.photoModeEnabled
    }

    function toggleSortMode() {
        root.clearSelection()
        clipModel.sortMode = clipModel.sortMode === "Newest First"
                           ? "Oldest First"
                           : "Newest First"
    }

    function thumbnailHeightForCell(cellWidth) {
        var imageWidth = Math.max(2, cellWidth - (root.clipCardMargin * 2) - (root.clipThumbnailInset * 2))
        return Math.round(imageWidth * 9 / 16)
    }

    function clipCellHeightForCell(cellWidth) {
        return Math.round(root.clipCardMargin * 2
                          + root.clipThumbnailTopMargin
                          + root.thumbnailHeightForCell(cellWidth)
                          + root.clipThumbnailFooterGap
                          + root.clipFooterHeight
                          + root.clipFooterBottomMargin)
    }

    FontLoader { id: interRegular; source: "qrc:/qml/fonts/Inter/Inter-Regular.ttf" }
    FontLoader { id: interMedium;  source: "qrc:/qml/fonts/Inter/Inter-Medium.ttf" }
    FontLoader { id: interBold;    source: "qrc:/qml/fonts/Inter/Inter-Bold.ttf" }

    Timer {
        id: thumbnailCacheReleaseTimer
        interval: 750
        repeat: false
        onTriggered: {
            if ((!root.enabled || !root.visible) && cdngThumbProvider)
                cdngThumbProvider.clearCache()
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#000000"
    }

    Column {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: root.largeLandscapeLayout ? 32 : 24
        anchors.topMargin: root.largeLandscapeLayout ? 28 : (root.standardLandscapeLayout ? 22 : (root.compactLandscapeLayout ? 20 : 22))
        spacing: root.largeLandscapeLayout ? 4 : (root.standardLandscapeLayout ? 3 : (root.compactLandscapeLayout ? 2 : 4))

        Text {
            text: "PLAYBACK"
            color: "#66ffffff"
            font.family: interMedium.font.family
            font.pixelSize: root.largeLandscapeLayout ? 15 : (root.standardLandscapeLayout ? 12 : (root.compactLandscapeLayout ? 9 : 12))
            font.capitalization: Font.AllUppercase
            font.letterSpacing: 3
            renderType: Text.NativeRendering
        }

        Text {
            text: root.stillMode ? "Still Browser" : "Clip Browser"
            color: "white"
            font.family: interBold.font.family
            font.pixelSize: root.largeLandscapeLayout ? 46 : (root.standardLandscapeLayout ? 36 : (root.compactLandscapeLayout ? 26 : 34))
            renderType: Text.NativeRendering
        }
    }

    Rectangle {
        id: modeButton
        anchors.right: backButton.left
        anchors.top: parent.top
        anchors.rightMargin: root.browserTopButtonGap
        anchors.topMargin: root.browserTopButtonMargin
        width: root.browserModeButtonWidth
        height: root.browserTopButtonHeight
        radius: root.browserTopButtonRadius
        color: root.stillMode
               ? (modeArea.containsPress ? "#22323c" : "#1f2c36")
               : (modeArea.containsPress ? "#20ffffff" : "#14ffffff")
        border.width: 1
        border.color: root.stillMode ? "#3fd0ff" : "#1affffff"
        scale: modeArea.containsPress ? 0.985 : 1.0

        Behavior on scale {
            NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
        }

        Behavior on color {
            ColorAnimation { duration: 150; easing.type: Easing.OutCubic }
        }

        Behavior on border.color {
            ColorAnimation { duration: 180; easing.type: Easing.OutCubic }
        }

        Column {
            anchors.centerIn: parent
            spacing: root.largeLandscapeLayout ? 3 : (root.standardLandscapeLayout ? 2 : (root.mediumLandscapeLayout ? 2 : (root.compactLandscapeLayout ? 1 : 2)))

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Capture"
                color: root.stillMode ? "#79ddff" : "#8f9096"
                font.family: interMedium.font.family
                font.pixelSize: root.largeLandscapeLayout ? 13 : (root.standardLandscapeLayout ? 11 : (root.mediumLandscapeLayout ? 10 : (root.compactLandscapeLayout ? 8 : 10)))
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 2.1
                renderType: Text.NativeRendering
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.stillMode ? "Still Mode" : "Clip Mode"
                color: "white"
                font.family: interMedium.font.family
                font.pixelSize: root.largeLandscapeLayout ? 21 : (root.standardLandscapeLayout ? 17 : (root.mediumLandscapeLayout ? 15 : (root.compactLandscapeLayout ? 13 : 15)))
                renderType: Text.NativeRendering
            }
        }

        MouseArea {
            id: modeArea
            anchors.fill: parent
            enabled: !!settingsState && !clipModel.loading
            onClicked: root.toggleCaptureMode()
        }
    }

    Rectangle {
        id: refreshButton
        anchors.right: modeButton.left
        anchors.top: parent.top
        anchors.rightMargin: root.browserTopButtonGap
        anchors.topMargin: root.browserTopButtonMargin
        width: root.browserRefreshButtonWidth
        height: root.browserTopButtonHeight
        radius: root.browserTopButtonRadius
        color: refreshArea.containsPress ? "#20ffffff" : "#14ffffff"
        border.width: 1
        border.color: "#1affffff"
        scale: refreshArea.containsPress ? 0.985 : 1.0

        Behavior on scale {
            NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
        }

        Behavior on color {
            ColorAnimation { duration: 150; easing.type: Easing.OutCubic }
        }

        Row {
            anchors.centerIn: parent
            spacing: root.largeLandscapeLayout ? 11 : (root.standardLandscapeLayout ? 9 : (root.mediumLandscapeLayout ? 7 : (root.compactLandscapeLayout ? 6 : 8)))

            Image {
                width: root.largeLandscapeLayout ? 25 : (root.standardLandscapeLayout ? 20 : (root.mediumLandscapeLayout ? 18 : (root.compactLandscapeLayout ? 14 : 18)))
                height: root.largeLandscapeLayout ? 25 : (root.standardLandscapeLayout ? 20 : (root.mediumLandscapeLayout ? 18 : (root.compactLandscapeLayout ? 14 : 18)))
                anchors.verticalCenter: parent.verticalCenter
                source: "qrc:/qml/icons/refresh.png"
                fillMode: Image.PreserveAspectFit
                smooth: true
                mipmap: true
                opacity: refreshArea.enabled ? 0.92 : 0.48
            }

            Text {
                text: clipModel.loading ? "Scanning..." : "Refresh Media"
                color: refreshArea.enabled ? "white" : "#8a8a8f"
                font.family: interMedium.font.family
                font.pixelSize: root.largeLandscapeLayout ? 21 : (root.standardLandscapeLayout ? 17 : (root.mediumLandscapeLayout ? 16 : (root.compactLandscapeLayout ? 13 : 16)))
                renderType: Text.NativeRendering
            }
        }

        MouseArea {
            id: refreshArea
            anchors.fill: parent
            enabled: !clipModel.loading
            onClicked: {
                root.clearSelection()
                clipModel.refresh()
            }
        }
    }

    Rectangle {
        id: backButton
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: root.largeLandscapeLayout ? 32 : 24
        anchors.topMargin: root.browserTopButtonMargin
        width: root.browserBackButtonWidth
        height: root.browserTopButtonHeight
        radius: root.browserTopButtonRadius
        color: backArea.containsPress ? "#20ffffff" : "#14ffffff"
        border.width: 1
        border.color: "#1affffff"
        scale: backArea.containsPress ? 0.985 : 1.0

        Behavior on scale {
            NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
        }

        Behavior on color {
            ColorAnimation { duration: 150; easing.type: Easing.OutCubic }
        }

        Row {
            anchors.centerIn: parent
            spacing: root.largeLandscapeLayout ? 11 : (root.standardLandscapeLayout ? 9 : (root.mediumLandscapeLayout ? 7 : (root.compactLandscapeLayout ? 6 : 8)))

            Text {
                text: "←"
                color: "white"
                font.family: interMedium.font.family
                font.pixelSize: root.largeLandscapeLayout ? 25 : (root.standardLandscapeLayout ? 20 : (root.mediumLandscapeLayout ? 18 : (root.compactLandscapeLayout ? 16 : 18)))
                renderType: Text.NativeRendering
            }

            Text {
                text: root.selectionMode ? "Cancel" : "Back"
                color: "white"
                font.family: interMedium.font.family
                font.pixelSize: root.largeLandscapeLayout ? 21 : (root.standardLandscapeLayout ? 17 : (root.mediumLandscapeLayout ? 16 : (root.compactLandscapeLayout ? 13 : 16)))
                renderType: Text.NativeRendering
            }
        }

        MouseArea {
            id: backArea
            anchors.fill: parent
            onClicked: {
                if (root.selectionMode)
                    root.clearSelection()
                else
                    root.backRequested()
            }
        }
    }

    Rectangle {
        id: browserPanel
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: root.largeLandscapeLayout ? 32 : 24
        anchors.topMargin: root.largeLandscapeLayout ? 124 : (root.standardLandscapeLayout ? 100 : (root.mediumLandscapeLayout ? 90 : (root.compactLandscapeLayout ? 82 : 112)))
        anchors.bottomMargin: root.compactLandscapeLayout ? 0 : -32
        radius: root.largeLandscapeLayout ? 34 : (root.compactLandscapeLayout ? 24 : 28)
        color: "#151515"
        border.width: 1
        border.color: "#1affffff"

        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: root.compactLandscapeLayout ? 23 : 27
            color: "#151515"
            visible: false
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: browserPanel.radius + (root.compactLandscapeLayout ? 30 : 40)
            color: "#151515"
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: browserPanel.radius + (root.compactLandscapeLayout ? 18 : 24)
            color: "#151515"
        }

        Row {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: root.largeLandscapeLayout ? 36 : (root.standardLandscapeLayout ? 28 : (root.compactLandscapeLayout ? 22 : 24))
            anchors.topMargin: root.largeLandscapeLayout ? 28 : (root.standardLandscapeLayout ? 22 : (root.compactLandscapeLayout ? 16 : 20))
            spacing: root.largeLandscapeLayout ? 15 : (root.standardLandscapeLayout ? 12 : (root.compactLandscapeLayout ? 8 : 10))

            Image {
                width: root.largeLandscapeLayout ? 38 : (root.standardLandscapeLayout ? 30 : (root.compactLandscapeLayout ? 20 : 24))
                height: root.largeLandscapeLayout ? 38 : (root.standardLandscapeLayout ? 30 : (root.compactLandscapeLayout ? 20 : 24))
                anchors.verticalCenter: parent.verticalCenter
                source: "qrc:/qml/icons/film.png"
                fillMode: Image.PreserveAspectFit
                smooth: true
                mipmap: true
                opacity: 1.0
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.selectionMode ? "SELECTION" : (root.stillMode ? "STILLS" : "CLIPS")
                color: "white"
                font.family: interMedium.font.family
                font.pixelSize: root.largeLandscapeLayout ? 20 : (root.standardLandscapeLayout ? 16 : (root.compactLandscapeLayout ? 11 : 13))
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 3
                renderType: Text.NativeRendering
            }
        }

        Rectangle {
            id: removeSelectedButton
            anchors.right: selectionCountChip.left
            anchors.top: parent.top
            anchors.rightMargin: root.largeLandscapeLayout ? 16 : (root.standardLandscapeLayout ? 12 : (root.compactLandscapeLayout ? 8 : 10))
            anchors.topMargin: root.largeLandscapeLayout ? 22 : (root.standardLandscapeLayout ? 16 : (root.compactLandscapeLayout ? 12 : 14))
            width: root.largeLandscapeLayout ? 250 : (root.standardLandscapeLayout ? 204 : (root.compactLandscapeLayout ? 154 : 178))
            height: root.largeLandscapeLayout ? 56 : (root.standardLandscapeLayout ? 46 : (root.compactLandscapeLayout ? 32 : 40))
            radius: root.largeLandscapeLayout ? 22 : (root.standardLandscapeLayout ? 18 : (root.compactLandscapeLayout ? 14 : 16))
            color: removeSelectedArea.containsPress ? "#a32828" : "#8d2020"
            border.width: 1
            border.color: "#ba4a4a"
            visible: root.selectionMode
            opacity: root.selectionMode ? 1.0 : 0.0
            scale: removeSelectedArea.containsPress ? 0.985 : 1.0

            Behavior on opacity {
                NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
            }

            Behavior on scale {
                NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
            }

            Row {
                anchors.centerIn: parent
                spacing: root.largeLandscapeLayout ? 11 : (root.standardLandscapeLayout ? 9 : (root.compactLandscapeLayout ? 6 : 8))

                Image {
                    width: root.largeLandscapeLayout ? 23 : (root.standardLandscapeLayout ? 19 : (root.compactLandscapeLayout ? 13 : 16))
                    height: root.largeLandscapeLayout ? 23 : (root.standardLandscapeLayout ? 19 : (root.compactLandscapeLayout ? 13 : 16))
                    anchors.verticalCenter: parent.verticalCenter
                    source: "qrc:/qml/icons/delete.png"
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                    opacity: 0.92
                }

                Text {
                    text: "Remove Selected"
                    color: "white"
                    font.family: interMedium.font.family
                    font.pixelSize: root.largeLandscapeLayout ? 19 : (root.standardLandscapeLayout ? 16 : (root.compactLandscapeLayout ? 12 : 14))
                    renderType: Text.NativeRendering
                }
            }

            MouseArea {
                id: removeSelectedArea
                anchors.fill: parent
                onClicked: root.deleteConfirmOpen = true
            }
        }

        Rectangle {
            id: sortModeChip
            anchors.right: selectionCountChip.left
            anchors.top: parent.top
            anchors.rightMargin: root.largeLandscapeLayout ? 16 : (root.standardLandscapeLayout ? 12 : (root.compactLandscapeLayout ? 8 : 10))
            anchors.topMargin: root.largeLandscapeLayout ? 22 : (root.standardLandscapeLayout ? 16 : (root.compactLandscapeLayout ? 12 : 14))
            width: sortModeLabel.width + (root.largeLandscapeLayout ? 58 : (root.standardLandscapeLayout ? 44 : (root.compactLandscapeLayout ? 28 : 34)))
            height: root.largeLandscapeLayout ? 48 : (root.standardLandscapeLayout ? 38 : (root.compactLandscapeLayout ? 24 : 30))
            radius: height / 2
            color: sortModeArea.containsPress ? "#1f2c36" : "#171717"
            border.width: 1
            border.color: "#3fd0ff"
            visible: !root.selectionMode
            opacity: root.selectionMode ? 0.0 : 1.0
            scale: sortModeArea.containsPress ? 0.985 : 1.0

            Behavior on opacity {
                NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
            }

            Behavior on scale {
                NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
            }

            Text {
                id: sortModeLabel
                anchors.centerIn: parent
                text: clipModel.sortMode
                color: "#79ddff"
                font.family: interBold.font.family
                font.weight: Font.Bold
                font.pixelSize: root.largeLandscapeLayout ? 16 : (root.standardLandscapeLayout ? 13 : (root.compactLandscapeLayout ? 9 : 11))
                font.capitalization: Font.AllUppercase
                font.letterSpacing: root.largeLandscapeLayout ? 2.1 : (root.standardLandscapeLayout ? 1.8 : (root.compactLandscapeLayout ? 1.5 : 1.8))
                renderType: Text.NativeRendering
            }

            MouseArea {
                id: sortModeArea
                anchors.fill: parent
                enabled: !clipModel.loading
                onClicked: root.toggleSortMode()
            }
        }

        Rectangle {
            id: selectionCountChip
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.rightMargin: root.largeLandscapeLayout ? 30 : (root.standardLandscapeLayout ? 24 : (root.compactLandscapeLayout ? 18 : 20))
            anchors.topMargin: root.largeLandscapeLayout ? 22 : (root.standardLandscapeLayout ? 16 : (root.compactLandscapeLayout ? 12 : 14))
            width: root.selectionMode
                   ? (root.largeLandscapeLayout ? 212 : (root.standardLandscapeLayout ? 172 : (root.compactLandscapeLayout ? 122 : 148)))
                   : (root.largeLandscapeLayout ? 154 : (root.standardLandscapeLayout ? 126 : (root.compactLandscapeLayout ? 86 : 108)))
            height: root.largeLandscapeLayout ? 48 : (root.standardLandscapeLayout ? 38 : (root.compactLandscapeLayout ? 24 : 30))
            radius: height / 2
            color: "#14ffffff"
            border.width: 1
            border.color: "#1affffff"

            Text {
                anchors.centerIn: parent
                text: root.selectionMode ? (root.selectedClipPaths.length + " selected") : (clipModel.count + " items")
                color: "white"
                font.family: interBold.font.family
                font.weight: Font.Bold
                font.pixelSize: root.largeLandscapeLayout ? 16 : (root.standardLandscapeLayout ? 13 : (root.compactLandscapeLayout ? 9 : 11))
                font.capitalization: Font.AllUppercase
                font.letterSpacing: root.largeLandscapeLayout ? 2.2 : (root.standardLandscapeLayout ? 2 : (root.compactLandscapeLayout ? 1.5 : 2))
                renderType: Text.NativeRendering
            }
        }

        Text {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: root.largeLandscapeLayout ? 36 : 24
            anchors.rightMargin: root.largeLandscapeLayout ? 190 : 140
            anchors.topMargin: root.selectionMode
                               ? (root.largeLandscapeLayout ? 96 : (root.standardLandscapeLayout ? 76 : (root.compactLandscapeLayout ? 54 : 64)))
                               : (root.largeLandscapeLayout ? 82 : (root.standardLandscapeLayout ? 64 : (root.compactLandscapeLayout ? 44 : 52)))
            text: root.selectionMode
                  ? (root.stillMode
                     ? "Long press starts selection. Tap stills to add or remove them."
                     : "Long press starts selection. Tap clips to add or remove them.")
                  : clipModel.statusText
            color: "#6f7076"
            font.family: interRegular.font.family
            font.pixelSize: root.largeLandscapeLayout ? 19 : (root.standardLandscapeLayout ? 16 : (root.compactLandscapeLayout ? 10 : 12))
            elide: Text.ElideRight
            renderType: Text.NativeRendering
        }

        GridView {
            id: clipGrid
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: 16
            anchors.rightMargin: 12
            anchors.topMargin: root.selectionMode
                               ? (root.largeLandscapeLayout ? 144 : (root.standardLandscapeLayout ? 116 : (root.compactLandscapeLayout ? 78 : 92)))
                               : (root.largeLandscapeLayout ? 124 : (root.standardLandscapeLayout ? 100 : (root.compactLandscapeLayout ? 64 : 78)))
            anchors.bottomMargin: root.compactLandscapeLayout ? 0 : 44
            cellWidth: (width - ((root.gridColumnCount - 1) * root.gridColumnGap)) / root.gridColumnCount
            cellHeight: root.clipCellHeightForCell(cellWidth)
            cacheBuffer: cellHeight * 2
            model: root.enabled ? clipModel : null
            clip: true
            boundsBehavior: Flickable.DragAndOvershootBounds
            bottomMargin: root.compactLandscapeLayout ? 12 : 0
            reuseItems: true

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: Item {
                id: clipDelegate

                required property int index
                required property string name
                required property string path
                required property int frameCount
                required property string durationText
                required property string thumbnailSource
                required property string shotDate
                required property string formatLabel

                property bool holdTriggered: false

                width: clipGrid.cellWidth
                height: clipGrid.cellHeight

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: root.clipCardMargin
                    radius: root.clipCardRadius
                    color: root.isClipSelected(path)
                           ? "#241c1f"
                           : (root.selectedClipPath === path ? "#1d1d1d" : "#181818")
                    border.color: root.isClipSelected(path)
                                  ? "#ba4a4a"
                                  : (root.selectedClipPath === path ? "#33ffffff" : "#1affffff")
                    scale: clipCardArea.containsPress ? 0.985 : 1.0

                    Behavior on scale {
                        NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
                    }

                    Behavior on color {
                        ColorAnimation { duration: 180; easing.type: Easing.OutCubic }
                    }
                    Behavior on border.color {
                        ColorAnimation { duration: 180; easing.type: Easing.OutCubic }
                    }

                    Rectangle {
                        id: thumbnailFrame
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.leftMargin: root.clipThumbnailInset
                        anchors.rightMargin: root.clipThumbnailInset
                        anchors.topMargin: root.clipThumbnailTopMargin
                        height: root.thumbnailHeightForCell(clipGrid.cellWidth)
                        radius: root.clipThumbnailRadius
                        clip: true
                        color: root.isClipSelected(path)
                               ? "#2a1d1d"
                               : (root.selectedClipPath === path ? "#202020" : "#171717")
                        border.width: 1
                        border.color: "#16ffffff"

                        Image {
                            id: thumbnailImage
                            anchors.fill: parent
                            fillMode: Image.PreserveAspectFit
                            cache: false
                            asynchronous: true
                            smooth: false
                            sourceSize.width: Math.max(2, Math.round(width * 0.9))
                            sourceSize.height: Math.max(2, Math.round(height * 0.9))
                            source: root.enabled ? thumbnailSource : ""
                        }

                        Rectangle {
                            anchors.fill: parent
                            color: root.isClipSelected(path) ? "#25101010" : "#12000000"
                        }

                        Text {
                            anchors.centerIn: parent
                            visible: thumbnailImage.status !== Image.Ready
                            text: "NO PREVIEW"
                            color: "#72737a"
                            font.family: interMedium.font.family
                            font.pixelSize: root.largeLandscapeLayout ? 15 : (root.compactLandscapeLayout ? 10 : 12)
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 2.4
                            renderType: Text.NativeRendering
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.bottom: parent.bottom
                            anchors.leftMargin: root.largeLandscapeLayout ? 14 : (root.compactLandscapeLayout ? 10 : 12)
                            anchors.bottomMargin: root.largeLandscapeLayout ? 14 : (root.compactLandscapeLayout ? 10 : 12)
                            height: root.clipDurationHeight
                            radius: root.clipDurationRadius
                            color: "#66000000"
                            border.color: "#22ffffff"
                            width: durationLabel.width + (root.largeLandscapeLayout ? 30 : (root.compactLandscapeLayout ? 20 : 24))

                            Text {
                                id: durationLabel
                                anchors.centerIn: parent
                                text: durationText
                                color: "white"
                                font.family: interBold.font.family
                                font.weight: Font.Bold
                                font.pixelSize: root.clipDurationFontSize
                                font.capitalization: Font.AllUppercase
                                font.letterSpacing: 2
                                renderType: Text.NativeRendering
                            }
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.leftMargin: root.largeLandscapeLayout ? 14 : (root.compactLandscapeLayout ? 10 : 12)
                            anchors.topMargin: root.largeLandscapeLayout ? 14 : (root.compactLandscapeLayout ? 10 : 12)
                            width: root.clipSelectionBadgeSize
                            height: root.clipSelectionBadgeSize
                            radius: width / 2
                            color: root.isClipSelected(path) ? "#8d2020" : "#55000000"
                            border.color: root.isClipSelected(path) ? "#ba4a4a" : "#22ffffff"
                            visible: root.selectionMode

                            Text {
                                anchors.centerIn: parent
                                text: root.isClipSelected(path) ? "✓" : ""
                                color: "white"
                                font.family: interBold.font.family
                                font.pixelSize: root.clipSelectionBadgeFontSize
                                renderType: Text.NativeRendering
                            }
                        }
                    }

                    Item {
                        id: footerBlock
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: root.clipFooterSideMargin
                        anchors.rightMargin: root.clipFooterSideMargin
                        anchors.bottomMargin: root.clipFooterBottomMargin
                        height: root.clipFooterHeight

                        Column {
                            anchors.fill: parent
                            spacing: root.largeLandscapeLayout ? 6 : 4

                            Text {
                                text: name
                                color: "white"
                                font.family: interBold.font.family
                                font.pixelSize: root.clipNameFontSize
                                elide: Text.ElideRight
                                renderType: Text.NativeRendering
                            }

                            Text {
                                text: root.stillMode ? durationText.replace(" STILL", " still")
                                                     : formatLabel
                                color: "#8f9096"
                                font.family: interRegular.font.family
                                font.pixelSize: root.clipTypeFontSize
                                renderType: Text.NativeRendering
                            }

                            Text {
                                text: shotDate.length > 0 ? shotDate : "Shot date unavailable"
                                color: "#5f6066"
                                font.family: interRegular.font.family
                                font.pixelSize: root.clipDateFontSize
                                font.letterSpacing: 1.2
                                elide: Text.ElideMiddle
                                renderType: Text.NativeRendering
                            }
                        }
                    }

                    MouseArea {
                        id: clipCardArea
                        anchors.fill: parent
                        onPressed: clipDelegate.holdTriggered = false
                        onPressAndHold: {
                            clipDelegate.holdTriggered = true
                            if (!root.selectionMode)
                                root.selectOnlyClip(path)
                            else
                                root.toggleClipSelection(path)
                        }
                        onClicked: {
                            if (clipDelegate.holdTriggered) {
                                clipDelegate.holdTriggered = false
                                return
                            }

                            if (root.selectionMode)
                                root.toggleClipSelection(path)
                            else
                                root.clipOpened(path, name, frameCount, index)
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#cc000000"
        visible: root.deleteConfirmOpen
        opacity: root.deleteConfirmOpen ? 1.0 : 0.0
        z: 20

        Behavior on opacity {
            NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: root.deleteConfirmOpen = false
        }
    }

    Rectangle {
        width: root.compactLandscapeLayout ? 340 : 408
        height: root.compactLandscapeLayout ? 218 : 308
        anchors.centerIn: parent
        anchors.verticalCenterOffset: root.compactLandscapeLayout ? -8 : -12
        radius: root.compactLandscapeLayout ? 22 : 26
        color: "#151515"
        border.width: 1
        border.color: "#1affffff"
        visible: root.deleteConfirmOpen
        opacity: root.deleteConfirmOpen ? 1.0 : 0.0
        scale: root.deleteConfirmOpen ? 1.0 : 0.96
        z: 21

        Behavior on opacity {
            NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
        }

        Behavior on scale {
            NumberAnimation { duration: 240; easing.type: Easing.OutCubic }
        }

        Column {
            anchors.fill: parent
            anchors.margins: root.compactLandscapeLayout ? 16 : 24
            spacing: root.compactLandscapeLayout ? 10 : 16

            Row {
                spacing: root.compactLandscapeLayout ? 8 : 10

                Rectangle {
                    width: root.compactLandscapeLayout ? 32 : 40
                    height: root.compactLandscapeLayout ? 32 : 40
                    radius: root.compactLandscapeLayout ? 16 : 20
                    color: "#2a1717"
                    border.color: "#4a2a2a"

                    Image {
                        anchors.centerIn: parent
                        width: root.compactLandscapeLayout ? 15 : 18
                        height: root.compactLandscapeLayout ? 15 : 18
                        source: "qrc:/qml/icons/delete.png"
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: true
                        opacity: 0.92
                    }
                }

                Column {
                    spacing: 4

                    Text {
                        text: "Remove Selected Clips?"
                        color: "white"
                        font.family: interBold.font.family
                        font.pixelSize: root.compactLandscapeLayout ? 20 : 24
                        renderType: Text.NativeRendering
                    }

                    Text {
                        text: "This will permanently delete the selected clips from media."
                        color: "#8f9096"
                        font.family: interRegular.font.family
                        font.pixelSize: root.compactLandscapeLayout ? 12 : 14
                        wrapMode: Text.WordWrap
                        width: root.compactLandscapeLayout ? 240 : 290
                        renderType: Text.NativeRendering
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: root.compactLandscapeLayout ? 54 : 82
                radius: root.compactLandscapeLayout ? 16 : 18
                color: "#181818"
                border.width: 1
                border.color: "#1affffff"

                Column {
                    anchors.fill: parent
                    anchors.margins: root.compactLandscapeLayout ? 9 : 14
                    spacing: root.compactLandscapeLayout ? 1 : 4

                    Text {
                        text: root.selectedClipPaths.length + " clip(s) selected"
                        color: "white"
                        font.family: interBold.font.family
                        font.pixelSize: root.compactLandscapeLayout ? 12 : 16
                        elide: Text.ElideRight
                        renderType: Text.NativeRendering
                    }

                    Text {
                        text: root.selectedClipPaths.length > 0 ? root.selectedClipPaths[0].split("/").pop() : ""
                        color: "#8f9096"
                        font.family: interRegular.font.family
                        font.pixelSize: root.compactLandscapeLayout ? 10 : 13
                        elide: Text.ElideMiddle
                        renderType: Text.NativeRendering
                    }

                    Text {
                        text: root.selectedClipPaths.length > 1 ? ("+" + (root.selectedClipPaths.length - 1) + " more") : "This action cannot be undone."
                        color: "#6f7076"
                        font.family: interRegular.font.family
                        font.pixelSize: root.compactLandscapeLayout ? 10 : 13
                        renderType: Text.NativeRendering
                    }
                }
            }

            Item {
                width: 1
                height: root.compactLandscapeLayout ? -2 : 18
            }

            Row {
                spacing: root.compactLandscapeLayout ? 10 : 12

                Rectangle {
                    width: root.compactLandscapeLayout ? 132 : 156
                    height: root.compactLandscapeLayout ? 42 : 54
                    radius: root.compactLandscapeLayout ? 15 : 18
                    color: cancelDeleteArea.containsPress ? "#20ffffff" : "#14ffffff"
                    border.width: 1
                    border.color: "#1affffff"
                    scale: cancelDeleteArea.containsPress ? 0.985 : 1.0

                    Behavior on scale {
                        NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                    }

                    Text {
                        anchors.centerIn: parent
                        text: "Cancel"
                        color: "white"
                        font.family: interMedium.font.family
                        font.pixelSize: root.compactLandscapeLayout ? 13 : 16
                        renderType: Text.NativeRendering
                    }

                    MouseArea {
                        id: cancelDeleteArea
                        anchors.fill: parent
                        onClicked: root.deleteConfirmOpen = false
                    }
                }

                Rectangle {
                    width: root.compactLandscapeLayout ? 160 : 192
                    height: root.compactLandscapeLayout ? 42 : 54
                    radius: root.compactLandscapeLayout ? 15 : 18
                    color: confirmDeleteArea.containsPress ? "#a32828" : "#8d2020"
                    border.width: 1
                    border.color: "#ba4a4a"
                    scale: confirmDeleteArea.containsPress ? 0.985 : 1.0

                    Behavior on scale {
                        NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                    }

                    Behavior on color {
                        ColorAnimation { duration: 140; easing.type: Easing.OutCubic }
                    }

                Row {
                    anchors.centerIn: parent
                        spacing: root.compactLandscapeLayout ? 6 : 8

                        Image {
                            width: root.compactLandscapeLayout ? 15 : 18
                            height: root.compactLandscapeLayout ? 15 : 18
                            anchors.verticalCenter: parent.verticalCenter
                            source: "qrc:/qml/icons/delete.png"
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            mipmap: true
                            opacity: 0.92
                        }

                        Text {
                            text: "Remove Clips"
                            color: "white"
                            font.family: interMedium.font.family
                            font.pixelSize: root.compactLandscapeLayout ? 13 : 16
                            renderType: Text.NativeRendering
                        }
                    }

                    MouseArea {
                        id: confirmDeleteArea
                        anchors.fill: parent
                        onClicked: root.removeSelectedClips()
                    }
                }
            }
        }
    }
}
