import QtQuick
import QtQuick.Controls

Item {
    id: root

    property bool stillMode: false
    property string selectedClipPath: ""
    property var selectedClipPaths: []
    property bool deleteConfirmOpen: false
    readonly property bool selectionMode: selectedClipPaths.length > 0

    signal backRequested()
    signal clipOpened(string clipPath, string clipName, int frameCount, int clipIndex)

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

    FontLoader { id: gothamThin;   source: "qrc:/qml/fonts/Gotham/Gotham-Thin.ttf" }
    FontLoader { id: gothamLight;  source: "qrc:/qml/fonts/Gotham/Gotham-Light.ttf" }
    FontLoader { id: gothamMedium; source: "qrc:/qml/fonts/Gotham/Gotham-Medium.ttf" }
    FontLoader { id: gothamBold;   source: "qrc:/qml/fonts/Gotham/Gotham-Bold.ttf" }
    FontLoader { id: gothamBlack;  source: "qrc:/qml/fonts/Gotham/Gotham-Black.ttf" }
    FontLoader { id: interThin;    source: "qrc:/qml/fonts/Inter/Inter-Thin.ttf" }
    FontLoader { id: interLight;   source: "qrc:/qml/fonts/Inter/Inter-Light.ttf" }
    FontLoader { id: interRegular; source: "qrc:/qml/fonts/Inter/Inter-Regular.ttf" }
    FontLoader { id: interMedium;  source: "qrc:/qml/fonts/Inter/Inter-Medium.ttf" }
    FontLoader { id: interBold;    source: "qrc:/qml/fonts/Inter/Inter-Bold.ttf" }
    FontLoader { id: interBlack;   source: "qrc:/qml/fonts/Inter/Inter-Black.ttf" }

    Rectangle {
        anchors.fill: parent
        color: "#000000"
    }

    Column {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 24
        anchors.topMargin: 22
        spacing: 4

        Text {
            text: "PLAYBACK"
            color: "#66ffffff"
            font.family: interMedium.font.family
            font.pixelSize: 12
            font.capitalization: Font.AllUppercase
            font.letterSpacing: 3
            renderType: Text.NativeRendering
        }

        Text {
            text: root.stillMode ? "Still Browser" : "Clip Browser"
            color: "white"
            font.family: interBold.font.family
            font.pixelSize: 34
            renderType: Text.NativeRendering
        }
    }

    Rectangle {
        id: refreshButton
        anchors.right: backButton.left
        anchors.top: parent.top
        anchors.rightMargin: 12
        anchors.topMargin: 22
        width: 184
        height: 54
        radius: 18
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
            spacing: 8

            Image {
                width: 18
                height: 18
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
                font.pixelSize: 16
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
        anchors.rightMargin: 24
        anchors.topMargin: 22
        width: 112
        height: 54
        radius: 18
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
            spacing: 8

            Text {
                text: "←"
                color: "white"
                font.family: interMedium.font.family
                font.pixelSize: 18
                renderType: Text.NativeRendering
            }

            Text {
                text: root.selectionMode ? "Cancel" : "Back"
                color: "white"
                font.family: interMedium.font.family
                font.pixelSize: 16
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
        anchors.margins: 24
        anchors.topMargin: 112
        anchors.bottomMargin: -32
        radius: 28
        color: "#151515"
        border.width: 1
        border.color: "#1affffff"

        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: 27
            color: "#151515"
            visible: false
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: browserPanel.radius + 40
            color: "#151515"
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: browserPanel.radius + 24
            color: "#151515"
        }

        Row {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: 24
            anchors.topMargin: 20
            spacing: 10

            Image {
                width: 24
                height: 24
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
                font.pixelSize: 13
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 3
                renderType: Text.NativeRendering
            }
        }

        Rectangle {
            id: removeSelectedButton
            anchors.right: selectionCountChip.left
            anchors.top: parent.top
            anchors.rightMargin: 10
            anchors.topMargin: 14
            width: 178
            height: 40
            radius: 16
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
                spacing: 8

                Image {
                    width: 16
                    height: 16
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
                    font.pixelSize: 14
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
            id: selectionCountChip
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.rightMargin: 20
            anchors.topMargin: 14
            width: root.selectionMode ? 148 : 108
            height: 30
            radius: 15
            color: "#14ffffff"
            border.width: 1
            border.color: "#1affffff"

            Text {
                anchors.centerIn: parent
                text: root.selectionMode ? (root.selectedClipPaths.length + " selected") : (clipModel.count + " items")
                color: "white"
                font.family: interBold.font.family
                font.weight: Font.Bold
                font.pixelSize: 11
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 2
                renderType: Text.NativeRendering
            }
        }

        Text {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: 24
            anchors.rightMargin: 140
            anchors.topMargin: root.selectionMode ? 64 : 52
            text: root.selectionMode
                  ? (root.stillMode
                     ? "Long press starts selection. Tap stills to add or remove them."
                     : "Long press starts selection. Tap clips to add or remove them.")
                  : clipModel.statusText
            color: "#6f7076"
            font.family: interRegular.font.family
            font.pixelSize: 12
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
            anchors.topMargin: root.selectionMode ? 92 : 78
            anchors.bottomMargin: 44
            cellWidth: (width - 14) / 2
            cellHeight: 246
            cacheBuffer: cellHeight * 2
            model: clipModel
            clip: true
            boundsBehavior: Flickable.DragAndOvershootBounds
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

                property bool holdTriggered: false

                width: clipGrid.cellWidth
                height: clipGrid.cellHeight

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 5
                    radius: 24
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
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        anchors.topMargin: 12
                        height: 122
                        radius: 18
                        clip: true
                        color: root.isClipSelected(path)
                               ? "#2a1d1d"
                               : (root.selectedClipPath === path ? "#202020" : "#171717")
                        border.width: 1
                        border.color: "#16ffffff"

                        Image {
                            id: thumbnailImage
                            anchors.fill: parent
                            fillMode: Image.PreserveAspectCrop
                            cache: true
                            asynchronous: true
                            smooth: false
                            sourceSize.width: Math.max(2, Math.round(width * 0.9))
                            sourceSize.height: Math.max(2, Math.round(height * 0.9))
                            source: thumbnailSource
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
                            font.pixelSize: 12
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 2.4
                            renderType: Text.NativeRendering
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.bottom: parent.bottom
                            anchors.leftMargin: 12
                            anchors.bottomMargin: 12
                            height: 28
                            radius: 14
                            color: "#66000000"
                            border.color: "#22ffffff"
                            width: durationLabel.width + 24

                            Text {
                                id: durationLabel
                                anchors.centerIn: parent
                                text: durationText
                                color: "white"
                                font.family: interBold.font.family
                                font.weight: Font.Bold
                                font.pixelSize: 11
                                font.capitalization: Font.AllUppercase
                                font.letterSpacing: 2
                                renderType: Text.NativeRendering
                            }
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.leftMargin: 12
                            anchors.topMargin: 12
                            width: 28
                            height: 28
                            radius: 14
                            color: root.isClipSelected(path) ? "#8d2020" : "#55000000"
                            border.color: root.isClipSelected(path) ? "#ba4a4a" : "#22ffffff"
                            visible: root.selectionMode

                            Text {
                                anchors.centerIn: parent
                                text: root.isClipSelected(path) ? "✓" : ""
                                color: "white"
                                font.family: interBold.font.family
                                font.pixelSize: 15
                                renderType: Text.NativeRendering
                            }
                        }
                    }

                    Item {
                        id: footerBlock
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14
                        anchors.bottomMargin: 24
                        height: 58

                        Column {
                            anchors.fill: parent
                            spacing: 4

                            Text {
                                text: name
                                color: "white"
                                font.family: interBold.font.family
                                font.pixelSize: 17
                                elide: Text.ElideRight
                                renderType: Text.NativeRendering
                            }

                            Text {
                                text: root.stillMode ? "DNG still" : "cDNG clip"
                                color: "#8f9096"
                                font.family: interRegular.font.family
                                font.pixelSize: 13
                                renderType: Text.NativeRendering
                            }

                            Text {
                                text: shotDate.length > 0 ? shotDate : "Shot date unavailable"
                                color: "#5f6066"
                                font.family: interRegular.font.family
                                font.pixelSize: 11
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
        width: 408
        height: 308
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -12
        radius: 26
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
            anchors.margins: 24
            spacing: 16

            Row {
                spacing: 10

                Rectangle {
                    width: 40
                    height: 40
                    radius: 20
                    color: "#2a1717"
                    border.color: "#4a2a2a"

                    Image {
                        anchors.centerIn: parent
                        width: 18
                        height: 18
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
                        font.pixelSize: 24
                        renderType: Text.NativeRendering
                    }

                    Text {
                        text: "This will permanently delete the selected clips from media."
                        color: "#8f9096"
                        font.family: interRegular.font.family
                        font.pixelSize: 14
                        wrapMode: Text.WordWrap
                        width: 290
                        renderType: Text.NativeRendering
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 82
                radius: 18
                color: "#181818"
                border.width: 1
                border.color: "#1affffff"

                Column {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 4

                    Text {
                        text: root.selectedClipPaths.length + " clip(s) selected"
                        color: "white"
                        font.family: interBold.font.family
                        font.pixelSize: 16
                        elide: Text.ElideRight
                        renderType: Text.NativeRendering
                    }

                    Text {
                        text: root.selectedClipPaths.length > 0 ? root.selectedClipPaths[0].split("/").pop() : ""
                        color: "#8f9096"
                        font.family: interRegular.font.family
                        font.pixelSize: 13
                        elide: Text.ElideMiddle
                        renderType: Text.NativeRendering
                    }

                    Text {
                        text: root.selectedClipPaths.length > 1 ? ("+" + (root.selectedClipPaths.length - 1) + " more") : "This action cannot be undone."
                        color: "#6f7076"
                        font.family: interRegular.font.family
                        font.pixelSize: 13
                        renderType: Text.NativeRendering
                    }
                }
            }

            Item {
                width: 1
                height: 18
            }

            Row {
                spacing: 12

                Rectangle {
                    width: 156
                    height: 54
                    radius: 18
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
                        font.pixelSize: 16
                        renderType: Text.NativeRendering
                    }

                    MouseArea {
                        id: cancelDeleteArea
                        anchors.fill: parent
                        onClicked: root.deleteConfirmOpen = false
                    }
                }

                Rectangle {
                    width: 192
                    height: 54
                    radius: 18
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
                        spacing: 8

                        Image {
                            width: 18
                            height: 18
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
                            font.pixelSize: 16
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
