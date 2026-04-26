import QtQuick
import QtQuick.Window
import Apertar 1.0

Window {
    id: root
    visible: true
    color: "#000000"
    flags: Qt.FramelessWindowHint
    visibility: Window.FullScreen

    width: 720
    height: 720

    property string currentPage: "camera"
    property bool settingsOpen: false
    property string selectedClipPath: ""
    property string selectedClipName: ""
    property int selectedClipFrames: 0
    property int selectedClipIndex: -1
    property bool autoPowerOffWarningOpen: false
    property int autoPowerOffCountdown: 10
    readonly property bool browserLayerVisible: currentPage === "browser" || currentPage === "player"

    function cancelAutoPowerOffWarning() {
        autoPowerOffWarningOpen = false
        autoPowerOffCountdown = 10
        autoPowerOffCountdownTimer.stop()
    }

    function startAutoPowerOffWarning() {
        autoPowerOffCountdown = 10
        autoPowerOffWarningOpen = true
        autoPowerOffCountdownTimer.restart()
    }

    Component.onCompleted: {
        sleepManager.sleepMode = settingsBridge.sleepMode
        clipModel.stillMode = settingsBridge.photoModeEnabled
        apertarPreviewBridge.connectToCore()
    }

    Connections {
        target: settingsBridge

        function onSleepModeChanged() {
            sleepManager.sleepMode = settingsBridge.sleepMode
            if (settingsBridge.sleepMode === "Off")
                root.cancelAutoPowerOffWarning()
        }

        function onPhotoModeEnabledChanged() {
            clipModel.stillMode = settingsBridge.photoModeEnabled
        }
    }

    Connections {
        target: sleepManager

        function onSleepTriggered() {
            if (settingsBridge.sleepMode === "Off")
                return

            if (cameraPage.recording) {
                sleepManager.restartIdleTimerNow()
                return
            }

            root.startAutoPowerOffWarning()
        }

        function onActivityDetected() {
            if (root.autoPowerOffWarningOpen)
                root.cancelAutoPowerOffWarning()
        }
    }

    Timer {
        id: autoPowerOffCountdownTimer
        interval: 1000
        repeat: true
        running: false
        onTriggered: {
            if (!root.autoPowerOffWarningOpen) {
                stop()
                return
            }

            if (cameraPage.recording) {
                root.cancelAutoPowerOffWarning()
                sleepManager.restartIdleTimerNow()
                return
            }

            root.autoPowerOffCountdown -= 1
            if (root.autoPowerOffCountdown <= 0) {
                stop()
                root.autoPowerOffWarningOpen = false
                systemActionBridge.shutdownCamera()
            }
        }
    }

    function selectClip(path, name, frameCount, index) {
        selectedClipPath = path
        selectedClipName = name
        selectedClipFrames = frameCount
        selectedClipIndex = index
        playbackController.loadClip(path)
        currentPage = "player"
    }

    function selectClipAt(index) {
        if (index < 0 || index >= clipModel.count)
            return

        var clipPath = clipModel.pathAt(index)
        if (!clipPath || clipPath.length === 0)
            return

        selectClip(clipPath, clipModel.nameAt(index), clipModel.frameCountAt(index), index)
    }

    function hasPreviousClip() {
        return selectedClipIndex > 0
    }

    function hasNextClip() {
        return selectedClipIndex >= 0 && selectedClipIndex < clipModel.count - 1
    }

    function removeSelectedClip() {
        if (!selectedClipPath || selectedClipPath.length === 0)
            return

        var removedIndex = selectedClipIndex
        var removed = clipModel.removeClip(selectedClipPath)
        if (!removed)
            return

        playbackController.stop()

        if (clipModel.count <= 0) {
            selectedClipPath = ""
            selectedClipName = ""
            selectedClipFrames = 0
            selectedClipIndex = -1
            currentPage = "browser"
            return
        }

        var nextIndex = Math.max(0, Math.min(removedIndex, clipModel.count - 1))
        selectClipAt(nextIndex)
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

    CameraPage {
        id: cameraPage
        anchors.fill: parent
        bridge: apertarPreviewBridge
        controlBridge: apertarControlBridge
        settingsState: settingsBridge
        onOpenSettingsRequested: root.settingsOpen = true
        onOpenClipBrowserRequested: {
            root.settingsOpen = false
            root.currentPage = "browser"
        }
        onRecordingChanged: {
            if (cameraPage.recording && root.autoPowerOffWarningOpen) {
                root.cancelAutoPowerOffWarning()
                sleepManager.restartIdleTimerNow()
            }
        }
    }

    Rectangle {
        id: settingsScrim
        anchors.fill: parent
        color: "#cc000000"
        opacity: root.currentPage === "camera" && root.settingsOpen ? 1.0 : 0.0
        visible: opacity > 0.0
        z: 1000

        Behavior on opacity {
            NumberAnimation {
                duration: 520
                easing.type: Easing.OutCubic
            }
        }
    }

    SettingsPage {
        id: settingsPage
        anchors.fill: parent
        z: 1001
        settingsState: settingsBridge

        visible: root.currentPage === "camera" && (opacity > 0.0 || root.settingsOpen)
        opacity: root.currentPage === "camera" && root.settingsOpen ? 1.0 : 0.0
        y: root.currentPage === "camera" && root.settingsOpen ? 0 : 36
        scale: root.currentPage === "camera" && root.settingsOpen ? 1.0 : 0.985

        onBackRequested: root.settingsOpen = false

        Behavior on opacity {
            NumberAnimation {
                duration: 500
                easing.type: Easing.OutCubic
            }
        }

        Behavior on y {
            NumberAnimation {
                duration: 620
                easing.type: Easing.OutCubic
            }
        }

        Behavior on scale {
            NumberAnimation {
                duration: 620
                easing.type: Easing.OutCubic
            }
        }
    }

    ClipBrowserPage {
        id: clipBrowserPage
        anchors.fill: parent
        z: 900
        visible: opacity > 0.0 || root.browserLayerVisible
        opacity: root.browserLayerVisible ? 1.0 : 0.0
        y: root.browserLayerVisible ? 0 : 36
        scale: root.browserLayerVisible ? 1.0 : 0.985
        enabled: root.currentPage === "browser"
        stillMode: settingsBridge.photoModeEnabled
        selectedClipPath: root.selectedClipPath

        onBackRequested: root.currentPage = "camera"
        onClipOpened: function(clipPath, clipName, frameCount, clipIndex) {
            root.selectClip(clipPath, clipName, frameCount, clipIndex)
        }

        Behavior on opacity {
            NumberAnimation {
                duration: 500
                easing.type: Easing.OutCubic
            }
        }

        Behavior on y {
            NumberAnimation {
                duration: 620
                easing.type: Easing.OutCubic
            }
        }

        Behavior on scale {
            NumberAnimation {
                duration: 620
                easing.type: Easing.OutCubic
            }
        }
    }

    ClipPlayerPage {
        id: clipPlayerPage
        anchors.fill: parent
        z: 901
        visible: opacity > 0.0 || root.currentPage === "player"
        opacity: root.currentPage === "player" ? 1.0 : 0.0
        y: root.currentPage === "player" ? 0 : 36
        scale: root.currentPage === "player" ? 1.0 : 0.985
        enabled: root.currentPage === "player"
        selectedClipName: root.selectedClipName
        selectedClipIndex: root.selectedClipIndex
        hasPreviousClip: root.hasPreviousClip()
        hasNextClip: root.hasNextClip()

        onBackRequested: root.currentPage = "browser"
        onPreviousClipRequested: root.selectClipAt(root.selectedClipIndex - 1)
        onNextClipRequested: root.selectClipAt(root.selectedClipIndex + 1)
        onDeleteRequested: root.removeSelectedClip()

        Behavior on opacity {
            NumberAnimation {
                duration: 500
                easing.type: Easing.OutCubic
            }
        }

        Behavior on y {
            NumberAnimation {
                duration: 620
                easing.type: Easing.OutCubic
            }
        }

        Behavior on scale {
            NumberAnimation {
                duration: 620
                easing.type: Easing.OutCubic
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#cc000000"
        visible: root.autoPowerOffWarningOpen
        opacity: root.autoPowerOffWarningOpen ? 1.0 : 0.0
        z: 6000

        Behavior on opacity {
            NumberAnimation {
                duration: 180
                easing.type: Easing.OutCubic
            }
        }
    }

    Rectangle {
        width: 430
        height: 196
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -10
        radius: 26
        color: "#151515"
        border.width: 1
        border.color: "#1affffff"
        visible: root.autoPowerOffWarningOpen
        opacity: root.autoPowerOffWarningOpen ? 1.0 : 0.0
        scale: root.autoPowerOffWarningOpen ? 1.0 : 0.96
        z: 6001

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
            spacing: 14

            Text {
                text: "Auto Power Off"
                color: "white"
                font.family: interBold.font.family
                font.pixelSize: 28
                renderType: Text.NativeRendering
            }

            Text {
                text: "The camera will shut down in " + root.autoPowerOffCountdown + " seconds due to inactivity."
                color: "#8f9096"
                font.family: interRegular.font.family
                font.pixelSize: 17
                wrapMode: Text.WordWrap
                width: parent.width
                renderType: Text.NativeRendering
            }

            Rectangle {
                width: parent.width
                height: 54
                radius: 18
                color: stayAwakeArea.containsPress ? "#20ffffff" : "#14ffffff"
                border.width: 1
                border.color: "#1affffff"
                scale: stayAwakeArea.containsPress ? 0.985 : 1.0

                Behavior on scale {
                    NumberAnimation {
                        duration: 140
                        easing.type: Easing.OutCubic
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: "Stay Awake"
                    color: "white"
                    font.family: interMedium.font.family
                    font.pixelSize: 18
                    renderType: Text.NativeRendering
                }

                MouseArea {
                    id: stayAwakeArea
                    anchors.fill: parent
                    onClicked: {
                        root.cancelAutoPowerOffWarning()
                        sleepManager.restartIdleTimerNow()
                    }
                }
            }
        }
    }
}
