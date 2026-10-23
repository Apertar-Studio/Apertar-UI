import QtQuick
import Apertar 1.0
import "TimelapseIntervals.js" as Intervals

Item {
    id: root
	
	signal openSettingsRequested()
    signal openClipBrowserRequested()
	
    FontLoader { id: gothamBold;   source: "qrc:/qml/fonts/Gotham/Gotham-Bold.ttf" }
	FontLoader { id: interLight;   source: "qrc:/qml/fonts/Inter/Inter-Light.ttf" }
	FontLoader { id: interRegular;   source: "qrc:/qml/fonts/Inter/Inter-Regular.ttf" }
	FontLoader { id: interMedium;   source: "qrc:/qml/fonts/Inter/Inter-Medium.ttf" }
	FontLoader { id: interBold;   source: "qrc:/qml/fonts/Inter/Inter-Bold.ttf" }



    property var bridge
    property var controlBridge
    readonly property bool whiteBalanceAvailable: !controlBridge || controlBridge.whiteBalanceAvailable
    onWhiteBalanceAvailableChanged: {
        if (!whiteBalanceAvailable && openDropdown === "wb") openDropdown = ""
    }
    property bool recording: false
    property bool timelapseActive: false
    property bool recordWarningOpen: false
    property string recordWarningTitle: "No Media Mounted"
    property string recordWarningSubtitle: ""
    property string recordWarningDetailTitle: ""
    property string recordWarningDetailBody: ""
	
    property var settingsState
    property string displayLayout: "square"
    property real controlsOpacity: 1.0
    property string controlsMode: "Light"
    property int sceneRotationDegrees: 0
    readonly property bool landscapeLayout: displayLayout === "landscape_compact"
                                            || displayLayout === "landscape_medium"
                                            || displayLayout === "landscape"
                                            || displayLayout === "landscape_large"
    readonly property bool landscapeCompactLayout: displayLayout === "landscape_compact"
    readonly property bool mediumLandscapeLayout: displayLayout === "landscape_medium"
    readonly property bool largeLandscapeLayout: displayLayout === "landscape_large"
    readonly property bool standardLandscapeLayout: displayLayout === "landscape"
                                                   || largeLandscapeLayout
    readonly property bool regularLandscapeLayout: mediumLandscapeLayout
                                                   || standardLandscapeLayout
    readonly property bool compactLandscapeLayout: landscapeCompactLayout || regularLandscapeLayout
    readonly property bool portraitLayout: displayLayout === "portrait_compact"
                                           || displayLayout === "portrait"
                                           || displayLayout === "portrait_large"
    readonly property bool compactPortraitLayout: displayLayout === "portrait_compact"
    readonly property bool rectangularLayout: landscapeLayout || portraitLayout
    readonly property bool compactRectangularLayout: compactLandscapeLayout || compactPortraitLayout
    readonly property bool compactDarkControls: compactLandscapeLayout && controlsMode === "Dark"
    readonly property real effectiveControlsOpacity: Math.max(0.15, Math.min(1.0, controlsOpacity))
    function fadedControlColor(colorValue) {
        return root.withAlphaFactor(colorValue, root.effectiveControlsOpacity)
    }

    function compactControlColor(defaultColor, compactLightColor, compactDarkColor) {
        var color = defaultColor
        if (root.compactLandscapeLayout)
            color = root.compactDarkControls && typeof compactDarkColor !== "undefined"
                    ? compactDarkColor
                    : compactLightColor
        return root.fadedControlColor(color)
    }

    function updateHorizontalLevelPolling() {
        if (typeof gyroConfigBridge !== "undefined" && gyroConfigBridge) {
            var active = root.settingsState
                         && root.settingsState.horizontalLevelEnabled
                         && (!root.controlBridge || !root.controlBridge.standby)
                         && !root.smpteEnabled
            gyroConfigBridge.setLivePollingRequest("camera-horizontal-level", active, true, 80)
        }
    }

    function withAlphaFactor(colorValue, factor) {
        var text = String(colorValue)
        if (text === "transparent")
            return "transparent"

        if (text.length > 0 && text[0] === "#") {
            var hex = text.slice(1)
            var alpha = 1.0
            var offset = 0

            if (hex.length === 8) {
                alpha = parseInt(hex.slice(0, 2), 16) / 255.0
                offset = 2
            } else if (hex.length !== 6) {
                return colorValue
            }

            var r = parseInt(hex.slice(offset, offset + 2), 16) / 255.0
            var g = parseInt(hex.slice(offset + 2, offset + 4), 16) / 255.0
            var b = parseInt(hex.slice(offset + 4, offset + 6), 16) / 255.0
            return Qt.rgba(r, g, b, Math.max(0.0, Math.min(1.0, alpha * factor)))
        }

        return colorValue
    }

    readonly property int previewTopOffset: rectangularLayout ? 0 : 157
    readonly property int previewVisibleHeight: rectangularLayout ? height : 405
    readonly property int topControlsSideMargin: rectangularLayout ? (largeLandscapeLayout ? 26 : (standardLandscapeLayout ? 20 : (compactLandscapeLayout ? 16 : (compactPortraitLayout ? 12 : 18)))) : 20
    readonly property int topControlsTopMargin: rectangularLayout ? (largeLandscapeLayout ? 24 : (standardLandscapeLayout ? 18 : (compactLandscapeLayout ? 14 : (compactPortraitLayout ? 14 : 18)))) : 20
    readonly property int topControlsSpacing: rectangularLayout ? (largeLandscapeLayout ? 18 : (standardLandscapeLayout ? 14 : (compactLandscapeLayout ? 10 : (compactPortraitLayout ? 8 : 10)))) : 12
    readonly property int topStatHeight: rectangularLayout ? (largeLandscapeLayout ? 100 : (standardLandscapeLayout ? 82 : (compactLandscapeLayout ? 64 : (compactPortraitLayout ? 72 : 80)))) : 86
    readonly property int topControlRadius: largeLandscapeLayout ? 28 : 22
    readonly property int topSettingsButtonSize: rectangularLayout ? (largeLandscapeLayout ? 92 : (standardLandscapeLayout ? 74 : (compactLandscapeLayout ? 56 : (compactPortraitLayout ? 62 : 70)))) : 76
    readonly property int topSettingsIconSize: largeLandscapeLayout ? 42 : (standardLandscapeLayout ? 34 : 28)
    readonly property int topValueFontSize: rectangularLayout ? (largeLandscapeLayout ? 37 : (standardLandscapeLayout ? 29 : (compactLandscapeLayout ? 19 : (compactPortraitLayout ? 21 : 23)))) : 25
    readonly property int topLabelFontSize: largeLandscapeLayout ? 16 : (standardLandscapeLayout ? 13 : (compactLandscapeLayout ? 11 : 13))
    readonly property int topArrowFontSize: largeLandscapeLayout ? 25 : (standardLandscapeLayout ? 20 : (compactLandscapeLayout ? 16 : 18))
    readonly property int topDropdownPopupMaxHeight: largeLandscapeLayout ? 500 : (standardLandscapeLayout ? 360 : (mediumLandscapeLayout ? 304 : (compactLandscapeLayout ? 220 : 340)))
    readonly property int topDropdownPopupMargin: largeLandscapeLayout ? 14 : (standardLandscapeLayout ? 10 : (mediumLandscapeLayout ? 8 : (compactLandscapeLayout ? 6 : 8)))
    readonly property int topDropdownPopupSpacing: largeLandscapeLayout ? 12 : (standardLandscapeLayout ? 8 : (mediumLandscapeLayout ? 6 : (compactLandscapeLayout ? 4 : 6)))
    readonly property int topDropdownPopupOptionHeight: largeLandscapeLayout ? 64 : (standardLandscapeLayout ? 46 : (mediumLandscapeLayout ? 38 : (compactLandscapeLayout ? 28 : 36)))
    readonly property int topDropdownPopupOptionRadius: largeLandscapeLayout ? 21 : (standardLandscapeLayout ? 16 : (mediumLandscapeLayout ? 13 : (compactLandscapeLayout ? 10 : 12)))
    readonly property int topDropdownPopupOptionFontSize: largeLandscapeLayout ? 44 : (standardLandscapeLayout ? 34 : (mediumLandscapeLayout ? 30 : (compactLandscapeLayout ? 22 : 34)))
    readonly property int topDropdownPopupRadius: largeLandscapeLayout ? 31 : (standardLandscapeLayout ? 22 : (mediumLandscapeLayout ? 18 : (compactLandscapeLayout ? 14 : 18)))
    readonly property int topDropdownPopupTopMargin: largeLandscapeLayout ? 14 : (standardLandscapeLayout ? 10 : (mediumLandscapeLayout ? 8 : (compactLandscapeLayout ? 6 : 8)))
    readonly property int bottomOverlayMargin: rectangularLayout ? (compactRectangularLayout ? 16 : 34) : 40
    readonly property int bottomControlsSpacing: largeLandscapeLayout ? 22 : (standardLandscapeLayout ? 18 : (mediumLandscapeLayout ? 14 : (compactLandscapeLayout ? 12 : 16)))
    readonly property int clipBrowserButtonSize: largeLandscapeLayout ? 88 : (standardLandscapeLayout ? 72 : (mediumLandscapeLayout ? 58 : (compactLandscapeLayout ? 52 : 64)))
    readonly property int clipBrowserIconSize: largeLandscapeLayout ? 34 : (standardLandscapeLayout ? 28 : (mediumLandscapeLayout ? 22 : (compactLandscapeLayout ? 20 : 24)))
    readonly property int recordButtonSize: largeLandscapeLayout ? 100 : (standardLandscapeLayout ? 82 : (mediumLandscapeLayout ? 64 : (compactLandscapeLayout ? 58 : 74)))
    readonly property int recordButtonCenterSize: largeLandscapeLayout ? 40 : (standardLandscapeLayout ? 32 : (mediumLandscapeLayout ? 24 : (compactLandscapeLayout ? 22 : 28)))
    readonly property int timecodeHeight: largeLandscapeLayout ? 66 : (standardLandscapeLayout ? 54 : (mediumLandscapeLayout ? 44 : (compactLandscapeLayout ? 40 : 52)))
    readonly property int timecodeRadius: largeLandscapeLayout ? 21 : (standardLandscapeLayout ? 17 : (mediumLandscapeLayout ? 14 : (compactLandscapeLayout ? 13 : 16)))
    readonly property int timecodeHorizontalPadding: largeLandscapeLayout ? 54 : (standardLandscapeLayout ? 44 : (mediumLandscapeLayout ? 30 : (compactLandscapeLayout ? 26 : 40)))
    readonly property int timecodeTextInset: largeLandscapeLayout ? 27 : (standardLandscapeLayout ? 22 : (mediumLandscapeLayout ? 15 : (compactLandscapeLayout ? 13 : 20)))
    readonly property int timecodeFontSize: largeLandscapeLayout ? 37 : (standardLandscapeLayout ? 30 : (mediumLandscapeLayout ? 24 : (compactLandscapeLayout ? 22 : 28)))
    readonly property int bottomControlsBaseMargin: compactLandscapeLayout ? 10 : bottomOverlayMargin
    readonly property int dateTimeChipHeight: largeLandscapeLayout ? 42 : (standardLandscapeLayout ? 34 : (mediumLandscapeLayout ? 28 : (compactLandscapeLayout ? 26 : 32)))
    readonly property int dateTimeChipRadius: largeLandscapeLayout ? 21 : (standardLandscapeLayout ? 17 : (mediumLandscapeLayout ? 14 : (compactLandscapeLayout ? 13 : 16)))
    readonly property int dateTimeChipHorizontalPadding: largeLandscapeLayout ? 34 : (standardLandscapeLayout ? 28 : (mediumLandscapeLayout ? 20 : (compactLandscapeLayout ? 18 : 22)))
    readonly property int dateTimeChipFontSize: largeLandscapeLayout ? 20 : (standardLandscapeLayout ? 16 : (mediumLandscapeLayout ? 13 : (compactLandscapeLayout ? 12 : 14)))
    readonly property string compactControlFill: "#24ffffff"
    readonly property string compactControlFillSoft: "#1cffffff"
    readonly property string compactControlFillActive: "#30ffffff"
    readonly property string compactControlBorder: "#30ffffff"
    readonly property string compactControlBorderActive: "#44ffffff"
    readonly property string compactDarkControlFill: "#54000000"
    readonly property string compactDarkControlFillSoft: "#44000000"
    readonly property string compactDarkControlFillActive: "#66000000"
    readonly property string compactDarkControlBorder: "#2affffff"
    readonly property string compactDarkControlBorderActive: "#3dffffff"
    readonly property int quickMenuPanelRadius: largeLandscapeLayout ? 34 : (standardLandscapeLayout ? 28 : (mediumLandscapeLayout ? 22 : (compactLandscapeLayout ? 20 : 26)))
    readonly property int quickMenuPanelMargin: largeLandscapeLayout ? 20 : (standardLandscapeLayout ? 16 : (mediumLandscapeLayout ? 12 : (compactLandscapeLayout ? 10 : 14)))
    readonly property int quickMenuPanelSpacing: largeLandscapeLayout ? 14 : (standardLandscapeLayout ? 11 : (mediumLandscapeLayout ? 8 : (compactLandscapeLayout ? 7 : 10)))
    readonly property int quickMenuResolutionHeight: largeLandscapeLayout ? 78 : (standardLandscapeLayout ? 64 : (mediumLandscapeLayout ? 48 : (compactLandscapeLayout ? 42 : 56)))
    readonly property int quickMenuResolutionRadius: largeLandscapeLayout ? 25 : (standardLandscapeLayout ? 21 : (mediumLandscapeLayout ? 16 : (compactLandscapeLayout ? 14 : 18)))
    readonly property int quickMenuResolutionLabelFontSize: largeLandscapeLayout ? 14 : (standardLandscapeLayout ? 12 : (mediumLandscapeLayout ? 10 : (compactLandscapeLayout ? 9 : 10)))
    readonly property int quickMenuResolutionValueFontSize: largeLandscapeLayout ? 23 : (standardLandscapeLayout ? 19 : (mediumLandscapeLayout ? 14 : (compactLandscapeLayout ? 12 : 15)))
    readonly property int quickMenuResolutionArrowFontSize: largeLandscapeLayout ? 27 : (standardLandscapeLayout ? 22 : (mediumLandscapeLayout ? 16 : (compactLandscapeLayout ? 14 : 18)))
    readonly property int quickMenuPopupMaxHeight: largeLandscapeLayout ? 286 : (standardLandscapeLayout ? 236 : (mediumLandscapeLayout ? 168 : (compactLandscapeLayout ? 142 : 196)))
    readonly property int quickMenuPopupOptionHeight: largeLandscapeLayout ? 54 : (standardLandscapeLayout ? 44 : (mediumLandscapeLayout ? 34 : (compactLandscapeLayout ? 30 : 38)))
    readonly property int quickMenuPopupOptionFontSize: largeLandscapeLayout ? 25 : (standardLandscapeLayout ? 21 : (mediumLandscapeLayout ? 16 : (compactLandscapeLayout ? 14 : 19)))
    readonly property int quickMenuButtonSize: largeLandscapeLayout ? 90 : (standardLandscapeLayout ? 74 : (mediumLandscapeLayout ? 56 : (compactLandscapeLayout ? 48 : 67)))
    readonly property int quickMenuButtonRadius: largeLandscapeLayout ? 28 : (standardLandscapeLayout ? 23 : (mediumLandscapeLayout ? 17 : (compactLandscapeLayout ? 15 : 20)))
    readonly property int quickMenuButtonSpacing: largeLandscapeLayout ? 14 : (standardLandscapeLayout ? 11 : (mediumLandscapeLayout ? 8 : (compactLandscapeLayout ? 6 : 10)))
    readonly property int quickMenuIconColumns: 2
    readonly property int quickMenuIconRows: Math.ceil(quickMenuButtonCount / quickMenuIconColumns)
    readonly property int quickMenuGridWidth: quickMenuIconColumns * quickMenuButtonSize + (quickMenuIconColumns - 1) * quickMenuButtonSpacing
    readonly property int quickMenuGridHeight: quickMenuIconRows * quickMenuButtonSize + Math.max(0, quickMenuIconRows - 1) * quickMenuButtonSpacing
    readonly property int quickMenuResolutionStackHeight: quickMenuResolutionHeight + (root.cropControlsVisible ? quickMenuPanelSpacing + quickMenuResolutionHeight : 0)
    readonly property int quickMenuPanelWidth: quickMenuGridWidth + (quickMenuPanelMargin * 2)
    readonly property int quickMenuPanelHeight: (quickMenuPanelMargin * 2) + quickMenuResolutionStackHeight + quickMenuPanelSpacing + quickMenuGridHeight
    readonly property int quickMenuIconSize: largeLandscapeLayout ? 52 : (standardLandscapeLayout ? 43 : (mediumLandscapeLayout ? 32 : (compactLandscapeLayout ? 27 : 38)))
    readonly property int quickMenuArrowButtonWidth: largeLandscapeLayout ? 58 : (standardLandscapeLayout ? 46 : (mediumLandscapeLayout ? 35 : (compactLandscapeLayout ? 30 : 54)))
    readonly property int quickMenuArrowButtonHeight: largeLandscapeLayout ? 132 : (standardLandscapeLayout ? 106 : (mediumLandscapeLayout ? 80 : (compactLandscapeLayout ? 66 : 124)))
    readonly property int quickMenuArrowCanvasWidth: largeLandscapeLayout ? 24 : (standardLandscapeLayout ? 20 : (mediumLandscapeLayout ? 15 : (compactLandscapeLayout ? 12 : 20)))
    readonly property int quickMenuArrowCanvasHeight: largeLandscapeLayout ? 44 : (standardLandscapeLayout ? 36 : (mediumLandscapeLayout ? 27 : (compactLandscapeLayout ? 22 : 38)))
    readonly property real quickMenuArrowLineWidth: largeLandscapeLayout ? 4.4 : (standardLandscapeLayout ? 3.7 : (mediumLandscapeLayout ? 3.0 : (compactLandscapeLayout ? 2.4 : 4.0)))
    readonly property bool showCornerStats: compactLandscapeLayout || !compactRectangularLayout
    readonly property int bottomStatsSideMargin: largeLandscapeLayout ? 30 : (standardLandscapeLayout ? 24 : (compactLandscapeLayout ? 12 : 20))
    readonly property int bottomStatsWidth: largeLandscapeLayout ? 246 : (standardLandscapeLayout ? 198 : (mediumLandscapeLayout ? 152 : (compactLandscapeLayout ? 136 : 180)))
    readonly property int bottomStatsRightWidth: largeLandscapeLayout ? 242 : (standardLandscapeLayout ? 194 : (mediumLandscapeLayout ? 148 : (compactLandscapeLayout ? 132 : 178)))
    readonly property int bottomStatsSpacing: largeLandscapeLayout ? 8 : (standardLandscapeLayout ? 6 : (mediumLandscapeLayout ? 4 : (compactLandscapeLayout ? 3 : 18)))
    readonly property int bottomStatsIconSize: largeLandscapeLayout ? 21 : (standardLandscapeLayout ? 17 : (mediumLandscapeLayout ? 13 : (compactLandscapeLayout ? 11 : 18)))
    readonly property int bottomStatsRowSpacing: largeLandscapeLayout ? 10 : (standardLandscapeLayout ? 8 : (mediumLandscapeLayout ? 6 : (compactLandscapeLayout ? 5 : 8)))
    readonly property int bottomStatsTextSpacing: compactLandscapeLayout ? 0 : 1
    readonly property int bottomStatsLabelFontSize: largeLandscapeLayout ? 16 : (standardLandscapeLayout ? 13 : (mediumLandscapeLayout ? 10 : (compactLandscapeLayout ? 9 : 13)))
    readonly property int bottomStatsValueFontSize: largeLandscapeLayout ? 24 : (standardLandscapeLayout ? 20 : (mediumLandscapeLayout ? 15 : (compactLandscapeLayout ? 13 : 18)))
    readonly property int bottomStatsSubFontSize: largeLandscapeLayout ? 16 : (standardLandscapeLayout ? 13 : (mediumLandscapeLayout ? 10 : (compactLandscapeLayout ? 9 : 13)))
    readonly property int bottomStatsHeightWithSub: largeLandscapeLayout ? 70 : (standardLandscapeLayout ? 58 : (mediumLandscapeLayout ? 42 : (compactLandscapeLayout ? 38 : 56)))
    readonly property int bottomStatsHeightWithoutSub: largeLandscapeLayout ? 58 : (standardLandscapeLayout ? 48 : (mediumLandscapeLayout ? 36 : (compactLandscapeLayout ? 32 : 46)))
    readonly property int bottomStatsVerticalOffset: largeLandscapeLayout ? -20 : (standardLandscapeLayout ? -16 : (compactLandscapeLayout ? -12 : 0))
    readonly property int bottomStatsBottomMargin: standardLandscapeLayout ? (bottomOverlayMargin + 14)
                                                   : (root.rectangularLayout ? root.bottomOverlayMargin : 14)
    readonly property real falseColorLegendDisplayScale: largeLandscapeLayout ? 1.6 : (standardLandscapeLayout ? 1.12 : 1.0)
    readonly property real photoStatusChipScale: compactLandscapeLayout
                                                 ? (largeLandscapeLayout ? 1.38
                                                    : (standardLandscapeLayout ? 1.16
                                                       : (regularLandscapeLayout ? 0.96 : 0.80)))
                                                 : 1.0
    readonly property int photoStatusChipHeight: Math.round(30 * photoStatusChipScale)
    readonly property int photoStatusChipHorizontalPadding: Math.round(18 * photoStatusChipScale)
    readonly property int photoStatusChipDotSize: compactLandscapeLayout
                                                 ? (largeLandscapeLayout ? 14
                                                    : (standardLandscapeLayout ? 12
                                                       : (regularLandscapeLayout ? 10 : 8)))
                                                 : 10
    readonly property int photoStatusChipFontSize: compactLandscapeLayout
                                                  ? (largeLandscapeLayout ? 18
                                                     : (standardLandscapeLayout ? 16
                                                        : (regularLandscapeLayout ? 14 : 12)))
                                                  : 14
    readonly property string timecodeMode: settingsState ? settingsState.timecodeMode : "Free Run"
    readonly property bool photoModeEnabled: settingsState ? settingsState.photoModeEnabled : false
    readonly property string uiOrientation: settingsState ? settingsState.uiOrientation : "Landscape"
    readonly property int uiPreviewRotation: uiOrientation === "Left Side" ? -90
                                              : (uiOrientation === "Right Side" ? 90
                                                                           : (uiOrientation === "Upside Down" ? 180 : 0))
    readonly property string photoTimerSetting: settingsState ? settingsState.photoTimer : "Off"
    readonly property string photoBurstSetting: settingsState ? settingsState.photoBurst : "Single"
    readonly property string photoFormatSetting: settingsState ? settingsState.photoFormat : "DNG"
    readonly property string photoTimelapseSetting: settingsState ? settingsState.photoTimelapse : "Off"
    readonly property bool photoTimelapseEnabled: root.photoModeEnabled
                                                 && root.photoTimelapseSetting
                                                 && root.photoTimelapseSetting !== "Off"
    readonly property bool dateTimeOverlayEnabled: settingsState ? settingsState.dateTimeOverlayEnabled : false
    readonly property bool smpteEnabled: settingsState ? settingsState.smpteEnabled : false
    readonly property bool falseColorLegendVisible: settingsState ? (settingsState.falseColorEnabled && !settingsState.smpteEnabled) : false
    readonly property bool recordAudioEnabled: settingsState ? settingsState.recordAudioEnabled : false
    readonly property bool soundRecorderEnabled: settingsState ? settingsState.soundRecorderEnabled : false
    readonly property bool audioMeterEnabled: settingsState ? settingsState.audioMeterEnabled : false
    readonly property bool audioInputAvailable: (typeof audioMeterBridge !== "undefined" && audioMeterBridge)
                                               ? audioMeterBridge.inputDeviceAvailable
                                               : false
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

    function activeBatteryType() {
        return root.settingsState && root.settingsState.batteryType
               ? root.settingsState.batteryType
               : "V-Mount"
    }

    function batterySupportsPercent(type) {
        return type !== "USB-C / DC"
    }

    function defaultBatteryFullVoltageForType(type) {
        if (type === "NP-F" || type === "LP-E6")
            return 8.4
        return 16.8
    }

    function defaultBatteryEmptyVoltageForType(type) {
        if (type === "NP-F" || type === "LP-E6")
            return 6.0
        return 13.2
    }

    function estimatedBatteryPercent() {
        if (!powerBridge.sensorAvailable || !root.batteryInputPresent)
            return -1

        var type = root.activeBatteryType()
        if (!root.batterySupportsPercent(type))
            return -1

        var full = root.defaultBatteryFullVoltageForType(type)
        var empty = root.defaultBatteryEmptyVoltageForType(type)
        var sag = type === "V-Mount" ? 0.26 : 0.08
        if (type === "Custom") {
            full = root.settingsState ? root.settingsState.customBatteryFullVoltage : 16.8
            empty = root.settingsState ? root.settingsState.customBatteryEmptyVoltage : 13.2
            sag = 0.0
        }

        var compensatedVoltage = powerBridge.busVoltageV + Math.max(0, powerBridge.currentA) * sag
        var percent = ((compensatedVoltage - empty) / Math.max(0.1, full - empty)) * 100.0
        return Math.max(0, Math.min(100, percent))
    }

    readonly property int inputVolumeLevel: settingsState ? settingsState.inputVolume : 60
    readonly property int headphoneVolumeLevel: settingsState ? settingsState.headphoneVolume : 55
    readonly property bool showAudioMeter: root.recordAudioEnabled
                                           && root.audioMeterEnabled
                                           && !root.photoModeEnabled
                                           && root.audioInputAvailable
    readonly property bool batteryInputPresent: powerBridge.sensorAvailable && powerBridge.busVoltageV > 3.0
    readonly property string batteryPercentText: {
        var percent = root.estimatedBatteryPercent()
        return percent >= 0 ? (Math.round(percent) + "%") : "--%"
    }
    readonly property string powerOverlayValue: !powerBridge.sensorAvailable
                                               ? "INA219"
                                               : (!root.batteryInputPresent
                                                  ? "External"
                                                  : (root.batterySupportsPercent(root.activeBatteryType())
                                                     ? (powerBridge.voltageText + " • " + root.batteryPercentText)
                                                     : powerBridge.voltageText))
    readonly property string powerOverlaySubtext: !powerBridge.sensorAvailable
                                                 ? "Unavailable"
                                                 : (!root.batteryInputPresent
                                                    ? "DC INPUT"
                                                    : (root.activeBatteryType().toUpperCase() + " • " + powerBridge.powerText))
    property bool photoCapturePulse: false
    property bool photoCaptureInProgress: false
    property int pendingPhotoShots: 0
    property int photoCountdownSecondsRemaining: 0
    property date currentDateTime: new Date()
    readonly property string dateTimeOverlayText: Qt.formatDateTime(root.currentDateTime, "dd MMM yyyy  HH:mm")

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
	        root.updateHorizontalLevelPolling()
	    }
	
    property string iso: "800"
    property string shutterAngle: "180°"
    property string shutterSpeed: "1/48"
    property string wb: "5600K"
    property string resolution: "1920x1080"
    property bool controlResolutionSynced: false
    readonly property string recordingFormat: root.controlBridge && root.controlBridge.recordingFormat.length > 0
                                             ? root.controlBridge.recordingFormat
                                             : "cDNG"
    readonly property bool h264RecordFormat: !root.photoModeEnabled && root.recordingFormat === "H.264"
    readonly property bool processedVideoRecordFormat: root.h264RecordFormat
    readonly property string processedVideoSizeLabel: "H.264 Size"
    readonly property string processedVideoCropLabel: "H.264 Crop"
    readonly property string h264ResolutionValue: root.controlBridge && root.controlBridge.h264Resolution.length > 0
                                                  ? root.controlBridge.h264Resolution
                                                  : "Match Capture"
    readonly property string rawBitDepthValue: root.controlBridge && root.controlBridge.rawBitDepth.length > 0
                                               ? root.controlBridge.rawBitDepth
                                               : "12-bit"
    readonly property string cdngCompressionValue: root.controlBridge && root.controlBridge.cdngCompression.length > 0
                                                   ? root.controlBridge.cdngCompression
                                                   : "none"
    readonly property string activeSensorName: root.controlBridge
                                               && root.controlBridge.sensorName
                                               && root.controlBridge.sensorName.toLowerCase().indexOf("unknown") === -1
                                               ? root.controlBridge.sensorName
                                               : ((typeof deviceInfoBridge !== "undefined"
                                                   && deviceInfoBridge
                                                   && deviceInfoBridge.sensorName)
                                                  ? deviceInfoBridge.sensorName
                                                  : "")
    readonly property string sensorNameLower: root.activeSensorName.toLowerCase()
    readonly property bool imx585Detected: root.sensorNameLower.indexOf("imx585") !== -1
    readonly property bool imx294Detected: root.sensorNameLower.indexOf("imx294") !== -1
    readonly property bool imx577Detected: root.sensorNameLower.indexOf("imx577") !== -1
    readonly property bool imx477Detected: root.sensorNameLower.indexOf("imx477") !== -1
    readonly property var imx477ResolutionOptions: ["1332x990", "2028x1080", "2028x1140", "2028x1096", "2028x1068", "2028x1014", "2028x922", "2028x914", "2028x864", "2028x848", "2028x812", "2028x796", "2028x1520", "4056x2160", "4056x2280", "4056x2192", "4056x2136", "4056x2028", "4056x1844", "4056x1828", "4056x1728", "4056x1696", "4056x1624", "4056x1592", "4056x3040"]
    readonly property var imx477BaseResolutionOptions: [
        { value: "2028x1080", label: "2K" },
        { value: "4056x2160", label: "4K" }
    ]
    readonly property var imx477TwoKCropRatioOptions: [
        { value: "2028x1520", label: "4:3" },
        { value: "2028x1140", label: "16:9" },
        { value: "2028x1080", label: "17:9" },
        { value: "2028x1096", label: "1.85:1" },
        { value: "2028x1068", label: "1.90:1" },
        { value: "2028x1014", label: "2.00:1" },
        { value: "2028x922", label: "2.20:1" },
        { value: "2028x914", label: "2.22:1" },
        { value: "2028x864", label: "2.35:1" },
        { value: "2028x848", label: "2.39:1" },
        { value: "2028x812", label: "2.50:1" },
        { value: "2028x796", label: "2.55:1" }
    ]
    readonly property var imx477FourKCropRatioOptions: [
        { value: "4056x3040", label: "4:3" },
        { value: "4056x2280", label: "16:9" },
        { value: "4056x2160", label: "17:9" },
        { value: "4056x2192", label: "1.85:1" },
        { value: "4056x2136", label: "1.90:1" },
        { value: "4056x2028", label: "2.00:1" },
        { value: "4056x1844", label: "2.20:1" },
        { value: "4056x1828", label: "2.22:1" },
        { value: "4056x1728", label: "2.35:1" },
        { value: "4056x1696", label: "2.39:1" },
        { value: "4056x1624", label: "2.50:1" },
        { value: "4056x1592", label: "2.55:1" }
    ]
    readonly property var imx294Raw10ResolutionOptions: ["4144x2176", "4096x2048", "4096x1844", "4096x1744", "4096x1716", "4096x1640", "3872x2176", "3792x2824", "3792x2528", "3392x2824"]
    readonly property var imx294Raw12ResolutionOptions: ["4176x2176", "4096x2048", "4096x1844", "4096x1744", "4096x1716", "4096x1640", "3872x2176", "3792x2824", "3792x2528", "3392x2824"]
    readonly property var imx294ResolutionOptions: root.rawBitDepthValue === "10-bit"
                                                   ? root.imx294Raw10ResolutionOptions
                                                   : root.imx294Raw12ResolutionOptions
    readonly property var imx294BaseResolutionOptions: [
        { value: root.imx294ResolutionOptions[0], label: "4K" }
    ]
    readonly property var imx585Raw12ResolutionOptions: ["1920x1080", "1920x1038", "1920x1012", "1920x960", "1920x872", "1920x864", "1920x816", "1920x804", "1920x768", "1920x752", "3840x2160", "3840x2076", "3840x2024", "3840x1920", "3840x1744", "3840x1728", "3840x1632", "3840x1608", "3840x1536", "3840x1504"]
    readonly property var imx585BaseResolutionOptions: [
        { value: "1920x1080", label: "Full HD" },
        { value: "3840x2160", label: "4K" }
    ]
    readonly property var imx585CropRatioOptions: [
        { value: "3840x2160", label: "16:9" },
        { value: "3840x2076", label: "1.85:1" },
        { value: "3840x2024", label: "1.90:1" },
        { value: "3840x1920", label: "2.00:1" },
        { value: "3840x1744", label: "2.20:1" },
        { value: "3840x1728", label: "2.22:1" },
        { value: "3840x1632", label: "2.35:1" },
        { value: "3840x1608", label: "2.39:1" },
        { value: "3840x1536", label: "2.50:1" },
        { value: "3840x1504", label: "2.55:1" }
    ]
    readonly property var imx585FullHdCropRatioOptions: [
        { value: "1920x1080", label: "16:9" },
        { value: "1920x1038", label: "1.85:1" },
        { value: "1920x1012", label: "1.90:1" },
        { value: "1920x960", label: "2.00:1" },
        { value: "1920x872", label: "2.20:1" },
        { value: "1920x864", label: "2.22:1" },
        { value: "1920x816", label: "2.35:1" },
        { value: "1920x804", label: "2.39:1" },
        { value: "1920x768", label: "2.50:1" },
        { value: "1920x752", label: "2.55:1" }
    ]
    readonly property bool imx585CropControlsAvailable: root.imx585Detected
    readonly property bool imx585FourKSelected: root.isFourKResolution(root.resolution)
    readonly property bool imx585FullHdSelected: root.isImx585FullHdResolution(root.resolution)
    readonly property var imx585ActiveCropOptions: root.imx585FourKSelected ? root.imx585CropRatioOptions : root.imx585FullHdCropRatioOptions
    readonly property bool imx477FourKSelected: root.isImx477FourKResolution(root.resolution)
    readonly property bool sensorCropControlsAvailable: root.imx585Detected || root.imx477Detected || root.imx294Detected
    readonly property bool cropControlsVisible: root.processedVideoRecordFormat || root.sensorCropControlsAvailable
    readonly property var sensorBaseResolutionOptions: root.imx294Detected ? root.imx294BaseResolutionOptions
                                                      : root.imx477Detected ? root.imx477BaseResolutionOptions : root.imx585BaseResolutionOptions
    readonly property var sensorActiveCropOptions: root.sensorCropControlsAvailable
                                                   ? root.sensorCropOptionsForBase(root.resolutionBaseValue)
                                                   : []
    readonly property string resolutionBaseValue: root.processedVideoRecordFormat
                                                 ? root.h264BaseResolutionValue
                                                 : (root.sensorCropControlsAvailable
                                                    ? root.sensorBaseResolutionForValue(root.resolution)
                                                    : root.resolution)
    readonly property string resolutionBaseLabel: root.processedVideoRecordFormat
                                                 ? root.h264BaseResolutionLabel
                                                 : root.sensorBaseResolutionLabel(root.resolutionBaseValue)
    readonly property string cropModeValue: root.processedVideoRecordFormat ? root.h264CropModeValue : root.resolution
    readonly property string cropRatioLabel: root.processedVideoRecordFormat ? root.h264CropRatioLabel : root.cropRatioLabelForResolution(root.cropModeValue)
    readonly property string processedVideoDefaultResolutionValue: root.controlBridge && root.controlBridge.h264MaxWidth < 2560 ? "1920x1080" : "2560x1440"
    readonly property string h264OutputResolutionValue: root.h264ResolutionValue === "Match Capture" ? root.processedVideoDefaultResolutionValue : root.h264ResolutionValue
    readonly property string h264BaseResolutionValue: root.h264BaseResolutionForValue(root.h264OutputResolutionValue)
    readonly property string h264BaseResolutionLabel: root.h264BaseResolutionValue
    readonly property string h264CropRatioLabel: root.cropRatioLabelForResolution(root.h264OutputResolutionValue)
    readonly property string h264CropModeValue: root.h264ResolutionForBaseAndRatio(root.h264BaseResolutionValue, root.h264CropRatioLabel)
    readonly property var h264ActiveCropOptions: root.h264CropOptionsForBase(root.h264BaseResolutionValue)
    readonly property var h264BaseResolutionOptions: [
        { value: "1920x1080", label: "1920x1080" }
    ].concat(!root.controlBridge || root.controlBridge.h264MaxWidth >= 2560 ? [
        { value: "2560x1440", label: "2560x1440" }
    ] : [])
    readonly property real previewImageAspectRatio: root.aspectRatioForResolution(root.processedVideoRecordFormat ? root.h264OutputResolutionValue : root.resolution)
    readonly property real previewDisplayAspectRatio: previewImageAspectRatio
                                                      * (anamorphicDesqueezeEnabled && !smpteEnabled ? anamorphicDesqueezeRatio : 1.0)
    readonly property var resolutionOptions: root.imx577Detected
                                             ? ["1332x990", "2028x1080", "2028x1520", "4056x2160", "4056x3040"]
                                             : (root.imx477Detected
                                                ? root.imx477ResolutionOptions
                                                : (root.imx294Detected
                                                   ? root.imx294ResolutionOptions
                                                   : root.imx585Raw12ResolutionOptions))
    readonly property string shutterDisplayValue: root.photoModeEnabled ? root.shutterSpeed : root.shutterAngle
    readonly property var shutterOptions: root.photoModeEnabled
                                         ? Intervals.shutterOptions(["Auto"].concat(settingsState && settingsState.photoLongExposureEnabled
                                             ? ["30s", "25s", "20s", "15s", "10s", "8s", "6s", "5s", "4s", "3s", "2s", "1s", "1/2", "1/4", "1/8", "1/12", "1/15"] : [])
                                             .concat(["1/24", "1/30", "1/40", "1/48", "1/50", "1/60", "1/80", "1/100", "1/125", "1/160", "1/200", "1/250", "1/320", "1/500", "1/1000"]),
                                             root.photoTimelapseEnabled ? root.photoTimelapseSetting : "Off")
                                         : ["Auto", "11.25°", "15°", "22.5°", "30°", "37.5°", "45°", "60°", "72°", "75°", "90°", "108°", "120°", "144°", "150°", "172.8°", "180°", "216°", "270°", "324°", "360°"]
    readonly property var fpsOptions: root.availableFpsOptions()
    readonly property bool losslessFourKMode: !root.photoModeEnabled && !root.imx294Detected
                                                && root.recordingFormat === "cDNG"
                                                && root.cdngCompressionValue === "lossless_jpeg"
                                                && root.isFourKCaptureResolution(root.resolution)
    readonly property bool losslessTwoKMode: !root.photoModeEnabled && !root.imx294Detected
                                               && root.recordingFormat === "cDNG"
                                               && root.cdngCompressionValue === "lossless_jpeg"
                                               && root.isTwoKCaptureResolution(root.resolution)
    readonly property bool losslessImx585Mode: !root.photoModeEnabled && root.imx585Detected
                                               && root.recordingFormat === "cDNG"
                                               && root.cdngCompressionValue === "lossless_jpeg"
    readonly property bool losslessIsoLimitedMode: root.losslessImx585Mode || root.losslessFourKMode || root.losslessTwoKMode
    readonly property var isoOptions: root.losslessIsoLimitedMode
                                      ? ["100", "200", "400", "800"]
                                      : ["Auto", "100", "200", "400", "800", "1600", "3200", "6400", "12800", "25600", "51200"]
    readonly property string formatDisplayValue: root.processedVideoRecordFormat
                                                 ? root.h264OutputResolutionValue
                                                 : root.resolution
    readonly property string formatDisplaySubtext: root.photoModeEnabled ? root.photoFormatSetting : root.recordingFormat.indexOf("ProRes") === 0 ? root.recordingFormat : root.h264RecordFormat ? "H.264"
                                                   : (root.recordingFormat === "MLV" ? "MLV RAW"
                                                         : (root.cdngCompressionValue === "lossless_jpeg" ? "Lossless"
                                                            : (root.cdngCompressionValue === "lossy_3to1" ? "Lossy 3:1"
                                                               : (root.cdngCompressionValue === "lossy_5to1" ? "Lossy 5:1" : "Uncompressed"))))
	
	//Zoom Property
	property real previewZoom: 1.0
    property real minPreviewZoom: 1.0
    property real maxPreviewZoom: 4.0
    readonly property real hardwarePreviewZoom: typeof displayConfigBridge !== "undefined" && displayConfigBridge
                                               ? displayConfigBridge.zoomToggleFactor : root.maxPreviewZoom
	
	//Pan Property
	property real previewPanX: 0.0
    property real previewPanY: 0.0

    property string openDropdown: ""
    property bool quickMenuOpen: false
    property bool captureModeMenuOpen: false
    readonly property bool stillExposureActive: root.controlBridge ? root.controlBridge.stillExposureActive : false
    readonly property bool mediaWritesPending: root.controlBridge ? root.controlBridge.mediaWritesPending : false
    readonly property bool canSelectCaptureMode: !root.recording && !root.timelapseActive && !root.stillExposureActive
                                                 && !root.photoCaptureInProgress && !root.mediaWritesPending
                                                 && (!root.controlBridge || !root.controlBridge.standby)
    readonly property string selectedCaptureMode: root.soundRecorderEnabled ? "sound"
                                                  : !root.photoModeEnabled ? "clip"
                                                  : root.photoTimelapseEnabled ? "timelapse" : "still"
    onCanSelectCaptureModeChanged: { if (!canSelectCaptureMode) captureModeMenuOpen = false }
    onVisibleChanged: { if (!visible) captureModeMenuOpen = false }
    onEnabledChanged: { if (!enabled) captureModeMenuOpen = false }
    onWidthChanged: { if (captureModeMenuOpen) Qt.callLater(root.positionCaptureModeMenu) }
    onHeightChanged: { if (captureModeMenuOpen) Qt.callLater(root.positionCaptureModeMenu) }
    property bool quickMenuResolutionOpen: false
    property string quickMenuFieldOpen: ""
    readonly property int quickMenuButtonCount: 6
    readonly property real quickMenuSwipeEdgeWidth: 44

    onQuickMenuOpenChanged: {
        if (!quickMenuOpen) {
            quickMenuResolutionOpen = false
            quickMenuFieldOpen = ""
        }
    }

    onProcessedVideoRecordFormatChanged: {
        if (processedVideoRecordFormat) {
            quickMenuResolutionOpen = false
            quickMenuFieldOpen = ""
        }
        root.ensureValidFpsSelection()
        root.updateMediaEstimate()
    }

    function quickMenuSlotLabel(index) {
        if (index <= 5)
            return ""
        return "Q" + (index + 1)
    }

    function quickMenuSlotSubtitle(index) {
        if (index <= 5)
            return ""
        return "Soon"
    }

    function quickMenuSlotIconSource(index) {
        if (index === 0)
            return "qrc:/qml/icons/guides.png"
        if (index === 1)
            return "qrc:/qml/icons/center-marker.png"
        if (index === 2)
            return "qrc:/qml/icons/focuspeaking.png"
        if (index === 3)
            return "qrc:/qml/icons/false-color.png"
        if (index === 4)
            return "qrc:/qml/icons/zebra.png"
        if (index === 5)
            return "qrc:/qml/icons/grayscale.png"
        return ""
    }

    function quickMenuSlotActive(index) {
        if (!root.settingsState)
            return false
        if (index === 0)
            return root.settingsState.guidesEnabled
        if (index === 1)
            return root.settingsState.centerMarkerEnabled
        if (index === 2)
            return root.settingsState.focusPeakingEnabled
        if (index === 3)
            return root.settingsState.falseColorEnabled
        if (index === 4)
            return root.settingsState.zebraEnabled
        if (index === 5)
            return root.settingsState.grayscaleEnabled
        return false
    }

    function triggerQuickMenuSlot(index) {
        if (!root.settingsState)
            return

        if (index === 0)
            root.settingsState.guidesEnabled = !root.settingsState.guidesEnabled
        else if (index === 1)
            root.settingsState.centerMarkerEnabled = !root.settingsState.centerMarkerEnabled
        else if (index === 2)
            root.settingsState.focusPeakingEnabled = !root.settingsState.focusPeakingEnabled
        else if (index === 3)
            root.settingsState.falseColorEnabled = !root.settingsState.falseColorEnabled
        else if (index === 4)
            root.settingsState.zebraEnabled = !root.settingsState.zebraEnabled
        else if (index === 5)
            root.settingsState.grayscaleEnabled = !root.settingsState.grayscaleEnabled
    }

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

function currentRawBitDepthNumber() {
    if (root.rawBitDepthValue === "8-bit")
        return 8
    if (root.rawBitDepthValue === "10-bit")
        return 10
    return 12
}

function imx477MaximumSupportedFpsForResolution(resolutionValue) {
    var value = String(resolutionValue || "")
    var bits = root.currentRawBitDepthNumber()
    var limits = bits === 8 ? {
        "1332x990": 280.0,
        "2028x796": 240.0, "2028x812": 230.0, "2028x848": 220.0, "2028x864": 220.0,
        "2028x914": 210.0, "2028x922": 210.0, "2028x1014": 190.0, "2028x1068": 180.0,
        "2028x1080": 180.0, "2028x1096": 170.0, "2028x1140": 170.0, "2028x1520": 130.0,
        "4056x1592": 60.0, "4056x1624": 60.0, "4056x1696": 60.0, "4056x1728": 60.0,
        "4056x1828": 50.0, "4056x1844": 50.0, "4056x2028": 50.0,
        "4056x2136": 45.0, "4056x2160": 45.0, "4056x2192": 45.0, "4056x2280": 45.0,
        "4056x3040": 35.0
    } : (bits === 10 ? {
        "1332x990": 230.0,
        "2028x796": 190.0, "2028x812": 190.0, "2028x848": 180.0, "2028x864": 180.0,
        "2028x914": 170.0, "2028x922": 170.0, "2028x1014": 150.0, "2028x1068": 150.0,
        "2028x1080": 140.0, "2028x1096": 140.0, "2028x1140": 140.0, "2028x1520": 100.0,
        "4056x1592": 50.0, "4056x1624": 50.0, "4056x1696": 50.0,
        "4056x1728": 50.0, "4056x1828": 45.0, "4056x1844": 45.0, "4056x2028": 40.0,
        "4056x2136": 40.0, "4056x2160": 40.0, "4056x2192": 35.0, "4056x2280": 35.0,
        "4056x3040": 25.0
    } : {
        "1332x990": 130.0,
        "2028x796": 160.0, "2028x812": 160.0, "2028x848": 150.0, "2028x864": 150.0,
        "2028x914": 140.0, "2028x922": 140.0, "2028x1014": 130.0, "2028x1068": 120.0,
        "2028x1080": 120.0, "2028x1096": 120.0, "2028x1140": 110.0, "2028x1520": 60.0,
        "4056x1592": 45.0, "4056x1624": 40.0, "4056x1696": 40.0, "4056x1728": 40.0,
        "4056x1828": 35.0, "4056x1844": 35.0, "4056x2028": 35.0, "4056x2136": 30.0,
        "4056x2160": 30.0, "4056x2192": 30.0, "4056x2280": 30.0, "4056x3040": 24.0
    })
    return limits[value] !== undefined ? limits[value] : 120.0
}

function imx477MaximumUncompressedFpsForResolution(resolutionValue) {
    var value = String(resolutionValue || "")
    var bits = root.currentRawBitDepthNumber()
    var limits = bits === 8 ? {
        "2028x796": 140.0, "2028x812": 140.0, "2028x848": 140.0, "2028x864": 120.0,
        "2028x914": 120.0, "2028x922": 120.0, "2028x1014": 100.0, "2028x1068": 100.0,
        "2028x1080": 100.0, "2028x1096": 90.0, "2028x1140": 90.0, "2028x1520": 90.0,
        "4056x1592": 30.0, "4056x1624": 30.0, "4056x1696": 30.0, "4056x1728": 30.0,
        "4056x1828": 30.0, "4056x1844": 30.0, "4056x2028": 30.0, "4056x2136": 30.0,
        "4056x2160": 30.0, "4056x2192": 30.0, "4056x2280": 30.0, "4056x3040": 25.0
    } : (bits === 10 ? {
        "2028x796": 120.0, "2028x812": 120.0, "2028x848": 120.0, "2028x864": 120.0,
        "2028x914": 130.0, "2028x922": 130.0, "2028x1014": 100.0, "2028x1068": 100.0,
        "2028x1080": 100.0, "2028x1096": 90.0, "2028x1140": 90.0, "2028x1520": 90.0,
        "4056x1592": 30.0, "4056x1624": 30.0, "4056x1696": 30.0, "4056x1728": 30.0,
        "4056x1828": 30.0, "4056x1844": 30.0, "4056x2028": 30.0, "4056x2136": 30.0,
        "4056x2160": 30.0, "4056x2192": 30.0, "4056x2280": 30.0, "4056x3040": 24.0
    } : {
        "2028x796": 120.0, "2028x812": 120.0, "2028x848": 120.0, "2028x864": 120.0,
        "2028x914": 120.0, "2028x922": 120.0, "2028x1014": 100.0, "2028x1068": 100.0,
        "2028x1080": 100.0, "2028x1096": 90.0, "2028x1140": 60.0, "2028x1520": 60.0,
        "4056x1592": 30.0, "4056x1624": 30.0, "4056x1696": 30.0, "4056x1728": 30.0,
        "4056x1828": 30.0, "4056x1844": 30.0, "4056x2028": 30.0, "4056x2136": 30.0,
        "4056x2160": 30.0, "4056x2192": 30.0, "4056x2280": 30.0, "4056x3040": 24.0
    })
    return limits[value] !== undefined ? limits[value] : 1000.0
}

function maximumSupportedFpsForResolution(resolutionValue) {
    if (root.imx577Detected) {
        if (resolutionValue === "1332x990")
            return 101.68
        if (resolutionValue === "2028x1080" || resolutionValue === "1928x1090" || resolutionValue === "1920x1080")
            return 62.81
        if (resolutionValue === "2028x1520")
            return 45.19
        if (resolutionValue === "4056x2160" || resolutionValue === "3856x2180" || resolutionValue === "3840x2160")
            return 16.39
        if (resolutionValue === "4056x3040")
            return 11.72
    }

    if (root.imx477Detected) {
        var imx477Resolution = root.normalizedResolutionForCurrentSensor(resolutionValue)
        return root.imx477MaximumSupportedFpsForResolution(imx477Resolution)
    }

    if (root.imx294Detected) {
        if (resolutionValue === "3792x2824" ||
                resolutionValue === "3792x2528" ||
                resolutionValue === "3392x2824")
            return 34.70
        if (resolutionValue === "4176x2176")
            return 40.95
        if (resolutionValue === "4096x2048" ||
                resolutionValue === "4096x1844" ||
                resolutionValue === "4096x1744" ||
                resolutionValue === "4096x1716" ||
                resolutionValue === "4096x1640" ||
                resolutionValue === "2048x1080" ||
                resolutionValue === "1920x1080")
            return 40.95
        if (resolutionValue === "4144x2176")
            return 41.08
        if (resolutionValue === "3872x2176")
            return 44.17
    }

    if (root.imx585Detected) {
        var imx585Max = 60.0
        if (resolutionValue === "3856x2180" || resolutionValue === "3840x2160")
            imx585Max = 30.0
        else if (resolutionValue === "3840x2024" ||
                 resolutionValue === "3840x2076" ||
                 resolutionValue === "3840x1920" ||
                 resolutionValue === "3840x1744" ||
                 resolutionValue === "3840x1728" ||
                 resolutionValue === "3840x1632" ||
                 resolutionValue === "3840x1608" ||
                 resolutionValue === "3840x1536" ||
                 resolutionValue === "3840x1504" ||
                 resolutionValue === "1920x1038" ||
                 resolutionValue === "1920x1012" ||
                 resolutionValue === "1920x960" ||
                 resolutionValue === "1920x872" ||
                 resolutionValue === "1920x864" ||
                 resolutionValue === "1920x816" ||
                 resolutionValue === "1920x804" ||
                 resolutionValue === "1920x768" ||
                 resolutionValue === "1920x752")
            imx585Max = 60.0
        return imx585Max
    }

    return 60.0
}

function availableFpsOptions() {
    var presets = root.imx477Detected
                  ? [24.000, 25.000, 30.000, 35.000, 40.000, 45.000, 50.000, 60.000, 70.000, 80.000, 90.000, 100.000, 110.000, 120.000, 130.000, 140.000, 150.000, 160.000, 170.000, 180.000, 190.000, 200.000, 210.000, 220.000, 230.000, 240.000, 280.000]
                  : ((root.imx577Detected)
                     ? [24.000, 25.000, 30.000, 50.000, 60.000, 100.000]
                     : [24.000, 25.000, 30.000, 50.000, 60.000])
    var max = root.maximumSupportedFpsForResolution(root.resolution)
    if (root.h264RecordFormat && root.controlBridge && root.controlBridge.h264MaxFps > 0)
        max = Math.min(max, root.controlBridge.h264MaxFps)
    if (!root.photoModeEnabled && root.imx477Detected && root.recordingFormat === "cDNG" && root.cdngCompressionValue === "none") {
        var normalizedResolution = root.normalizedResolutionForCurrentSensor(root.resolution)
        max = Math.min(max, root.imx477MaximumUncompressedFpsForResolution(normalizedResolution))
    }
    if (root.losslessImx585Mode)
        max = Math.min(max, 30.0)
    else if (root.losslessFourKMode)
        max = Math.min(max, root.isApproximatelyFourThree(root.resolution) ? 24.0 : 30.0)
    else if (root.losslessTwoKMode)
        max = Math.min(max, 60.0)
    var filtered = []
    for (var i = 0; i < presets.length; ++i) {
        if (presets[i] <= max + 0.0005)
            filtered.push(root.formatFpsValue(presets[i]))
    }
    return filtered.length > 0 ? filtered : [root.formatFpsValue(max)]
}

function isFourKCaptureResolution(resolutionValue) {
    var value = String(resolutionValue || "")
    if (root.imx477Detected || root.imx577Detected)
        return value.indexOf("4056x") === 0
    if (root.imx585Detected)
        return value.indexOf("3840x") === 0 || value === "3856x2180"
    if (root.imx294Detected) {
        var imx294Width = Number(value.split("x")[0]) || 0
        return imx294Width >= 3392
    }
    return (Number(value.split("x")[0]) || 0) >= 3840
}

function isTwoKCaptureResolution(resolutionValue) {
    var value = String(resolutionValue || "")
    return (root.imx477Detected || root.imx577Detected) && value.indexOf("2028x") === 0
}

function isApproximatelyFourThree(resolutionValue) {
    var parts = String(resolutionValue || "").split("x")
    if (parts.length !== 2)
        return false
    var width = Number(parts[0]) || 0
    var height = Number(parts[1]) || 0
    if (width <= 0 || height <= 0)
        return false
    var widthTerm = width * 3
    var heightTerm = height * 4
    return Math.abs(widthTerm - heightTerm) <= Math.max(8, Math.max(widthTerm, heightTerm) / 50)
}

function parsedResolutionSize(resolutionValue) {
    var parts = String(resolutionValue || "").split("x")
    if (parts.length !== 2)
        return { width: 0, height: 0 }
    return {
        width: Math.max(0, Math.round(Number(parts[0]) || 0)),
        height: Math.max(0, Math.round(Number(parts[1]) || 0))
    }
}

function rawFrameSizeMiB(width, height, bitDepth) {
    if (width <= 0 || height <= 0 || bitDepth <= 0)
        return 5.3
    return (width * height * bitDepth / 8.0) / (1024.0 * 1024.0)
}

function losslessCdngRatio(bitDepth) {
    if (bitDepth <= 8)
        return 0.82
    if (bitDepth <= 10)
        return 0.78
    return 0.72
}

function h264EstimatedMbps(width, height, fpsValue) {
    var pixels = width * height
    var fpsScale = Math.max(0.5, (Number(fpsValue) || 30.0) / 30.0)
    if (pixels >= 8000000)
        return 70.0 * fpsScale
    if (pixels >= 3500000)
        return 42.0 * fpsScale
    if (pixels >= 2000000)
        return 24.0 * fpsScale
    return 14.0 * fpsScale
}

function estimatedFrameSizeMB() {
    var normalized = root.normalizedResolutionForCurrentSensor(root.resolution)
    var size = root.parsedResolutionSize(normalized)
    var bits = root.currentRawBitDepthNumber()
    var rawMiB = root.rawFrameSizeMiB(size.width, size.height, bits)

    if (root.photoModeEnabled)
        return Math.max(1.0, rawMiB * 1.06)

    if (root.recordingFormat.indexOf("ProRes") === 0) {
        var nominalMbps = root.recordingFormat === "ProRes Proxy" ? 45.0
                          : root.recordingFormat === "ProRes LT" ? 102.0
                          : root.recordingFormat === "ProRes HQ" ? 220.0 : 147.0
        return nominalMbps * 1000000.0 / 8.0 / 30.0
                * size.width * size.height / (1920.0 * 1080.0) * 1.05 / (1024.0 * 1024.0)
    }

    if (root.processedVideoRecordFormat) {
        var h264Size = root.parsedResolutionSize(root.h264OutputResolutionValue)
        if (h264Size.width <= 0 || h264Size.height <= 0)
            h264Size = size
        var fpsValue = parseFloat(root.fps)
        var mbps = root.h264EstimatedMbps(h264Size.width, h264Size.height, fpsValue)
        var bytesPerSecond = mbps * 1000000.0 / 8.0
        var framesPerSecond = Math.max(1.0, isNaN(fpsValue) ? 30.0 : fpsValue)
        return Math.max(0.05, (bytesPerSecond / framesPerSecond) / (1024.0 * 1024.0))
    }

    if (root.recordingFormat === "MLV")
        return Math.max(1.0, rawMiB * 1.04 + 0.02)

    if (root.recordingFormat === "cDNG" && root.cdngCompressionValue === "lossy_3to1")
        return Math.max(0.35, rawMiB / 3.0 + 0.12)

    if (root.recordingFormat === "cDNG" && root.cdngCompressionValue === "lossy_5to1")
        return Math.max(0.25, rawMiB / 5.0 + 0.10)

    if (root.recordingFormat === "cDNG" && root.cdngCompressionValue === "lossless_jpeg")
        return Math.max(1.0, rawMiB * root.losslessCdngRatio(bits) + 0.15)

    return Math.max(1.0, rawMiB * 1.06 + 0.03)
}

function updateMediaEstimate() {
    if (typeof mediaBridge === "undefined" || !mediaBridge)
        return

    mediaBridge.frameSizeMB = root.estimatedFrameSizeMB()
}

function resolutionOptionsForCurrentSensor() {
    if (root.imx577Detected)
        return ["1332x990", "2028x1080", "2028x1520", "4056x2160", "4056x3040"]
    if (root.imx294Detected)
        return root.imx294ResolutionOptions
    return root.imx477Detected
           ? root.imx477ResolutionOptions
           : root.imx585Raw12ResolutionOptions
}

function isFourKResolution(resolutionValue) {
    return resolutionValue === "3840x2160" ||
           resolutionValue === "3856x2180" ||
           resolutionValue === "3840x2076" ||
           resolutionValue === "3840x2024" ||
           resolutionValue === "3840x1920" ||
           resolutionValue === "3840x1744" ||
           resolutionValue === "3840x1728" ||
           resolutionValue === "3840x1632" ||
           resolutionValue === "3840x1608" ||
           resolutionValue === "3840x1536" ||
           resolutionValue === "3840x1504"
}

function isImx477FourKResolution(resolutionValue) {
    var value = String(resolutionValue || "")
    return value === "4056x3040" ||
           value === "4056x2280" ||
           value === "4056x2192" ||
           value === "4056x2160" ||
           value === "4056x2136" ||
           value === "4056x2028" ||
           value === "4056x1844" ||
           value === "4056x1828" ||
           value === "4056x1728" ||
           value === "4056x1696" ||
           value === "4056x1624" ||
           value === "4056x1592"
}

function isImx477TwoKResolution(resolutionValue) {
    var value = String(resolutionValue || "")
    return value === "2028x1520" ||
           value === "2028x1140" ||
           value === "2028x1096" ||
           value === "2028x1080" ||
           value === "2028x1068" ||
           value === "2028x1014" ||
           value === "2028x922" ||
           value === "2028x914" ||
           value === "2028x864" ||
           value === "2028x848" ||
           value === "2028x812" ||
           value === "2028x796"
}

function isImx585FullHdResolution(resolutionValue) {
    return resolutionValue === "1920x1080" ||
           resolutionValue === "1928x1090" ||
           resolutionValue === "1920x1038" ||
           resolutionValue === "1920x1012" ||
           resolutionValue === "1920x960" ||
           resolutionValue === "1920x872" ||
           resolutionValue === "1920x864" ||
           resolutionValue === "1920x816" ||
           resolutionValue === "1920x804" ||
           resolutionValue === "1920x768" ||
           resolutionValue === "1920x752"
}

function cropRatioLabelForResolution(resolutionValue) {
    var imx294Label = root.imx294CropRatioLabel(resolutionValue)
    if (imx294Label.length > 0)
        return imx294Label
    if (resolutionValue === "4056x3040" || resolutionValue === "2028x1520" ||
            resolutionValue === "1332x990")
        return "4:3"
    if (resolutionValue === "4056x2280" || resolutionValue === "2028x1140")
        return "16:9"
    if (resolutionValue === "3840x2076" || resolutionValue === "2560x1384" ||
            resolutionValue === "1920x1038" ||
            resolutionValue === "4056x2192" || resolutionValue === "2028x1096")
        return "1.85:1"
    if (resolutionValue === "3840x2024" || resolutionValue === "2560x1348" ||
            resolutionValue === "1920x1012" ||
            resolutionValue === "4056x2136" || resolutionValue === "2028x1068")
        return "1.90:1"
    if (resolutionValue === "3840x1920" || resolutionValue === "2560x1280" ||
            resolutionValue === "1920x960" ||
            resolutionValue === "4056x2028" || resolutionValue === "2028x1014")
        return "2.00:1"
    if (resolutionValue === "3840x1744" || resolutionValue === "2560x1164" ||
            resolutionValue === "1920x872" ||
            resolutionValue === "4056x1844" || resolutionValue === "2028x922")
        return "2.20:1"
    if (resolutionValue === "3840x1728" || resolutionValue === "2560x1152" ||
            resolutionValue === "1920x864" ||
            resolutionValue === "4056x1828" || resolutionValue === "2028x914")
        return "2.22:1"
    if (resolutionValue === "3840x1632" || resolutionValue === "2560x1088" ||
            resolutionValue === "1920x816" ||
            resolutionValue === "4056x1728" || resolutionValue === "2028x864")
        return "2.35:1"
    if (resolutionValue === "3840x1608" || resolutionValue === "2560x1072" ||
            resolutionValue === "1920x804" ||
            resolutionValue === "4056x1696" || resolutionValue === "2028x848")
        return "2.39:1"
    if (resolutionValue === "3840x1536" || resolutionValue === "2560x1024" ||
            resolutionValue === "1920x768" ||
            resolutionValue === "4056x1624" || resolutionValue === "2028x812")
        return "2.50:1"
    if (resolutionValue === "3840x1504" || resolutionValue === "2560x1004" ||
            resolutionValue === "1920x752" ||
            resolutionValue === "4056x1592" || resolutionValue === "2028x796")
        return "2.55:1"
    if (resolutionValue === "4056x2160" || resolutionValue === "2028x1080")
        return "17:9"
    return "16:9"
}

function imx294CropRatioLabel(resolutionValue) {
    var labels = {
        "4144x2176": "17:9", "4176x2176": "17:9", "2048x1080": "17:9",
        "1920x1080": "16:9", "3872x2176": "16:9",
        "3792x2824": "4:3", "3792x2528": "3:2", "3392x2824": "6:5",
        "4096x2048": "2.00:1", "4096x1844": "2.22:1",
        "4096x1744": "2.35:1", "4096x1716": "2.39:1", "4096x1640": "2.50:1"
    }
    return labels[resolutionValue] || ""
}

function sensorBaseResolutionForValue(resolutionValue) {
    if (root.imx294Detected)
        return root.imx294ResolutionOptions[0]
    if (root.imx477Detected)
        return root.isImx477FourKResolution(resolutionValue) ? "4056x2160" : "2028x1080"
    if (root.imx585Detected)
        return root.isFourKResolution(resolutionValue) ? "3840x2160" : "1920x1080"
    return resolutionValue
}

function sensorBaseResolutionLabel(baseValue) {
    if (baseValue === "1920x1080")
        return "Full HD"
    if (baseValue === "2028x1080" || baseValue === "2048x1080")
        return "2K"
    if (baseValue === "3840x2160" || baseValue === "4056x2160"
            || baseValue === "4144x2176" || baseValue === "4176x2176")
        return "4K"
    return baseValue
}

function sensorCropOptionsForBase(baseValue) {
    if (root.imx294Detected) {
        var options = []
        for (var i = 0; i < root.imx294ResolutionOptions.length; ++i) {
            var value = root.imx294ResolutionOptions[i]
            if (root.sensorBaseResolutionForValue(value) === baseValue)
                options.push({ value: value, label: root.imx294CropRatioLabel(value) })
        }
        var order = ["4:3", "3:2", "6:5", "16:9", "17:9", "2.00:1", "2.22:1", "2.35:1", "2.39:1", "2.50:1"]
        options.sort(function(a, b) { return order.indexOf(a.label) - order.indexOf(b.label) })
        return options
    }
    if (root.imx477Detected)
        return baseValue === "4056x2160" ? root.imx477FourKCropRatioOptions : root.imx477TwoKCropRatioOptions
    if (root.imx585Detected)
        return baseValue === "3840x2160" ? root.imx585CropRatioOptions : root.imx585FullHdCropRatioOptions
    return []
}

function sensorResolutionForBaseAndRatio(baseValue, ratioLabel) {
    var options = root.safeOptionList(root.sensorCropOptionsForBase(baseValue))
    for (var i = 0; i < options.length; ++i) {
        if (options[i].label === ratioLabel || options[i].value === ratioLabel)
            return options[i].value
    }
    return baseValue
}

function safeOptionList(options) {
    if (!options || options.length === undefined)
        return []

    var safe = []
    for (var i = 0; i < options.length; ++i) {
        if (options[i] !== undefined && options[i] !== null)
            safe.push(options[i])
    }
    return safe
}

function aspectRatioForResolution(resolutionValue) {
    var rawParts = String(resolutionValue || "").split("x")
    if (rawParts.length === 2) {
        var rawWidth = parseFloat(rawParts[0])
        var rawHeight = parseFloat(rawParts[1])
        if (rawWidth > 0 && rawHeight > 0)
            return rawWidth / rawHeight
    }

    var normalized = root.normalizedResolutionForCurrentSensor(resolutionValue)
    var parts = String(normalized).split("x")
    if (parts.length === 2) {
        var width = parseFloat(parts[0])
        var height = parseFloat(parts[1])
        if (width > 0 && height > 0)
            return width / height
    }
    return 16.0 / 9.0
}

function h264BaseResolutionForValue(resolutionValue) {
    var value = String(resolutionValue || "")
    if (value.indexOf("1920x") === 0 || value.indexOf("2028x") === 0)
        return "1920x1080"
    if (value.indexOf("2560x") === 0)
        return "2560x1440"
    return "2560x1440"
}

function h264CropOptionsForBase(baseValue) {
    if (baseValue === "1920x1080") {
        return [
            { value: "1920x1080", label: "16:9" },
            { value: "1920x1038", label: "1.85:1" },
            { value: "1920x1012", label: "1.90:1" },
            { value: "1920x960", label: "2.00:1" },
            { value: "1920x872", label: "2.20:1" },
            { value: "1920x864", label: "2.22:1" },
            { value: "1920x816", label: "2.35:1" },
            { value: "1920x804", label: "2.39:1" },
            { value: "1920x768", label: "2.50:1" },
            { value: "1920x752", label: "2.55:1" }
        ]
    }
    if (baseValue === "2560x1440") {
        return [
            { value: "2560x1440", label: "16:9" },
            { value: "2560x1384", label: "1.85:1" },
            { value: "2560x1348", label: "1.90:1" },
            { value: "2560x1280", label: "2.00:1" },
            { value: "2560x1164", label: "2.20:1" },
            { value: "2560x1152", label: "2.22:1" },
            { value: "2560x1088", label: "2.35:1" },
            { value: "2560x1072", label: "2.39:1" },
            { value: "2560x1024", label: "2.50:1" },
            { value: "2560x1004", label: "2.55:1" }
        ]
    }
    return root.h264CropOptionsForBase("2560x1440")
}

function h264ResolutionForBaseAndRatio(baseValue, ratioLabel) {
    var options = root.h264CropOptionsForBase(baseValue)
    for (var i = 0; i < options.length; ++i) {
        if (options[i].label === ratioLabel || options[i].value === ratioLabel)
            return options[i].value
    }
    return baseValue
}

function applyResolutionValue(resolutionValue) {
    var normalized = root.normalizedResolutionForCurrentSensor(resolutionValue)
    if (root.controlBridge) {
        if (root.controlBridge.applyResolution(normalized))
            root.resolution = normalized
    } else {
        root.resolution = normalized
    }
}

function applyResolutionBase(resolutionValue) {
    if (resolutionValue === undefined || resolutionValue === null || String(resolutionValue).length === 0)
        return

    if (root.processedVideoRecordFormat) {
        if (root.controlBridge)
            root.controlBridge.applyH264Resolution(root.h264ResolutionForBaseAndRatio(resolutionValue, root.h264CropRatioLabel))
        return
    }
    if (root.sensorCropControlsAvailable) {
        var currentRatio = root.cropRatioLabelForResolution(root.resolution)
        root.applyResolutionValue(root.sensorResolutionForBaseAndRatio(resolutionValue, currentRatio))
        return
    }
    root.applyResolutionValue(resolutionValue)
}

function applyCropResolution(resolutionValue) {
    if (resolutionValue === undefined || resolutionValue === null || String(resolutionValue).length === 0)
        return

    if (root.processedVideoRecordFormat) {
        if (root.controlBridge)
            root.controlBridge.applyH264Resolution(resolutionValue)
        return
    }
    if (!root.sensorCropControlsAvailable)
        return
    root.applyResolutionValue(resolutionValue)
}

function optionValue(option) {
    if (option === undefined || option === null)
        return ""
    if (option && typeof option === "object" && option.value !== undefined)
        return String(option.value)
    return String(option)
}

function optionLabel(option) {
    if (option === undefined || option === null)
        return ""
    if (option && typeof option === "object") {
        if (option.label !== undefined)
            return String(option.label)
        if (option.text !== undefined)
            return String(option.text)
        if (option.value !== undefined)
            return String(option.value)
    }
    return String(option)
}

function normalizedResolutionForCurrentSensor(resolutionValue) {
    var currentOptions = root.resolutionOptionsForCurrentSensor()

    if (root.imx577Detected) {
        if (resolutionValue === "1928x1090" || resolutionValue === "1920x1080")
            return "2028x1080"
        if (resolutionValue === "3856x2180" || resolutionValue === "3840x2160")
            return "4056x2160"
        if (currentOptions.indexOf(resolutionValue) !== -1)
            return resolutionValue
        return "2028x1080"
    }

    if (root.imx477Detected) {
        if (currentOptions.indexOf(resolutionValue) !== -1)
            return resolutionValue
        if (resolutionValue === "1928x1090" || resolutionValue === "1920x1080")
            return "2028x1080"
        if (resolutionValue === "3856x2180" || resolutionValue === "3840x2160")
            return "4056x2160"
        if (resolutionValue === "4056x3040")
            return "4056x3040"
        return "2028x1080"
    }

    if (root.imx294Detected) {
        if (currentOptions.indexOf(resolutionValue) !== -1)
            return resolutionValue
        if (resolutionValue === "4056x3040")
            return "3792x2824"
        if (resolutionValue === "2028x1080")
            return currentOptions[0]
        if (resolutionValue === "1920x1080")
            return "3872x2176"
        if (resolutionValue === "1928x1090" ||
                resolutionValue === "1920x1080" ||
                resolutionValue === "2028x1520" ||
                resolutionValue === "3856x2180" ||
                resolutionValue === "3840x2160" ||
                resolutionValue === "4056x2160")
            return currentOptions.length > 0 ? currentOptions[0] : "4144x2176"
        return currentOptions.length > 0 ? currentOptions[0] : "4144x2176"
    }

    if (root.imx585Detected) {
        if (resolutionValue === "1332x990" ||
                resolutionValue === "2028x1080" ||
                resolutionValue === "2028x1520")
            return "1920x1080"
        if (resolutionValue === "1928x1090" || resolutionValue === "1920x1080")
            return "1920x1080"
        if (resolutionValue === "1920x1038" ||
                resolutionValue === "1920x1012" ||
                resolutionValue === "1920x960" ||
                resolutionValue === "1920x872" ||
                resolutionValue === "1920x864" ||
                resolutionValue === "1920x816" ||
                resolutionValue === "1920x804" ||
                resolutionValue === "1920x768" ||
                resolutionValue === "1920x752")
            return resolutionValue
        if (resolutionValue === "3856x2180" || resolutionValue === "3840x2160")
            return "3840x2160"
        if (resolutionValue === "3840x2076" ||
                resolutionValue === "3840x2024" ||
                resolutionValue === "3840x1920" ||
                resolutionValue === "3840x1744" ||
                resolutionValue === "3840x1728" ||
                resolutionValue === "3840x1632" ||
                resolutionValue === "3840x1608" ||
                resolutionValue === "3840x1536" ||
                resolutionValue === "3840x1504")
            return resolutionValue
        if (resolutionValue === "4056x2160" ||
                resolutionValue === "4056x3040")
            return "3840x2160"
        if (currentOptions.indexOf(resolutionValue) !== -1)
            return resolutionValue
        return "1920x1080"
    }

    return resolutionValue
}

function ensureValidResolutionSelection() {
    // Core validates its complete settings, not partially delivered UI fields.
    if (root.controlBridge)
        return
    var normalized = root.normalizedResolutionForCurrentSensor(root.resolution)
    if (normalized === root.resolution)
        return

    if (!root.controlResolutionSynced) {
        root.resolution = normalized
    } else if (root.controlBridge) {
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
    // Core queues the complete exposure bracket in one transaction.
    if (root.settingsState && root.settingsState.photoBracketingEnabled) return 1
    if (root.photoBurstSetting === "3 Shots")
        return 3
    if (root.photoBurstSetting === "5 Shots")
        return 5
    if (root.photoBurstSetting === "10 Shots")
        return 10
    return 1
}

function cyclePhotoBurst() {
    if (!root.settingsState || !root.canSelectCaptureMode || !root.photoModeEnabled || root.photoTimelapseEnabled || root.settingsState.photoBracketingEnabled)
        return
    var options = ["Single", "3 Shots", "5 Shots", "10 Shots"]
    var index = options.indexOf(root.photoBurstSetting)
    root.settingsState.photoBurst = options[(index + 1) % options.length]
}

function applyQuickTimelapseInterval(value) {
    if (!root.settingsState || !root.canSelectCaptureMode || !root.photoTimelapseEnabled)
        return
    var format = ["DNG", "JPEG", "PNG", "TIFF"].indexOf(root.photoFormatSetting) !== -1 ? root.photoFormatSetting : "DNG"
    if (root.controlBridge && !root.controlBridge.applyPhotoTimelapse(value, format)) {
        root.showRecordWarning("Interval Change Failed", root.controlBridge.lastError,
                               "Timelapse interval was not changed", "Stop capture and try again.")
        return
    }
    root.settingsState.photoFormat = format
    root.settingsState.photoTimelapse = value
}

function cancelPhotoSequence() {
    root.photoCaptureInProgress = false
    root.pendingPhotoShots = 0
    root.photoCountdownSecondsRemaining = 0
    photoDelayTimer.stop()
    photoCountdownTimer.stop()
}

function triggerPhotoCaptureFeedback() {
    root.photoCapturePulse = false
    photoCapturePulseTimer.stop()
    root.photoCapturePulse = true
    photoCapturePulseTimer.start()
    photoCaptureFeedbackAnimation.restart()
}

function queueNextPhotoInSequence() {
    if (root.pendingPhotoShots <= 0) {
        root.photoCaptureInProgress = false
        return
    }

    var captureQueued = true
    if (root.controlBridge)
        captureQueued = root.controlBridge.capturePhoto(root.photoFormatSetting, root.pendingPhotoShots)
    if (!captureQueued) {
        root.cancelPhotoSequence()
        return
    }

    if (!root.controlBridge)
        root.triggerPhotoCaptureFeedback()

    root.pendingPhotoShots = 0
    root.photoCaptureInProgress = false
}

function beginPhotoCaptureSequence() {
    if (root.photoCaptureInProgress || root.mediaWritesPending)
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

function positionCaptureModeMenu() {
    var point = recordBtn.mapToItem(dynamicOverlay, recordBtn.width / 2, 0)
    captureModeMenu.x = Math.max(12, Math.min(root.width - captureModeMenu.width - 12, point.x - captureModeMenu.width / 2))
    captureModeMenu.y = Math.max(12, point.y - captureModeMenu.height - 12)
}

function openCaptureModeMenu() {
    if (!root.canSelectCaptureMode) return
    root.openDropdown = ""
    root.quickMenuOpen = false
    root.captureModeMenuOpen = true
    root.positionCaptureModeMenu()
}

function selectCaptureMode(mode) {
    root.captureModeMenuOpen = false
    if (!root.canSelectCaptureMode || !root.settingsState) return
    if (root.controlBridge && !root.controlBridge.applyCaptureMode(mode)) {
        root.showRecordWarning("Mode Change Failed", root.controlBridge.lastError,
                               "Capture mode was not changed", "Stop capture and try again.")
        return
    }
    root.settingsState.soundRecorderEnabled = false
    if (!root.controlBridge) {
        root.settingsState.photoTimelapse = mode === "timelapse" ? "5s" : "Off"
        root.settingsState.photoModeEnabled = mode !== "clip"
    }
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

function toggleHardwarePreviewZoom() {
    if (root.previewZoom > 1.01) {
        root.resetPreviewTransform()
    } else {
        root.previewZoom = Math.max(root.minPreviewZoom,
                                    Math.min(root.maxPreviewZoom, root.hardwarePreviewZoom))
        root.previewPanX = 0.0
        root.previewPanY = 0.0
    }
}

function ensureValidFpsSelection() {
    if (root.controlBridge)
        return
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

    if (root.controlBridge.resolution && root.controlBridge.resolution.length > 0) {
        root.controlResolutionSynced = true
        root.resolution = root.controlBridge.resolution
    }

    root.recording = root.controlBridge.recording
    root.timelapseActive = root.controlBridge.timelapseActive
}

function requestRecordingState(recordingState) {
    if (root.controlBridge) {
        if (recordingState && typeof audioMeterBridge !== "undefined" && audioMeterBridge)
            audioMeterBridge.suspendMonitoring()

        const success = root.controlBridge.setRecording(recordingState)
        if (!success) {
            if (recordingState && typeof audioMeterBridge !== "undefined" && audioMeterBridge)
                audioMeterBridge.resumeMonitoring()

            var fallbackError = recordingState
                                ? "ApertarCore did not start recording."
                                : "ApertarCore did not stop recording."
            var errorText = (root.controlBridge.lastError && root.controlBridge.lastError.length > 0)
                            ? root.controlBridge.lastError
                            : fallbackError
            var installHint = errorText.indexOf("ffmpeg") >= 0
                              ? "Install ffmpeg, then try H.264 again."
                              : "Check media and audio device settings, then try again."
            root.showRecordWarning(recordingState ? "Recording Failed" : "Stop Failed",
                                   errorText,
                                   recordingState ? "The camera is still in standby"
                                                  : "Recording is still active",
                                   recordingState
                                       ? installHint
                                       : "Please try stopping again in a moment.")
        }
    } else {
        root.recording = recordingState
    }
}

function requestTimelapseState(activeState) {
    if (activeState && !root.photoModeEnabled) {
        root.showRecordWarning("Timelapse Unavailable",
                               "Turn on Still Mode before starting a timelapse.",
                               "Still Mode is off",
                               "Timelapse capture is part of Still Mode.")
        return
    }

    if (activeState && !root.photoTimelapseEnabled) {
        root.showRecordWarning("Timelapse Unavailable",
                               "Enable a timelapse interval in Still Mode settings first.",
                               "No interval selected",
                               "Choose an interval, then press the shutter button again.")
        return
    }

    if (root.controlBridge) {
        const success = root.controlBridge.setTimelapseActive(activeState,
                                                             root.photoTimelapseSetting,
                                                             root.photoFormatSetting)
        if (!success) {
            var fallbackError = activeState
                                ? "ApertarCore did not start timelapse."
                                : "ApertarCore did not stop timelapse."
            var errorText = (root.controlBridge.lastError && root.controlBridge.lastError.length > 0)
                            ? root.controlBridge.lastError
                            : fallbackError
            root.showRecordWarning(activeState ? "Timelapse Failed" : "Stop Failed",
                                   errorText,
                                   activeState ? "The timelapse did not start"
                                               : "Timelapse is still active",
                                   activeState
                                       ? "Check media, still format, and interval settings, then try again."
                                       : "Please try stopping again in a moment.")
        }
    } else {
        root.timelapseActive = activeState
    }
}

function showRecordWarning(title, subtitle, detailTitle, detailBody) {
    root.recordWarningTitle = title
    root.recordWarningSubtitle = subtitle
    root.recordWarningDetailTitle = detailTitle
    root.recordWarningDetailBody = detailBody
    root.recordWarningOpen = true
}

function showNoMediaWarning(forPhoto) {
    root.showRecordWarning("No Media Mounted",
                           forPhoto
                               ? ("Insert " + mediaBridge.mediaPromptLabel + " to capture a still.")
                               : ("Insert " + mediaBridge.mediaPromptLabel + " to start recording."),
                           forPhoto
                               ? "Still capture is unavailable"
                               : "Recording is unavailable",
                           forPhoto
                               ? "The camera will stay in still mode until media is detected."
                               : "The camera will stay in standby until media is detected.")
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

function toggleTimecodeMode() {
    var nextMode = root.timecodeMode === "Free Run" ? "Rec Run" : "Free Run"
    if (root.settingsState)
        root.settingsState.timecodeMode = nextMode
    if (typeof apertarControlBridge !== "undefined" && apertarControlBridge)
        apertarControlBridge.applyTimecodeMode(nextMode)
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
    interval: 1000
    running: root.dateTimeOverlayEnabled && (!root.controlBridge || !root.controlBridge.standby)
    repeat: true
    triggeredOnStart: true
    onTriggered: root.currentDateTime = new Date()
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
    if (!root.photoModeEnabled) {
        root.cancelPhotoSequence()
        if (root.timelapseActive)
            root.requestTimelapseState(false)
    }
    root.updateMediaEstimate()
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
onImx294DetectedChanged: {
    root.ensureValidResolutionSelection()
    root.ensureValidFpsSelection()
    root.updateMediaEstimate()
}
onImx577DetectedChanged: {
    root.ensureValidResolutionSelection()
    root.ensureValidFpsSelection()
    root.updateMediaEstimate()
}
onImx477DetectedChanged: {
    root.ensureValidResolutionSelection()
    root.ensureValidFpsSelection()
    root.updateMediaEstimate()
}
onRawBitDepthValueChanged: {
    root.ensureValidResolutionSelection()
    root.ensureValidFpsSelection()
    root.updateMediaEstimate()
}
onRecordingFormatChanged: {
    root.ensureValidFpsSelection()
    root.updateMediaEstimate()
}
onCdngCompressionValueChanged: {
    root.ensureValidFpsSelection()
    root.updateMediaEstimate()
}
onH264ResolutionValueChanged: root.updateMediaEstimate()
onLosslessFourKModeChanged: {
    root.ensureValidFpsSelection()
    root.updateMediaEstimate()
}
onLosslessTwoKModeChanged: {
    root.ensureValidFpsSelection()
    root.updateMediaEstimate()
}
onLosslessImx585ModeChanged: {
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

    function onRecordingFailed(error) {
        root.showRecordWarning("Recording Stopped", "The clip could not be written completely.",
                               "Recording Error", error)
    }

    function onPhotoCaptureFailed(error) {
        root.cancelPhotoSequence()
        root.showRecordWarning("Still Not Saved", "The still image could not be saved completely.",
                               "Storage Error", error)
    }

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
        if (root.controlBridge && root.controlBridge.resolution.length > 0) {
            root.controlResolutionSynced = true
            root.resolution = root.controlBridge.resolution
        }
    }

    function onRecordingChanged() {
        var wasRecording = root.recording
        root.recording = root.controlBridge ? root.controlBridge.recording : false
        if (root.timecodeMode === "Rec Run" && !wasRecording && root.recording)
            root.timecodeFrames = 0
        root.updateTimecodeDisplay()
    }

    function onTimelapseActiveChanged() {
        root.timelapseActive = root.controlBridge ? root.controlBridge.timelapseActive : false
    }

    function onStandbyChanged() {
        if (root.controlBridge && root.controlBridge.standby)
            root.cancelPhotoSequence()
        root.updateHorizontalLevelPolling()
    }

    function onPhotoCaptured() {
        root.triggerPhotoCaptureFeedback()
    }

    function onTimelapseFrameCaptured() {
        if (root.timelapseActive)
            root.triggerPhotoCaptureFeedback()
    }
}

Connections {
    target: root.bridge

    function onFrameArrived() {
        if (root.timecodeMode === "Free Run") {
            root.updateTimecodeDisplay()
        } else if (root.recording) {
            root.timecodeFrames += 1
            root.updateTimecodeDisplay()
        }
    }
}

Connections {
    target: root.settingsState

    function onHorizontalLevelEnabledChanged() {
        root.updateHorizontalLevelPolling()
    }

    function onSmpteEnabledChanged() {
        root.updateHorizontalLevelPolling()
    }
}

Component.onDestruction: {
    if (typeof gyroConfigBridge !== "undefined" && gyroConfigBridge)
        gyroConfigBridge.setLivePollingRequest("camera-horizontal-level", false, true, 80)
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


    // =========================
    // PREVIEW AREA
    // =========================
	
    Item {
        id: previewContainer
        objectName: "previewContainer"
        anchors.horizontalCenter: parent.horizontalCenter
        y: root.previewTopOffset
        width: parent.width
        height: root.previewVisibleHeight

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
            smpteEnabled: root.smpteEnabled
            anamorphicDesqueezeEnabled: root.anamorphicDesqueezeEnabled
            anamorphicDesqueezeRatio: root.anamorphicDesqueezeRatio
            falseColorEnabled: root.settingsState ? root.settingsState.falseColorEnabled : false
            falseColorMode: root.settingsState ? root.settingsState.falseColorMode : 0
            lutPreviewEnabled: root.settingsState ? root.settingsState.lutPreviewEnabled : false
            lutPreviewFile: root.settingsState ? root.settingsState.lutPreviewFile : ""
            exposureReference: root.controlBridge ? root.controlBridge.previewExposureReference : ({})
            displayRotation: root.uiPreviewRotation - root.sceneRotationDegrees
            previewAspectRatio: root.previewImageAspectRatio
        }

        StillExposureOverlay {
            anchors.fill: parent
            z: 29
            controlBridge: root.controlBridge
            fontFamily: interBold.font.family
        }

        Rectangle {
            id: photoFlashOverlay
            anchors.fill: parent
            color: "white"
            opacity: 0.0
            visible: opacity > 0.01
            z: 27
        }

        Rectangle {
            id: photoCaptureFramePulse
            anchors.fill: parent
            anchors.margins: 10
            radius: 28
            color: "transparent"
            border.width: 2
            border.color: "white"
            opacity: 0.0
            scale: 0.992
            visible: opacity > 0.01
            z: 28
        }

        SequentialAnimation {
            id: photoCaptureFeedbackAnimation

            ScriptAction {
                script: {
                    photoFlashOverlay.opacity = 0.0
                    photoCaptureFramePulse.opacity = 0.0
                    photoCaptureFramePulse.scale = 0.992
                }
            }

            ParallelAnimation {
                NumberAnimation {
                    target: photoFlashOverlay
                    property: "opacity"
                    from: 0.58
                    to: 0.0
                    duration: 320
                    easing.type: Easing.OutCubic
                }

                SequentialAnimation {
                    NumberAnimation {
                        target: photoCaptureFramePulse
                        property: "opacity"
                        from: 0.88
                        to: 0.48
                        duration: 110
                        easing.type: Easing.OutCubic
                    }
                    NumberAnimation {
                        target: photoCaptureFramePulse
                        property: "opacity"
                        to: 0.0
                        duration: 360
                        easing.type: Easing.OutCubic
                    }
                }

                NumberAnimation {
                    target: photoCaptureFramePulse
                    property: "scale"
                    from: 0.992
                    to: 1.018
                    duration: 460
                    easing.type: Easing.OutCubic
                }
            }
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
            property bool blockedForQuickMenuSwipe: false
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

            function shouldReserveForQuickMenuSwipe(point) {
                return !!point
                        && quickMenuEdgeSwipeZone.visible
                        && point.x >= (previewTouchArea.width - root.quickMenuSwipeEdgeWidth)
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
                    blockedForQuickMenuSwipe = false
                    beginPinchGesture()
                } else if (count === 1) {
                    var point = activePrimaryPoint()
                    blockedForQuickMenuSwipe = shouldReserveForQuickMenuSwipe(point)
                    if (blockedForQuickMenuSwipe) {
                        gestureMode = 0
                        singleMoved = true
                        return
                    }
                    beginSingleTouchGesture(false)
                }
            }

            onUpdated: {
                if (blockedForQuickMenuSwipe)
                    return

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
                if (blockedForQuickMenuSwipe) {
                    if (activePointCount() === 0)
                        blockedForQuickMenuSwipe = false
                    gestureMode = 0
                    return
                }

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
                blockedForQuickMenuSwipe = false
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
            visible: !root.smpteEnabled

            readonly property real activeAspectRatio: root.previewDisplayAspectRatio
            readonly property real activeImageWidth: (parent.width / Math.max(1, parent.height)) > activeAspectRatio
                                                     ? parent.height * activeAspectRatio
                                                     : parent.width
            readonly property real activeImageHeight: (parent.width / Math.max(1, parent.height)) > activeAspectRatio
                                                      ? parent.height
                                                      : parent.width / activeAspectRatio
            readonly property real activeImageX: Math.round((parent.width - activeImageWidth) / 2)
            readonly property real activeImageY: Math.round((parent.height - activeImageHeight) / 2)
            readonly property real guideInset: Math.min(24, Math.max(8, activeImageHeight * 0.06))
            readonly property real guideX: activeImageX + guideInset
            readonly property real guideY: activeImageY + guideInset
            readonly property real guideWidth: activeImageWidth - (guideInset * 2)
            readonly property real guideHeight: activeImageHeight - (guideInset * 2)
            readonly property bool guidesEnabled: root.settingsState ? root.settingsState.guidesEnabled : true
            readonly property string guideType: root.settingsState ? root.settingsState.guidesType : "Thirds"
            readonly property int guidesThickness: root.settingsState ? Math.max(1, root.settingsState.guidesThickness) : 1
            readonly property bool centerMarkerEnabled: root.settingsState ? root.settingsState.centerMarkerEnabled : true
            readonly property string centerMarkerType: root.settingsState ? root.settingsState.centerMarkerType : "Circle/Dot"
            readonly property int centerMarkerCircleSize: root.standardLandscapeLayout ? 72 : 56
            readonly property int centerMarkerDotSize: root.standardLandscapeLayout ? 10 : 8
            readonly property int centerMarkerCrosshairSize: root.standardLandscapeLayout ? 62 : 48
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
                x: guideOverlay.guideX
                y: guideOverlay.guideY
                width: guideOverlay.guideWidth
                height: guideOverlay.guideHeight
                radius: 22
                color: "transparent"
                border.width: guideOverlay.guidesThickness
                border.color: "#4dffffff"
                antialiasing: false
            }

            Rectangle {
                visible: guideOverlay.showThirds
                width: guideOverlay.guidesThickness
                color: "#4dffffff"
                x: Math.round(guideOverlay.guideX + (guideOverlay.guideWidth / 3))
                y: guideOverlay.guideY
                height: guideOverlay.guideHeight
                antialiasing: false
            }

            Rectangle {
                visible: guideOverlay.showThirds
                width: guideOverlay.guidesThickness
                color: "#4dffffff"
                x: Math.round(guideOverlay.guideX + ((guideOverlay.guideWidth * 2) / 3))
                y: guideOverlay.guideY
                height: guideOverlay.guideHeight
                antialiasing: false
            }

            Rectangle {
                visible: guideOverlay.showThirds
                height: guideOverlay.guidesThickness
                color: "#4dffffff"
                x: guideOverlay.guideX
                y: Math.round(guideOverlay.guideY + (guideOverlay.guideHeight / 3))
                width: guideOverlay.guideWidth
                antialiasing: false
            }

            Rectangle {
                visible: guideOverlay.showThirds
                height: guideOverlay.guidesThickness
                color: "#4dffffff"
                x: guideOverlay.guideX
                y: Math.round(guideOverlay.guideY + ((guideOverlay.guideHeight * 2) / 3))
                width: guideOverlay.guideWidth
                antialiasing: false
            }

            Rectangle {
                visible: guideOverlay.showAspectFrame
                width: guideOverlay.aspectGuideWidth
                height: guideOverlay.aspectGuideHeight
                x: Math.round(guideOverlay.guideX + ((guideOverlay.guideWidth - width) / 2))
                y: Math.round(guideOverlay.guideY + ((guideOverlay.guideHeight - height) / 2))
                color: "transparent"
                border.width: guideOverlay.guidesThickness
                border.color: "#4dffffff"
                antialiasing: false
            }

            Rectangle {
                visible: guideOverlay.centerMarkerEnabled && guideOverlay.centerMarkerType === "Circle/Dot"
                width: guideOverlay.centerMarkerCircleSize
                height: guideOverlay.centerMarkerCircleSize
                radius: width / 2
                color: "transparent"
                border.width: 1
                border.color: "#4dffffff"
                anchors.centerIn: parent
                antialiasing: false
            }

            Rectangle {
                visible: guideOverlay.centerMarkerEnabled && guideOverlay.centerMarkerType === "Circle/Dot"
                width: guideOverlay.centerMarkerDotSize
                height: guideOverlay.centerMarkerDotSize
                radius: width / 2
                color: "white"
                anchors.centerIn: parent
                antialiasing: false
            }

            Rectangle {
                visible: guideOverlay.centerMarkerEnabled && guideOverlay.centerMarkerType === "Crosshair"
                width: 1
                height: guideOverlay.centerMarkerCrosshairSize
                color: "#4dffffff"
                anchors.centerIn: parent
                antialiasing: false
            }

            Rectangle {
                visible: guideOverlay.centerMarkerEnabled && guideOverlay.centerMarkerType === "Crosshair"
                width: guideOverlay.centerMarkerCrosshairSize
                height: 1
                color: "#4dffffff"
                anchors.centerIn: parent
                antialiasing: false
            }
        }

        HorizontalLevelOverlay {
            id: horizontalLevelOverlay
            z: 15
            anchors.fill: parent
            levelEnabled: root.settingsState ? root.settingsState.horizontalLevelEnabled : false
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
            levelColor: root.settingsState ? root.settingsState.horizontalLevelColor : "Blue"
            large: root.largeLandscapeLayout
            compact: root.compactLandscapeLayout || root.compactPortraitLayout
        }

        FalseColorLegend {
            id: falseColorLegend
            z: 16
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: root.largeLandscapeLayout ? 20 : 12
            transformOrigin: Item.Left
            mode: root.intToFalseColorMode(root.settingsState ? root.settingsState.falseColorMode : 0)
            backgroundColor: root.rectangularLayout
                             ? root.compactControlColor("#70000000", root.compactControlFill, root.compactDarkControlFill)
                             : "#9c000000"
            borderColor: root.rectangularLayout
                         ? root.compactControlColor("#0affffff", root.compactControlBorder, root.compactDarkControlBorder)
                         : "#16ffffff"
            opacity: root.falseColorLegendVisible ? 1.0 : 0.0
            scale: root.falseColorLegendDisplayScale * (root.falseColorLegendVisible ? 1.0 : 0.96)
        }

    // =========================
    // STATUS CHIP
	
    StatusChip {
        id: captureStatusChip
        writing: root.mediaWritesPending
        z: 20
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 16
        anchors.topMargin: root.rectangularLayout ? (root.topControlsTopMargin + root.topStatHeight + 16) : 16
        recording: root.recording
        timelapse: root.timelapseActive
        mediaMounted: mediaBridge.mediaMounted
        backgroundOpacity: root.effectiveControlsOpacity
        compact: root.compactLandscapeLayout
        largeCompact: root.regularLandscapeLayout
        extraLargeCompact: root.standardLandscapeLayout
        oversizedCompact: root.largeLandscapeLayout
        darkBackground: root.compactDarkControls
        useControlBackground: root.compactLandscapeLayout
        }

        DroppedFrameWarning {
            z: 20
            anchors.left: captureStatusChip.right
            anchors.leftMargin: Math.max(7, Math.round(captureStatusChip.height * 0.25))
            anchors.verticalCenter: captureStatusChip.verticalCenter
            height: captureStatusChip.height
            visible: root.controlBridge ? root.controlBridge.recordingHasDroppedFrames : false
            opacity: root.effectiveControlsOpacity
        }

        Column {
            // Interactive still controls must sit above previewTouchArea (z: 25).
            z: 26
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.rightMargin: 16
            anchors.topMargin: root.rectangularLayout ? (root.topControlsTopMargin + root.topStatHeight + 16) : 16
            spacing: 8
            visible: root.photoModeEnabled

            Rectangle {
                visible: root.photoModeEnabled && !root.photoTimelapseEnabled && root.photoTimerSetting !== "Off"
                height: root.photoStatusChipHeight
                width: timerChipRow.implicitWidth + root.photoStatusChipHorizontalPadding
                radius: height / 2
                color: root.compactControlColor("#70000000", root.compactControlFill, root.compactDarkControlFill)
                border.width: 1
                border.color: root.compactControlColor("#0affffff", root.compactControlBorder, root.compactDarkControlBorder)

                Row {
                    id: timerChipRow
                    anchors.centerIn: parent
                    spacing: 6

                    Rectangle {
                        width: root.photoStatusChipDotSize
                        height: root.photoStatusChipDotSize
                        radius: width / 2
                        color: "#ffd54a"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: "TIMER " + root.photoTimerSetting
                        color: "white"
                        font.family: interBold.font.family
                        font.weight: Font.Bold
                        font.pixelSize: root.photoStatusChipFontSize
                        renderType: Text.NativeRendering
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            Rectangle {
                visible: root.photoModeEnabled && !root.photoTimelapseEnabled
                enabled: !!root.settingsState && root.canSelectCaptureMode
                height: root.photoStatusChipHeight
                width: burstChipRow.implicitWidth + root.photoStatusChipHorizontalPadding
                radius: height / 2
                color: root.compactControlColor("#70000000", root.compactControlFill, root.compactDarkControlFill)
                border.width: 1
                border.color: root.compactControlColor("#0affffff", root.compactControlBorder, root.compactDarkControlBorder)
                opacity: burstChipTap.pressed ? 0.75 : 1.0
                Accessible.role: Accessible.Button
                Accessible.name: root.settingsState && root.settingsState.photoBracketingEnabled ? "Exposure bracketing: 3 shots" : "Burst mode: " + root.photoBurstSetting
                Accessible.description: root.settingsState && root.settingsState.photoBracketingEnabled
                    ? "Bracketing is enabled in Still Mode settings"
                    : "Tap to cycle Single, Burst 3, Burst 5 and Burst 10"
                Accessible.onPressAction: root.cyclePhotoBurst()

                Row {
                    id: burstChipRow
                    anchors.centerIn: parent
                    spacing: 6

                    Rectangle {
                        width: root.photoStatusChipDotSize
                        height: root.photoStatusChipDotSize
                        radius: width / 2
                        color: "#66d9ff"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: root.settingsState && root.settingsState.photoBracketingEnabled ? "BRACKET 3"
                              : root.photoBurstSetting === "Single"
                              ? "SINGLE"
                              : ("BURST " + root.photoBurstSetting.toUpperCase())
                        color: "white"
                        font.family: interBold.font.family
                        font.weight: Font.Bold
                        font.pixelSize: root.photoStatusChipFontSize
                        renderType: Text.NativeRendering
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                MouseArea {
                    id: burstChipTap
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.cyclePhotoBurst()
                }
            }

            TimelapseIntervalPicker {
                visible: root.photoTimelapseEnabled
                currentValue: root.photoTimelapseSetting
                popupParent: dynamicOverlay
                interactionAllowed: !!root.settingsState && root.canSelectCaptureMode
                height: root.photoStatusChipHeight
                horizontalPadding: root.photoStatusChipHorizontalPadding
                dotSize: root.photoStatusChipDotSize
                fontSize: root.photoStatusChipFontSize
                fontFamily: interBold.font.family
                compact: root.compactLandscapeLayout || root.compactPortraitLayout
                large: root.largeLandscapeLayout
                color: root.compactControlColor("#70000000", root.compactControlFill, root.compactDarkControlFill)
                border.color: root.compactControlColor("#0affffff", root.compactControlBorder, root.compactDarkControlBorder)
                onIntervalSelected: function(value) { root.applyQuickTimelapseInterval(value) }
                onMenuOpened: {
                    root.openDropdown = ""
                    root.quickMenuOpen = false
                    root.captureModeMenuOpen = false
                }
            }

            Rectangle {
                objectName: "timelapseShotCounter"
                visible: root.photoTimelapseEnabled || root.timelapseActive
                height: root.photoStatusChipHeight
                width: shotCountLabel.implicitWidth + root.photoStatusChipHorizontalPadding
                radius: height / 2
                color: root.compactControlColor("#70000000", root.compactControlFill, root.compactDarkControlFill)
                border.width: 1
                border.color: root.compactControlColor("#0affffff", root.compactControlBorder, root.compactDarkControlBorder)
                Accessible.role: Accessible.StaticText
                Accessible.name: shotCountLabel.text + " saved in this timelapse"

                Text {
                    id: shotCountLabel
                    anchors.centerIn: parent
                    readonly property var count: root.controlBridge ? root.controlBridge.timelapseShotCount : 0
                    text: count + (count === 1 ? " SHOT" : " SHOTS")
                    color: "white"
                    font.family: interBold.font.family
                    font.weight: Font.Bold
                    font.pixelSize: root.photoStatusChipFontSize
                    renderType: Text.NativeRendering
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
            height: root.previewTopOffset
            color: "#000000"
            visible: !root.rectangularLayout
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: root.rectangularLayout ? 0 : 158
            color: "#000000"
            visible: !root.rectangularLayout
        }


    // =========================
    // Top Menu
    // =========================
	

        Row {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: root.topControlsSideMargin
            anchors.rightMargin: root.topControlsSideMargin
            anchors.topMargin: root.topControlsTopMargin
            spacing: root.topControlsSpacing

            Repeater {
                model: 4

	                delegate: Rectangle {

                    opacity: index === 3 && !root.whiteBalanceAvailable ? 0.4 : 1.0
	                    width: (parent.width - root.topSettingsButtonSize - (root.topControlsSpacing * 4)) / 4
	                    height: root.topStatHeight
	                    radius: root.topControlRadius
                    color: root.compactControlColor(index === 0 ? "#1affffff" : "#14ffffff",
                                                    index === 0 ? root.compactControlFillActive : root.compactControlFill,
                                                    index === 0 ? root.compactDarkControlFillActive : root.compactDarkControlFill)
                    border.width: 1
                    border.color: root.compactControlColor(index === 0 ? "#33ffffff" : "#1affffff",
                                                           index === 0 ? root.compactControlBorderActive : root.compactControlBorder,
                                                           index === 0 ? root.compactDarkControlBorderActive : root.compactDarkControlBorder)
                    readonly property bool dropdownOpen: (index === 0 && root.openDropdown === "fps")
                                                         || (index === 1 && root.openDropdown === "iso")
                                                         || (index === 2 && root.openDropdown === "shutter")
                                                         || (index === 3 && root.openDropdown === "wb")

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
    font.pixelSize: root.topLabelFontSize
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
    font.pixelSize: root.topArrowFontSize
    rotation: parent.dropdownOpen ? 180 : 0
    renderType: Text.NativeRendering

    Behavior on rotation {
        NumberAnimation { duration: 170; easing.type: Easing.OutCubic }
    }
}
                }
            }

	            Rectangle {
	                width: root.topSettingsButtonSize
	                height: root.topSettingsButtonSize
	                radius: root.topControlRadius
                color: root.compactControlColor("#14ffffff", root.compactControlFill, root.compactDarkControlFill)
                border.width: 1
                border.color: root.compactControlColor("#1affffff", root.compactControlBorder, root.compactDarkControlBorder)

                Image {
                    anchors.centerIn: parent
                    width: root.topSettingsIconSize
                    height: root.topSettingsIconSize
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

        MouseArea {
            anchors.fill: parent
            visible: root.openDropdown !== "" || root.quickMenuOpen
            enabled: visible
            onClicked: function(mouse) {
                if (root.quickMenuOpen) {
                    var panelPoint = quickMenuPanel.mapFromItem(dynamicOverlay, mouse.x, mouse.y)
                    if (panelPoint.x >= 0
                            && panelPoint.y >= 0
                            && panelPoint.x <= quickMenuPanel.width
                            && panelPoint.y <= quickMenuPanel.height) {
                        return
                    }
                }

                root.openDropdown = ""
                root.quickMenuResolutionOpen = false
                root.quickMenuOpen = false
            }
        }

        Row {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: root.topControlsSideMargin
            anchors.rightMargin: root.topControlsSideMargin
            anchors.topMargin: root.topControlsTopMargin
            spacing: root.topControlsSpacing

            TopDropdownStat {
                width: (parent.width - root.topSettingsButtonSize - (root.topControlsSpacing * 4)) / 4
                height: root.topStatHeight
                valueFontSize: root.topValueFontSize
                backgroundOpacity: root.effectiveControlsOpacity
                maxPopupHeight: root.topDropdownPopupMaxHeight
                popupMargin: root.topDropdownPopupMargin
                popupSpacing: root.topDropdownPopupSpacing
                popupOptionHeight: root.topDropdownPopupOptionHeight
                popupOptionRadius: root.topDropdownPopupOptionRadius
                popupOptionFontSize: root.topDropdownPopupOptionFontSize
                popupRadius: root.topDropdownPopupRadius
                popupTopMargin: root.topDropdownPopupTopMargin
                value: root.fps
                isOpen: root.openDropdown === "fps"
                options: root.fpsOptions
                onToggleRequested: {
                    root.quickMenuOpen = false
                    root.quickMenuResolutionOpen = false
                    root.quickMenuFieldOpen = ""
                    root.openDropdown = root.openDropdown === "fps" ? "" : "fps"
                }
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
                width: (parent.width - root.topSettingsButtonSize - (root.topControlsSpacing * 4)) / 4
                height: root.topStatHeight
                valueFontSize: root.topValueFontSize
                backgroundOpacity: root.effectiveControlsOpacity
                maxPopupHeight: root.topDropdownPopupMaxHeight
                popupMargin: root.topDropdownPopupMargin
                popupSpacing: root.topDropdownPopupSpacing
                popupOptionHeight: root.topDropdownPopupOptionHeight
                popupOptionRadius: root.topDropdownPopupOptionRadius
                popupOptionFontSize: root.topDropdownPopupOptionFontSize
                popupRadius: root.topDropdownPopupRadius
                popupTopMargin: root.topDropdownPopupTopMargin
                value: root.iso
                isOpen: root.openDropdown === "iso"
                options: root.isoOptions
                onToggleRequested: {
                    root.quickMenuOpen = false
                    root.quickMenuResolutionOpen = false
                    root.quickMenuFieldOpen = ""
                    root.openDropdown = root.openDropdown === "iso" ? "" : "iso"
                }
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
                width: (parent.width - root.topSettingsButtonSize - (root.topControlsSpacing * 4)) / 4
                height: root.topStatHeight
                valueFontSize: root.topValueFontSize
                backgroundOpacity: root.effectiveControlsOpacity
                maxPopupHeight: root.topDropdownPopupMaxHeight
                popupMargin: root.topDropdownPopupMargin
                popupSpacing: root.topDropdownPopupSpacing
                popupOptionHeight: root.topDropdownPopupOptionHeight
                popupOptionRadius: root.topDropdownPopupOptionRadius
                popupOptionFontSize: root.topDropdownPopupOptionFontSize
                popupRadius: root.topDropdownPopupRadius
                popupTopMargin: root.topDropdownPopupTopMargin
                value: root.shutterDisplayValue
                isOpen: root.openDropdown === "shutter"
                options: root.shutterOptions
                onToggleRequested: {
                    root.quickMenuOpen = false
                    root.quickMenuResolutionOpen = false
                    root.quickMenuFieldOpen = ""
                    root.openDropdown = root.openDropdown === "shutter" ? "" : "shutter"
                }
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
                width: (parent.width - root.topSettingsButtonSize - (root.topControlsSpacing * 4)) / 4
                height: root.topStatHeight
                valueFontSize: root.topValueFontSize
                backgroundOpacity: root.effectiveControlsOpacity
                maxPopupHeight: root.topDropdownPopupMaxHeight
                popupMargin: root.topDropdownPopupMargin
                popupSpacing: root.topDropdownPopupSpacing
                popupOptionHeight: root.topDropdownPopupOptionHeight
                popupOptionRadius: root.topDropdownPopupOptionRadius
                popupOptionFontSize: root.topDropdownPopupOptionFontSize
                popupRadius: root.topDropdownPopupRadius
                popupTopMargin: root.topDropdownPopupTopMargin
                value: root.wb
                enabled: root.whiteBalanceAvailable
                opacity: enabled ? 1.0 : 0.4
                isOpen: enabled && root.openDropdown === "wb"
                options: ["Auto", "2500K", "2800K", "3000K", "3200K", "3400K", "3600K", "4000K", "4500K", "4800K", "5000K", "5200K", "5400K", "5600K", "5800K", "6000K", "6500K", "7000K", "7500K", "8000K"]
                onToggleRequested: {
                    root.quickMenuOpen = false
                    root.quickMenuResolutionOpen = false
                    root.quickMenuFieldOpen = ""
                    root.openDropdown = root.openDropdown === "wb" ? "" : "wb"
                }
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
	                width: root.topSettingsButtonSize
	                height: root.topSettingsButtonSize
	                radius: root.topControlRadius
                color: "transparent"

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.quickMenuOpen = false
                        root.quickMenuResolutionOpen = false
                        root.openSettingsRequested()
                    }
                }
            }
        }

        Item {
            id: quickMenuEdgeSwipeZone
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            width: root.quickMenuSwipeEdgeWidth
            visible: !root.quickMenuOpen && root.openDropdown === ""
            z: 180

            Item {
                id: quickMenuOpenButton
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: 1
                width: root.quickMenuArrowButtonWidth
                height: root.quickMenuArrowButtonHeight

                Canvas {
                    anchors.centerIn: parent
                    width: root.quickMenuArrowCanvasWidth
                    height: root.quickMenuArrowCanvasHeight

                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        ctx.strokeStyle = "white"
                        ctx.lineWidth = root.quickMenuArrowLineWidth
                        ctx.lineCap = "round"
                        ctx.lineJoin = "round"
                        ctx.beginPath()
                        ctx.moveTo(width * 0.75, height * 0.16)
                        ctx.lineTo(width * 0.25, height * 0.50)
                        ctx.lineTo(width * 0.75, height * 0.84)
                        ctx.stroke()
                    }
                }

                TapHandler {
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.TouchScreen
                    gesturePolicy: TapHandler.DragThreshold
                    onTapped: {
                        root.openDropdown = ""
                        root.quickMenuResolutionOpen = false
                        root.quickMenuOpen = true
                    }
                }
            }

            DragHandler {
                id: quickMenuOpenSwipe
                target: null
                yAxis.enabled: false
                xAxis.enabled: true
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.TouchScreen
                grabPermissions: PointerHandler.CanTakeOverFromAnything | PointerHandler.ApprovesTakeOverByAnything

                property real swipeDeltaX: 0

                onActiveChanged: {
                    if (active) {
                        swipeDeltaX = 0
                        return
                    }

                    if (swipeDeltaX <= -42) {
                        root.openDropdown = ""
                        root.quickMenuResolutionOpen = false
                        root.quickMenuOpen = true
                    }
                }

                onActiveTranslationChanged: swipeDeltaX = activeTranslation.x
            }
        }

        Rectangle {
            id: quickMenuPanel
            width: root.quickMenuPanelWidth
            height: root.quickMenuPanelHeight
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: root.quickMenuOpen ? 14 : -(width + 20)
            radius: root.quickMenuPanelRadius
            color: root.fadedControlColor("#151515")
            border.width: 1
            border.color: root.fadedControlColor("#1affffff")
            opacity: root.quickMenuOpen ? 1.0 : 0.0
            visible: opacity > 0.0
            z: 190

            Behavior on anchors.rightMargin {
                NumberAnimation { duration: 280; easing.type: Easing.OutCubic }
            }

            Behavior on opacity {
                NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
            }

            DragHandler {
                id: quickMenuCloseSwipe
                target: null
                yAxis.enabled: false
                xAxis.enabled: true
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.TouchScreen
                grabPermissions: PointerHandler.CanTakeOverFromAnything | PointerHandler.ApprovesTakeOverByAnything

                property real swipeDeltaX: 0

                onActiveChanged: {
                    if (active) {
                        swipeDeltaX = 0
                        return
                    }

                    if (swipeDeltaX >= 42) {
                        root.quickMenuResolutionOpen = false
                        root.quickMenuOpen = false
                    }
                }

                onActiveTranslationChanged: swipeDeltaX = activeTranslation.x
            }

            Column {
                anchors.fill: parent
                anchors.margins: root.quickMenuPanelMargin
                spacing: root.quickMenuPanelSpacing

                Item {
                    id: quickResolutionRow
                    width: root.quickMenuGridWidth
                    height: root.quickMenuResolutionStackHeight
                    x: Math.round((parent.width - width) / 2)
                    z: 10

                    readonly property real fieldWidth: width

                    Item {
                        id: quickResolutionControl
                        width: quickResolutionRow.fieldWidth
                        height: root.quickMenuResolutionHeight
                        z: root.quickMenuFieldOpen === "resolution" ? 30 : 10

                        Rectangle {
                            id: quickResolutionField
                            anchors.fill: parent
                            radius: root.quickMenuResolutionRadius
                            color: root.fadedControlColor(quickResolutionArea.containsPress ? "#1f2c36" : "#171717")
                            border.width: 1
                            border.color: root.fadedControlColor(root.quickMenuFieldOpen === "resolution" ? "#3fd0ff" : "#1affffff")

                            Behavior on color {
                                ColorAnimation { duration: 140; easing.type: Easing.OutCubic }
                            }

                            Behavior on border.color {
                                ColorAnimation { duration: 180; easing.type: Easing.OutCubic }
                            }

                            Column {
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.leftMargin: root.compactLandscapeLayout ? 12 : 14
                                spacing: 2

                                Text {
                                    text: root.processedVideoRecordFormat ? root.processedVideoSizeLabel : "Resolution"
                                    color: root.quickMenuFieldOpen === "resolution" ? "#79ddff" : "#8f9096"
                                    font.family: interMedium.font.family
                                    font.pixelSize: root.quickMenuResolutionLabelFontSize
                                    font.capitalization: Font.AllUppercase
                                    font.letterSpacing: 2.1
                                    renderType: Text.NativeRendering
                                }

                                Text {
                                    text: root.cropControlsVisible ? root.resolutionBaseLabel : root.resolution
                                    color: "white"
                                    font.family: interMedium.font.family
                                    font.pixelSize: root.quickMenuResolutionValueFontSize
                                    renderType: Text.NativeRendering
                                }
                            }

                            Text {
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.rightMargin: root.compactLandscapeLayout ? 12 : 14
                                text: root.quickMenuFieldOpen === "resolution" ? "⌃" : "⌄"
                                color: "#ccffffff"
                                font.family: interRegular.font.family
                                font.pixelSize: root.quickMenuResolutionArrowFontSize
                                renderType: Text.NativeRendering
                            }

                            MouseArea {
                                id: quickResolutionArea
                                anchors.fill: parent
                                onClicked: {
                                    root.openDropdown = ""
                                    if (root.quickMenuFieldOpen === "resolution") {
                                        root.quickMenuFieldOpen = ""
                                        root.quickMenuResolutionOpen = false
                                    } else {
                                        root.quickMenuFieldOpen = "resolution"
                                        root.quickMenuResolutionOpen = true
                                    }
                                }
                            }
                        }

                        Rectangle {
                            id: quickResolutionPopup
                            width: parent.width
                            anchors.top: quickResolutionField.bottom
                            anchors.topMargin: root.compactLandscapeLayout ? 6 : 8
                            radius: root.quickMenuResolutionRadius
                            color: root.fadedControlColor("#151515")
                            border.width: 1
                            border.color: root.fadedControlColor("#1affffff")
                            clip: true
                            opacity: root.quickMenuFieldOpen === "resolution" ? 1.0 : 0.0
                            visible: opacity > 0.0
                            height: Math.min(quickResolutionFlick.contentHeight + 16, root.quickMenuPopupMaxHeight)

                            Behavior on opacity {
                                NumberAnimation { duration: 170; easing.type: Easing.OutCubic }
                            }

                            Flickable {
                                id: quickResolutionFlick
                                anchors.fill: parent
                                anchors.margins: 8
                                contentWidth: width
                                contentHeight: quickResolutionOptions.implicitHeight
                                clip: true
                                boundsBehavior: Flickable.StopAtBounds
                                flickableDirection: Flickable.VerticalFlick
                                interactive: root.quickMenuFieldOpen === "resolution"

                                Column {
                                    id: quickResolutionOptions
                                    width: quickResolutionFlick.width
                                    spacing: 6

                                    Repeater {
                                        model: root.quickMenuOpen
                                               ? root.safeOptionList(root.processedVideoRecordFormat
                                                                     ? root.h264BaseResolutionOptions
                                                                     : (root.sensorCropControlsAvailable ? root.sensorBaseResolutionOptions : root.resolutionOptions))
                                               : []

                                        delegate: Rectangle {
                                            required property var modelData
                                            readonly property string itemValue: root.optionValue(modelData)
                                            readonly property string itemLabel: root.optionLabel(modelData)

                                            width: quickResolutionOptions.width
                                            height: root.quickMenuPopupOptionHeight
                                            radius: 12
                                            color: root.fadedControlColor(itemValue === root.resolutionBaseValue ? "#22ffffff" : "transparent")

                                            Text {
                                                anchors.centerIn: parent
                                                text: parent.itemLabel
                                                color: "white"
                                                font.family: interRegular.font.family
                                                font.pixelSize: root.quickMenuPopupOptionFontSize
                                                renderType: Text.NativeRendering
                                            }

                                            MouseArea {
                                                anchors.fill: parent
                                                onClicked: {
                                                    root.applyResolutionBase(parent.itemValue)
                                                    root.quickMenuResolutionOpen = false
                                                    root.quickMenuFieldOpen = ""
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Item {
                        id: quickCropControl
                        x: 0
                        y: root.quickMenuResolutionHeight + root.quickMenuPanelSpacing
                        width: root.cropControlsVisible ? quickResolutionRow.fieldWidth : 0
                        height: root.cropControlsVisible ? root.quickMenuResolutionHeight : 0
                        visible: root.cropControlsVisible
                        z: root.quickMenuFieldOpen === "crop" ? 29 : 9

                        Rectangle {
                            id: quickCropField
                            anchors.fill: parent
                            radius: root.quickMenuResolutionRadius
                            color: root.fadedControlColor(quickCropArea.containsPress ? "#1f2c36" : "#171717")
                            border.width: 1
                            border.color: root.fadedControlColor(root.quickMenuFieldOpen === "crop" ? "#3fd0ff" : "#1affffff")

                            Column {
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.leftMargin: root.compactLandscapeLayout ? 12 : 14
                                spacing: 2

                                Text {
                                    text: root.processedVideoRecordFormat ? root.processedVideoCropLabel : "Crop"
                                    color: root.quickMenuFieldOpen === "crop" ? "#79ddff" : "#8f9096"
                                    font.family: interMedium.font.family
                                    font.pixelSize: root.quickMenuResolutionLabelFontSize
                                    font.capitalization: Font.AllUppercase
                                    font.letterSpacing: 2.1
                                    renderType: Text.NativeRendering
                                }

                                Text {
                                    text: root.cropRatioLabel
                                    color: "white"
                                    font.family: interMedium.font.family
                                    font.pixelSize: root.quickMenuResolutionValueFontSize
                                    renderType: Text.NativeRendering
                                }
                            }

                            Text {
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.rightMargin: root.compactLandscapeLayout ? 12 : 14
                                text: root.quickMenuFieldOpen === "crop" ? "⌃" : "⌄"
                                color: "#ccffffff"
                                font.family: interRegular.font.family
                                font.pixelSize: root.quickMenuResolutionArrowFontSize
                                renderType: Text.NativeRendering
                            }

                            MouseArea {
                                id: quickCropArea
                                anchors.fill: parent
                                enabled: root.cropControlsVisible
                                onClicked: {
                                    root.openDropdown = ""
                                    if (root.quickMenuFieldOpen === "crop") {
                                        root.quickMenuFieldOpen = ""
                                        root.quickMenuResolutionOpen = false
                                    } else {
                                        root.quickMenuFieldOpen = "crop"
                                        root.quickMenuResolutionOpen = true
                                    }
                                }
                            }
                        }

                        Rectangle {
                            width: parent.width
                            anchors.top: quickCropField.bottom
                            anchors.topMargin: root.compactLandscapeLayout ? 6 : 8
                            radius: root.quickMenuResolutionRadius
                            color: root.fadedControlColor("#151515")
                            border.width: 1
                            border.color: root.fadedControlColor("#1affffff")
                            clip: true
                            opacity: root.quickMenuFieldOpen === "crop" ? 1.0 : 0.0
                            visible: opacity > 0.0
                            height: Math.min(quickCropFlick.contentHeight + 16, root.quickMenuPopupMaxHeight)

                            Behavior on opacity {
                                NumberAnimation { duration: 170; easing.type: Easing.OutCubic }
                            }

                            Flickable {
                                id: quickCropFlick
                                anchors.fill: parent
                                anchors.margins: 8
                                contentWidth: width
                                contentHeight: quickCropOptions.implicitHeight
                                clip: true
                                boundsBehavior: Flickable.StopAtBounds
                                flickableDirection: Flickable.VerticalFlick
                                interactive: root.quickMenuFieldOpen === "crop"

                                Column {
                                    id: quickCropOptions
                                    width: quickCropFlick.width
                                    spacing: 6

                                    Repeater {
                                        model: root.quickMenuOpen
                                               ? root.safeOptionList(root.processedVideoRecordFormat ? root.h264ActiveCropOptions : root.sensorActiveCropOptions)
                                               : []

                                        delegate: Rectangle {
                                            required property var modelData
                                            readonly property string itemValue: root.optionValue(modelData)
                                            readonly property string itemLabel: root.optionLabel(modelData)

                                            width: quickCropOptions.width
                                            height: root.quickMenuPopupOptionHeight
                                            radius: 12
                                            color: root.fadedControlColor(itemValue === root.cropModeValue ? "#22ffffff" : "transparent")

                                            Text {
                                                anchors.centerIn: parent
                                                text: parent.itemLabel
                                                color: "white"
                                                font.family: interRegular.font.family
                                                font.pixelSize: root.quickMenuPopupOptionFontSize
                                                renderType: Text.NativeRendering
                                            }

                                            MouseArea {
                                                anchors.fill: parent
                                                enabled: parent.itemValue.length > 0
                                                onClicked: {
                                                    root.applyCropResolution(parent.itemValue)
                                                    root.quickMenuResolutionOpen = false
                                                    root.quickMenuFieldOpen = ""
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Grid {
                    width: implicitWidth
                    x: Math.round((parent.width - width) / 2)
                    columns: root.quickMenuIconColumns
                    columnSpacing: root.quickMenuButtonSpacing
                    rowSpacing: root.quickMenuButtonSpacing

                    Repeater {
                        model: root.quickMenuOpen ? root.quickMenuButtonCount : 0

                        delegate: Rectangle {
                            width: root.quickMenuButtonSize
                             height: root.quickMenuButtonSize
                             radius: root.quickMenuButtonRadius
                             color: root.fadedControlColor(root.quickMenuSlotActive(index)
                                    ? (quickSlotArea.containsPress ? "#22323c" : "#1f2c36")
                                     : (root.compactLandscapeLayout
                                       ? (root.compactDarkControls
                                          ? (quickSlotArea.containsPress ? root.compactDarkControlFillActive : root.compactDarkControlFillSoft)
                                          : (quickSlotArea.containsPress ? root.compactControlFillActive : root.compactControlFillSoft))
                                       : (quickSlotArea.containsPress ? "#20ffffff" : "#14ffffff")))
                             border.width: 1
                             border.color: root.fadedControlColor(root.quickMenuSlotActive(index)
                                                                  ? "#3fd0ff"
                                                                  : (root.compactLandscapeLayout
                                                                     ? (root.compactDarkControls ? root.compactDarkControlBorder : root.compactControlBorder)
                                                                     : "#1affffff"))
                            scale: quickSlotArea.containsPress ? 0.985 : 1.0

                            Behavior on scale {
                                NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                            }

                            Behavior on color {
                                ColorAnimation { duration: 140; easing.type: Easing.OutCubic }
                            }

                            Behavior on border.color {
                                ColorAnimation { duration: 160; easing.type: Easing.OutCubic }
                            }

                            Column {
                                anchors.centerIn: parent
                                spacing: 4

                                Image {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    width: index <= 5 ? root.quickMenuIconSize : 24
                                    height: index <= 5 ? root.quickMenuIconSize : 24
                                    visible: root.quickMenuSlotIconSource(index).length > 0
                                    source: root.quickMenuSlotIconSource(index)
                                    fillMode: Image.PreserveAspectFit
                                    smooth: true
                                    mipmap: true
                                    opacity: 0.96
                                }

                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: root.quickMenuSlotLabel(index)
                                    visible: text.length > 0
                                    color: "white"
                                    font.family: interBold.font.family
                                    font.pixelSize: index === 0 ? 10 : 14
                                    font.capitalization: Font.MixedCase
                                    renderType: Text.NativeRendering
                                }

                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: root.quickMenuSlotSubtitle(index)
                                    visible: text.length > 0
                                    color: root.quickMenuSlotActive(index) ? "#79ddff" : "#8cffffff"
                                    font.family: interRegular.font.family
                                    font.pixelSize: 9
                                    font.capitalization: Font.AllUppercase
                                    font.letterSpacing: 1.6
                                    renderType: Text.NativeRendering
                                }
                            }

                            MouseArea {
                                id: quickSlotArea
                                anchors.fill: parent
                                onClicked: root.triggerQuickMenuSlot(index)
                            }
                        }
                    }
                }
            }
        }
		
    // =========================
    // BOTTOM STATS
    // =========================

        Column {
    anchors.left: parent.left
    anchors.bottom: root.compactLandscapeLayout ? undefined : parent.bottom
    anchors.verticalCenter: root.compactLandscapeLayout ? bottomControlsRow.verticalCenter : undefined
    anchors.verticalCenterOffset: root.bottomStatsVerticalOffset
    anchors.leftMargin: root.bottomStatsSideMargin
    anchors.bottomMargin: root.bottomStatsBottomMargin
    spacing: root.bottomStatsSpacing
    width: root.bottomStatsWidth
    visible: root.showCornerStats

    InfoLine {
        width: parent.width
        label: "FORMAT"
        value: root.formatDisplayValue
        sub: root.formatDisplaySubtext
        iconSource: "qrc:/qml/icons/camera.png"
        iconSize: root.bottomStatsIconSize
        rowSpacing: root.bottomStatsRowSpacing
        textSpacing: root.bottomStatsTextSpacing
        labelFontSize: root.bottomStatsLabelFontSize
        valueFontSize: root.bottomStatsValueFontSize
        subFontSize: root.bottomStatsSubFontSize
        heightWithSub: root.bottomStatsHeightWithSub
        heightWithoutSub: root.bottomStatsHeightWithoutSub
    }

    InfoLine {
    width: parent.width
    label: "MEDIA"
    value: root.photoModeEnabled ? mediaBridge.remainingStillsText : mediaBridge.remainingMinutesText
    sub: mediaBridge.mediaMounted ? mediaBridge.mediaTypeLabel : "Insert Media"
    iconSource: "qrc:/qml/icons/media.png"
    iconSize: root.bottomStatsIconSize
    rowSpacing: root.bottomStatsRowSpacing
    textSpacing: root.bottomStatsTextSpacing
    labelFontSize: root.bottomStatsLabelFontSize
    valueFontSize: root.bottomStatsValueFontSize
    subFontSize: root.bottomStatsSubFontSize
    heightWithSub: root.bottomStatsHeightWithSub
    heightWithoutSub: root.bottomStatsHeightWithoutSub
}

}

Column {
    anchors.right: parent.right
    anchors.bottom: root.compactLandscapeLayout ? undefined : parent.bottom
    anchors.verticalCenter: root.compactLandscapeLayout ? bottomControlsRow.verticalCenter : undefined
    anchors.verticalCenterOffset: root.bottomStatsVerticalOffset
    anchors.rightMargin: root.bottomStatsSideMargin
    anchors.bottomMargin: root.bottomStatsBottomMargin
    spacing: root.bottomStatsSpacing
    width: root.bottomStatsRightWidth
    visible: root.showCornerStats

    InfoLine {
    width: parent.width
    alignRight: true
    label: "CPU"
    value: statsBridge.cpuText
    sub: statsBridge.cpuPercent < 30 ? "Idle"
     : statsBridge.cpuPercent < 70 ? "Normal"
     : "High Load"
    iconSource: "qrc:/qml/icons/cpu.png"
    iconSize: root.bottomStatsIconSize
    rowSpacing: root.bottomStatsRowSpacing
    textSpacing: root.bottomStatsTextSpacing
    labelFontSize: root.bottomStatsLabelFontSize
    valueFontSize: root.bottomStatsValueFontSize
    subFontSize: root.bottomStatsSubFontSize
    heightWithSub: root.bottomStatsHeightWithSub
    heightWithoutSub: root.bottomStatsHeightWithoutSub
}

    InfoLine {
        width: parent.width
        alignRight: true
        label: "POWER"
        value: root.powerOverlayValue
        sub: root.powerOverlaySubtext
        iconSource: "qrc:/qml/icons/power.png"
        iconSize: root.bottomStatsIconSize
        rowSpacing: root.bottomStatsRowSpacing
        textSpacing: root.bottomStatsTextSpacing
        labelFontSize: root.bottomStatsLabelFontSize
        valueFontSize: root.bottomStatsValueFontSize
        subFontSize: root.bottomStatsSubFontSize
        heightWithSub: root.bottomStatsHeightWithSub
        heightWithoutSub: root.bottomStatsHeightWithoutSub
    }
}

    // =========================
    // TIMECODE
    // =========================


TextMetrics {
    id: timecodeMetrics
    font.family: interRegular.font.family
    font.pixelSize: root.timecodeFontSize
    text: "00:00:00:00"
}

        Row {
            id: bottomControlsRow
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: root.rectangularLayout
                                  ? ((root.photoModeEnabled && root.dateTimeOverlayEnabled && !root.compactLandscapeLayout)
                                     ? root.bottomControlsBaseMargin + 34
                                     : root.bottomControlsBaseMargin)
                                  : ((root.photoModeEnabled && root.dateTimeOverlayEnabled)
                                     ? 54
                                     : 40)
            anchors.verticalCenter: undefined
            spacing: root.bottomControlsSpacing
            height: Math.max(centerStack.visible ? centerStack.height : 0,
                             clipBrowserBtn.height,
                             recordBtn.height)

            ClipBrowserButton {
                id: clipBrowserBtn
                anchors.verticalCenter: parent.verticalCenter
                backgroundOpacity: root.effectiveControlsOpacity
                buttonSize: root.clipBrowserButtonSize
                iconSize: root.clipBrowserIconSize
                strongBackground: root.compactLandscapeLayout
                darkBackground: root.compactDarkControls
                enabled: !root.recording
                onClicked: {
                    root.quickMenuOpen = false
                    root.openClipBrowserRequested()
                }
            }

            Item {
                id: centerStack
                anchors.verticalCenter: parent.verticalCenter
                visible: !root.photoModeEnabled
                width: timecodeBox.width
                height: timecodeBox.height
                readonly property int dateTimeGap: 8

                AudioMeterOverlay {
                    id: audioMeterOverlay
                    width: root.standardLandscapeLayout ? Math.max(timecodeBox.width, 248) : timecodeBox.width
                    visible: root.showAudioMeter && !root.photoModeEnabled && !root.compactLandscapeLayout
                    anchors.horizontalCenter: timecodeBox.horizontalCenter
                    anchors.bottom: timecodeBox.top
                    anchors.bottomMargin: 10
                    large: root.standardLandscapeLayout
                    muted: root.inputVolumeLevel <= 0
                    inputLevel: (typeof audioMeterBridge !== "undefined" && audioMeterBridge)
                                ? audioMeterBridge.inputLevel
                                : 0
                }

                Rectangle {
                    id: timecodeBox
                    height: root.timecodeHeight
                    radius: root.timecodeRadius
                    color: root.compactControlColor(root.recording ? "#1aff4444" : "#14ffffff",
                                                    root.recording ? "#33ff4444" : root.compactControlFill,
                                                    root.recording ? "#6b230000" : root.compactDarkControlFill)
                    border.width: 1
                    border.color: root.compactControlColor(root.recording ? "#4dff4444" : "#1affffff",
                                                           root.recording ? "#4dff4444" : root.compactControlBorder,
                                                           root.recording ? "#66ff4444" : root.compactDarkControlBorder)
                    width: Math.ceil(timecodeMetrics.width) + root.timecodeHorizontalPadding
                    anchors.centerIn: parent

                    Text {
                        id: timecodeText
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: root.timecodeTextInset
                        width: parent.width - (root.timecodeTextInset * 2)
                        text: root.timecode
                        color: root.recording ? "#ff4d4d" : "white"
                        font.family: interRegular.font.family
                        font.pixelSize: root.timecodeFontSize
                        horizontalAlignment: Text.AlignLeft
                        renderType: Text.QtRendering

                        Behavior on color {
                            ColorAnimation {
                                duration: 260
                                easing.type: Easing.OutCubic
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.toggleTimecodeMode()
                    }
                }
            }

            // =========================
            // RECORD BUTTON
            // =========================

            RecordButton {
                id: recordBtn
                enabled: root.recording || root.timelapseActive || (!root.mediaWritesPending && !root.photoCaptureInProgress)
                anchors.verticalCenter: parent.verticalCenter
                recording: root.recording
                timelapse: root.timelapseActive
                timelapseMode: root.photoTimelapseEnabled
                photoMode: root.photoModeEnabled
                soundRecorderMode: root.soundRecorderEnabled
                photoCapturePulse: root.photoCapturePulse
                backgroundOpacity: root.effectiveControlsOpacity
                buttonSize: root.recordButtonSize
                centerSize: root.recordButtonCenterSize
                strongBackground: root.compactLandscapeLayout
                darkBackground: root.compactDarkControls

                modeSelectionEnabled: root.canSelectCaptureMode
                onPressAndHold: root.openCaptureModeMenu()

                onClicked: {
                    if (root.photoModeEnabled) {
                        if (root.photoTimelapseEnabled) {
                            if (!root.timelapseActive && !mediaBridge.mediaMounted) {
                                root.showNoMediaWarning(true)
                                return
                            }

                            root.cancelPhotoSequence()
                            root.requestTimelapseState(!root.timelapseActive)
                            return
                        }

                        if (!mediaBridge.mediaMounted) {
                            root.showNoMediaWarning(true)
                            return
                        }

                        root.beginPhotoCaptureSequence()
                        return
                    }

                    if (!root.recording) {
                        if (!mediaBridge.mediaMounted) {
                            root.showNoMediaWarning(false)
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

        Item {
            anchors.fill: parent
            z: 5000
            visible: root.captureModeMenuOpen

            MouseArea {
                anchors.fill: parent
                onPressed: root.captureModeMenuOpen = false
            }

            Rectangle {
                id: captureModeMenu
                objectName: "captureModeMenu"
                readonly property int optionHeight: root.largeLandscapeLayout ? 64 : root.compactLandscapeLayout ? 42 : 52
                width: Math.min(root.width - 24, root.largeLandscapeLayout ? 300 : root.compactLandscapeLayout ? 210 : 250)
                height: optionHeight * 3 + 24
                radius: root.compactLandscapeLayout ? 14 : 18
                color: "#f0181818"
                border.width: 1
                border.color: "#30ffffff"

                Column {
                    id: captureModeColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                    spacing: 4

                    Repeater {
                        model: root.captureModeMenuOpen ? [
                            { mode: "clip", label: "Clip mode" },
                            { mode: "still", label: "Still mode" },
                            { mode: "timelapse", label: "Timelapse" }
                        ] : []

                        delegate: Rectangle {
                            required property var modelData
                            width: captureModeColumn.width
                            height: captureModeMenu.optionHeight
                            radius: 10
                            readonly property bool selected: root.selectedCaptureMode === modelData.mode
                            color: selected ? "#1f2c36" : (captureModeArea.pressed ? "#16ffffff" : "transparent")
                            border.width: selected ? 1 : 0
                            border.color: "#3fd0ff"

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 16
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.label
                                color: selected ? "#3fd0ff" : "white"
                                font.family: interMedium.font.family
                                font.pixelSize: root.largeLandscapeLayout ? 24 : root.compactLandscapeLayout ? 16 : 19
                                renderType: Text.NativeRendering
                            }

                            MouseArea {
                                id: captureModeArea
                                anchors.fill: parent
                                onClicked: root.selectCaptureMode(modelData.mode)
                            }
                        }
                    }
                }
            }
        }

        AudioMeterOverlay {
            id: compactAudioMeterOverlay
            z: 21
            width: root.largeLandscapeLayout ? 230 : (root.standardLandscapeLayout ? 190 : (root.mediumLandscapeLayout ? 146 : 126))
            visible: root.showAudioMeter && !root.photoModeEnabled && root.compactLandscapeLayout
            anchors.horizontalCenter: parent.horizontalCenter
            y: root.dateTimeOverlayEnabled
               ? (bottomControlsRow.y
                  + ((bottomControlsRow.height - timecodeBox.height) / 2)
                  - height
                  - centerStack.dateTimeGap)
               : (bottomControlsRow.y
                  + ((bottomControlsRow.height - timecodeBox.height) / 2)
                  - height
                  - centerStack.dateTimeGap)
            muted: root.inputVolumeLevel <= 0
            inputLevel: (typeof audioMeterBridge !== "undefined" && audioMeterBridge)
                        ? audioMeterBridge.inputLevel
                        : 0
            compact: true
            largeCompact: root.regularLandscapeLayout
            extraLargeCompact: root.standardLandscapeLayout
            oversizedCompact: root.largeLandscapeLayout
            backgroundOpacity: root.effectiveControlsOpacity
            darkBackground: root.compactDarkControls
        }

        Rectangle {
            id: dateTimePageChip
            visible: root.dateTimeOverlayEnabled
            anchors.horizontalCenter: parent.horizontalCenter
            y: root.compactLandscapeLayout && !root.photoModeEnabled
               ? (root.showAudioMeter
                  ? (bottomControlsRow.y
                     + ((bottomControlsRow.height - timecodeBox.height) / 2)
                     - compactAudioMeterOverlay.height
                     - height
                     - centerStack.dateTimeGap
                     - 6
                     - (root.regularLandscapeLayout ? 2 : 0))
                   : (bottomControlsRow.y
                      + ((bottomControlsRow.height - timecodeBox.height) / 2)
                      - height
                      - centerStack.dateTimeGap
                      - (root.regularLandscapeLayout ? 3 : 0)))
               : root.photoModeEnabled
               ? (root.compactLandscapeLayout
                  ? (bottomControlsRow.y - height - centerStack.dateTimeGap)
                  : (bottomControlsRow.y + bottomControlsRow.height + 8))
               : (bottomControlsRow.y
                  + ((bottomControlsRow.height - timecodeBox.height) / 2)
                  + timecodeBox.height
                  + centerStack.dateTimeGap)
            height: root.dateTimeChipHeight
            width: dateTimeChipText.implicitWidth + root.dateTimeChipHorizontalPadding
            radius: root.dateTimeChipRadius
            color: root.compactControlColor("#14ffffff", root.compactControlFill, root.compactDarkControlFill)
            border.width: 1
            border.color: root.compactControlColor("#1affffff", root.compactControlBorder, root.compactDarkControlBorder)

            Text {
                id: dateTimeChipText
                anchors.centerIn: parent
                text: root.dateTimeOverlayText
                color: "white"
                font.family: interBold.font.family
                font.weight: Font.Bold
                font.pixelSize: root.dateTimeChipFontSize
                renderType: Text.NativeRendering

                Behavior on color {
                    ColorAnimation {
                        duration: 260
                        easing.type: Easing.OutCubic
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
        id: recordWarningCard
        readonly property int popupMargin: root.standardLandscapeLayout ? 26 : (root.compactLandscapeLayout ? 16 : 24)
        readonly property int bottomButtonGap: root.standardLandscapeLayout ? 24 : 0
        width: root.standardLandscapeLayout ? 500 : (root.compactLandscapeLayout ? 340 : 404)
        height: root.standardLandscapeLayout ? (recordWarningColumn.implicitHeight + popupMargin + bottomButtonGap) : (root.compactLandscapeLayout ? 210 : 286)
        anchors.centerIn: parent
        anchors.verticalCenterOffset: root.compactLandscapeLayout ? -8 : -18
        radius: root.standardLandscapeLayout ? 30 : (root.compactLandscapeLayout ? 22 : 28)
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
            id: recordWarningColumn
            anchors.fill: parent
            anchors.leftMargin: recordWarningCard.popupMargin
            anchors.rightMargin: recordWarningCard.popupMargin
            anchors.topMargin: recordWarningCard.popupMargin
            anchors.bottomMargin: root.standardLandscapeLayout ? recordWarningCard.bottomButtonGap : recordWarningCard.popupMargin
            spacing: root.standardLandscapeLayout ? 16 : (root.compactLandscapeLayout ? 10 : 16)

            Row {
                spacing: root.standardLandscapeLayout ? 14 : (root.compactLandscapeLayout ? 8 : 12)

                Rectangle {
                    width: root.standardLandscapeLayout ? 52 : (root.compactLandscapeLayout ? 32 : 42)
                    height: root.standardLandscapeLayout ? 52 : (root.compactLandscapeLayout ? 32 : 42)
                    radius: width / 2
                    color: "#6b5200"
                    border.width: 1
                    border.color: "#d7b44a"

                    Text {
                        anchors.centerIn: parent
                        text: "!"
                        color: "white"
                        font.family: interBold.font.family
                        font.weight: Font.Bold
                        font.pixelSize: root.standardLandscapeLayout ? 27 : (root.compactLandscapeLayout ? 17 : 22)
                        renderType: Text.NativeRendering
                    }
                }

                Column {
                    spacing: root.standardLandscapeLayout ? 5 : (root.compactLandscapeLayout ? 3 : 4)

                    Text {
                        text: root.recordWarningTitle
                        color: "white"
                        font.family: interBold.font.family
                        font.weight: Font.Bold
                        font.pixelSize: root.standardLandscapeLayout ? 30 : (root.compactLandscapeLayout ? 20 : 25)
                        renderType: Text.NativeRendering
                    }

                    Text {
                        text: root.recordWarningSubtitle
                        color: "#8f9096"
                        font.family: interRegular.font.family
                        font.pixelSize: root.standardLandscapeLayout ? 16 : (root.compactLandscapeLayout ? 12 : 14)
                        width: root.standardLandscapeLayout ? 380 : (root.compactLandscapeLayout ? 248 : 280)
                        wrapMode: Text.WordWrap
                        renderType: Text.NativeRendering
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: root.standardLandscapeLayout ? 100 : (root.compactLandscapeLayout ? 62 : 84)
                radius: root.standardLandscapeLayout ? 22 : (root.compactLandscapeLayout ? 16 : 18)
                color: "#0d0e10"
                border.width: 1
                border.color: "#202227"

                Column {
                    anchors.fill: parent
                    anchors.margins: root.standardLandscapeLayout ? 18 : (root.compactLandscapeLayout ? 10 : 14)
                    spacing: root.standardLandscapeLayout ? 5 : (root.compactLandscapeLayout ? 2 : 4)

                    Text {
                        text: root.recordWarningDetailTitle
                        color: "white"
                        font.family: interBold.font.family
                        font.weight: Font.Bold
                        font.pixelSize: root.standardLandscapeLayout ? 19 : (root.compactLandscapeLayout ? 13 : 16)
                        renderType: Text.NativeRendering
                    }

                    Text {
                        text: root.recordWarningDetailBody
                        color: "#6f7076"
                        font.family: interRegular.font.family
                        font.pixelSize: root.standardLandscapeLayout ? 15 : (root.compactLandscapeLayout ? 11 : 13)
                        wrapMode: Text.WordWrap
                        width: parent.width
                        renderType: Text.NativeRendering
                    }
                }
            }

            Item {
                width: 1
                height: root.standardLandscapeLayout ? 2 : (root.compactLandscapeLayout ? 4 : 1)
            }

            Rectangle {
                width: root.standardLandscapeLayout ? 154 : (root.compactLandscapeLayout ? 112 : 132)
                height: root.standardLandscapeLayout ? 60 : (root.compactLandscapeLayout ? 40 : 52)
                radius: root.standardLandscapeLayout ? 20 : (root.compactLandscapeLayout ? 14 : 18)
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
                    font.pixelSize: root.standardLandscapeLayout ? 18 : (root.compactLandscapeLayout ? 13 : 16)
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

