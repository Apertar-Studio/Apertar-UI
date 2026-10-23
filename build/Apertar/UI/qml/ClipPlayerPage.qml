import QtQuick

Item {
    id: root

    property string selectedClipName: ""
    property int selectedClipIndex: -1
    property bool hasPreviousClip: false
    property bool hasNextClip: false
    property bool infoOpen: false
    property bool deleteConfirmOpen: false
    property int clipTransitionDirection: 0
    property bool clipTransitionRunning: false
    property string outgoingFrameSource: ""
    property string outgoingClipName: ""
    property string outgoingMetaText: ""
    property string outgoingFpsText: ""
    property string outgoingPlayStateText: ""
    property string outgoingFrameBadgeText: ""
    property string displayLayout: "square"
    property string videoFrameSource: ""
    readonly property bool landscapeCompactLayout: displayLayout === "landscape_compact"
    readonly property bool largeLandscapeLayout: displayLayout === "landscape_large"
    readonly property bool standardLandscapeLayout: displayLayout === "landscape"
                                                   || largeLandscapeLayout
    readonly property bool regularLandscapeLayout: displayLayout === "landscape_medium"
                                                   || standardLandscapeLayout
    readonly property bool splitLandscapePlaybackLayout: regularLandscapeLayout
    readonly property bool compactLandscapeLayout: landscapeCompactLayout || regularLandscapeLayout
    readonly property int splitButtonWidth: largeLandscapeLayout ? 176 : 142
    readonly property int splitButtonHeight: largeLandscapeLayout ? 72 : 58
    readonly property int splitButtonRadius: largeLandscapeLayout ? 20 : 15
    readonly property int splitButtonIconSize: largeLandscapeLayout ? 22 : 17
    readonly property int splitButtonFontSize: largeLandscapeLayout ? 19 : 15
    readonly property int splitSmallButtonBox: largeLandscapeLayout ? 72 : 52
    readonly property int splitSmallButtonSize: largeLandscapeLayout ? 68 : 50
    readonly property int splitSmallIconSize: largeLandscapeLayout ? 25 : 19
    readonly property int splitPlayButtonBox: largeLandscapeLayout ? 88 : 66
    readonly property int splitPlayButtonSize: largeLandscapeLayout ? 84 : 64
    readonly property int splitPlayIconSize: largeLandscapeLayout ? 31 : 24
    readonly property int splitPauseIconSize: largeLandscapeLayout ? 29 : 22
    readonly property int splitControlsHeight: largeLandscapeLayout ? 88 : 66
    readonly property int splitControlsBottomMargin: largeLandscapeLayout ? 18 : 12
    readonly property int splitTransportSpacing: largeLandscapeLayout ? 14 : 10
    readonly property int splitProgressHeight: largeLandscapeLayout ? 38 : 30
    readonly property int splitProgressRailHeight: largeLandscapeLayout ? 10 : 8
    readonly property int splitProgressHandleSize: largeLandscapeLayout ? 30 : 24
    readonly property int splitProgressHandleDotSize: largeLandscapeLayout ? 10 : 8
    readonly property int splitHistogramWidth: largeLandscapeLayout ? 176 : 142
    readonly property int splitHistogramHeight: largeLandscapeLayout ? 72 : 58
    readonly property int splitHistogramRadius: largeLandscapeLayout ? 20 : 15
    readonly property int splitHistogramMargin: largeLandscapeLayout ? 10 : 8
    readonly property int splitHistogramSpacing: largeLandscapeLayout ? 7 : 5
    readonly property int splitHistogramLabelFontSize: largeLandscapeLayout ? 10 : 8
    readonly property int splitHistogramCanvasHeight: largeLandscapeLayout ? 36 : 28
    readonly property int splitBadgeHeight: largeLandscapeLayout ? 38 : 30
    readonly property int splitBadgeRadius: largeLandscapeLayout ? 19 : 15
    readonly property int splitBadgeFontSize: largeLandscapeLayout ? 14 : 11
    readonly property int splitBadgePadding: largeLandscapeLayout ? 34 : 26
    readonly property int splitBadgeMargin: largeLandscapeLayout ? 18 : 14
    readonly property int landscapeInfoLabelFontSize: largeLandscapeLayout ? 13 : 11
    readonly property int landscapeInfoValueFontSize: largeLandscapeLayout ? 18 : 15

    signal backRequested()
    signal previousClipRequested()
    signal nextClipRequested()
    signal deleteRequested()

    onEnabledChanged: {
        if (!enabled && cdngPlayProvider)
            cdngPlayProvider.clearCache()
        videoProvider.setActive(enabled && visible && playbackController.videoClip)
        updateVideoFrame(true)
    }
    onVisibleChanged: {
        videoProvider.setActive(enabled && visible && playbackController.videoClip)
        updateVideoFrame(true)
    }
    Component.onCompleted: {
        videoProvider.setActive(enabled && visible && playbackController.videoClip)
        updateVideoFrame(true)
    }
    Component.onDestruction: videoProvider.setActive(false)

    FontLoader { id: interRegular; source: "qrc:/qml/fonts/Inter/Inter-Regular.ttf" }
    FontLoader { id: interMedium;  source: "qrc:/qml/fonts/Inter/Inter-Medium.ttf" }
    FontLoader { id: interBold;    source: "qrc:/qml/fonts/Inter/Inter-Bold.ttf" }

    function playbackProgress() {
        if (playbackController.frameCount <= 0 || playbackController.currentFrameIndex < 0)
            return 0
        return (playbackController.currentFrameIndex + 1) / playbackController.frameCount
    }

    function scrubPlayback(mouseX, barWidth) {
        if (playbackController.frameCount <= 0 || barWidth <= 0)
            return

        var normalized = Math.max(0, Math.min(1, mouseX / barWidth))
        var frameIndex = Math.max(0, Math.min(playbackController.frameCount - 1,
                                              Math.round(normalized * (playbackController.frameCount - 1))))
        playbackController.seekToFrame(frameIndex)
    }

    function currentShotFps() {
        return metadataValue("frameRate", Number(playbackController.fps).toFixed(3) + " fps")
    }

    function metadataValue(key, fallbackText) {
        var value = playbackController.clipMetadata[key]
        return value && String(value).length > 0 ? String(value) : fallbackText
    }

    function startClipReveal() {
        clipContent.x = clipTransitionDirection > 0 ? 44 : -44
        clipContent.opacity = 0.0
        clipContent.scale = 0.985
        clipRevealAnim.restart()
    }

    function previewWidthForHeight(previewHeight, availableWidth) {
        return Math.min(Math.max(2, availableWidth), Math.round(previewHeight * 16 / 9))
    }

    onSelectedClipIndexChanged: {
        if (clipTransitionDirection !== 0 && selectedClipIndex >= 0)
            startClipReveal()
    }

    onDeleteConfirmOpenChanged: {
        if (deleteConfirmOpen)
            infoOpen = false
    }

    function resolvedFrameSource(frameSource) {
        if (!frameSource || frameSource.length === 0)
            return "image://cdng/empty"
        return frameSource.indexOf("?") >= 0 ? frameSource + "&radius=24" : frameSource
    }

    function updateVideoFrame(force) {
        if (!enabled || !visible || !playbackController.videoClip) {
            videoFrameSource = ""
            return
        }
        // Replacing a Loading source on every timer tick starves slow decoders:
        // Qt cancels each image before it can ever be displayed.
        if (!force && playbackImage.status === Image.Loading)
            return
        videoFrameSource = resolvedFrameSource(playbackController.frameSource)
    }

    function currentClipTitle() {
        return playbackController.currentClipName.length > 0 ? playbackController.currentClipName : root.selectedClipName
    }

    function warmPlaybackFrames() {
        if (playbackController.videoClip)
            return
        if (!playbackController.currentFramePath || playbackController.currentFramePath.length === 0)
            return

        var warmWidth = Math.max(2, Math.round(playbackImage.width * 0.75))
        var warmHeight = Math.max(2, Math.round(playbackImage.height * 0.75))
        cdngPlayProvider.prefetchAround(playbackController.currentFramePath, warmWidth, warmHeight, 24, 8)
    }

    function clipInfoText(frameCount, statusText) {
        if (frameCount <= 0)
            return statusText

        var typeText = metadataValue("type", "Clip")
        return typeText + " \u2022 " + frameCount + (frameCount === 1 ? " frame" : " frames")
    }

    function frameBadgeText(frameIndex, frameCount) {
        return frameCount > 0
               ? ("FRAME " + (frameIndex + 1) + " / " + frameCount)
               : "FRAME 0 / 0"
    }

    function beginClipTransition(direction) {
        if (clipTransitionRunning || direction === 0)
            return

        outgoingFrameSource = resolvedFrameSource(playbackController.frameSource)
        outgoingClipName = currentClipTitle()
        outgoingMetaText = clipInfoText(playbackController.frameCount, playbackController.statusText)
        outgoingFpsText = currentShotFps().toUpperCase()
        outgoingPlayStateText = playbackController.playing ? "PLAYING" : "PAUSED"
        outgoingFrameBadgeText = frameBadgeText(playbackController.currentFrameIndex, playbackController.frameCount)

        clipTransitionDirection = direction
        clipTransitionRunning = true

        clipContent.x = direction > 0 ? 44 : -44
        clipContent.opacity = 0.0
        clipContent.scale = 0.985
        outgoingClipContent.x = 0
        outgoingClipContent.opacity = 1.0
        outgoingClipContent.scale = 1.0

        if (direction < 0)
            root.previousClipRequested()
        else
            root.nextClipRequested()

        clipTransitionAnim.restart()
    }

    Rectangle {
        anchors.fill: parent
        color: "#000000"
    }

    Column {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: root.largeLandscapeLayout ? 32 : 24
        anchors.topMargin: root.largeLandscapeLayout ? 24 : (root.standardLandscapeLayout ? 18 : (root.compactLandscapeLayout ? 20 : 22))
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
            text: "Clip Viewer"
            color: "white"
            font.family: interBold.font.family
            font.pixelSize: root.largeLandscapeLayout ? 40 : (root.standardLandscapeLayout ? 31 : (root.compactLandscapeLayout ? 26 : 34))
            renderType: Text.NativeRendering
        }
    }

    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: root.largeLandscapeLayout ? 32 : 24
        anchors.topMargin: root.largeLandscapeLayout ? 20 : (root.standardLandscapeLayout ? 14 : (root.compactLandscapeLayout ? 22 : 22))
        width: root.largeLandscapeLayout ? 158 : (root.standardLandscapeLayout ? 132 : (root.displayLayout === "landscape_medium" ? 108 : (root.compactLandscapeLayout ? 90 : 112)))
        height: root.largeLandscapeLayout ? 72 : (root.standardLandscapeLayout ? 60 : (root.displayLayout === "landscape_medium" ? 48 : (root.compactLandscapeLayout ? 40 : 54)))
        radius: root.largeLandscapeLayout ? 24 : (root.standardLandscapeLayout ? 20 : (root.displayLayout === "landscape_medium" ? 17 : (root.compactLandscapeLayout ? 15 : 18)))
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
            spacing: root.largeLandscapeLayout ? 11 : (root.standardLandscapeLayout ? 9 : (root.displayLayout === "landscape_medium" ? 7 : (root.compactLandscapeLayout ? 6 : 8)))

            Text {
                text: "←"
                color: "white"
                font.family: interMedium.font.family
                font.pixelSize: root.largeLandscapeLayout ? 25 : (root.standardLandscapeLayout ? 20 : (root.displayLayout === "landscape_medium" ? 18 : (root.compactLandscapeLayout ? 16 : 18)))
                renderType: Text.NativeRendering
            }

            Text {
                text: "Back"
                color: "white"
                font.family: interMedium.font.family
                font.pixelSize: root.largeLandscapeLayout ? 22 : (root.standardLandscapeLayout ? 18 : (root.displayLayout === "landscape_medium" ? 15 : (root.compactLandscapeLayout ? 13 : 16)))
                renderType: Text.NativeRendering
            }
        }

        MouseArea {
            id: backArea
            anchors.fill: parent
            onClicked: {
                playbackController.stop()
                root.backRequested()
            }
        }
    }

    Rectangle {
        id: playerPanel
        anchors.left: parent.left
        anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
        anchors.margins: root.largeLandscapeLayout ? 32 : 24
        anchors.topMargin: root.largeLandscapeLayout ? 112 : (root.compactLandscapeLayout ? 86 : 112)
        radius: root.largeLandscapeLayout ? 32 : (root.compactLandscapeLayout ? 24 : 28)
        color: "#151515"
        border.width: 1
        border.color: "#1affffff"
        readonly property int landscapeContentMargin: root.largeLandscapeLayout ? 24 : 18
        readonly property int landscapeInfoWidth: root.splitLandscapePlaybackLayout
                                              ? (root.largeLandscapeLayout
                                                 ? Math.min(470, Math.max(410, Math.round(width * 0.25)))
                                                 : root.standardLandscapeLayout
                                                 ? Math.min(420, Math.max(340, Math.round(width * 0.27)))
                                                 : Math.min(320, Math.max(300, Math.round(width * 0.32))))
                                              : 0
        readonly property int landscapeGap: root.splitLandscapePlaybackLayout
                                            ? (root.largeLandscapeLayout ? 32 : (root.standardLandscapeLayout ? 24 : 18))
                                            : 0
        readonly property int landscapeControlGap: root.splitLandscapePlaybackLayout
                                                   ? (root.largeLandscapeLayout ? 24 : (root.standardLandscapeLayout ? 18 : 14))
                                                   : 0
        readonly property int landscapeTransportWidth: root.splitLandscapePlaybackLayout
                                                      ? Math.max(transportControls.width,
                                                                 transportControls.implicitWidth)
                                                      : 0
        readonly property real landscapePreviewLeft: root.splitLandscapePlaybackLayout
                                                    ? (clipContent.x + playerPreviewFrame.x)
                                                    : 0
        readonly property int landscapePreviewMaxWidth: root.splitLandscapePlaybackLayout
                                                     ? (width - (landscapeContentMargin * 2) - landscapeGap - landscapeInfoWidth)
                                                     : 0
        readonly property int landscapePreviewHeight: root.splitLandscapePlaybackLayout
                                                   ? Math.max(root.standardLandscapeLayout ? 300 : 260,
                                                              Math.min(Math.round(landscapePreviewMaxWidth * 9 / 16),
                                                                       Math.round(height - (root.largeLandscapeLayout ? 196 : 132)),
                                                                       root.largeLandscapeLayout ? 820 : (root.standardLandscapeLayout ? 720 : 360)))
                                                   : 0
        readonly property int landscapeControlsWidth: root.splitLandscapePlaybackLayout
                                                   ? (deleteButton.width
                                                      + landscapeTransportWidth
                                                      + histogramPanel.width
                                                      + (landscapeControlGap * 2))
                                                   : 0
        readonly property int landscapeControlsLeft: root.splitLandscapePlaybackLayout
                                                  ? Math.round(landscapePreviewLeft
                                                               + ((playerPreviewFrame.width - landscapeControlsWidth) / 2))
                                                  : 0

        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: root.compactLandscapeLayout ? 23 : 27
            color: "#151515"
            visible: false
        }

        Item {
            id: clipContent
            anchors.fill: parent

        Rectangle {
            id: playerPreviewFrame
            anchors.left: root.splitLandscapePlaybackLayout ? parent.left : undefined
            anchors.right: undefined
            anchors.horizontalCenter: root.splitLandscapePlaybackLayout ? undefined : parent.horizontalCenter
            anchors.top: parent.top
            anchors.leftMargin: root.splitLandscapePlaybackLayout ? playerPanel.landscapeContentMargin : (root.compactLandscapeLayout ? 16 : 18)
            anchors.rightMargin: 0
            anchors.topMargin: root.compactLandscapeLayout ? 16 : 18
            width: root.splitLandscapePlaybackLayout
                   ? Math.round(height * 16 / 9)
                   : root.previewWidthForHeight(height,
                                                parent.width - ((root.compactLandscapeLayout ? 16 : 18) * 2))
            height: root.splitLandscapePlaybackLayout
                    ? playerPanel.landscapePreviewHeight
                    : (root.regularLandscapeLayout ? 344 : (root.compactLandscapeLayout ? 248 : 344))
            radius: root.largeLandscapeLayout ? 30 : (root.regularLandscapeLayout ? 24 : (root.compactLandscapeLayout ? 22 : 24))
            clip: true
            color: "#171717"
            border.width: 1
            border.color: "#1affffff"

            Image {
                id: playbackImage
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
                cache: !playbackController.videoClip && root.enabled && playbackController.playing
                asynchronous: playbackController.videoClip
                retainWhileLoading: true
                smooth: true
                onStatusChanged: {
                    if (status === Image.Ready && playbackController.videoClip)
                        Qt.callLater(root.updateVideoFrame, false)
                }
                sourceSize.width: playbackController.playing
                                  ? Math.max(2, Math.round(width * 0.75))
                                  : Math.max(2, Math.round(width))
                sourceSize.height: playbackController.playing
                                   ? Math.max(2, Math.round(height * 0.75))
                                   : Math.max(2, Math.round(height))
                source: {
                    if (!root.enabled)
                        return ""
                    if (playbackController.videoClip)
                        return root.videoFrameSource

                    var activeSource = playbackController.playing
                                     ? playbackController.fastFrameSource
                                     : playbackController.frameSource
                    return activeSource.indexOf("?") >= 0
                         ? activeSource + "&radius=24"
                         : activeSource
                }
            }

            Connections {
                target: playbackController

                function onClipChanged() {
                    videoProvider.setActive(root.enabled && root.visible && playbackController.videoClip)
                    root.updateVideoFrame(true)
                    if (!playbackController.playing)
                        root.warmPlaybackFrames()
                }

                function onCurrentFrameChanged() {
                    root.updateVideoFrame(false)
                    if (!playbackController.playing)
                        root.warmPlaybackFrames()
                }
            }

            Component.onCompleted: root.warmPlaybackFrames()
            onWidthChanged: {
                if (!playbackController.playing)
                    root.warmPlaybackFrames()
            }
            onHeightChanged: {
                if (!playbackController.playing)
                    root.warmPlaybackFrames()
            }

            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.rightMargin: root.largeLandscapeLayout ? root.splitBadgeMargin : (root.compactLandscapeLayout ? 12 : 14)
                anchors.topMargin: root.largeLandscapeLayout ? root.splitBadgeMargin : (root.compactLandscapeLayout ? 12 : 14)
                height: root.largeLandscapeLayout ? root.splitBadgeHeight : (root.compactLandscapeLayout ? 26 : 30)
                radius: root.largeLandscapeLayout ? root.splitBadgeRadius : (root.compactLandscapeLayout ? 13 : 15)
                color: "#66000000"
                border.color: "#22ffffff"
                width: fpsChipLabel.width + (root.largeLandscapeLayout ? root.splitBadgePadding : (root.compactLandscapeLayout ? 22 : 26))

                Text {
                    id: fpsChipLabel
                    anchors.centerIn: parent
                    text: root.currentShotFps().toUpperCase()
                    color: "white"
                    font.family: interBold.font.family
                    font.weight: Font.Bold
                    font.pixelSize: root.largeLandscapeLayout ? root.splitBadgeFontSize : (root.compactLandscapeLayout ? 9 : 11)
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: root.compactLandscapeLayout ? 1.6 : 2.0
                    renderType: Text.NativeRendering
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.leftMargin: root.largeLandscapeLayout ? root.splitBadgeMargin : (root.compactLandscapeLayout ? 12 : 14)
                anchors.bottomMargin: root.largeLandscapeLayout ? root.splitBadgeMargin : (root.compactLandscapeLayout ? 12 : 14)
                height: root.largeLandscapeLayout ? root.splitBadgeHeight : (root.compactLandscapeLayout ? 26 : 30)
                radius: root.largeLandscapeLayout ? root.splitBadgeRadius : (root.compactLandscapeLayout ? 13 : 15)
                color: "#66000000"
                border.color: "#22ffffff"
                width: playStateLabel.width + (root.largeLandscapeLayout ? root.splitBadgePadding : (root.compactLandscapeLayout ? 22 : 26))

                Text {
                    id: playStateLabel
                    anchors.centerIn: parent
                    text: playbackController.playing ? "PLAYING" : "PAUSED"
                    color: "white"
                    font.family: interBold.font.family
                    font.weight: Font.Bold
                    font.pixelSize: root.largeLandscapeLayout ? root.splitBadgeFontSize : (root.compactLandscapeLayout ? 9 : 11)
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: root.compactLandscapeLayout ? 1.7 : 2.2
                    renderType: Text.NativeRendering
                }
            }

            Rectangle {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.rightMargin: root.largeLandscapeLayout ? root.splitBadgeMargin : (root.compactLandscapeLayout ? 12 : 14)
                anchors.bottomMargin: root.largeLandscapeLayout ? root.splitBadgeMargin : (root.compactLandscapeLayout ? 12 : 14)
                height: root.largeLandscapeLayout ? root.splitBadgeHeight : (root.compactLandscapeLayout ? 26 : 30)
                radius: root.largeLandscapeLayout ? root.splitBadgeRadius : (root.compactLandscapeLayout ? 13 : 15)
                color: "#66000000"
                border.color: "#22ffffff"
                width: frameBadge.width + (root.largeLandscapeLayout ? root.splitBadgePadding : (root.compactLandscapeLayout ? 22 : 26))

                Text {
                    id: frameBadge
                    anchors.centerIn: parent
                    text: playbackController.frameCount > 0
                          ? ("FRAME " + (playbackController.currentFrameIndex + 1) + " / " + playbackController.frameCount)
                          : "FRAME 0 / 0"
                    color: "white"
                    font.family: interBold.font.family
                    font.weight: Font.Bold
                    font.pixelSize: root.largeLandscapeLayout ? root.splitBadgeFontSize : (root.compactLandscapeLayout ? 9 : 11)
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: root.compactLandscapeLayout ? 1.6 : 2.0
                    renderType: Text.NativeRendering
                }
            }
        }

        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: playerPreviewFrame.bottom
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            anchors.topMargin: 16
            spacing: 6
            visible: !root.compactLandscapeLayout

            Item {
                width: parent.width
                height: 72

                Column {
                    anchors.left: parent.left
                    anchors.right: infoButton.left
                    anchors.top: parent.top
                    anchors.rightMargin: 14
                    spacing: 4

                    Text {
                        text: playbackController.currentClipName.length > 0 ? playbackController.currentClipName : root.selectedClipName
                        color: "white"
                        font.family: interBold.font.family
                        font.pixelSize: 27
                        elide: Text.ElideRight
                        renderType: Text.NativeRendering
                    }

                    Text {
                        text: root.clipInfoText(playbackController.frameCount, playbackController.statusText)
                        color: "#8f9096"
                        font.family: interRegular.font.family
                        font.pixelSize: 14
                        wrapMode: Text.WordWrap
                        width: parent.width
                        renderType: Text.NativeRendering
                    }
                }

                Rectangle {
                    id: infoButton
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: 126
                    height: 52
                    radius: 18
                    color: infoArea.containsPress ? "#20ffffff" : "#14ffffff"
                    border.width: 1
                    border.color: "#1affffff"
                    scale: infoArea.containsPress ? 0.985 : 1.0
                    visible: playbackController.currentClipName.length > 0

                    Behavior on scale {
                        NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                    }

                    Behavior on color {
                        ColorAnimation { duration: 150; easing.type: Easing.OutCubic }
                    }

                    Row {
                        anchors.centerIn: parent
                        spacing: 8

                        Image {
                            width: 20
                            height: 20
                            source: "qrc:/qml/icons/info.png"
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            mipmap: true
                            opacity: 0.96
                        }

                        Text {
                            text: "Info"
                            color: "white"
                            font.family: interMedium.font.family
                            font.pixelSize: 17
                            renderType: Text.NativeRendering
                        }
                    }

                    MouseArea {
                        id: infoArea
                        anchors.fill: parent
                        onClicked: root.infoOpen = true
                    }
                }
            }

            Item {
                id: progressBarTrack
                width: parent.width
                height: 28

                Rectangle {
                    id: progressTrackRail
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    height: 8
                    radius: 4
                    color: "#1e1f23"
                }

                Rectangle {
                    width: progressTrackRail.width * root.playbackProgress()
                    height: progressTrackRail.height
                    radius: progressTrackRail.radius
                    anchors.left: progressTrackRail.left
                    anchors.verticalCenter: progressTrackRail.verticalCenter
                    color: "#d7d9de"
                }

                Rectangle {
                    id: progressHandle
                    width: 24
                    height: 24
                    radius: 12
                    anchors.verticalCenter: progressTrackRail.verticalCenter
                    x: Math.max(0, Math.min(progressTrackRail.width - width,
                                            (progressTrackRail.width * root.playbackProgress()) - width / 2))
                    color: "#f4f5f7"
                    border.width: 1
                    border.color: "#131417"

                    Rectangle {
                        anchors.centerIn: parent
                        width: 8
                        height: 8
                        radius: 4
                        color: "#151515"
                        opacity: 0.28
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onPressed: function(mouse) { root.scrubPlayback(mouse.x, progressTrackRail.width) }
                    onPositionChanged: function(mouse) {
                        if (pressed)
                            root.scrubPlayback(mouse.x, progressTrackRail.width)
                    }
                }
            }
        }
        }

        Rectangle {
            id: landscapeInfoPanel
            visible: root.splitLandscapePlaybackLayout
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.topMargin: playerPanel.landscapeContentMargin
            anchors.rightMargin: playerPanel.landscapeContentMargin
            anchors.bottomMargin: playerPanel.landscapeContentMargin
            width: playerPanel.landscapeInfoWidth
            radius: root.largeLandscapeLayout ? 30 : 24
            color: "#181818"
            border.width: 1
            border.color: "#1affffff"
            clip: true

            Flickable {
                id: landscapeInfoFlick
                anchors.fill: parent
                anchors.margins: root.largeLandscapeLayout ? 24 : 18
                contentWidth: width
                contentHeight: landscapeInfoColumn.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.VerticalFlick

                Column {
                    id: landscapeInfoColumn
                    width: landscapeInfoFlick.width
                    spacing: root.largeLandscapeLayout ? 22 : 18

                    Row {
                        width: parent.width
                        spacing: root.largeLandscapeLayout ? 14 : 12

                        Rectangle {
                            width: root.largeLandscapeLayout ? 52 : 42
                            height: width
                            radius: width / 2
                            color: "#1f1f1f"
                            border.width: 1
                            border.color: "#1affffff"

                            Text {
                                anchors.centerIn: parent
                                text: "i"
                                color: "white"
                                font.family: interBold.font.family
                                font.weight: Font.Bold
                                font.pixelSize: root.largeLandscapeLayout ? 24 : 19
                                renderType: Text.NativeRendering
                            }
                        }

                        Column {
                            width: parent.width - (root.largeLandscapeLayout ? 66 : 54)
                            spacing: root.largeLandscapeLayout ? 5 : 4

                            Text {
                                text: "Clip Metadata"
                                color: "white"
                                font.family: interBold.font.family
                                font.pixelSize: root.largeLandscapeLayout ? 31 : 25
                                renderType: Text.NativeRendering
                            }

                            Text {
                                width: parent.width
                                text: playbackController.currentClipName
                                color: "#8f9096"
                                font.family: interRegular.font.family
                                font.pixelSize: root.largeLandscapeLayout ? 17 : 14
                                elide: Text.ElideMiddle
                                renderType: Text.NativeRendering
                            }
                        }
                    }

                    Grid {
                        id: landscapeInfoGrid
                        width: parent.width
                        columns: 2
                        rowSpacing: root.largeLandscapeLayout ? 16 : 12
                        columnSpacing: root.largeLandscapeLayout ? 20 : 16
                        readonly property int fieldWidth: Math.floor((width - columnSpacing) / 2)

                        Column {
                            width: landscapeInfoGrid.fieldWidth
                            spacing: 4

                            Text {
                                text: "CAPTURED"
                                color: "#66ffffff"
                                font.family: interMedium.font.family
                                font.pixelSize: root.landscapeInfoLabelFontSize
                                font.capitalization: Font.AllUppercase
                                font.letterSpacing: 2.0
                                renderType: Text.NativeRendering
                            }

                            Text {
                                text: root.metadataValue("captured", "Unavailable")
                                color: "white"
                                font.family: interBold.font.family
                                font.pixelSize: root.landscapeInfoValueFontSize
                                wrapMode: Text.WordWrap
                                width: parent.width
                                renderType: Text.NativeRendering
                            }
                        }

                        Column {
                            width: landscapeInfoGrid.fieldWidth
                            spacing: 4

                            Text {
                                text: "TYPE"
                                color: "#66ffffff"
                                font.family: interMedium.font.family
                                font.pixelSize: root.landscapeInfoLabelFontSize
                                font.capitalization: Font.AllUppercase
                                font.letterSpacing: 2.0
                                renderType: Text.NativeRendering
                            }

                            Text {
                                text: root.metadataValue("type", "Unavailable")
                                color: "white"
                                font.family: interBold.font.family
                                font.pixelSize: root.landscapeInfoValueFontSize
                                wrapMode: Text.WordWrap
                                width: parent.width
                                renderType: Text.NativeRendering
                            }
                        }

                        Column {
                            width: landscapeInfoGrid.fieldWidth
                            spacing: 4

                            Text {
                                text: "RESOLUTION"
                                color: "#66ffffff"
                                font.family: interMedium.font.family
                                font.pixelSize: root.landscapeInfoLabelFontSize
                                font.capitalization: Font.AllUppercase
                                font.letterSpacing: 2.0
                                renderType: Text.NativeRendering
                            }

                            Text {
                                text: root.metadataValue("resolution", "Unavailable")
                                color: "white"
                                font.family: interBold.font.family
                                font.pixelSize: root.landscapeInfoValueFontSize
                                renderType: Text.NativeRendering
                            }
                        }

                        Column {
                            width: landscapeInfoGrid.fieldWidth
                            spacing: 4

                            Text {
                                text: "BIT DEPTH"
                                color: "#66ffffff"
                                font.family: interMedium.font.family
                                font.pixelSize: root.landscapeInfoLabelFontSize
                                font.capitalization: Font.AllUppercase
                                font.letterSpacing: 2.0
                                renderType: Text.NativeRendering
                            }

                            Text {
                                text: root.metadataValue("bitDepth", "Unavailable")
                                color: "white"
                                font.family: interBold.font.family
                                font.pixelSize: root.landscapeInfoValueFontSize
                                renderType: Text.NativeRendering
                            }
                        }

                        Column {
                            width: landscapeInfoGrid.fieldWidth
                            spacing: 4

                            Text {
                                text: "FRAME RATE"
                                color: "#66ffffff"
                                font.family: interMedium.font.family
                                font.pixelSize: root.landscapeInfoLabelFontSize
                                font.capitalization: Font.AllUppercase
                                font.letterSpacing: 2.0
                                renderType: Text.NativeRendering
                            }

                            Text {
                                text: root.metadataValue("frameRate", "Unavailable")
                                color: "white"
                                font.family: interBold.font.family
                                font.pixelSize: root.landscapeInfoValueFontSize
                                renderType: Text.NativeRendering
                            }
                        }

                        Column {
                            width: landscapeInfoGrid.fieldWidth
                            spacing: 4

                            Text {
                                text: "DURATION"
                                color: "#66ffffff"
                                font.family: interMedium.font.family
                                font.pixelSize: root.landscapeInfoLabelFontSize
                                font.capitalization: Font.AllUppercase
                                font.letterSpacing: 2.0
                                renderType: Text.NativeRendering
                            }

                            Text {
                                text: root.metadataValue("duration", "Unavailable")
                                color: "white"
                                font.family: interBold.font.family
                                font.pixelSize: root.landscapeInfoValueFontSize
                                renderType: Text.NativeRendering
                            }
                        }

                        Column {
                            width: landscapeInfoGrid.fieldWidth
                            spacing: 4

                            Text {
                                text: "FRAMES"
                                color: "#66ffffff"
                                font.family: interMedium.font.family
                                font.pixelSize: root.landscapeInfoLabelFontSize
                                font.capitalization: Font.AllUppercase
                                font.letterSpacing: 2.0
                                renderType: Text.NativeRendering
                            }

                            Text {
                                text: root.metadataValue("frames", "0")
                                color: "white"
                                font.family: interBold.font.family
                                font.pixelSize: root.landscapeInfoValueFontSize
                                renderType: Text.NativeRendering
                            }
                        }

                        Column {
                            width: landscapeInfoGrid.fieldWidth
                            spacing: 4

                            Text {
                                text: "CLIP SIZE"
                                color: "#66ffffff"
                                font.family: interMedium.font.family
                                font.pixelSize: root.landscapeInfoLabelFontSize
                                font.capitalization: Font.AllUppercase
                                font.letterSpacing: 2.0
                                renderType: Text.NativeRendering
                            }

                            Text {
                                text: root.metadataValue("clipSize", "Unavailable")
                                color: "white"
                                font.family: interBold.font.family
                                font.pixelSize: root.landscapeInfoValueFontSize
                                renderType: Text.NativeRendering
                            }
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: root.largeLandscapeLayout ? 116 : 92
                        radius: root.largeLandscapeLayout ? 22 : 18
                        color: "#151515"
                        border.width: 1
                        border.color: "#1affffff"

                        Column {
                            anchors.fill: parent
                            anchors.margins: root.largeLandscapeLayout ? 18 : 14
                            spacing: root.largeLandscapeLayout ? 7 : 5

                            Text {
                                text: "PATH"
                                color: "#66ffffff"
                                font.family: interMedium.font.family
                                font.pixelSize: root.landscapeInfoLabelFontSize
                                font.capitalization: Font.AllUppercase
                                font.letterSpacing: 2.0
                                renderType: Text.NativeRendering
                            }

                            Text {
                                text: root.metadataValue("path", "Unavailable")
                                color: "white"
                                font.family: interRegular.font.family
                                font.pixelSize: root.largeLandscapeLayout ? 16 : 13
                                wrapMode: Text.WrapAnywhere
                                width: parent.width
                                renderType: Text.NativeRendering
                            }
                        }
                    }
                }
            }
        }

        Row {
            id: landscapePlaybackControls
            visible: root.splitLandscapePlaybackLayout
            spacing: playerPanel.landscapeControlGap
            width: implicitWidth
            height: root.splitControlsHeight
            x: root.splitLandscapePlaybackLayout
               ? Math.round(playerPanel.landscapePreviewLeft + ((playerPreviewFrame.width - width) / 2))
               : 0
            anchors.bottom: parent.bottom
            anchors.bottomMargin: root.splitControlsBottomMargin

            Rectangle {
                id: landscapeDeleteButton
                y: Math.round((landscapePlaybackControls.height - height) / 2)
                width: root.splitButtonWidth
                height: root.splitButtonHeight
                radius: root.splitButtonRadius
                color: landscapeDeleteArea.containsPress ? "#a32828" : "#8d2020"
                border.width: 1
                border.color: "#ba4a4a"
                scale: landscapeDeleteArea.containsPress ? 0.95 : 1.0

                Behavior on scale {
                    NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                }

                Behavior on color {
                    ColorAnimation { duration: 140; easing.type: Easing.OutCubic }
                }

                Row {
                    anchors.centerIn: parent
                    spacing: root.largeLandscapeLayout ? 10 : 8

                    Image {
                        width: root.splitButtonIconSize
                        height: root.splitButtonIconSize
                        anchors.verticalCenter: parent.verticalCenter
                        source: "qrc:/qml/icons/delete.png"
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: true
                        opacity: 0.92
                    }

                    Text {
                        text: "Remove"
                        color: "white"
                        font.family: interMedium.font.family
                        font.pixelSize: root.splitButtonFontSize
                        renderType: Text.NativeRendering
                    }
                }

                MouseArea {
                    id: landscapeDeleteArea
                    anchors.fill: parent
                    onClicked: root.deleteConfirmOpen = true
                }
            }

            Row {
                id: landscapeTransportControls
                width: implicitWidth
                height: landscapePlaybackControls.height
                spacing: root.splitTransportSpacing

                Item {
                    width: root.splitSmallButtonBox
                    height: root.splitSmallButtonBox
                    anchors.verticalCenter: parent.verticalCenter
                    opacity: root.hasPreviousClip ? 1.0 : 0.45

                    Rectangle {
                        anchors.centerIn: parent
                        width: root.splitSmallButtonSize
                        height: root.splitSmallButtonSize
                        radius: width / 2
                        color: landscapePreviousClipArea.containsPress ? "#20ffffff" : "#14ffffff"
                        border.width: 1
                        border.color: "#1affffff"
                        scale: landscapePreviousClipArea.containsPress ? 0.95 : 1.0

                        Behavior on scale {
                            NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                        }

                        Image {
                            anchors.centerIn: parent
                            width: root.splitSmallIconSize
                            height: root.splitSmallIconSize
                            source: "qrc:/qml/icons/previous.png"
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            mipmap: true
                            opacity: 0.92
                        }

                        MouseArea {
                            id: landscapePreviousClipArea
                            anchors.fill: parent
                            enabled: root.hasPreviousClip && !clipRevealAnim.running
                            onClicked: {
                                root.clipTransitionDirection = -1
                                root.previousClipRequested()
                            }
                        }
                    }
                }

                Item {
                    width: root.splitPlayButtonBox
                    height: root.splitPlayButtonBox

                    Rectangle {
                        anchors.centerIn: parent
                        width: root.splitPlayButtonSize
                        height: root.splitPlayButtonSize
                        radius: width / 2
                        color: landscapePlayArea.containsPress ? "#20ffffff" : "#14ffffff"
                        border.width: 1
                        border.color: "#1affffff"
                        scale: landscapePlayArea.containsPress ? 0.94 : 1.0

                        Behavior on scale {
                            NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                        }

                        Image {
                            anchors.centerIn: parent
                            width: playbackController.playing ? root.splitPauseIconSize : root.splitPlayIconSize
                            height: playbackController.playing ? root.splitPauseIconSize : root.splitPlayIconSize
                            source: playbackController.playing ? "qrc:/qml/icons/pause.png" : "qrc:/qml/icons/play.png"
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            mipmap: true
                            opacity: 0.96
                        }

                        MouseArea {
                            id: landscapePlayArea
                            anchors.fill: parent
                            onClicked: playbackController.togglePlayback()
                        }
                    }
                }

                Item {
                    width: root.splitSmallButtonBox
                    height: root.splitSmallButtonBox
                    anchors.verticalCenter: parent.verticalCenter
                    opacity: root.hasNextClip ? 1.0 : 0.45

                    Rectangle {
                        anchors.centerIn: parent
                        width: root.splitSmallButtonSize
                        height: root.splitSmallButtonSize
                        radius: width / 2
                        color: landscapeNextClipArea.containsPress ? "#20ffffff" : "#14ffffff"
                        border.width: 1
                        border.color: "#1affffff"
                        scale: landscapeNextClipArea.containsPress ? 0.95 : 1.0

                        Behavior on scale {
                            NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                        }

                        Image {
                            anchors.centerIn: parent
                            width: root.splitSmallIconSize
                            height: root.splitSmallIconSize
                            source: "qrc:/qml/icons/next.png"
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            mipmap: true
                            opacity: 0.92
                        }

                        MouseArea {
                            id: landscapeNextClipArea
                            anchors.fill: parent
                            enabled: root.hasNextClip && !clipRevealAnim.running
                            onClicked: {
                                root.clipTransitionDirection = 1
                                root.nextClipRequested()
                            }
                        }
                    }
                }
            }

            Rectangle {
                id: landscapeHistogramPanel
                y: Math.round((landscapePlaybackControls.height - height) / 2)
                width: root.splitHistogramWidth
                height: root.splitHistogramHeight
                radius: root.splitHistogramRadius
                color: "#14ffffff"
                border.width: 1
                border.color: "#1affffff"

                Column {
                    anchors.fill: parent
                    anchors.margins: root.splitHistogramMargin
                    spacing: root.splitHistogramSpacing

                    Text {
                        text: "HISTOGRAM"
                        color: "#ccffffff"
                        font.family: interMedium.font.family
                        font.pixelSize: root.splitHistogramLabelFontSize
                        font.capitalization: Font.AllUppercase
                        font.letterSpacing: 1.6
                        renderType: Text.NativeRendering
                    }

                    Canvas {
                        id: landscapeHistogramCanvas
                        width: parent.width
                        height: root.splitHistogramCanvasHeight
                        antialiasing: true

                        onPaint: {
                            var ctx = getContext("2d")
                            var bins = playbackController.histogramBins
                            ctx.clearRect(0, 0, width, height)

                            ctx.strokeStyle = "rgba(255,255,255,0.18)"
                            ctx.beginPath()
                            ctx.moveTo(0, height - 0.5)
                            ctx.lineTo(width, height - 0.5)
                            ctx.stroke()

                            if (!bins || bins.length === 0)
                                return

                            var gap = 1
                            var barCount = bins.length
                            var barWidth = Math.max(1, (width - gap * (barCount - 1)) / barCount)
                            var x = 0
                            var gradient = ctx.createLinearGradient(0, 0, 0, height)
                            gradient.addColorStop(0.0, "rgba(255,255,255,0.96)")
                            gradient.addColorStop(1.0, "rgba(255,255,255,0.42)")
                            ctx.fillStyle = gradient

                            for (var i = 0; i < barCount; ++i) {
                                var value = Number(bins[i])
                                var barHeight = Math.max(2, value * (height - 2))
                                ctx.fillRect(x, height - barHeight, barWidth, barHeight)
                                x += barWidth + gap
                            }
                        }

                        onWidthChanged: requestPaint()
                        onHeightChanged: requestPaint()

                        Component.onCompleted: requestPaint()

                        Connections {
                            target: playbackController

                            function onHistogramChanged() {
                                landscapeHistogramCanvas.requestPaint()
                            }
                        }
                    }
                }
            }
        }

        Item {
            id: compactProgressBar
            visible: root.compactLandscapeLayout
            anchors.left: root.splitLandscapePlaybackLayout ? undefined : parent.left
            anchors.right: root.splitLandscapePlaybackLayout ? undefined : parent.right
            anchors.bottom: root.splitLandscapePlaybackLayout ? landscapePlaybackControls.top : transportControls.top
            anchors.leftMargin: root.splitLandscapePlaybackLayout ? 0 : (root.compactLandscapeLayout ? 18 : 88)
            anchors.rightMargin: root.splitLandscapePlaybackLayout ? 0 : (root.compactLandscapeLayout ? 16 : 88)
            anchors.bottomMargin: root.splitLandscapePlaybackLayout ? (root.largeLandscapeLayout ? 12 : 8) : (root.regularLandscapeLayout ? 6 : 2)
            x: root.splitLandscapePlaybackLayout ? playerPanel.landscapePreviewLeft : 0
            width: root.splitLandscapePlaybackLayout ? playerPreviewFrame.width : 0
            height: root.largeLandscapeLayout ? root.splitProgressHeight : (root.regularLandscapeLayout ? 30 : 22)

            Rectangle {
                id: compactProgressRail
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: root.largeLandscapeLayout ? root.splitProgressRailHeight : (root.regularLandscapeLayout ? 8 : 6)
                radius: height / 2
                color: "#1e1f23"
            }

            Rectangle {
                width: compactProgressRail.width * root.playbackProgress()
                height: compactProgressRail.height
                radius: compactProgressRail.radius
                anchors.left: compactProgressRail.left
                anchors.verticalCenter: compactProgressRail.verticalCenter
                color: "#d7d9de"
            }

            Rectangle {
                width: root.largeLandscapeLayout ? root.splitProgressHandleSize : (root.regularLandscapeLayout ? 24 : 18)
                height: root.largeLandscapeLayout ? root.splitProgressHandleSize : (root.regularLandscapeLayout ? 24 : 18)
                radius: height / 2
                anchors.verticalCenter: compactProgressRail.verticalCenter
                x: Math.max(0, Math.min(compactProgressRail.width - width,
                                        (compactProgressRail.width * root.playbackProgress()) - width / 2))
                color: "#f4f5f7"
                border.width: 1
                border.color: "#131417"

                Rectangle {
                    anchors.centerIn: parent
                    width: root.largeLandscapeLayout ? root.splitProgressHandleDotSize : (root.regularLandscapeLayout ? 8 : 6)
                    height: root.largeLandscapeLayout ? root.splitProgressHandleDotSize : (root.regularLandscapeLayout ? 8 : 6)
                    radius: height / 2
                    color: "#151515"
                    opacity: 0.28
                }
            }

            MouseArea {
                anchors.fill: parent
                onPressed: function(mouse) { root.scrubPlayback(mouse.x, compactProgressRail.width) }
                onPositionChanged: function(mouse) {
                    if (pressed)
                        root.scrubPlayback(mouse.x, compactProgressRail.width)
                }
            }
        }

        Rectangle {
            id: deleteButton
            visible: !root.splitLandscapePlaybackLayout
            anchors.left: root.splitLandscapePlaybackLayout ? undefined : parent.left
            anchors.right: undefined
            anchors.bottom: parent.bottom
            anchors.leftMargin: root.splitLandscapePlaybackLayout ? 0 : (root.compactLandscapeLayout ? 18 : 18)
            anchors.rightMargin: 0
            anchors.bottomMargin: root.regularLandscapeLayout ? 12 : (root.compactLandscapeLayout ? 12 : 18)
            x: root.splitLandscapePlaybackLayout ? playerPanel.landscapeControlsLeft : 0
            width: root.splitLandscapePlaybackLayout ? 142 : (root.regularLandscapeLayout ? 118 : (root.compactLandscapeLayout ? 100 : 132))
            height: root.splitLandscapePlaybackLayout ? histogramPanel.height : (root.regularLandscapeLayout ? 44 : (root.compactLandscapeLayout ? 38 : 54))
            radius: root.splitLandscapePlaybackLayout ? histogramPanel.radius : (root.regularLandscapeLayout ? 15 : (root.compactLandscapeLayout ? 13 : 18))
            color: deleteArea.containsPress ? "#a32828" : "#8d2020"
            border.width: 1
            border.color: "#ba4a4a"
            scale: deleteArea.containsPress ? 0.95 : 1.0

            Behavior on scale {
                NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
            }

            Behavior on color {
                ColorAnimation { duration: 140; easing.type: Easing.OutCubic }
            }

            Row {
                anchors.centerIn: parent
                spacing: root.splitLandscapePlaybackLayout ? 8 : (root.regularLandscapeLayout ? 7 : (root.compactLandscapeLayout ? 6 : 8))

                Image {
                    width: root.splitLandscapePlaybackLayout ? 17 : (root.regularLandscapeLayout ? 15 : (root.compactLandscapeLayout ? 13 : 18))
                    height: root.splitLandscapePlaybackLayout ? 17 : (root.regularLandscapeLayout ? 15 : (root.compactLandscapeLayout ? 13 : 18))
                    anchors.verticalCenter: parent.verticalCenter
                    source: "qrc:/qml/icons/delete.png"
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                    opacity: 0.92
                }

                Text {
                    text: "Remove"
                    color: "white"
                    font.family: interMedium.font.family
                    font.pixelSize: root.splitLandscapePlaybackLayout ? 15 : (root.regularLandscapeLayout ? 14 : (root.compactLandscapeLayout ? 12 : 16))
                    renderType: Text.NativeRendering
                }
            }

            MouseArea {
                id: deleteArea
                anchors.fill: parent
                onClicked: root.deleteConfirmOpen = true
            }
        }

        Rectangle {
            id: histogramPanel
            visible: !root.splitLandscapePlaybackLayout
            anchors.right: root.splitLandscapePlaybackLayout ? undefined : parent.right
            anchors.left: undefined
            anchors.bottom: parent.bottom
            anchors.rightMargin: root.splitLandscapePlaybackLayout ? 0 : (root.compactLandscapeLayout ? 16 : 18)
            anchors.leftMargin: 0
            anchors.bottomMargin: root.regularLandscapeLayout ? 10 : (root.compactLandscapeLayout ? 8 : 12)
            x: root.splitLandscapePlaybackLayout
               ? (playerPanel.landscapeControlsLeft
                  + deleteButton.width
                  + playerPanel.landscapeControlGap
                  + playerPanel.landscapeTransportWidth
                  + playerPanel.landscapeControlGap)
               : 0
            width: root.regularLandscapeLayout ? 142 : (root.landscapeCompactLayout ? 92 : (root.compactLandscapeLayout ? 116 : 152))
            height: root.regularLandscapeLayout ? 58 : (root.compactLandscapeLayout ? 48 : 68)
            radius: root.regularLandscapeLayout ? 15 : (root.compactLandscapeLayout ? 13 : 18)
            color: "#14ffffff"
            border.width: 1
            border.color: "#1affffff"

            Column {
                anchors.fill: parent
                anchors.margins: root.regularLandscapeLayout ? 8 : (root.compactLandscapeLayout ? 7 : 10)
                spacing: root.regularLandscapeLayout ? 5 : (root.compactLandscapeLayout ? 4 : 8)

                Text {
                    text: "HISTOGRAM"
                    color: "#ccffffff"
                    font.family: interMedium.font.family
                    font.pixelSize: root.regularLandscapeLayout ? 8 : (root.compactLandscapeLayout ? 7 : 10)
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: root.regularLandscapeLayout ? 1.6 : (root.compactLandscapeLayout ? 1.4 : 2.2)
                    renderType: Text.NativeRendering
                }

                Canvas {
                    id: histogramCanvas
                    width: parent.width
                    height: root.regularLandscapeLayout ? 28 : (root.compactLandscapeLayout ? 22 : 30)
                    antialiasing: true

                    onPaint: {
                        var ctx = getContext("2d")
                        var bins = playbackController.histogramBins
                        ctx.clearRect(0, 0, width, height)

                        ctx.strokeStyle = "rgba(255,255,255,0.18)"
                        ctx.beginPath()
                        ctx.moveTo(0, height - 0.5)
                        ctx.lineTo(width, height - 0.5)
                        ctx.stroke()

                        if (!bins || bins.length === 0)
                            return

                        var gap = 1
                        var barCount = bins.length
                        var barWidth = Math.max(1, (width - gap * (barCount - 1)) / barCount)
                        var x = 0
                        var gradient = ctx.createLinearGradient(0, 0, 0, height)
                        gradient.addColorStop(0.0, "rgba(255,255,255,0.96)")
                        gradient.addColorStop(1.0, "rgba(255,255,255,0.42)")
                        ctx.fillStyle = gradient

                        for (var i = 0; i < barCount; ++i) {
                            var value = Number(bins[i])
                            var barHeight = Math.max(2, value * (height - 2))
                            ctx.fillRect(x, height - barHeight, barWidth, barHeight)
                            x += barWidth + gap
                        }
                    }

                    onWidthChanged: requestPaint()
                    onHeightChanged: requestPaint()

                    Component.onCompleted: requestPaint()

                    Connections {
                        target: playbackController

                        function onHistogramChanged() {
                            histogramCanvas.requestPaint()
                        }
                    }
                }
            }
        }

        Rectangle {
            id: compactInfoButton
            visible: (root.landscapeCompactLayout || (root.regularLandscapeLayout && !root.splitLandscapePlaybackLayout))
                     && playbackController.currentClipName.length > 0
            anchors.right: histogramPanel.left
            anchors.bottom: histogramPanel.bottom
            anchors.rightMargin: root.regularLandscapeLayout ? 10 : 8
            width: root.splitLandscapePlaybackLayout ? histogramPanel.width : (root.regularLandscapeLayout ? 88 : 42)
            height: root.regularLandscapeLayout ? histogramPanel.height : histogramPanel.height
            radius: root.regularLandscapeLayout ? histogramPanel.radius : histogramPanel.radius
            color: compactInfoArea.containsPress ? "#20ffffff" : "#14ffffff"
            border.width: 1
            border.color: "#1affffff"
            scale: compactInfoArea.containsPress ? 0.96 : 1.0

            Behavior on scale {
                NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
            }

            Behavior on color {
                ColorAnimation { duration: 150; easing.type: Easing.OutCubic }
            }

            Row {
                anchors.centerIn: parent
                spacing: root.splitLandscapePlaybackLayout ? 8 : (root.regularLandscapeLayout ? 7 : 0)

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    width: root.splitLandscapePlaybackLayout ? 22 : (root.regularLandscapeLayout ? 18 : 17)
                    height: root.splitLandscapePlaybackLayout ? 22 : (root.regularLandscapeLayout ? 18 : 17)
                    source: "qrc:/qml/icons/info.png"
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                    opacity: 0.96
                }

                Text {
                    visible: root.regularLandscapeLayout
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Info"
                    color: "white"
                    font.family: interMedium.font.family
                    font.pixelSize: root.splitLandscapePlaybackLayout ? 18 : 14
                    renderType: Text.NativeRendering
                }
            }

            MouseArea {
                id: compactInfoArea
                anchors.fill: parent
                onClicked: root.infoOpen = true
            }
        }

        Row {
            id: transportControls
            visible: !root.splitLandscapePlaybackLayout
            anchors.horizontalCenter: root.splitLandscapePlaybackLayout ? undefined : parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: root.regularLandscapeLayout ? 12 : (root.compactLandscapeLayout ? 9 : 18)
            spacing: root.regularLandscapeLayout ? 10 : (root.compactLandscapeLayout ? 8 : 14)
            x: root.splitLandscapePlaybackLayout
               ? (playerPanel.landscapeControlsLeft + deleteButton.width + playerPanel.landscapeControlGap)
               : 0

            Item {
                width: root.regularLandscapeLayout ? 52 : (root.compactLandscapeLayout ? 42 : 58)
                height: root.regularLandscapeLayout ? 62 : (root.compactLandscapeLayout ? 52 : 78)
                opacity: root.hasPreviousClip ? 1.0 : 0.45

                Rectangle {
                    anchors.centerIn: parent
                    width: root.regularLandscapeLayout ? 50 : (root.compactLandscapeLayout ? 40 : 58)
                    height: root.regularLandscapeLayout ? 50 : (root.compactLandscapeLayout ? 40 : 58)
                    radius: width / 2
                    color: previousClipArea.containsPress ? "#20ffffff" : "#14ffffff"
                    border.width: 1
                    border.color: "#1affffff"
                    scale: previousClipArea.containsPress ? 0.95 : 1.0

                    Behavior on scale {
                        NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                    }

                    Image {
                        anchors.centerIn: parent
                        width: root.regularLandscapeLayout ? 19 : (root.compactLandscapeLayout ? 16 : 22)
                        height: root.regularLandscapeLayout ? 19 : (root.compactLandscapeLayout ? 16 : 22)
                        source: "qrc:/qml/icons/previous.png"
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: true
                        opacity: 0.92
                    }

                    MouseArea {
                        id: previousClipArea
                        anchors.fill: parent
                        enabled: root.hasPreviousClip && !clipRevealAnim.running
                        onClicked: {
                            root.clipTransitionDirection = -1
                            root.previousClipRequested()
                        }
                    }
                }
            }

            Item {
                width: root.regularLandscapeLayout ? 66 : (root.compactLandscapeLayout ? 56 : 78)
                height: root.regularLandscapeLayout ? 66 : (root.compactLandscapeLayout ? 56 : 78)

                Rectangle {
                    anchors.centerIn: parent
                    width: root.regularLandscapeLayout ? 64 : (root.compactLandscapeLayout ? 54 : 78)
                    height: root.regularLandscapeLayout ? 64 : (root.compactLandscapeLayout ? 54 : 78)
                    radius: width / 2
                    color: playArea.containsPress ? "#20ffffff" : "#14ffffff"
                    border.width: 1
                    border.color: "#1affffff"
                    scale: playArea.containsPress ? 0.94 : 1.0

                    Behavior on scale {
                        NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                    }

                    Image {
                        anchors.centerIn: parent
                        width: playbackController.playing
                               ? (root.regularLandscapeLayout ? 22 : (root.compactLandscapeLayout ? 18 : 24))
                               : (root.regularLandscapeLayout ? 24 : (root.compactLandscapeLayout ? 20 : 26))
                        height: playbackController.playing
                                ? (root.regularLandscapeLayout ? 22 : (root.compactLandscapeLayout ? 18 : 24))
                                : (root.regularLandscapeLayout ? 24 : (root.compactLandscapeLayout ? 20 : 26))
                        source: playbackController.playing ? "qrc:/qml/icons/pause.png" : "qrc:/qml/icons/play.png"
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: true
                        opacity: 0.96
                    }

                    MouseArea {
                        id: playArea
                        anchors.fill: parent
                        onClicked: playbackController.togglePlayback()
                    }
                }
            }

            Item {
                width: root.regularLandscapeLayout ? 52 : (root.compactLandscapeLayout ? 42 : 58)
                height: root.regularLandscapeLayout ? 62 : (root.compactLandscapeLayout ? 52 : 78)
                opacity: root.hasNextClip ? 1.0 : 0.45

                Rectangle {
                    anchors.centerIn: parent
                    width: root.regularLandscapeLayout ? 50 : (root.compactLandscapeLayout ? 40 : 58)
                    height: root.regularLandscapeLayout ? 50 : (root.compactLandscapeLayout ? 40 : 58)
                    radius: width / 2
                    color: nextClipArea.containsPress ? "#20ffffff" : "#14ffffff"
                    border.width: 1
                    border.color: "#1affffff"
                    scale: nextClipArea.containsPress ? 0.95 : 1.0

                    Behavior on scale {
                        NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                    }

                    Image {
                        anchors.centerIn: parent
                        width: root.regularLandscapeLayout ? 19 : (root.compactLandscapeLayout ? 16 : 22)
                        height: root.regularLandscapeLayout ? 19 : (root.compactLandscapeLayout ? 16 : 22)
                        source: "qrc:/qml/icons/next.png"
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: true
                        opacity: 0.92
                    }

                    MouseArea {
                        id: nextClipArea
                        anchors.fill: parent
                        enabled: root.hasNextClip && !clipRevealAnim.running
                        onClicked: {
                            root.clipTransitionDirection = 1
                            root.nextClipRequested()
                        }
                    }
                }
            }
        }

        ParallelAnimation {
            id: clipRevealAnim

            onFinished: {
                clipContent.x = 0
                clipContent.opacity = 1.0
                clipContent.scale = 1.0
                root.clipTransitionDirection = 0
            }

            NumberAnimation {
                target: clipContent
                property: "x"
                from: clipContent.x
                to: 0
                duration: 320
                easing.type: Easing.OutCubic
            }

            NumberAnimation {
                target: clipContent
                property: "opacity"
                from: 0.0
                to: 1.0
                duration: 260
                easing.type: Easing.OutCubic
            }

            NumberAnimation {
                target: clipContent
                property: "scale"
                from: 0.985
                to: 1.0
                duration: 320
                easing.type: Easing.OutCubic
            }
        }

        Rectangle {
            parent: root
            anchors.fill: parent
            radius: 28
            color: "#cc000000"
            visible: root.infoOpen && !root.splitLandscapePlaybackLayout
            opacity: root.infoOpen && !root.splitLandscapePlaybackLayout ? 1.0 : 0.0
            z: 18

            Behavior on opacity {
                NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: root.infoOpen = false
            }
        }

        Rectangle {
            parent: root
            width: 448
            height: infoColumn.implicitHeight + 32
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -6
            radius: 26
            color: "#151515"
            border.width: 1
            border.color: "#1affffff"
            visible: root.infoOpen && !root.splitLandscapePlaybackLayout
            opacity: root.infoOpen && !root.splitLandscapePlaybackLayout ? 1.0 : 0.0
            scale: root.infoOpen && !root.splitLandscapePlaybackLayout ? 1.0 : 0.96
            z: 19

            Behavior on opacity {
                NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
            }

            Behavior on scale {
                NumberAnimation { duration: 240; easing.type: Easing.OutCubic }
            }

            Column {
                id: infoColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                anchors.topMargin: 16
                spacing: 16

                Row {
                    spacing: 12

                    Rectangle {
                        width: 40
                        height: 40
                        radius: 20
                        color: "#1d1d1d"
                        border.width: 1
                        border.color: "#1affffff"

                        Text {
                            anchors.centerIn: parent
                            text: "i"
                            color: "white"
                            font.family: interBold.font.family
                            font.weight: Font.Bold
                            font.pixelSize: 18
                            renderType: Text.NativeRendering
                        }
                    }

                    Column {
                        spacing: 4

                        Text {
                            text: "Clip Metadata"
                            color: "white"
                            font.family: interBold.font.family
                            font.pixelSize: 24
                            renderType: Text.NativeRendering
                        }

                        Text {
                            text: playbackController.currentClipName
                            color: "#8f9096"
                            font.family: interRegular.font.family
                            font.pixelSize: 14
                            elide: Text.ElideMiddle
                            width: 320
                            renderType: Text.NativeRendering
                        }
                    }
                }

                Grid {
                    width: parent.width
                    columns: 2
                    rowSpacing: 10
                    columnSpacing: 20

                    Column {
                        width: 188
                        spacing: 4

                        Text {
                            text: "CAPTURED"
                            color: "#66ffffff"
                            font.family: interMedium.font.family
                            font.pixelSize: 11
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 2.0
                            renderType: Text.NativeRendering
                        }

                        Text {
                            text: root.metadataValue("captured", "Unavailable")
                            color: "white"
                            font.family: interBold.font.family
                            font.pixelSize: 15
                            wrapMode: Text.WordWrap
                            width: parent.width
                            renderType: Text.NativeRendering
                        }
                    }

                    Column {
                        width: 188
                        spacing: 4

                        Text {
                            text: "TYPE"
                            color: "#66ffffff"
                            font.family: interMedium.font.family
                            font.pixelSize: 11
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 2.0
                            renderType: Text.NativeRendering
                        }

                        Text {
                            text: root.metadataValue("type", "Unavailable")
                            color: "white"
                            font.family: interBold.font.family
                            font.pixelSize: 15
                            wrapMode: Text.WordWrap
                            width: parent.width
                            renderType: Text.NativeRendering
                        }
                    }

                    Column {
                        width: 188
                        spacing: 4

                        Text {
                            text: "RESOLUTION"
                            color: "#66ffffff"
                            font.family: interMedium.font.family
                            font.pixelSize: 11
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 2.0
                            renderType: Text.NativeRendering
                        }

                        Text {
                            text: root.metadataValue("resolution", "Unavailable")
                            color: "white"
                            font.family: interBold.font.family
                            font.pixelSize: 15
                            renderType: Text.NativeRendering
                        }
                    }

                    Column {
                        width: 188
                        spacing: 4

                        Text {
                            text: "BIT DEPTH"
                            color: "#66ffffff"
                            font.family: interMedium.font.family
                            font.pixelSize: 11
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 2.0
                            renderType: Text.NativeRendering
                        }

                        Text {
                            text: root.metadataValue("bitDepth", "Unavailable")
                            color: "white"
                            font.family: interBold.font.family
                            font.pixelSize: 15
                            renderType: Text.NativeRendering
                        }
                    }

                    Column {
                        width: 188
                        spacing: 4

                        Text {
                            text: "FRAME RATE"
                            color: "#66ffffff"
                            font.family: interMedium.font.family
                            font.pixelSize: 11
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 2.0
                            renderType: Text.NativeRendering
                        }

                        Text {
                            text: root.metadataValue("frameRate", "Unavailable")
                            color: "white"
                            font.family: interBold.font.family
                            font.pixelSize: 15
                            renderType: Text.NativeRendering
                        }
                    }

                    Column {
                        width: 188
                        spacing: 4

                        Text {
                            text: "DURATION"
                            color: "#66ffffff"
                            font.family: interMedium.font.family
                            font.pixelSize: 11
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 2.0
                            renderType: Text.NativeRendering
                        }

                        Text {
                            text: root.metadataValue("duration", "Unavailable")
                            color: "white"
                            font.family: interBold.font.family
                            font.pixelSize: 15
                            renderType: Text.NativeRendering
                        }
                    }

                    Column {
                        width: 188
                        spacing: 4

                        Text {
                            text: "FRAMES"
                            color: "#66ffffff"
                            font.family: interMedium.font.family
                            font.pixelSize: 11
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 2.0
                            renderType: Text.NativeRendering
                        }

                        Text {
                            text: root.metadataValue("frames", "0")
                            color: "white"
                            font.family: interBold.font.family
                            font.pixelSize: 15
                            renderType: Text.NativeRendering
                        }
                    }

                    Column {
                        width: 188
                        spacing: 4

                        Text {
                            text: "CLIP SIZE"
                            color: "#66ffffff"
                            font.family: interMedium.font.family
                            font.pixelSize: 11
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 2.0
                            renderType: Text.NativeRendering
                        }

                        Text {
                            text: root.metadataValue("clipSize", "Unavailable")
                            color: "white"
                            font.family: interBold.font.family
                            font.pixelSize: 15
                            renderType: Text.NativeRendering
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 84
                    radius: 18
                    color: "#181818"
                    border.width: 1
                    border.color: "#1affffff"

                    Column {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 4

                        Text {
                            text: "PATH"
                            color: "#66ffffff"
                            font.family: interMedium.font.family
                            font.pixelSize: 11
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 2.0
                            renderType: Text.NativeRendering
                        }

                        Text {
                            text: root.metadataValue("path", "Unavailable")
                            color: "white"
                            font.family: interRegular.font.family
                            font.pixelSize: 13
                            wrapMode: Text.WrapAnywhere
                            width: parent.width
                            renderType: Text.NativeRendering
                        }
                    }
                }

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 132
                    height: 52
                    radius: 18
                    color: infoCloseArea.containsPress ? "#20ffffff" : "#14ffffff"
                    border.width: 1
                    border.color: "#1affffff"
                    scale: infoCloseArea.containsPress ? 0.985 : 1.0

                    Behavior on scale {
                        NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                    }

                    Text {
                        anchors.centerIn: parent
                        text: "Close"
                        color: "white"
                        font.family: interMedium.font.family
                        font.pixelSize: 16
                        renderType: Text.NativeRendering
                    }

                    MouseArea {
                        id: infoCloseArea
                        anchors.fill: parent
                        onClicked: root.infoOpen = false
                    }
                }
            }
        }

        Rectangle {
            parent: root
            anchors.fill: parent
            radius: 28
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
            parent: root
            width: root.compactLandscapeLayout ? 340 : 392
            height: root.compactLandscapeLayout ? 218 : 300
            anchors.centerIn: parent
            anchors.verticalCenterOffset: root.compactLandscapeLayout ? -8 : -18
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
                            text: "Remove Clip?"
                            color: "white"
                            font.family: interBold.font.family
                            font.pixelSize: root.compactLandscapeLayout ? 20 : 24
                            renderType: Text.NativeRendering
                        }

                        Text {
                            text: "This will permanently delete the current clip from media."
                            color: "#8f9096"
                            font.family: interRegular.font.family
                            font.pixelSize: root.compactLandscapeLayout ? 12 : 14
                            wrapMode: Text.WordWrap
                            width: root.compactLandscapeLayout ? 240 : 280
                            renderType: Text.NativeRendering
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: root.compactLandscapeLayout ? 54 : 70
                    radius: root.compactLandscapeLayout ? 16 : 18
                    color: "#181818"
                    border.width: 1
                    border.color: "#1affffff"

                    Column {
                        anchors.fill: parent
                        anchors.margins: root.compactLandscapeLayout ? 12 : 14
                        spacing: root.compactLandscapeLayout ? 3 : 4

                        Text {
                            text: playbackController.currentClipName.length > 0 ? playbackController.currentClipName : root.selectedClipName
                            color: "white"
                            font.family: interBold.font.family
                            font.pixelSize: root.compactLandscapeLayout ? 13 : 16
                            elide: Text.ElideRight
                            renderType: Text.NativeRendering
                        }

                        Text {
                            text: "This action cannot be undone."
                            color: "#6f7076"
                            font.family: interRegular.font.family
                            font.pixelSize: root.compactLandscapeLayout ? 11 : 13
                            renderType: Text.NativeRendering
                        }
                    }
                }

                Item {
                    width: 1
                    height: root.compactLandscapeLayout ? -2 : 1
                }

                Row {
                    anchors.horizontalCenter: undefined
                    spacing: root.compactLandscapeLayout ? 10 : 12

                    Rectangle {
                        width: root.compactLandscapeLayout ? 132 : 150
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
                        width: root.compactLandscapeLayout ? 160 : 182
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
                                text: "Remove Clip"
                                color: "white"
                                font.family: interMedium.font.family
                                font.pixelSize: root.compactLandscapeLayout ? 13 : 16
                                renderType: Text.NativeRendering
                            }
                        }

                        MouseArea {
                            id: confirmDeleteArea
                            anchors.fill: parent
                            onClicked: {
                                root.deleteConfirmOpen = false
                                playbackController.stop()
                                root.deleteRequested()
                            }
                        }
                    }
                }
            }
        }
    }
}
