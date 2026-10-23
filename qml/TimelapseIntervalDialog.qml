import QtQuick
import QtQuick.Controls
import "TimelapseIntervals.js" as Intervals

Popup {
    id: root
    objectName: "timelapseCustomPopup"
    property string currentValue: "5s"
    property string fontFamily: ""
    signal intervalSelected(string value)

    width: Math.min(parent.width - 24, 420)
    height: Math.min(body.implicitHeight + padding * 2, parent.height - 24)
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    modal: true
    focus: true
    padding: 18
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    onOpened: {
        var value = Intervals.seconds(currentValue)
        intervalSecondsInput.text = String(isFinite(value) ? value : 5)
    }
    background: Rectangle { color: "#181818"; radius: 22; border.color: "#3fd0ff" }
    contentItem: Flickable {
        clip: true
        contentHeight: body.implicitHeight
        boundsBehavior: Flickable.StopAtBounds
        Column {
            id: body
            width: parent.width
            spacing: 10
            Text {
                text: "Custom Timelapse Interval"
                color: "white"
                font.family: root.fontFamily
                font.pixelSize: 19
            }
            Text { text: "Seconds (0.1 to 86400)"; color: "#99ffffff"; font.pixelSize: 13 }
            TextField {
                id: intervalSecondsInput
                objectName: "timelapseCustomSeconds"
                width: parent.width
                height: 46
                color: "white"
                font.pixelSize: 24
                horizontalAlignment: TextInput.AlignHCenter
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                validator: DoubleValidator { bottom: 0.1; top: 86400; decimals: 1; notation: DoubleValidator.StandardNotation; locale: "C" }
                background: Rectangle { color: "#101010"; radius: 10; border.color: "#40ffffff" }
                onAccepted: if (acceptableInput) intervalApply.clicked()
            }
            Grid {
                width: parent.width
                columns: 3
                spacing: 8
                Repeater {
                    model: root.visible ? ["1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "0", "Back"] : []
                    delegate: Button {
                        required property string modelData
                        width: (parent.width - 16) / 3
                        height: Math.min(44, Math.max(30, root.parent.height * 0.08))
                        text: modelData
                        onClicked: {
                            if (modelData === "Back") intervalSecondsInput.text = intervalSecondsInput.text.slice(0, -1)
                            else if (modelData === "." && intervalSecondsInput.text.indexOf(".") !== -1) return
                            else if (intervalSecondsInput.text.length < 7) intervalSecondsInput.text += modelData
                        }
                        contentItem: Text { text: parent.text; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; font.pixelSize: 17 }
                        background: Rectangle { color: parent.down ? "#303030" : "#242424"; radius: 10 }
                    }
                }
            }
            Row {
                width: parent.width
                spacing: 10
                Button {
                    objectName: "timelapseCustomCancel"
                    text: "Cancel"
                    width: (parent.width - 10) / 2
                    height: 44
                    onClicked: root.close()
                    contentItem: Text { text: "Cancel"; color: "#e2e2e2"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; font.pixelSize: 16 }
                    background: Rectangle { color: parent.down ? "#303030" : "#242424"; radius: 10; border.color: "#40ffffff" }
                }
                Button {
                    id: intervalApply
                    objectName: "timelapseCustomApply"
                    text: "Apply"
                    width: (parent.width - 10) / 2
                    height: 44
                    enabled: intervalSecondsInput.acceptableInput
                    onClicked: {
                        root.intervalSelected(String(Number(intervalSecondsInput.text)) + "s")
                        root.close()
                    }
                    contentItem: Text { text: "Apply"; color: "#e2f6ff"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; font.pixelSize: 16 }
                    background: Rectangle { color: "#123d59"; radius: 10; border.color: "#3fd0ff"; opacity: intervalApply.enabled ? 1 : 0.4 }
                }
            }
        }
    }
}
