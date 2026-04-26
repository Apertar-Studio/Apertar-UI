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

    signal backRequested()
    signal previousClipRequested()
    signal nextClipRequested()
    signal deleteRequested()

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

    function currentClipTitle() {
        return playbackController.currentClipName.length > 0 ? playbackController.currentClipName : root.selectedClipName
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
            text: "Clip Viewer"
            color: "white"
            font.family: interBold.font.family
            font.pixelSize: 34
            renderType: Text.NativeRendering
        }
    }

    Rectangle {
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
                text: "Back"
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
        anchors.margins: 24
        anchors.topMargin: 112
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

        Item {
            id: clipContent
            anchors.fill: parent

        Rectangle {
            id: playerPreviewFrame
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: 18
            anchors.rightMargin: 18
            anchors.topMargin: 18
            height: 344
            radius: 24
            clip: true
            color: "#171717"
            border.width: 1
            border.color: "#1affffff"

            Image {
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
                cache: playbackController.playing
                asynchronous: false
                smooth: !playbackController.playing
                sourceSize.width: playbackController.playing
                                  ? Math.max(2, Math.round(width * 0.75))
                                  : Math.max(2, Math.round(width))
                sourceSize.height: playbackController.playing
                                   ? Math.max(2, Math.round(height * 0.75))
                                   : Math.max(2, Math.round(height))
                source: {
                    var activeSource = playbackController.playing
                                     ? playbackController.fastFrameSource
                                     : playbackController.frameSource
                    return activeSource.indexOf("?") >= 0
                         ? activeSource + "&radius=24"
                         : activeSource
                }
            }

            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.rightMargin: 14
                anchors.topMargin: 14
                height: 30
                radius: 15
                color: "#66000000"
                border.color: "#22ffffff"
                width: fpsChipLabel.width + 26

                Text {
                    id: fpsChipLabel
                    anchors.centerIn: parent
                    text: root.currentShotFps().toUpperCase()
                    color: "white"
                    font.family: interBold.font.family
                    font.weight: Font.Bold
                    font.pixelSize: 11
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 2.0
                    renderType: Text.NativeRendering
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.leftMargin: 14
                anchors.bottomMargin: 14
                height: 30
                radius: 15
                color: "#66000000"
                border.color: "#22ffffff"
                width: playStateLabel.width + 26

                Text {
                    id: playStateLabel
                    anchors.centerIn: parent
                    text: playbackController.playing ? "PLAYING" : "PAUSED"
                    color: "white"
                    font.family: interBold.font.family
                    font.weight: Font.Bold
                    font.pixelSize: 11
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 2.2
                    renderType: Text.NativeRendering
                }
            }

            Rectangle {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.rightMargin: 14
                anchors.bottomMargin: 14
                height: 30
                radius: 15
                color: "#66000000"
                border.color: "#22ffffff"
                width: frameBadge.width + 26

                Text {
                    id: frameBadge
                    anchors.centerIn: parent
                    text: playbackController.frameCount > 0
                          ? ("FRAME " + (playbackController.currentFrameIndex + 1) + " / " + playbackController.frameCount)
                          : "FRAME 0 / 0"
                    color: "white"
                    font.family: interBold.font.family
                    font.weight: Font.Bold
                    font.pixelSize: 11
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 2.0
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
            id: deleteButton
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.leftMargin: 18
            anchors.bottomMargin: 18
            width: 132
            height: 54
            radius: 18
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
                    text: "Remove"
                    color: "white"
                    font.family: interMedium.font.family
                    font.pixelSize: 16
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
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: 18
            anchors.bottomMargin: 12
            width: 152
            height: 68
            radius: 18
            color: "#14ffffff"
            border.width: 1
            border.color: "#1affffff"

            Column {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                Text {
                    text: "HISTOGRAM"
                    color: "#ccffffff"
                    font.family: interMedium.font.family
                    font.pixelSize: 10
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 2.2
                    renderType: Text.NativeRendering
                }

                Canvas {
                    id: histogramCanvas
                    width: parent.width
                    height: 30
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

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 18
            spacing: 14

            Item {
                width: 58
                height: 78
                opacity: root.hasPreviousClip ? 1.0 : 0.45

                Rectangle {
                    anchors.centerIn: parent
                    width: 58
                    height: 58
                    radius: 29
                    color: previousClipArea.containsPress ? "#20ffffff" : "#14ffffff"
                    border.width: 1
                    border.color: "#1affffff"
                    scale: previousClipArea.containsPress ? 0.95 : 1.0

                    Behavior on scale {
                        NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                    }

                    Image {
                        anchors.centerIn: parent
                        width: 22
                        height: 22
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
                width: 78
                height: 78

                Rectangle {
                    anchors.centerIn: parent
                    width: 78
                    height: 78
                    radius: 39
                    color: playArea.containsPress ? "#20ffffff" : "#14ffffff"
                    border.width: 1
                    border.color: "#1affffff"
                    scale: playArea.containsPress ? 0.94 : 1.0

                    Behavior on scale {
                        NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                    }

                    Image {
                        anchors.centerIn: parent
                        width: playbackController.playing ? 24 : 26
                        height: playbackController.playing ? 24 : 26
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
                width: 58
                height: 78
                opacity: root.hasNextClip ? 1.0 : 0.45

                Rectangle {
                    anchors.centerIn: parent
                    width: 58
                    height: 58
                    radius: 29
                    color: nextClipArea.containsPress ? "#20ffffff" : "#14ffffff"
                    border.width: 1
                    border.color: "#1affffff"
                    scale: nextClipArea.containsPress ? 0.95 : 1.0

                    Behavior on scale {
                        NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                    }

                    Image {
                        anchors.centerIn: parent
                        width: 22
                        height: 22
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
            visible: root.infoOpen
            opacity: root.infoOpen ? 1.0 : 0.0
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
            visible: root.infoOpen
            opacity: root.infoOpen ? 1.0 : 0.0
            scale: root.infoOpen ? 1.0 : 0.96
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
            width: 392
            height: 300
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -18
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
                            text: "Remove Clip?"
                            color: "white"
                            font.family: interBold.font.family
                            font.pixelSize: 24
                            renderType: Text.NativeRendering
                        }

                        Text {
                            text: "This will permanently delete the current clip from media."
                            color: "#8f9096"
                            font.family: interRegular.font.family
                            font.pixelSize: 14
                            wrapMode: Text.WordWrap
                            width: 280
                            renderType: Text.NativeRendering
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 70
                    radius: 18
                    color: "#181818"
                    border.width: 1
                    border.color: "#1affffff"

                    Column {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 4

                        Text {
                            text: playbackController.currentClipName.length > 0 ? playbackController.currentClipName : root.selectedClipName
                            color: "white"
                            font.family: interBold.font.family
                            font.pixelSize: 16
                            elide: Text.ElideRight
                            renderType: Text.NativeRendering
                        }

                        Text {
                            text: "This action cannot be undone."
                            color: "#6f7076"
                            font.family: interRegular.font.family
                            font.pixelSize: 13
                            renderType: Text.NativeRendering
                        }
                    }
                }

                Item {
                    width: 1
                    height: 1
                }

                Row {
                    anchors.horizontalCenter: undefined
                    spacing: 12

                    Rectangle {
                        width: 150
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
                        width: 182
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
                                text: "Remove Clip"
                                color: "white"
                                font.family: interMedium.font.family
                                font.pixelSize: 16
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
