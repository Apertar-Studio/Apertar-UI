import QtQuick
import QtQuick.Window
import Apertar 1.0

Window {
    id: root
    color: "#000000"
    flags: Qt.FramelessWindowHint
    property bool shutdownPopupVisible: monitorRuntimeBridge
                                        ? monitorRuntimeBridge.shutdownPopupVisible
                                        : false
    property bool shutdownPopupTriggered: monitorRuntimeBridge
                                          ? monitorRuntimeBridge.shutdownPopupTriggered
                                          : false
    property bool shutdownHoldActive: monitorRuntimeBridge
                                      ? monitorRuntimeBridge.shutdownHoldActive
                                      : false
    property real shutdownHoldProgress: monitorRuntimeBridge
                                        ? monitorRuntimeBridge.shutdownHoldProgress
                                        : 0.0
    property string shutdownErrorText: monitorRuntimeBridge
                                       ? monitorRuntimeBridge.shutdownErrorText
                                       : ""
    readonly property bool synchronizedCaptureStateAvailable: monitorRuntimeBridge
                                                              ? monitorRuntimeBridge.captureStateAvailable
                                                              : false
    readonly property bool recordingActive: synchronizedCaptureStateAvailable
                                            ? monitorRuntimeBridge.recordingActive
                                            : (apertarControlBridge ? apertarControlBridge.recording : false)
    readonly property bool timelapseActive: synchronizedCaptureStateAvailable
                                            ? monitorRuntimeBridge.timelapseActive
                                            : (apertarControlBridge ? apertarControlBridge.timelapseActive : false)

    readonly property bool photoModeEnabled: settingsBridge ? settingsBridge.photoModeEnabled : false
    readonly property bool recordAudioEnabled: settingsBridge ? settingsBridge.recordAudioEnabled : false
    readonly property bool audioMeterEnabled: settingsBridge ? settingsBridge.audioMeterEnabled : false
    readonly property int inputVolumeLevel: settingsBridge ? settingsBridge.inputVolume : 60
    readonly property bool localAudioMeterAvailable: typeof audioMeterBridge !== "undefined"
                                                     && audioMeterBridge
    readonly property bool audioInputAvailable: localAudioMeterAvailable
                                               ? audioMeterBridge.inputDeviceAvailable
                                               : root.recordAudioEnabled
    readonly property bool showAudioMeter: root.recordAudioEnabled
                                           && root.audioMeterEnabled
                                           && !root.photoModeEnabled
                                           && root.audioInputAvailable
    readonly property string timecodeMode: settingsBridge ? settingsBridge.timecodeMode : "Free Run"
    readonly property bool overlayFeedEnabled: settingsBridge
                                               ? settingsBridge.externalMonitorMode === "Assist Feed"
                                               : false
    readonly property bool infoOverlayEnabled: settingsBridge
                                               ? settingsBridge.externalMonitorInfoOverlay !== "Off"
                                               : true
    readonly property string externalMonitorOrientation: settingsBridge
                                                         ? settingsBridge.externalMonitorOrientation
                                                         : "Landscape"
    readonly property bool externalMonitorPortrait: externalMonitorOrientation === "Portrait Left"
                                                    || externalMonitorOrientation === "Portrait Right"
    readonly property int externalMonitorRotation: externalMonitorOrientation === "Portrait Left" ? -90
                                                   : externalMonitorOrientation === "Portrait Right" ? 90
                                                   : externalMonitorOrientation === "Upside Down" ? 180
                                                   : 0
    readonly property real monitorContentWidth: externalMonitorPortrait ? height : width
    readonly property real monitorContentHeight: externalMonitorPortrait ? width : height
    readonly property bool falseColorLegendVisible: root.infoOverlayEnabled
                                                    && root.overlayFeedEnabled
                                                    && !root.smpteEnabled
                                                    && settingsBridge
                                                    ? settingsBridge.falseColorEnabled
                                                    : false
    readonly property bool horizontalLevelEnabled: root.infoOverlayEnabled
                                                   && root.overlayFeedEnabled
                                                   && !root.smpteEnabled
                                                   && settingsBridge
                                                   ? settingsBridge.horizontalLevelEnabled
                                                   : false
    readonly property color hudShadowColor: "#d8000000"
    readonly property int hudShadowOffset: 2
    readonly property color guideShadowColor: "#90000000"
    readonly property int guidesThickness: settingsBridge ? Math.max(1, settingsBridge.guidesThickness) : 1
    readonly property int guideStrokeWidth: guidesThickness * 2
    readonly property int guideShadowOffset: Math.max(1, guidesThickness)
    readonly property string shutterDisplayValue: photoModeEnabled
                                                 ? apertarControlBridge.shutterSpeed
                                                 : apertarControlBridge.shutterAngle
    readonly property string recordingFormatValue: apertarControlBridge && apertarControlBridge.recordingFormat.length > 0
                                                  ? apertarControlBridge.recordingFormat
                                                  : "cDNG"
    readonly property string h264ResolutionValue: apertarControlBridge && apertarControlBridge.h264Resolution.length > 0
                                                  ? apertarControlBridge.h264Resolution
                                                  : "Match Capture"
    readonly property string fpsValue: apertarControlBridge && apertarControlBridge.fps.length > 0
                                      ? apertarControlBridge.fps
                                      : "24.000"
    readonly property string isoValue: apertarControlBridge && apertarControlBridge.iso.length > 0
                                      ? apertarControlBridge.iso
                                      : "800"
    readonly property string wbValue: apertarControlBridge && apertarControlBridge.whiteBalance.length > 0
                                     ? apertarControlBridge.whiteBalance
                                     : "5600K"
    readonly property string resolutionValue: apertarControlBridge && apertarControlBridge.resolution.length > 0
                                             ? apertarControlBridge.resolution
                                             : "1920x1080"
    readonly property string processedVideoDefaultResolutionValue: "2560x1440"
    readonly property string h264OutputResolutionValue: root.h264ResolutionValue === "Match Capture"
                                                        ? root.processedVideoDefaultResolutionValue
                                                        : root.h264ResolutionValue
    readonly property bool processedVideoRecordFormat: !root.photoModeEnabled && root.recordingFormatValue === "H.264"
    readonly property string previewAspectResolutionValue: root.processedVideoRecordFormat
                                                          ? root.h264OutputResolutionValue
                                                          : root.resolutionValue
    readonly property string formatInfoValue: root.processedVideoRecordFormat
                                              ? (root.h264OutputResolutionValue + " " + root.recordingFormatValue)
                                              : root.recordingFormatValue
    readonly property real previewImageAspectRatio: root.aspectRatioForResolution(root.previewAspectResolutionValue)
    readonly property real previewDisplayAspectRatio: previewImageAspectRatio
                                                      * (settingsBridge && settingsBridge.anamorphicDesqueezeEnabled && !smpteEnabled
                                                         ? anamorphicRatioValue(settingsBridge.anamorphicRatio) : 1.0)
    readonly property string remainingValue: mediaBridge
                                             ? (photoModeEnabled ? mediaBridge.remainingStillsText
                                                                 : mediaBridge.remainingMinutesText)
                                             : "N/A"
    readonly property bool smpteEnabled: settingsBridge ? settingsBridge.smpteEnabled : false
    readonly property bool externalNoMedia: mediaBridge ? !mediaBridge.mediaMounted : true
    readonly property bool externalActiveCapture: root.recordingActive || root.timelapseActive
    readonly property bool guidesEnabled: (!smpteEnabled && infoOverlayEnabled && overlayFeedEnabled && settingsBridge)
                                          ? settingsBridge.guidesEnabled
                                          : false
    readonly property string guideType: settingsBridge ? settingsBridge.guidesType : "Thirds"
    readonly property bool centerMarkerEnabled: (!smpteEnabled && infoOverlayEnabled && overlayFeedEnabled && settingsBridge)
                                                ? settingsBridge.centerMarkerEnabled
                                                : false
    readonly property string centerMarkerType: settingsBridge ? settingsBridge.centerMarkerType : "Circle/Dot"
    readonly property real selectedAspectRatio: guideType === "16:9" ? (16.0 / 9.0)
                                             : guideType === "2.39:1" ? 2.39
                                             : guideType === "4:3" ? (4.0 / 3.0)
                                             : guideType === "1:1" ? 1.0
                                             : guideType === "Academy 1.37:1" ? 1.37
                                             : guideType === "5:4" ? 1.25
                                             : guideType === "9:16" ? (9.0 / 16.0)
                                             : guideType === "14:9" ? (14.0 / 9.0)
                                             : 0.0
    readonly property bool showThirdsGuides: guidesEnabled && guideType === "Thirds"
    readonly property bool showAspectGuides: guidesEnabled && selectedAspectRatio > 0.0
    readonly property real guideFrameWidth: showAspectGuides
                                           ? ((monitorContentWidth / monitorContentHeight) > selectedAspectRatio
                                              ? monitorContentHeight * selectedAspectRatio
                                              : monitorContentWidth)
                                           : 0
    readonly property real guideFrameHeight: showAspectGuides
                                            ? ((monitorContentWidth / monitorContentHeight) > selectedAspectRatio
                                               ? monitorContentHeight
                                               : monitorContentWidth / selectedAspectRatio)
                                            : 0
    readonly property color externalStatusFill: externalNoMedia ? "#806b5200"
                                                                : (externalActiveCapture ? "#80b71212" : "#70000000")
    readonly property color externalStatusBorder: externalNoMedia ? "#66ffd54a"
                                                                  : (externalActiveCapture ? "#66ff6666" : "#0affffff")
    readonly property color externalStatusDot: externalNoMedia ? "#ffd54a"
                                                               : (externalActiveCapture ? "#fb2c36" : apertarControlBridge && apertarControlBridge.mediaWritesPending ? "#ffd54a" : "#00c951")
    readonly property string externalStatusLabel: externalNoMedia ? "NO MEDIA"
                                                                  : (root.timelapseActive ? "TIMELAPSE" : (root.recordingActive ? "REC" : apertarControlBridge && apertarControlBridge.mediaWritesPending ? "WRITING" : "READY"))
    readonly property var topMonitorInfoItems: [
        { label: "FORMAT", value: root.formatInfoValue },
        { label: "FPS", value: root.fpsValue },
        { label: "ISO", value: root.isoValue },
        { label: "SHUTTER", value: root.shutterDisplayValue },
        { label: "WB", value: root.wbValue }
    ]
    property int timecodeFrames: 0
    property string localTimecode: "00:00:00:00"
    readonly property string timecode: synchronizedCaptureStateAvailable
                                       && monitorRuntimeBridge.timecode.length > 0
                                       ? monitorRuntimeBridge.timecode
                                       : localTimecode

    FontLoader { id: interRegular; source: "qrc:/qml/fonts/Inter/Inter-Regular.ttf" }
    FontLoader { id: interMedium; source: "qrc:/qml/fonts/Inter/Inter-Medium.ttf" }
    FontLoader { id: interBold; source: "qrc:/qml/fonts/Inter/Inter-Bold.ttf" }

    function anamorphicRatioValue(value) {
        if (!value || value.length === 0)
            return 1.33

        var normalized = value.replace("x", "")
        var parsed = Number(normalized)
        if (isNaN(parsed) || parsed < 1.0)
            return 1.33
        return parsed
    }

    function pad2(v) {
        return v < 10 ? "0" + v : "" + v
    }

    function currentFpsInt() {
        var parsed = parseFloat(root.fpsValue)
        if (isNaN(parsed) || parsed <= 0)
            return 24
        return Math.round(parsed)
    }

    function aspectRatioForResolution(resolutionValue) {
        var parts = String(resolutionValue || "").split("x")
        if (parts.length === 2) {
            var width = parseFloat(parts[0])
            var height = parseFloat(parts[1])
            if (width > 0 && height > 0)
                return width / height
        }
        return 16.0 / 9.0
    }

    function formatTimecode(totalFrames) {
        var fpsInt = currentFpsInt()
        var hours = Math.floor(totalFrames / (fpsInt * 3600))
        var remainder = totalFrames % (fpsInt * 3600)
        var minutes = Math.floor(remainder / (fpsInt * 60))
        remainder = remainder % (fpsInt * 60)
        var seconds = Math.floor(remainder / fpsInt)
        var frames = remainder % fpsInt
        return pad2(hours) + ":" + pad2(minutes) + ":" + pad2(seconds) + ":" + pad2(frames)
    }

    function updateTimecodeDisplay() {
        if (root.timecodeMode === "Free Run") {
            var now = new Date()
            var fpsInt = currentFpsInt()
            var frame = Math.floor((now.getMilliseconds() / 1000) * fpsInt)
            if (frame >= fpsInt)
                frame = fpsInt - 1

            root.localTimecode = pad2(now.getHours())
                               + ":" + pad2(now.getMinutes())
                               + ":" + pad2(now.getSeconds())
                               + ":" + pad2(frame)
        } else {
            root.localTimecode = root.formatTimecode(root.timecodeFrames)
        }
    }

    function triggerCaptureFeedback() {
        externalPhotoFlashOverlay.opacity = 0.0
        externalPhotoFramePulse.opacity = 0.0
        externalPhotoFramePulse.scale = 0.992
        externalPhotoCaptureFeedbackAnimation.restart()
    }

    function topHudSlotWidth(index, rowWidth) {
        if (index === 0 || index === root.topMonitorInfoItems.length - 1)
            return Math.max(220, rowWidth * 0.17)

        return Math.max(160, rowWidth * 0.125)
    }

    function topHudX(index, rowWidth) {
        var slotWidth = root.topHudSlotWidth(index, rowWidth)
        if (index === 0)
            return 0
        if (index === root.topMonitorInfoItems.length - 1)
            return rowWidth - slotWidth

        var positions = [0.29, 0.50, 0.71]
        return (rowWidth * positions[index - 1]) - (slotWidth / 2)
    }

    function updateHorizontalLevelPolling() {
        if (typeof gyroConfigBridge !== "undefined" && gyroConfigBridge) {
            gyroConfigBridge.setLivePollingRequest("external-horizontal-level",
                                                   root.horizontalLevelEnabled && !apertarControlBridge.standby,
                                                   true,
                                                   80)
        }
    }

    Component.onCompleted: {
        root.updateTimecodeDisplay()
        root.updateHorizontalLevelPolling()
    }

    onHorizontalLevelEnabledChanged: root.updateHorizontalLevelPolling()

    Connections {
        target: apertarControlBridge
        function onStandbyChanged() {
            root.updateHorizontalLevelPolling()
            if (apertarControlBridge.standby)
                apertarPreviewBridge.disconnectFromCore()
            else
                apertarPreviewBridge.connectToCore()
        }
    }

    Component.onDestruction: {
        if (typeof gyroConfigBridge !== "undefined" && gyroConfigBridge)
            gyroConfigBridge.setLivePollingRequest("external-horizontal-level", false, true, 80)
    }

    onTimecodeModeChanged: {
        if (root.timecodeMode === "Rec Run" && !root.recordingActive)
            root.timecodeFrames = 0
        root.updateTimecodeDisplay()
    }

    onRecordingActiveChanged: {
        if (!root.synchronizedCaptureStateAvailable
                && root.timecodeMode === "Rec Run"
                && root.recordingActive) {
            root.timecodeFrames = 0
        }
        root.updateTimecodeDisplay()
    }

    Connections {
        target: apertarPreviewBridge

        function onFrameArrived() {
            if (!root.synchronizedCaptureStateAvailable
                    && root.timecodeMode === "Rec Run"
                    && root.recordingActive) {
                root.timecodeFrames += 1
                root.updateTimecodeDisplay()
            }
        }
    }

    Connections {
        target: apertarControlBridge

        function onFpsChanged() {
            root.updateTimecodeDisplay()
        }

        function onPhotoCaptured() {
            root.triggerCaptureFeedback()
        }

        function onTimelapseFrameCaptured() {
            root.triggerCaptureFeedback()
        }
    }

    Timer {
        id: freeRunTimecodeTimer
        interval: Math.max(1, Math.round(1000 / root.currentFpsInt()))
        running: !apertarControlBridge.standby && !root.synchronizedCaptureStateAvailable && root.timecodeMode === "Free Run"
        repeat: true
        onTriggered: root.updateTimecodeDisplay()
    }

    Item {
        id: monitorSurface
        visible: !apertarControlBridge.standby
        width: root.monitorContentWidth
        height: root.monitorContentHeight
        anchors.centerIn: parent
        rotation: root.externalMonitorRotation
        transformOrigin: Item.Center

        StillExposureOverlay {
            anchors.fill: parent
            z: 1000
            controlBridge: apertarControlBridge
        }

        CameraPreviewItem {
            id: preview
            objectName: "externalPreviewItem"
            anchors.fill: parent
            bridge: apertarPreviewBridge
            zoom: 1.0
            panX: 0.0
            panY: 0.0
            zebraEnabled: root.overlayFeedEnabled && settingsBridge ? settingsBridge.zebraEnabled : false
            zebraThreshold: settingsBridge ? settingsBridge.zebraThreshold : 0.70
            focusPeakingEnabled: root.overlayFeedEnabled && settingsBridge ? settingsBridge.focusPeakingEnabled : false
            focusPeakingThreshold: settingsBridge ? settingsBridge.focusPeakingThreshold : 0.04
            focusPeakingColor: settingsBridge ? settingsBridge.focusPeakingColor : "Red"
            grayscaleEnabled: root.overlayFeedEnabled && settingsBridge ? settingsBridge.grayscaleEnabled : false
            smpteEnabled: root.smpteEnabled
            anamorphicDesqueezeEnabled: settingsBridge ? settingsBridge.anamorphicDesqueezeEnabled : false
            anamorphicDesqueezeRatio: root.anamorphicRatioValue(settingsBridge ? settingsBridge.anamorphicRatio : "1.33x")
            falseColorEnabled: root.overlayFeedEnabled && settingsBridge ? settingsBridge.falseColorEnabled : false
            falseColorMode: settingsBridge ? settingsBridge.falseColorMode : 0
            lutPreviewEnabled: settingsBridge ? settingsBridge.lutPreviewEnabled : false
            lutPreviewFile: settingsBridge ? settingsBridge.lutPreviewFile : ""
            exposureReference: apertarControlBridge ? apertarControlBridge.previewExposureReference : ({})
            displayRotation: root.externalMonitorRotation
            previewAspectRatio: root.previewImageAspectRatio
        }

        Rectangle {
            id: externalPhotoFlashOverlay
            anchors.fill: parent
            color: "white"
            opacity: 0.0
            visible: opacity > 0.01
            z: 80
        }

        Rectangle {
            id: externalPhotoFramePulse
            anchors.fill: parent
            anchors.margins: 18
            radius: 32
            color: "transparent"
            border.width: 3
            border.color: "white"
            opacity: 0.0
            scale: 0.992
            visible: opacity > 0.01
            z: 81
        }

        SequentialAnimation {
            id: externalPhotoCaptureFeedbackAnimation

            ScriptAction {
                script: {
                    externalPhotoFlashOverlay.opacity = 0.0
                    externalPhotoFramePulse.opacity = 0.0
                    externalPhotoFramePulse.scale = 0.992
                }
            }

            ParallelAnimation {
                NumberAnimation {
                    target: externalPhotoFlashOverlay
                    property: "opacity"
                    from: 0.58
                    to: 0.0
                    duration: 320
                    easing.type: Easing.OutCubic
                }

                SequentialAnimation {
                    NumberAnimation {
                        target: externalPhotoFramePulse
                        property: "opacity"
                        from: 0.88
                        to: 0.48
                        duration: 110
                        easing.type: Easing.OutCubic
                    }
                    NumberAnimation {
                        target: externalPhotoFramePulse
                        property: "opacity"
                        to: 0.0
                        duration: 360
                        easing.type: Easing.OutCubic
                    }
                }

                NumberAnimation {
                    target: externalPhotoFramePulse
                    property: "scale"
                    from: 0.992
                    to: 1.018
                    duration: 460
                    easing.type: Easing.OutCubic
                }
            }
        }

        Item {
            id: guideOverlay
            anchors.fill: parent
            z: 12
            layer.enabled: root.showThirdsGuides || root.showAspectGuides || root.centerMarkerEnabled
            layer.smooth: false

            readonly property real activeAspectRatio: root.previewDisplayAspectRatio
            readonly property real activeImageWidth: (parent.width / Math.max(1, parent.height)) > activeAspectRatio
                                                     ? parent.height * activeAspectRatio
                                                     : parent.width
            readonly property real activeImageHeight: (parent.width / Math.max(1, parent.height)) > activeAspectRatio
                                                      ? parent.height
                                                      : parent.width / activeAspectRatio
            readonly property real activeImageX: Math.round((parent.width - activeImageWidth) / 2)
            readonly property real activeImageY: Math.round((parent.height - activeImageHeight) / 2)
            readonly property real aspectGuideWidth: root.showAspectGuides
                                                     ? ((activeImageWidth / activeImageHeight) > root.selectedAspectRatio
                                                        ? activeImageHeight * root.selectedAspectRatio
                                                        : activeImageWidth)
                                                     : 0
            readonly property real aspectGuideHeight: root.showAspectGuides
                                                      ? ((activeImageWidth / activeImageHeight) > root.selectedAspectRatio
                                                         ? activeImageHeight
                                                         : activeImageWidth / root.selectedAspectRatio)
                                                      : 0

            Rectangle {
            visible: root.showThirdsGuides
            width: root.guideStrokeWidth
            color: root.guideShadowColor
            x: Math.round(guideOverlay.activeImageX + (guideOverlay.activeImageWidth / 3)) + root.guideShadowOffset
            y: guideOverlay.activeImageY + root.guideShadowOffset
            height: guideOverlay.activeImageHeight
            antialiasing: false
        }

            Rectangle {
            visible: root.showThirdsGuides
            width: root.guideStrokeWidth
            color: "#5cffffff"
            x: Math.round(guideOverlay.activeImageX + (guideOverlay.activeImageWidth / 3))
            y: guideOverlay.activeImageY
            height: guideOverlay.activeImageHeight
            antialiasing: false
        }

            Rectangle {
            visible: root.showThirdsGuides
            width: root.guideStrokeWidth
            color: root.guideShadowColor
            x: Math.round(guideOverlay.activeImageX + ((guideOverlay.activeImageWidth * 2) / 3)) + root.guideShadowOffset
            y: guideOverlay.activeImageY + root.guideShadowOffset
            height: guideOverlay.activeImageHeight
            antialiasing: false
        }

            Rectangle {
            visible: root.showThirdsGuides
            width: root.guideStrokeWidth
            color: "#5cffffff"
            x: Math.round(guideOverlay.activeImageX + ((guideOverlay.activeImageWidth * 2) / 3))
            y: guideOverlay.activeImageY
            height: guideOverlay.activeImageHeight
            antialiasing: false
        }

            Rectangle {
            visible: root.showThirdsGuides
            height: root.guideStrokeWidth
            color: root.guideShadowColor
            x: guideOverlay.activeImageX + root.guideShadowOffset
            y: Math.round(guideOverlay.activeImageY + (guideOverlay.activeImageHeight / 3)) + root.guideShadowOffset
            width: guideOverlay.activeImageWidth
            antialiasing: false
        }

            Rectangle {
            visible: root.showThirdsGuides
            height: root.guideStrokeWidth
            color: "#5cffffff"
            x: guideOverlay.activeImageX
            y: Math.round(guideOverlay.activeImageY + (guideOverlay.activeImageHeight / 3))
            width: guideOverlay.activeImageWidth
            antialiasing: false
        }

            Rectangle {
            visible: root.showThirdsGuides
            height: root.guideStrokeWidth
            color: root.guideShadowColor
            x: guideOverlay.activeImageX + root.guideShadowOffset
            y: Math.round(guideOverlay.activeImageY + ((guideOverlay.activeImageHeight * 2) / 3)) + root.guideShadowOffset
            width: guideOverlay.activeImageWidth
            antialiasing: false
        }

            Rectangle {
            visible: root.showThirdsGuides
            height: root.guideStrokeWidth
            color: "#5cffffff"
            x: guideOverlay.activeImageX
            y: Math.round(guideOverlay.activeImageY + ((guideOverlay.activeImageHeight * 2) / 3))
            width: guideOverlay.activeImageWidth
            antialiasing: false
        }

            Rectangle {
            visible: root.showAspectGuides
            width: root.guideStrokeWidth
            color: root.guideShadowColor
            x: Math.round(guideOverlay.activeImageX + ((guideOverlay.activeImageWidth - guideOverlay.aspectGuideWidth) / 2)) + root.guideShadowOffset
            y: guideOverlay.activeImageY + root.guideShadowOffset
            height: guideOverlay.activeImageHeight
            antialiasing: false
        }

            Rectangle {
            visible: root.showAspectGuides
            width: root.guideStrokeWidth
            color: "#5cffffff"
            x: Math.round(guideOverlay.activeImageX + ((guideOverlay.activeImageWidth - guideOverlay.aspectGuideWidth) / 2))
            y: guideOverlay.activeImageY
            height: guideOverlay.activeImageHeight
            antialiasing: false
        }

            Rectangle {
            visible: root.showAspectGuides
            width: root.guideStrokeWidth
            color: root.guideShadowColor
            x: Math.round(guideOverlay.activeImageX + ((guideOverlay.activeImageWidth + guideOverlay.aspectGuideWidth) / 2)) + root.guideShadowOffset
            y: guideOverlay.activeImageY + root.guideShadowOffset
            height: guideOverlay.activeImageHeight
            antialiasing: false
        }

            Rectangle {
            visible: root.showAspectGuides
            width: root.guideStrokeWidth
            color: "#5cffffff"
            x: Math.round(guideOverlay.activeImageX + ((guideOverlay.activeImageWidth + guideOverlay.aspectGuideWidth) / 2))
            y: guideOverlay.activeImageY
            height: guideOverlay.activeImageHeight
            antialiasing: false
        }

            Rectangle {
            visible: root.showAspectGuides
            height: root.guideStrokeWidth
            color: root.guideShadowColor
            x: guideOverlay.activeImageX + root.guideShadowOffset
            y: Math.round(guideOverlay.activeImageY + ((guideOverlay.activeImageHeight - guideOverlay.aspectGuideHeight) / 2)) + root.guideShadowOffset
            width: guideOverlay.activeImageWidth
            antialiasing: false
        }

            Rectangle {
            visible: root.showAspectGuides
            height: root.guideStrokeWidth
            color: "#5cffffff"
            x: guideOverlay.activeImageX
            y: Math.round(guideOverlay.activeImageY + ((guideOverlay.activeImageHeight - guideOverlay.aspectGuideHeight) / 2))
            width: guideOverlay.activeImageWidth
            antialiasing: false
        }

            Rectangle {
            visible: root.showAspectGuides
            height: root.guideStrokeWidth
            color: root.guideShadowColor
            x: guideOverlay.activeImageX + root.guideShadowOffset
            y: Math.round(guideOverlay.activeImageY + ((guideOverlay.activeImageHeight + guideOverlay.aspectGuideHeight) / 2)) + root.guideShadowOffset
            width: guideOverlay.activeImageWidth
            antialiasing: false
        }

            Rectangle {
            visible: root.showAspectGuides
            height: root.guideStrokeWidth
            color: "#5cffffff"
            x: guideOverlay.activeImageX
            y: Math.round(guideOverlay.activeImageY + ((guideOverlay.activeImageHeight + guideOverlay.aspectGuideHeight) / 2))
            width: guideOverlay.activeImageWidth
            antialiasing: false
        }

            Rectangle {
            visible: root.centerMarkerEnabled && root.centerMarkerType === "Circle/Dot"
            width: 108
            height: 108
            radius: 54
            color: "transparent"
            border.width: 2
            border.color: root.guideShadowColor
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: root.guideShadowOffset
            anchors.verticalCenterOffset: root.guideShadowOffset
            antialiasing: false
        }

            Rectangle {
            visible: root.centerMarkerEnabled && root.centerMarkerType === "Circle/Dot"
            width: 108
            height: 108
            radius: 54
            color: "transparent"
            border.width: 2
            border.color: "#4dffffff"
            anchors.centerIn: parent
            antialiasing: false
        }

            Rectangle {
            visible: root.centerMarkerEnabled && root.centerMarkerType === "Circle/Dot"
            width: 15
            height: 15
            radius: 7.5
            color: root.guideShadowColor
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: root.guideShadowOffset
            anchors.verticalCenterOffset: root.guideShadowOffset
            antialiasing: false
        }

            Rectangle {
            visible: root.centerMarkerEnabled && root.centerMarkerType === "Circle/Dot"
            width: 15
            height: 15
            radius: 7.5
            color: "white"
            anchors.centerIn: parent
            antialiasing: false
        }

            Rectangle {
            visible: root.centerMarkerEnabled && root.centerMarkerType === "Crosshair"
            width: 2
            height: 96
            color: root.guideShadowColor
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: root.guideShadowOffset
            anchors.verticalCenterOffset: root.guideShadowOffset
            antialiasing: false
        }

            Rectangle {
            visible: root.centerMarkerEnabled && root.centerMarkerType === "Crosshair"
            width: 2
            height: 96
            color: "#4dffffff"
            anchors.centerIn: parent
            antialiasing: false
        }

            Rectangle {
            visible: root.centerMarkerEnabled && root.centerMarkerType === "Crosshair"
            width: 96
            height: 2
            color: root.guideShadowColor
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: root.guideShadowOffset
            anchors.verticalCenterOffset: root.guideShadowOffset
            antialiasing: false
        }

            Rectangle {
            visible: root.centerMarkerEnabled && root.centerMarkerType === "Crosshair"
            width: 96
            height: 2
            color: "#4dffffff"
            anchors.centerIn: parent
            antialiasing: false
        }
        }

        HorizontalLevelOverlay {
            id: horizontalLevelOverlay
            z: 17
            anchors.fill: parent
            levelEnabled: root.horizontalLevelEnabled
            smpteEnabled: root.smpteEnabled
            previewAspectRatio: root.previewDisplayAspectRatio
            orientation: (typeof gyroConfigBridge !== "undefined" && gyroConfigBridge) ? gyroConfigBridge.orientation : "XYZ"
            liveAvailable: (typeof gyroConfigBridge !== "undefined" && gyroConfigBridge) ? gyroConfigBridge.liveAvailable : false
            liveStatusText: (typeof gyroConfigBridge !== "undefined" && gyroConfigBridge) ? gyroConfigBridge.liveStatusText : "Live gyro unavailable"
            accelXG: (typeof gyroConfigBridge !== "undefined" && gyroConfigBridge) ? gyroConfigBridge.accelXG : 0
            accelYG: (typeof gyroConfigBridge !== "undefined" && gyroConfigBridge) ? gyroConfigBridge.accelYG : 0
            accelZG: (typeof gyroConfigBridge !== "undefined" && gyroConfigBridge) ? gyroConfigBridge.accelZG : 0
            levelReferenceSet: (typeof gyroConfigBridge !== "undefined" && gyroConfigBridge) ? gyroConfigBridge.levelReferenceSet : false
            levelAccelXG: (typeof gyroConfigBridge !== "undefined" && gyroConfigBridge) ? gyroConfigBridge.levelAccelXG : 0
            levelAccelYG: (typeof gyroConfigBridge !== "undefined" && gyroConfigBridge) ? gyroConfigBridge.levelAccelYG : 0
            levelAccelZG: (typeof gyroConfigBridge !== "undefined" && gyroConfigBridge) ? gyroConfigBridge.levelAccelZG : 0
            levelColor: settingsBridge ? settingsBridge.horizontalLevelColor : "Blue"
            large: true
            compact: false
        }

        FalseColorLegend {
            id: falseColorLegend
            z: 18
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 18
            layer.enabled: root.falseColorLegendVisible
            layer.smooth: true
            mode: settingsBridge ? settingsBridge.falseColorMode === 0 ? "Exposure Based"
                                   : settingsBridge.falseColorMode === 1 ? "Skin Tone"
                                   : settingsBridge.falseColorMode === 2 ? "Highlight Priority"
                                   : settingsBridge.falseColorMode === 3 ? "Shadow Priority"
                                   : "Exposure Based"
                                : "Exposure Based"
            opacity: root.falseColorLegendVisible ? 1.0 : 0.0
            scale: root.falseColorLegendVisible ? 1.68 : 1.58
        }

        Item {
            anchors.fill: parent
            anchors.margins: 36
            z: 20
            visible: root.infoOverlayEnabled && !root.smpteEnabled

            Item {
            id: topInfoRow
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 58
            layer.enabled: visible
            layer.smooth: true

            Repeater {
                model: root.topMonitorInfoItems

                delegate: Item {
                    required property var modelData
                    required property int index

                    width: root.topHudSlotWidth(index, topInfoRow.width)
                    height: topInfoRow.height
                    x: root.topHudX(index, topInfoRow.width)

                    Column {
                        id: infoColumn
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        spacing: 2

                        Item {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: labelText.implicitWidth + root.hudShadowOffset
                            height: labelText.implicitHeight + root.hudShadowOffset

                            Text {
                                x: root.hudShadowOffset
                                y: root.hudShadowOffset
                                text: modelData.label
                                color: root.hudShadowColor
                                font.family: interBold.font.family
                                font.pixelSize: 13
                                font.capitalization: Font.AllUppercase
                                font.letterSpacing: 2.0
                                renderType: Text.NativeRendering
                            }

                            Text {
                                id: labelText
                                text: modelData.label
                                color: "#b8ffffff"
                                font.family: interBold.font.family
                                font.pixelSize: 13
                                font.capitalization: Font.AllUppercase
                                font.letterSpacing: 2.0
                                style: Text.Outline
                                styleColor: "#a0000000"
                                renderType: Text.NativeRendering
                            }
                        }

                        Item {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: valueText.implicitWidth + root.hudShadowOffset
                            height: valueText.implicitHeight + root.hudShadowOffset

                            Text {
                                x: root.hudShadowOffset
                                y: root.hudShadowOffset
                                text: modelData.value
                                color: root.hudShadowColor
                                font.family: interBold.font.family
                                font.pixelSize: 34
                                renderType: Text.NativeRendering
                            }

                            Text {
                                id: valueText
                                text: modelData.value
                                color: "white"
                                font.family: interBold.font.family
                                font.pixelSize: 34
                                style: Text.Outline
                                styleColor: "#a0000000"
                                renderType: Text.NativeRendering
                            }
                        }
                    }
                }
            }
        }

            Column {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            spacing: 2
            layer.enabled: visible
            layer.smooth: true

            Item {
                width: resolutionLabel.implicitWidth + root.hudShadowOffset
                height: resolutionLabel.implicitHeight + root.hudShadowOffset

                Text {
                    x: root.hudShadowOffset
                    y: root.hudShadowOffset
                    text: "RESOLUTION"
                    color: root.hudShadowColor
                    font.family: interBold.font.family
                    font.pixelSize: 13
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 2.0
                    renderType: Text.NativeRendering
                }

                Text {
                    id: resolutionLabel
                    text: "RESOLUTION"
                    color: "#b8ffffff"
                    font.family: interBold.font.family
                    font.pixelSize: 13
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 2.0
                    style: Text.Outline
                    styleColor: "#a0000000"
                    renderType: Text.NativeRendering
                }
            }

            Item {
                width: resolutionText.implicitWidth + root.hudShadowOffset
                height: resolutionText.implicitHeight + root.hudShadowOffset

                Text {
                    x: root.hudShadowOffset
                    y: root.hudShadowOffset
                    text: root.resolutionValue
                    color: root.hudShadowColor
                    font.family: interBold.font.family
                    font.pixelSize: 34
                    renderType: Text.NativeRendering
                }

                Text {
                    id: resolutionText
                    text: root.resolutionValue
                    color: "white"
                    font.family: interBold.font.family
                    font.pixelSize: 34
                    style: Text.Outline
                    styleColor: "#a0000000"
                    renderType: Text.NativeRendering
                }
            }
        }

            Column {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            spacing: 10

            Rectangle {
                id: hdmiAudioMeterPanel
                anchors.horizontalCenter: parent.horizontalCenter
                width: 304
                height: 40
                visible: root.showAudioMeter
                radius: 10
                color: "#38000000"
                border.width: 1
                border.color: root.inputVolumeLevel <= 0 ? "#664a2f2f" : "#26ffffff"
                layer.enabled: visible
                layer.smooth: true

                readonly property int meterLevel: root.localAudioMeterAvailable
                                                  ? audioMeterBridge.inputLevel
                                                  : (apertarControlBridge
                                                     ? apertarControlBridge.audioInputLevel
                                                     : 0)
                readonly property real normalizedLevel: Math.max(0, Math.min(1, meterLevel / 100))
                readonly property int segmentCount: 18

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 10

                    Rectangle {
                        id: audioMeterIconHolder
                        width: 28
                        height: 28
                        radius: 14
                        anchors.verticalCenter: parent.verticalCenter
                        color: root.inputVolumeLevel <= 0 ? "#4b1f1f" : "#18ffffff"
                        border.width: 1
                        border.color: root.inputVolumeLevel <= 0 ? "#8f3d3d" : "#1affffff"

                        Image {
                            anchors.centerIn: parent
                            width: 16
                            height: 16
                            source: root.inputVolumeLevel <= 0
                                    ? "qrc:/qml/icons/microphone-muted.png"
                                    : "qrc:/qml/icons/microphone.png"
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            mipmap: true
                        }
                    }

                    Rectangle {
                        width: parent.width - parent.spacing - audioMeterIconHolder.width
                        height: 12
                        anchors.verticalCenter: parent.verticalCenter
                        radius: 3
                        color: "#10000000"
                        border.width: 1
                        border.color: "#1affffff"

                        Item {
                            anchors.fill: parent
                            anchors.margins: 2

                            Row {
                                anchors.fill: parent
                                spacing: 2

                                Repeater {
                                    model: hdmiAudioMeterPanel.segmentCount

                                    delegate: Rectangle {
                                        readonly property real threshold: (index + 1) / hdmiAudioMeterPanel.segmentCount

                                        width: (parent.width - ((hdmiAudioMeterPanel.segmentCount - 1) * 2)) / hdmiAudioMeterPanel.segmentCount
                                        height: parent.height
                                        radius: 1
                                        color: root.inputVolumeLevel <= 0
                                               ? "#382020"
                                               : (hdmiAudioMeterPanel.normalizedLevel >= threshold
                                                  ? (threshold > 0.84 ? "#ff5d5d"
                                                     : threshold > 0.62 ? "#ffd55a"
                                                     : "#4ce28f")
                                                  : "#172126")

                                        Behavior on color {
                                            ColorAnimation {
                                                duration: 95
                                                easing.type: Easing.OutCubic
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: statusRow.implicitWidth + 28
                height: 44
                radius: 22
                color: root.externalStatusFill
                border.width: 1
                border.color: root.externalStatusBorder
                layer.enabled: visible
                layer.smooth: true

                Behavior on color {
                    ColorAnimation { duration: 220 }
                }

                Behavior on border.color {
                    ColorAnimation { duration: 220 }
                }

                Row {
                    id: statusRow
                    anchors.centerIn: parent
                    spacing: 10

                    Item {
                        width: 16
                        height: 16
                        anchors.verticalCenter: parent.verticalCenter

                        Rectangle {
                            anchors.centerIn: parent
                            width: 16
                            height: 16
                            radius: 8
                            color: root.externalStatusDot
                            opacity: root.externalActiveCapture && !root.externalNoMedia ? 0.22 : 0.0
                            visible: root.externalActiveCapture && !root.externalNoMedia
                        }

                        Rectangle {
                            anchors.centerIn: parent
                            width: 12
                            height: 12
                            radius: 6
                            color: root.externalStatusDot
                        }
                    }

                    Text {
                        text: root.externalStatusLabel
                        color: "white"
                        font.family: interBold.font.family
                        font.pixelSize: 20
                        font.capitalization: Font.AllUppercase
                        font.letterSpacing: 1.8
                        style: Text.Outline
                        styleColor: "#cc000000"
                        renderType: Text.NativeRendering
                    }

                    DroppedFrameWarning {
                        anchors.verticalCenter: parent.verticalCenter
                        height: 30
                        visible: apertarControlBridge ? apertarControlBridge.recordingHasDroppedFrames : false
                    }
                }
            }

            Item {
                anchors.horizontalCenter: parent.horizontalCenter
                width: timecodeText.implicitWidth + root.hudShadowOffset
                height: timecodeText.implicitHeight + root.hudShadowOffset

                Text {
                    x: root.hudShadowOffset
                    y: root.hudShadowOffset
                    text: root.timecode
                    color: root.hudShadowColor
                    font.family: interBold.font.family
                    font.pixelSize: 42
                    renderType: Text.NativeRendering
                }

                Text {
                    id: timecodeText
                    text: root.timecode
                    color: "white"
                    font.family: interBold.font.family
                    font.pixelSize: 42
                    style: Text.Outline
                    styleColor: "#a0000000"
                    renderType: Text.NativeRendering
                }
            }
        }

        Column {
            id: remainingColumn
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: Math.max(remainingLabelItem.width, remainingValueItem.width)
            spacing: 2
            layer.enabled: visible
            layer.smooth: true

            Item {
                id: remainingLabelItem
                anchors.right: parent.right
                width: remainingLabel.implicitWidth + root.hudShadowOffset
                height: remainingLabel.implicitHeight + root.hudShadowOffset

                Text {
                    x: root.hudShadowOffset
                    y: root.hudShadowOffset
                    text: "REMAINING"
                    color: root.hudShadowColor
                    font.family: interBold.font.family
                    font.pixelSize: 13
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 2.0
                    renderType: Text.NativeRendering
                }

                Text {
                    id: remainingLabel
                    anchors.right: parent.right
                    text: "REMAINING"
                    color: "#b8ffffff"
                    font.family: interBold.font.family
                    font.pixelSize: 13
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 2.0
                    style: Text.Outline
                    styleColor: "#a0000000"
                    renderType: Text.NativeRendering
                }
            }

            Item {
                id: remainingValueItem
                anchors.right: parent.right
                width: remainingText.implicitWidth + root.hudShadowOffset
                height: remainingText.implicitHeight + root.hudShadowOffset

                Text {
                    x: root.hudShadowOffset
                    y: root.hudShadowOffset
                    text: root.remainingValue
                    color: root.hudShadowColor
                    font.family: interBold.font.family
                    font.pixelSize: 34
                    renderType: Text.NativeRendering
                }

                Text {
                    id: remainingText
                    anchors.right: parent.right
                    text: root.remainingValue
                    color: "white"
                    font.family: interBold.font.family
                    font.pixelSize: 34
                    style: Text.Outline
                    styleColor: "#a0000000"
                    renderType: Text.NativeRendering
                }
            }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#cc000000"
        visible: root.shutdownPopupVisible
        opacity: root.shutdownPopupVisible ? 1.0 : 0.0
        z: 5000

        Behavior on opacity {
            NumberAnimation {
                duration: 180
                easing.type: Easing.OutCubic
            }
        }
    }

    Rectangle {
        width: 440
        height: 272
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -10
        radius: 26
        color: "#151515"
        border.width: 1
        border.color: "#1affffff"
        visible: root.shutdownPopupVisible
        opacity: root.shutdownPopupVisible ? 1.0 : 0.0
        scale: root.shutdownPopupVisible ? 1.0 : 0.96
        z: 5001

        Behavior on opacity {
            NumberAnimation {
                duration: 220
                easing.type: Easing.OutCubic
            }
        }

        Behavior on scale {
            NumberAnimation {
                duration: 240
                easing.type: Easing.OutCubic
            }
        }

        Column {
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            anchors.topMargin: 24
            anchors.bottomMargin: 30
            spacing: 14

            Text {
                text: "Shutdown System?"
                color: "white"
                font.family: interBold.font.family
                font.pixelSize: 28
                renderType: Text.NativeRendering
            }

            Text {
                text: root.shutdownPopupTriggered
                      ? "Shutting down..."
                      : "Hold the physical power button or press and hold below for 3 seconds to confirm shutdown."
                color: "#8f9096"
                font.family: interRegular.font.family
                font.pixelSize: 17
                wrapMode: Text.WordWrap
                width: parent.width
                renderType: Text.NativeRendering
            }

            Rectangle {
                width: parent.width
                height: 62
                radius: 18
                clip: true
                color: root.shutdownHoldActive ? "#a32828" : "#171717"
                border.width: 1
                border.color: "#ba4a4a"
                opacity: root.shutdownPopupTriggered ? 0.85 : 1.0

                Rectangle {
                    x: 1
                    y: 1
                    width: (parent.width - 2) * root.shutdownHoldProgress
                    height: parent.height - 2
                    radius: parent.radius - 1
                    color: "#c84242"
                    opacity: 0.6
                }

                Text {
                    anchors.centerIn: parent
                    text: root.shutdownPopupTriggered
                          ? "Shutting Down..."
                          : (root.shutdownHoldActive ? "Keep Holding..." : "Hold to Shut Down")
                    color: "#ffd6d6"
                    font.family: interMedium.font.family
                    font.pixelSize: 17
                    renderType: Text.NativeRendering
                }
            }

            Text {
                visible: root.shutdownErrorText.length > 0
                text: root.shutdownErrorText
                color: "#ff9b9b"
                font.family: interRegular.font.family
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                width: parent.width
                renderType: Text.NativeRendering
            }

            Rectangle {
                width: parent.width
                height: 54
                radius: 18
                color: "#14ffffff"
                border.width: 1
                border.color: "#1affffff"
                opacity: root.shutdownPopupTriggered ? 0.45 : 1.0

                Text {
                    anchors.centerIn: parent
                    text: "Waiting for Main Screen"
                    color: "white"
                    font.family: interMedium.font.family
                    font.pixelSize: 18
                    renderType: Text.NativeRendering
                }
            }
        }
    }
}
