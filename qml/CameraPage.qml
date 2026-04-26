import QtQuick
import Apertar 1.0

Item {
    id: root
	
	signal openSettingsRequested()
    signal openClipBrowserRequested()
	
	FontLoader { id: gothamThin;   source: "qrc:/qml/fonts/Gotham/Gotham-Thin.ttf" }
    FontLoader { id: gothamLight;  source: "qrc:/qml/fonts/Gotham/Gotham-Light.ttf" }
    FontLoader { id: gothamMedium; source: "qrc:/qml/fonts/Gotham/Gotham-Medium.ttf" }
    FontLoader { id: gothamBold;   source: "qrc:/qml/fonts/Gotham/Gotham-Bold.ttf" }
    FontLoader { id: gothamBlack;  source: "qrc:/qml/fonts/Gotham/Gotham-Black.ttf" }
	FontLoader { id: interThin;   source: "qrc:/qml/fonts/Inter/Inter-Thin.ttf" }
	FontLoader { id: interLight;   source: "qrc:/qml/fonts/Inter/Inter-Light.ttf" }
	FontLoader { id: interRegular;   source: "qrc:/qml/fonts/Inter/Inter-Regular.ttf" }
	FontLoader { id: interMedium;   source: "qrc:/qml/fonts/Inter/Inter-Medium.ttf" }
	FontLoader { id: interBold;   source: "qrc:/qml/fonts/Inter/Inter-Bold.ttf" }
	FontLoader { id: interBlack;   source: "qrc:/qml/fonts/Inter/Inter-Black.ttf" }



    property var bridge
    property var controlBridge
    property bool recording: false
    property bool recordWarningOpen: false
	
	property var settingsState
    readonly property string timecodeMode: settingsState ? settingsState.timecodeMode : "Free Run"
    readonly property bool photoModeEnabled: settingsState ? settingsState.photoModeEnabled : false
    readonly property string photoTimerSetting: settingsState ? settingsState.photoTimer : "Off"
    readonly property string photoBurstSetting: settingsState ? settingsState.photoBurst : "Single"
    readonly property bool falseColorLegendVisible: settingsState ? settingsState.falseColorEnabled : false
    readonly property bool recordAudioEnabled: settingsState ? settingsState.recordAudioEnabled : false
    readonly property bool audioMeterEnabled: settingsState ? settingsState.audioMeterEnabled : false
    readonly property bool anamorphicDesqueezeEnabled: settingsState ? settingsState.anamorphicDesqueezeEnabled : false
    readonly property real anamorphicDesqueezeRatio: {
        if (!settingsState)
            return 1.33

        var label = settingsState.anamorphicRatio
        if (!label || label.length === 0)
            return 1.33

        var parsed = parseFloat(String(label).replace("x", ""))
        return (isNaN(parsed) || parsed < 1.0) ? 1.33 : parsed
    }
    readonly property int inputVolumeLevel: settingsState ? settingsState.inputVolume : 60
    readonly property int headphoneVolumeLevel: settingsState ? settingsState.headphoneVolume : 55
    readonly property bool showAudioMeter: root.recordAudioEnabled && root.audioMeterEnabled && !root.photoModeEnabled
    readonly property bool vmountPowerPresent: powerBridge.sensorAvailable && powerBridge.busVoltageV > 3.0
    readonly property string powerOverlayValue: !powerBridge.sensorAvailable
                                               ? "INA219"
                                               : (!root.vmountPowerPresent
                                                  ? "External"
                                                  : (powerBridge.voltageText + " • " + powerBridge.batteryPercentText))
    readonly property string powerOverlaySubtext: !powerBridge.sensorAvailable
                                                 ? "Unavailable"
                                                 : (!root.vmountPowerPresent
                                                    ? "DC INPUT"
                                                    : ("VMOUNT • " + powerBridge.powerText))
    property bool photoCapturePulse: false
    property bool photoCaptureInProgress: false
    property int pendingPhotoShots: 0
    property int photoCountdownSecondsRemaining: 0

    property string fps: "24.000"
	
	onFpsChanged: {
	        mediaBridge.fps = parseFloat(fps)
	        root.updateMediaEstimate()
	        root.updateTimecodeDisplay()
	    }
		
	    Component.onCompleted: {
	        root.syncControlValuesFromBridge()
	        root.ensureValidResolutionSelection()
	        root.ensureValidFpsSelection()
	        mediaBridge.fps = parseFloat(fps)
	        root.updateMediaEstimate()
	        root.updateTimecodeDisplay()
	    }
	
    property string iso: "800"
    property string shutterAngle: "180°"
    property string shutterSpeed: "1/48"
    property string wb: "5600K"
    property string resolution: "1928x1090"
    readonly property string sensorNameLower: (typeof deviceInfoBridge !== "undefined"
                                               && deviceInfoBridge
                                               && deviceInfoBridge.sensorName)
                                              ? deviceInfoBridge.sensorName.toLowerCase()
                                              : ""
    readonly property bool imx585Detected: root.sensorNameLower.indexOf("imx585") !== -1
    readonly property bool imx477Detected: root.sensorNameLower.indexOf("imx477") !== -1
    readonly property var resolutionOptions: root.imx477Detected
                                             ? ["1332x990", "2028x1080", "2028x1520"]
                                             : ["1928x1090", "3856x2180"]
    readonly property string shutterDisplayValue: root.photoModeEnabled ? root.shutterSpeed : root.shutterAngle
    readonly property var shutterOptions: root.photoModeEnabled
                                         ? ["Auto", "1/24", "1/30", "1/40", "1/48", "1/50", "1/60", "1/80", "1/100", "1/125", "1/160", "1/200", "1/250", "1/320", "1/500", "1/1000"]
                                         : ["Auto", "11.25°", "15°", "22.5°", "30°", "37.5°", "45°", "60°", "72°", "75°", "90°", "108°", "120°", "144°", "150°", "172.8°", "180°", "216°", "270°", "324°", "360°"]
    readonly property var fpsOptions: root.availableFpsOptions()
    readonly property string formatDisplayValue: root.resolution
    readonly property string formatDisplaySubtext: "Uncompressed"
	
	//Zoom Property
	property real previewZoom: 1.0
    property real minPreviewZoom: 1.0
    property real maxPreviewZoom: 4.0
	
	//Pan Property
	property real previewPanX: 0.0
    property real previewPanY: 0.0

    property string openDropdown: ""

    function pad2(v) {
    return v < 10 ? "0" + v : "" + v
}

function currentFpsInt() {
    var parsed = parseFloat(root.fps)
    if (isNaN(parsed) || parsed <= 0)
        return 24
    return Math.round(parsed)
}

function currentFpsValue() {
    var parsed = parseFloat(root.fps)
    if (isNaN(parsed) || parsed <= 0)
        return 24.0
    return parsed
}

function formatFpsValue(value) {
    return Number(value).toFixed(3)
}

function maximumSelectableFps() {
    if (!root.fpsOptions || root.fpsOptions.length === 0)
        return 24.0
    var parsed = parseFloat(root.fpsOptions[root.fpsOptions.length - 1])
    return isNaN(parsed) || parsed <= 0 ? 24.0 : parsed
}

function maximumSupportedFpsForResolution(resolutionValue) {
    if (root.imx477Detected) {
        if (resolutionValue === "1332x990")
            return 100.0
        if (resolutionValue === "2028x1080")
            return 60.0
        if (resolutionValue === "2028x1520")
            return 30.0
        if (resolutionValue === "1928x1090")
            return 60.0
        if (resolutionValue === "3856x2180")
            return 30.0
    }

    if (root.imx585Detected && resolutionValue === "3856x2180")
        return 30.0

    return 60.0
}

function availableFpsOptions() {
    var presets = root.imx477Detected
                  ? [24.000, 25.000, 30.000, 50.000, 60.000, 100.000]
                  : [24.000, 25.000, 30.000, 50.000, 60.000]
    var max = root.maximumSupportedFpsForResolution(root.resolution)
    var filtered = []
    for (var i = 0; i < presets.length; ++i) {
        if (presets[i] <= max + 0.0005)
            filtered.push(root.formatFpsValue(presets[i]))
    }
    return filtered.length > 0 ? filtered : [root.formatFpsValue(max)]
}

function estimatedFrameSizeMB() {
    var normalized = root.normalizedResolutionForCurrentSensor(root.resolution)

    if (root.imx477Detected) {
        if (normalized === "1332x990")
            return 2.0
        if (normalized === "2028x1080")
            return 3.3
        if (normalized === "2028x1520")
            return 4.6
    }

    if (root.imx585Detected) {
        if (normalized === "3856x2180")
            return 13.0
        if (normalized === "1928x1090")
            return 3.2
    }

    if (normalized === "1332x990")
        return 2.0
    if (normalized === "2028x1080")
        return 3.3
    if (normalized === "2028x1520")
        return 4.6
    if (normalized === "3856x2180")
        return 13.0
    if (normalized === "1928x1090")
        return 3.2

    return 5.3
}

function updateMediaEstimate() {
    if (typeof mediaBridge === "undefined" || !mediaBridge)
        return

    mediaBridge.frameSizeMB = root.estimatedFrameSizeMB()
}

function normalizedResolutionForCurrentSensor(resolutionValue) {
    if (root.imx477Detected) {
        if (resolutionValue === "1928x1090")
            return "2028x1080"
        if (resolutionValue === "3856x2180")
            return "2028x1080"
        if (resolutionValue === "4056x2160")
            return "2028x1080"
        if (resolutionValue === "4056x3040")
            return "2028x1520"
        if (root.resolutionOptions.indexOf(resolutionValue) !== -1)
            return resolutionValue
        return "2028x1080"
    }

    if (root.imx585Detected) {
        if (resolutionValue === "1332x990" ||
                resolutionValue === "2028x1080" ||
                resolutionValue === "2028x1520")
            return "1928x1090"
        if (resolutionValue === "4056x2160" ||
                resolutionValue === "4056x3040")
            return "3856x2180"
        if (root.resolutionOptions.indexOf(resolutionValue) !== -1)
            return resolutionValue
        return "1928x1090"
    }

    return resolutionValue
}

function ensureValidResolutionSelection() {
    var normalized = root.normalizedResolutionForCurrentSensor(root.resolution)
    if (normalized === root.resolution)
        return

    if (root.controlBridge) {
        root.controlBridge.applyResolution(normalized)
    } else {
        root.resolution = normalized
    }
}

function photoTimerDelayMs() {
    if (root.photoTimerSetting === "2s")
        return 2000
    if (root.photoTimerSetting === "5s")
        return 5000
    if (root.photoTimerSetting === "10s")
        return 10000
    if (root.photoTimerSetting === "15s")
        return 15000
    if (root.photoTimerSetting === "20s")
        return 20000
    if (root.photoTimerSetting === "25s")
        return 25000
    if (root.photoTimerSetting === "30s")
        return 30000
    return 0
}

function photoBurstCount() {
    if (root.photoBurstSetting === "3 Shots")
        return 3
    if (root.photoBurstSetting === "5 Shots")
        return 5
    if (root.photoBurstSetting === "10 Shots")
        return 10
    return 1
}

function photoBurstIntervalMs() {
    return Math.max(16, Math.round(1000 / root.currentFpsValue()))
}

function cancelPhotoSequence() {
    root.photoCaptureInProgress = false
    root.pendingPhotoShots = 0
    root.photoCountdownSecondsRemaining = 0
    photoDelayTimer.stop()
    photoCountdownTimer.stop()
    photoBurstTimer.stop()
}

function queueNextPhotoInSequence() {
    if (root.pendingPhotoShots <= 0) {
        root.photoCaptureInProgress = false
        photoBurstTimer.stop()
        return
    }

    var captureQueued = true
    if (root.controlBridge)
        captureQueued = root.controlBridge.capturePhoto()
    if (!captureQueued) {
        root.cancelPhotoSequence()
        return
    }

    root.photoCapturePulse = false
    photoCapturePulseTimer.stop()
    root.photoCapturePulse = true
    photoCapturePulseTimer.start()

    root.pendingPhotoShots -= 1
    if (root.pendingPhotoShots > 0) {
        photoBurstTimer.interval = root.photoBurstIntervalMs()
        photoBurstTimer.start()
    } else {
        root.photoCaptureInProgress = false
    }
}

function beginPhotoCaptureSequence() {
    if (root.photoCaptureInProgress)
        return

    root.photoCaptureInProgress = true
    root.pendingPhotoShots = root.photoBurstCount()

    var delayMs = root.photoTimerDelayMs()
    if (delayMs > 0) {
        root.photoCountdownSecondsRemaining = Math.max(1, Math.ceil(delayMs / 1000))
        photoDelayTimer.interval = delayMs
        photoDelayTimer.start()
        photoCountdownTimer.start()
        return
    }

    root.queueNextPhotoInSequence()
}

function clampPreviewPan(value) {
    return Math.max(-1.0, Math.min(1.0, value))
}

function resetPreviewTransform() {
    root.previewZoom = 1.0
    root.previewPanX = 0.0
    root.previewPanY = 0.0
}

function togglePreviewZoom() {
    if (root.previewZoom > 1.01) {
        root.resetPreviewTransform()
    } else {
        root.previewZoom = 2.0
        root.previewPanX = 0.0
        root.previewPanY = 0.0
    }
}

function ensureValidFpsSelection() {
    var current = root.currentFpsValue()
    var max = root.maximumSelectableFps()
    if (current <= max + 0.0005)
        return

    var clamped = root.formatFpsValue(max)
    if (root.controlBridge) {
        root.controlBridge.applyFps(clamped)
    } else {
        root.fps = clamped
    }
}

function syncControlValuesFromBridge() {
    if (!root.controlBridge)
        return

    if (root.controlBridge.fps && root.controlBridge.fps.length > 0)
        root.fps = root.controlBridge.fps

    if (root.controlBridge.iso && root.controlBridge.iso.length > 0)
        root.iso = root.controlBridge.iso

    if (root.controlBridge.shutterAngle && root.controlBridge.shutterAngle.length > 0)
        root.shutterAngle = root.controlBridge.shutterAngle

    if (root.controlBridge.shutterSpeed && root.controlBridge.shutterSpeed.length > 0)
        root.shutterSpeed = root.controlBridge.shutterSpeed

    if (root.controlBridge.whiteBalance && root.controlBridge.whiteBalance.length > 0)
        root.wb = root.controlBridge.whiteBalance

    if (root.controlBridge.resolution && root.controlBridge.resolution.length > 0)
        root.resolution = root.controlBridge.resolution

    root.recording = root.controlBridge.recording
}

function requestRecordingState(recordingState) {
    if (root.controlBridge) {
        root.controlBridge.setRecording(recordingState)
    } else {
        root.recording = recordingState
    }
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

        root.timecode = pad2(now.getHours())
                      + ":" + pad2(now.getMinutes())
                      + ":" + pad2(now.getSeconds())
                      + ":" + pad2(frame)
    } else {
        root.timecode = root.formatTimecode(root.timecodeFrames)
    }
}

function falseColorModeToInt(mode) {
    if (mode === "Exposure Based") return 0
    if (mode === "Skin Tone") return 1
    if (mode === "Highlight Priority") return 2
    if (mode === "Shadow Priority") return 3
    return 0
}

function intToFalseColorMode(mode) {
    if (mode === 0) return "Exposure Based"
    if (mode === 1) return "Skin Tone"
    if (mode === 2) return "Highlight Priority"
    if (mode === 3) return "Shadow Priority"
    return "Exposure Based"
}

property int timecodeFrames: 0
property string timecode: "00:00:00:00"

Timer {
    id: recordTimer
    interval: Math.max(1, Math.round(1000 / currentFpsInt()))
    running: root.timecodeMode === "Free Run"
    repeat: true
    onTriggered: {
        if (root.timecodeMode === "Free Run") {
            root.updateTimecodeDisplay()
        }
    }
}

onTimecodeModeChanged: {
    if (root.timecodeMode === "Rec Run" && !root.recording)
        root.timecodeFrames = 0
    root.updateTimecodeDisplay()
}

onPhotoModeEnabledChanged: {
    if (root.photoModeEnabled && root.recording) {
        root.requestRecordingState(false)
        root.updateTimecodeDisplay()
    }
    if (!root.photoModeEnabled)
        root.cancelPhotoSequence()
}

onResolutionChanged: {
    root.ensureValidResolutionSelection()
    root.ensureValidFpsSelection()
    root.updateMediaEstimate()
}
onImx585DetectedChanged: {
    root.ensureValidResolutionSelection()
    root.ensureValidFpsSelection()
    root.updateMediaEstimate()
}
onImx477DetectedChanged: {
    root.ensureValidResolutionSelection()
    root.ensureValidFpsSelection()
    root.updateMediaEstimate()
}

Connections {
    target: mediaBridge

    function onMediaMountedChanged() {
        if (mediaBridge.mediaMounted)
            root.recordWarningOpen = false
    }
}

Connections {
    target: root.controlBridge

    function onFpsChanged() {
        if (root.controlBridge && root.controlBridge.fps.length > 0)
            root.fps = root.controlBridge.fps
    }

    function onIsoChanged() {
        if (root.controlBridge && root.controlBridge.iso.length > 0)
            root.iso = root.controlBridge.iso
    }

    function onShutterAngleChanged() {
        if (root.controlBridge && root.controlBridge.shutterAngle.length > 0)
            root.shutterAngle = root.controlBridge.shutterAngle
    }

    function onShutterSpeedChanged() {
        if (root.controlBridge && root.controlBridge.shutterSpeed.length > 0)
            root.shutterSpeed = root.controlBridge.shutterSpeed
    }

    function onWhiteBalanceChanged() {
        if (root.controlBridge && root.controlBridge.whiteBalance.length > 0)
            root.wb = root.controlBridge.whiteBalance
    }

    function onResolutionChanged() {
        if (root.controlBridge && root.controlBridge.resolution.length > 0)
            root.resolution = root.controlBridge.resolution
    }

    function onRecordingChanged() {
        root.recording = root.controlBridge ? root.controlBridge.recording : false
    }
}

Connections {
    target: root.bridge

    function onFrameArrived() {
        if (root.timecodeMode === "Rec Run" && root.recording) {
            root.timecodeFrames += 1
            root.updateTimecodeDisplay()
        }
    }
}

Timer {
    id: photoCapturePulseTimer
    interval: 220
    repeat: false
    onTriggered: root.photoCapturePulse = false
}

Timer {
    id: photoDelayTimer
    interval: 0
    repeat: false
    onTriggered: {
        root.photoCountdownSecondsRemaining = 0
        photoCountdownTimer.stop()
        root.queueNextPhotoInSequence()
    }
}

Timer {
    id: photoCountdownTimer
    interval: 1000
    repeat: true
    onTriggered: {
        if (root.photoCountdownSecondsRemaining > 1) {
            root.photoCountdownSecondsRemaining -= 1
        } else {
            root.photoCountdownSecondsRemaining = 1
            stop()
        }
    }
}

Timer {
    id: photoBurstTimer
    interval: 0
    repeat: false
    onTriggered: root.queueNextPhotoInSequence()
}


    // =========================
    // PREVIEW AREA
    // =========================
	
    Item {
        id: previewContainer
        objectName: "previewContainer"
        anchors.horizontalCenter: parent.horizontalCenter
        y: 157
        width: parent.width
        height: 405

        CameraPreviewItem {
            id: preview
            objectName: "previewItem"
            anchors.fill: parent
            bridge: root.bridge
            zoom: root.previewZoom
            panX: root.previewPanX
            panY: root.previewPanY
            zebraEnabled: root.settingsState ? root.settingsState.zebraEnabled : false
            zebraThreshold: root.settingsState ? root.settingsState.zebraThreshold : 0.70
            focusPeakingEnabled: root.settingsState ? root.settingsState.focusPeakingEnabled : false
            focusPeakingThreshold: root.settingsState ? root.settingsState.focusPeakingThreshold : 0.04
            focusPeakingColor: root.settingsState ? root.settingsState.focusPeakingColor : "Red"
            grayscaleEnabled: root.settingsState ? root.settingsState.grayscaleEnabled : false
            anamorphicDesqueezeEnabled: root.anamorphicDesqueezeEnabled
            anamorphicDesqueezeRatio: root.anamorphicDesqueezeRatio
            falseColorEnabled: root.settingsState ? root.settingsState.falseColorEnabled : false
            falseColorMode: root.settingsState ? root.settingsState.falseColorMode : 0
        }

        Rectangle {
            visible: root.photoCountdownSecondsRemaining > 0
            anchors.centerIn: parent
            width: 132
            height: 132
            radius: 66
            color: "#9a000000"
            border.width: 2
            border.color: "#66ffffff"
            z: 30

            Column {
                anchors.centerIn: parent
                spacing: -4

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: root.photoCountdownSecondsRemaining
                    color: "white"
                    font.family: gothamBold.font.family
                    font.pixelSize: 56
                    renderType: Text.NativeRendering
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "SECONDS"
                    color: "#d9ffffff"
                    font.family: interBold.font.family
                    font.pixelSize: 12
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 2.4
                    renderType: Text.NativeRendering
                }
            }
        }
		
        MultiPointTouchArea {
            id: previewTouchArea
            anchors.fill: parent
            minimumTouchPoints: 1
            maximumTouchPoints: 2
            mouseEnabled: false
            z: 25

            property int gestureMode: 0
            property real singleStartX: 0.0
            property real singleStartY: 0.0
            property real singleStartPanX: 0.0
            property real singleStartPanY: 0.0
            property bool singleMoved: false
            property double singlePressTimestamp: 0
            property double lastTapTimestamp: 0
            property real lastTapX: 0.0
            property real lastTapY: 0.0
            property real pinchStartZoom: 1.0
            property real pinchStartDistance: 1.0

            touchPoints: [
                TouchPoint { id: previewTouchPoint1 },
                TouchPoint { id: previewTouchPoint2 }
            ]

            function activePointCount() {
                var count = 0
                if (previewTouchPoint1.pressed)
                    count += 1
                if (previewTouchPoint2.pressed)
                    count += 1
                return count
            }

            function activePrimaryPoint() {
                return previewTouchPoint1.pressed ? previewTouchPoint1 : previewTouchPoint2
            }

            function currentPinchDistance() {
                var dx = previewTouchPoint1.x - previewTouchPoint2.x
                var dy = previewTouchPoint1.y - previewTouchPoint2.y
                return Math.sqrt((dx * dx) + (dy * dy))
            }

            function beginSingleTouchGesture(markAsMoved) {
                var point = activePrimaryPoint()
                if (!point)
                    return

                gestureMode = 1
                singleStartX = point.x
                singleStartY = point.y
                singleStartPanX = root.previewPanX
                singleStartPanY = root.previewPanY
                singleMoved = markAsMoved
                singlePressTimestamp = Date.now()
            }

            function beginPinchGesture() {
                gestureMode = 2
                pinchStartZoom = root.previewZoom
                pinchStartDistance = Math.max(1.0, currentPinchDistance())
                singleMoved = true
            }

            function finishSingleTap(pointX, pointY) {
                var now = Date.now()
                var withinTapTime = (now - singlePressTimestamp) <= 350
                if (!withinTapTime || singleMoved)
                    return

                var dx = pointX - lastTapX
                var dy = pointY - lastTapY
                var distance = Math.sqrt((dx * dx) + (dy * dy))
                if (lastTapTimestamp > 0 &&
                        (now - lastTapTimestamp) <= 350 &&
                        distance <= 48) {
                    root.togglePreviewZoom()
                    lastTapTimestamp = 0
                    return
                }

                lastTapTimestamp = now
                lastTapX = pointX
                lastTapY = pointY
            }

            onPressed: {
                var count = activePointCount()
                if (count >= 2) {
                    beginPinchGesture()
                } else if (count === 1) {
                    beginSingleTouchGesture(false)
                }
            }

            onUpdated: {
                var count = activePointCount()
                if (count >= 2) {
                    if (gestureMode !== 2)
                        beginPinchGesture()

                    var scale = currentPinchDistance() / Math.max(1.0, pinchStartDistance)
                    root.previewZoom = Math.max(root.minPreviewZoom,
                                                Math.min(root.maxPreviewZoom, pinchStartZoom * scale))

                    if (root.previewZoom <= 1.01) {
                        root.previewPanX = 0.0
                        root.previewPanY = 0.0
                    }
                    return
                }

                if (count === 1) {
                    if (gestureMode === 2) {
                        beginSingleTouchGesture(true)
                        return
                    }

                    if (gestureMode !== 1)
                        beginSingleTouchGesture(false)

                    var point = activePrimaryPoint()
                    var dx = point.x - singleStartX
                    var dy = point.y - singleStartY
                    if (Math.abs(dx) > 12 || Math.abs(dy) > 12)
                        singleMoved = true

                    if (root.previewZoom > 1.01) {
                        var nx = dx / previewContainer.width
                        var ny = dy / previewContainer.height
                        root.previewPanX = root.clampPreviewPan(singleStartPanX - (nx * 2.0))
                        root.previewPanY = root.clampPreviewPan(singleStartPanY - (ny * 2.0))
                    }
                }
            }

            onReleased: {
                var count = activePointCount()
                if (count === 1) {
                    beginSingleTouchGesture(true)
                    return
                }

                if (count === 0) {
                    if (gestureMode === 1)
                        finishSingleTap(singleStartX, singleStartY)
                    gestureMode = 0
                }
            }

            onCanceled: {
                gestureMode = 0
                singleMoved = false
            }
        }

        DragHandler {
            id: previewDragHandler
            target: null
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            minimumPointCount: 1
            maximumPointCount: 1
            grabPermissions: PointerHandler.CanTakeOverFromAnything | PointerHandler.ApprovesTakeOverByAnything
            enabled: root.previewZoom > 1.01

            property real startPanX: 0.0
            property real startPanY: 0.0

            onActiveChanged: {
                if (!active)
                    return

                startPanX = root.previewPanX
                startPanY = root.previewPanY
            }

            onActiveTranslationChanged: {
                if (!active)
                    return

                var nx = activeTranslation.x / previewContainer.width
                var ny = activeTranslation.y / previewContainer.height

                root.previewPanX = root.clampPreviewPan(startPanX - (nx * 2.0))
                root.previewPanY = root.clampPreviewPan(startPanY - (ny * 2.0))
            }
        }

        TapHandler {
            id: previewMouseTapHandler
            target: null
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            gesturePolicy: TapHandler.ReleaseWithinBounds
            grabPermissions: PointerHandler.CanTakeOverFromAnything | PointerHandler.ApprovesTakeOverByAnything

            onDoubleTapped: function(eventPoint, button) {
                if (previewDragHandler.active)
                    return

                root.togglePreviewZoom()
            }
        }

    // =========================
    // GUIDES
    // =========================

        Item {
            id: guideOverlay
            anchors.fill: parent
            z: 10

            readonly property real guideInset: 24
            readonly property real guideWidth: parent.width - (guideInset * 2)
            readonly property real guideHeight: parent.height - (guideInset * 2)
            readonly property bool guidesEnabled: root.settingsState ? root.settingsState.guidesEnabled : true
            readonly property string guideType: root.settingsState ? root.settingsState.guidesType : "Thirds"
            readonly property bool centerMarkerEnabled: root.settingsState ? root.settingsState.centerMarkerEnabled : true
            readonly property string centerMarkerType: root.settingsState ? root.settingsState.centerMarkerType : "Circle/Dot"
            readonly property real selectedAspectRatio: guideType === "16:9" ? (16.0 / 9.0)
                                                     : guideType === "2.39:1" ? 2.39
                                                     : guideType === "4:3" ? (4.0 / 3.0)
                                                     : guideType === "1:1" ? 1.0
                                                     : guideType === "Academy 1.37:1" ? 1.37
                                                     : guideType === "5:4" ? 1.25
                                                     : guideType === "9:16" ? (9.0 / 16.0)
                                                     : guideType === "14:9" ? (14.0 / 9.0)
                                                     : 0.0
            readonly property bool showThirds: guidesEnabled && guideType === "Thirds"
            readonly property bool showAspectFrame: guidesEnabled && selectedAspectRatio > 0.0
            readonly property real aspectGuideWidth: showAspectFrame
                                                    ? ((guideWidth / guideHeight) > selectedAspectRatio
                                                       ? guideHeight * selectedAspectRatio
                                                       : guideWidth)
                                                    : 0
            readonly property real aspectGuideHeight: showAspectFrame
                                                     ? ((guideWidth / guideHeight) > selectedAspectRatio
                                                        ? guideHeight
                                                        : guideWidth / selectedAspectRatio)
                                                     : 0

            Rectangle {
                x: guideOverlay.guideInset
                y: guideOverlay.guideInset
                width: guideOverlay.guideWidth
                height: guideOverlay.guideHeight
                radius: 22
                color: "transparent"
                border.width: 1
                border.color: "#4dffffff"
                antialiasing: false
            }

            Rectangle {
                visible: guideOverlay.showThirds
                width: 1
                color: "#4dffffff"
                x: Math.round(guideOverlay.guideInset + (guideOverlay.guideWidth / 3))
                y: guideOverlay.guideInset
                height: guideOverlay.guideHeight
                antialiasing: false
            }

            Rectangle {
                visible: guideOverlay.showThirds
                width: 1
                color: "#4dffffff"
                x: Math.round(guideOverlay.guideInset + ((guideOverlay.guideWidth * 2) / 3))
                y: guideOverlay.guideInset
                height: guideOverlay.guideHeight
                antialiasing: false
            }

            Rectangle {
                visible: guideOverlay.showThirds
                height: 1
                color: "#4dffffff"
                x: guideOverlay.guideInset
                y: Math.round(guideOverlay.guideInset + (guideOverlay.guideHeight / 3))
                width: guideOverlay.guideWidth
                antialiasing: false
            }

            Rectangle {
                visible: guideOverlay.showThirds
                height: 1
                color: "#4dffffff"
                x: guideOverlay.guideInset
                y: Math.round(guideOverlay.guideInset + ((guideOverlay.guideHeight * 2) / 3))
                width: guideOverlay.guideWidth
                antialiasing: false
            }

            Rectangle {
                visible: guideOverlay.showAspectFrame
                width: guideOverlay.aspectGuideWidth
                height: guideOverlay.aspectGuideHeight
                x: Math.round((parent.width - width) / 2)
                y: Math.round((parent.height - height) / 2)
                color: "transparent"
                border.width: 1
                border.color: "#4dffffff"
                antialiasing: false
            }

            Rectangle {
                visible: guideOverlay.centerMarkerEnabled && guideOverlay.centerMarkerType === "Circle/Dot"
                width: 56
                height: 56
                radius: 28
                color: "transparent"
                border.width: 1
                border.color: "#4dffffff"
                anchors.centerIn: parent
                antialiasing: false
            }

            Rectangle {
                visible: guideOverlay.centerMarkerEnabled && guideOverlay.centerMarkerType === "Circle/Dot"
                width: 8
                height: 8
                radius: 4
                color: "white"
                anchors.centerIn: parent
                antialiasing: false
            }

            Rectangle {
                visible: guideOverlay.centerMarkerEnabled && guideOverlay.centerMarkerType === "Crosshair"
                width: 1
                height: 48
                color: "#4dffffff"
                anchors.centerIn: parent
                antialiasing: false
            }

            Rectangle {
                visible: guideOverlay.centerMarkerEnabled && guideOverlay.centerMarkerType === "Crosshair"
                width: 48
                height: 1
                color: "#4dffffff"
                anchors.centerIn: parent
                antialiasing: false
            }
        }

        FalseColorLegend {
            id: falseColorLegend
            z: 16
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 12
            mode: root.intToFalseColorMode(root.settingsState ? root.settingsState.falseColorMode : 0)
            opacity: root.falseColorLegendVisible ? 1.0 : 0.0
            scale: root.falseColorLegendVisible ? 1.0 : 0.96
        }

    // =========================
    // STATUS CHIP
	
    StatusChip {
        z: 20
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 16
        anchors.topMargin: 16
        recording: root.recording
        mediaMounted: mediaBridge.mediaMounted
        }

        Column {
            z: 20
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.rightMargin: 16
            anchors.topMargin: 16
            spacing: 8
            visible: root.photoModeEnabled

            Rectangle {
                visible: root.photoTimerSetting !== "Off"
                height: 30
                width: timerChipRow.implicitWidth + 18
                radius: 15
                color: "#70000000"
                border.width: 1
                border.color: "#0affffff"

                Row {
                    id: timerChipRow
                    anchors.centerIn: parent
                    spacing: 6

                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: "#ffd54a"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: "TIMER " + root.photoTimerSetting
                        color: "white"
                        font.family: interBold.font.family
                        font.weight: Font.Bold
                        font.pixelSize: 13
                        renderType: Text.NativeRendering
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            Rectangle {
                height: 30
                width: burstChipRow.implicitWidth + 18
                radius: 15
                color: "#70000000"
                border.width: 1
                border.color: "#0affffff"

                Row {
                    id: burstChipRow
                    anchors.centerIn: parent
                    spacing: 6

                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: "#66d9ff"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: root.photoBurstSetting === "Single"
                              ? "SINGLE"
                              : ("BURST " + root.photoBurstSetting.toUpperCase())
                        color: "white"
                        font.family: interBold.font.family
                        font.weight: Font.Bold
                        font.pixelSize: 13
                        renderType: Text.NativeRendering
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }
    }

    // =========================
    // STATIC OVERLAY (cached)
    // =========================
	
    Item {
        id: staticOverlay
        anchors.fill: parent
		
		// =========================
        // Background
        // =========================
	

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 157
            color: "#000000"
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 158
            color: "#000000"
        }


    // =========================
    // Top Menu
    // =========================
	

        Row {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            anchors.topMargin: 20
            spacing: 12

            Repeater {
                model: 4

                delegate: Rectangle {
                    width: ((parent.width - 76 - 12) - 36) / 4
                    height: 86
                    radius: 22
                    color: index === 0 ? "#1affffff" : "#14ffffff"
                    border.width: 1
                    border.color: index === 0 ? "#33ffffff" : "#1affffff"

                    Text {
    id: menuLabel
    anchors.left: parent.left
    anchors.top: parent.top
    anchors.leftMargin: 16
    anchors.topMargin: 12
    text: index === 0 ? "FPS"
         : index === 1 ? "ISO"
         : index === 2 ? "SHUTTER"
         : "WB"
    color: "#99ffffff"
    font.family: interLight.font.family
    font.pixelSize: 13
    font.letterSpacing: 2
    font.capitalization: Font.AllUppercase
    renderType: Text.NativeRendering
}

Text {
    anchors.right: parent.right
    anchors.rightMargin: 14
    anchors.baseline: menuLabel.baseline
    text: "⌄"
    color: "#ccffffff"
    font.family: interRegular.font.family
    font.pixelSize: 18
    renderType: Text.NativeRendering
}
                }
            }

            Rectangle {
                width: 76
                height: 76
                radius: 22
                color: "#14ffffff"
                border.width: 1
                border.color: "#1affffff"

                Image {
                    anchors.centerIn: parent
                    width: 28
                    height: 28
                    source: "qrc:/qml/icons/settings.png"
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                }
            }
        }
    }

    // =========================
    // DYNAMIC OVERLAY
    // =========================
    Item {
        id: dynamicOverlay
        anchors.fill: parent
        z: 100

        Row {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            anchors.topMargin: 20
            spacing: 12

            TopDropdownStat {
                width: ((parent.width - 76 - 12) - 36) / 4
                height: 86
                value: root.fps
                isOpen: root.openDropdown === "fps"
                options: root.fpsOptions
                onToggleRequested: root.openDropdown = root.openDropdown === "fps" ? "" : "fps"
                onOptionSelected: function(v) {
                    if (root.controlBridge) {
                        root.controlBridge.applyFps(v)
                    } else {
                        root.fps = v
                    }
                    root.openDropdown = ""
                }
            }

            TopDropdownStat {
                width: ((parent.width - 76 - 12) - 36) / 4
                height: 86
                value: root.iso
                isOpen: root.openDropdown === "iso"
                options: ["Auto", "100", "200", "400", "800", "1600", "3200", "6400"]
                onToggleRequested: root.openDropdown = root.openDropdown === "iso" ? "" : "iso"
                onOptionSelected: function(v) {
                    if (root.controlBridge) {
                        root.controlBridge.applyIso(v)
                    } else {
                        root.iso = v
                    }
                    root.openDropdown = ""
                }
            }

            TopDropdownStat {
                width: ((parent.width - 76 - 12) - 36) / 4
                height: 86
                value: root.shutterDisplayValue
                isOpen: root.openDropdown === "shutter"
                options: root.shutterOptions
                onToggleRequested: root.openDropdown = root.openDropdown === "shutter" ? "" : "shutter"
                onOptionSelected: function(v) {
                    if (root.photoModeEnabled) {
                        if (root.controlBridge) {
                            root.controlBridge.applyShutterSpeed(v)
                        } else {
                            root.shutterSpeed = v
                        }
                    } else {
                        if (root.controlBridge) {
                            root.controlBridge.applyShutterAngle(v)
                        } else {
                            root.shutterAngle = v
                        }
                    }
                    root.openDropdown = ""
                }
            }

            TopDropdownStat {
                width: ((parent.width - 76 - 12) - 36) / 4
                height: 86
                value: root.wb
                isOpen: root.openDropdown === "wb"
                options: ["2500K", "2800K", "3000K", "3200K", "3400K", "3600K", "4000K", "4500K", "4800K", "5000K", "5200K", "5400K", "5600K", "5800K", "6000K", "6500K", "7000K", "7500K", "8000K"]
                onToggleRequested: root.openDropdown = root.openDropdown === "wb" ? "" : "wb"
                onOptionSelected: function(v) {
                    if (root.controlBridge) {
                        root.controlBridge.applyWhiteBalance(v)
                    } else {
                        root.wb = v
                    }
                    root.openDropdown = ""
                }
            }

            Rectangle {
                width: 76
                height: 76
                radius: 22
                color: "transparent"

                MouseArea {
                    anchors.fill: parent
                    onClicked: root.openSettingsRequested()
                }
            }
        }
		
    // =========================
    // BOTTOM STATS
    // =========================

        Column {
    anchors.left: parent.left
    anchors.bottom: parent.bottom
    anchors.leftMargin: 20
    anchors.bottomMargin: 14
    spacing: 18
    width: 180

    InfoLine {
        label: "FORMAT"
        value: root.formatDisplayValue
        sub: root.formatDisplaySubtext
        iconSource: "qrc:/qml/icons/camera.png"
    }

    InfoLine {
    label: "MEDIA"
    value: mediaBridge.remainingMinutesText
    sub: mediaBridge.mediaMounted ? "CFExpress Type B" : "Insert Media"
    iconSource: "qrc:/qml/icons/media.png"
}

}

Column {
    anchors.right: parent.right
    anchors.bottom: parent.bottom
    anchors.rightMargin: 20
    anchors.bottomMargin: 14
    spacing: 18
    width: 178

    InfoLine {
    alignRight: true
    label: "CPU"
    value: statsBridge.cpuText
    sub: statsBridge.cpuPercent < 30 ? "Idle"
     : statsBridge.cpuPercent < 70 ? "Normal"
     : "High Load"
    iconSource: "qrc:/qml/icons/cpu.png"
}

    InfoLine {
        alignRight: true
        label: "POWER"
        value: root.powerOverlayValue
        sub: root.powerOverlaySubtext
        iconSource: "qrc:/qml/icons/power.png"
    }
}

    // =========================
    // TIMECODE
    // =========================


TextMetrics {
    id: timecodeMetrics
    font.family: interRegular.font.family
    font.pixelSize: 28
    text: "00:00:00:00"
}

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: root.showAudioMeter ? 24 : 40
            anchors.verticalCenter: undefined
            spacing: 16
            height: Math.max(centerStack.visible ? centerStack.height : 0,
                             clipBrowserBtn.height,
                             recordBtn.height)

            ClipBrowserButton {
                id: clipBrowserBtn
                anchors.verticalCenter: parent.verticalCenter
                enabled: !root.recording
                onClicked: root.openClipBrowserRequested()
            }

            Column {
                id: centerStack
                anchors.verticalCenter: parent.verticalCenter
                visible: !root.photoModeEnabled
                spacing: root.showAudioMeter ? 10 : 0

                AudioMeterOverlay {
                    id: audioMeterOverlay
                    width: timecodeBox.width
                    visible: root.showAudioMeter
                    inputLevel: root.inputVolumeLevel
                }

                Rectangle {
                    id: timecodeBox
                    height: 52
                    radius: 16
                    color: root.recording ? "#1aff4444" : "#14ffffff"
                    border.width: 1
                    border.color: root.recording ? "#4dff4444" : "#1affffff"
                    width: Math.ceil(timecodeMetrics.width) + 40

                    Text {
                        id: timecodeText
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 20
                        width: parent.width - 40
                        text: root.timecode
                        color: root.recording ? "#ff4d4d" : "white"
                        font.family: interRegular.font.family
                        font.pixelSize: 28
                        horizontalAlignment: Text.AlignLeft
                        renderType: Text.QtRendering

                        Behavior on color {
                            ColorAnimation {
                                duration: 260
                                easing.type: Easing.OutCubic
                            }
                        }
                    }
                }
            }

            // =========================
            // RECORD BUTTON
            // =========================

            RecordButton {
                id: recordBtn
                anchors.verticalCenter: parent.verticalCenter
                recording: root.recording
                photoMode: root.photoModeEnabled
                photoCapturePulse: root.photoCapturePulse

                onClicked: {
                    if (root.photoModeEnabled) {
                        if (!mediaBridge.mediaMounted) {
                            root.recordWarningOpen = true
                            return
                        }

                        root.beginPhotoCaptureSequence()
                        return
                    }

                    if (!root.recording) {
                        if (!mediaBridge.mediaMounted) {
                            root.recordWarningOpen = true
                            return
                        }
                        if (root.timecodeMode === "Rec Run") {
                            root.timecodeFrames = 0
                            root.updateTimecodeDisplay()
                        }
                        root.requestRecordingState(true)
                    } else {
                        root.requestRecordingState(false)
                        if (root.timecodeMode === "Free Run")
                            root.updateTimecodeDisplay()
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#cc000000"
        visible: root.recordWarningOpen
        opacity: root.recordWarningOpen ? 1.0 : 0.0
        z: 300

        Behavior on opacity {
            NumberAnimation {
                duration: 180
                easing.type: Easing.OutCubic
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: root.recordWarningOpen = false
        }
    }

    Rectangle {
        width: 404
        height: 286
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -18
        radius: 28
        color: "#111214"
        border.width: 1
        border.color: "#2a2d31"
        visible: root.recordWarningOpen
        opacity: root.recordWarningOpen ? 1.0 : 0.0
        scale: root.recordWarningOpen ? 1.0 : 0.96
        z: 301

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
            anchors.margins: 24
            spacing: 16

            Row {
                spacing: 12

                Rectangle {
                    width: 42
                    height: 42
                    radius: 21
                    color: "#6b5200"
                    border.width: 1
                    border.color: "#d7b44a"

                    Text {
                        anchors.centerIn: parent
                        text: "!"
                        color: "white"
                        font.family: interBold.font.family
                        font.weight: Font.Bold
                        font.pixelSize: 22
                        renderType: Text.NativeRendering
                    }
                }

                Column {
                    spacing: 4

                    Text {
                        text: "No Media Mounted"
                        color: "white"
                        font.family: interBold.font.family
                        font.weight: Font.Bold
                        font.pixelSize: 25
                        renderType: Text.NativeRendering
                    }

                    Text {
                        text: root.photoModeEnabled
                              ? "Insert CFExpress card to capture a still."
                              : "Insert CFExpress card to start recording."
                        color: "#8f9096"
                        font.family: interRegular.font.family
                        font.pixelSize: 14
                        renderType: Text.NativeRendering
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 84
                radius: 18
                color: "#0d0e10"
                border.width: 1
                border.color: "#202227"

                Column {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 4

                    Text {
                        text: root.photoModeEnabled
                              ? "Still capture is unavailable"
                              : "Recording is unavailable"
                        color: "white"
                        font.family: interBold.font.family
                        font.weight: Font.Bold
                        font.pixelSize: 16
                        renderType: Text.NativeRendering
                    }

                    Text {
                        text: root.photoModeEnabled
                              ? "The camera will stay in still mode until media is detected."
                              : "The camera will stay in standby until media is detected."
                        color: "#6f7076"
                        font.family: interRegular.font.family
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                        width: parent.width
                        renderType: Text.NativeRendering
                    }
                }
            }

            Item {
                width: 1
                height: 1
            }

            Rectangle {
                width: 132
                height: 52
                radius: 18
                anchors.right: parent.right
                color: confirmWarningArea.containsPress ? "#242428" : "#18181b"
                border.width: 1
                border.color: "#2c2d31"
                scale: confirmWarningArea.containsPress ? 0.985 : 1.0

                Behavior on scale {
                    NumberAnimation {
                        duration: 140
                        easing.type: Easing.OutCubic
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: "OK"
                    color: "white"
                    font.family: interMedium.font.family
                    font.pixelSize: 16
                    renderType: Text.NativeRendering
                }

                MouseArea {
                    id: confirmWarningArea
                    anchors.fill: parent
                    onClicked: root.recordWarningOpen = false
                }
            }
        }
    }
}

