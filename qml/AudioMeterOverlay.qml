import QtQuick

Rectangle {
    id: root

    property int inputLevel: 0
    property bool muted: false
    property real backgroundOpacity: 1.0
    property bool compact: false
    property bool large: false
    property bool largeCompact: false
    property bool extraLargeCompact: false
    property bool oversizedCompact: false
    property bool darkBackground: false
    readonly property real normalizedLevel: Math.max(0, Math.min(1, inputLevel / 100))
    readonly property int segmentCount: 14
    readonly property int contentMargin: compact ? (oversizedCompact ? 7 : (extraLargeCompact ? 6 : (largeCompact ? 5 : 4))) : (large ? 7 : 6)
    readonly property int rowSpacing: compact ? (oversizedCompact ? 10 : (extraLargeCompact ? 8 : (largeCompact ? 6 : 5))) : (large ? 10 : 8)
    readonly property int iconContainerSize: compact ? (oversizedCompact ? 26 : (extraLargeCompact ? 22 : (largeCompact ? 18 : 16))) : (large ? 26 : 22)
    readonly property int iconSize: compact ? (oversizedCompact ? 15 : (extraLargeCompact ? 13 : (largeCompact ? 10 : 9))) : (large ? 15 : 12)
    readonly property int barHeight: compact ? (oversizedCompact ? 15 : (extraLargeCompact ? 12 : (largeCompact ? 9 : 8))) : (large ? 15 : 12)
    readonly property color compactFill: darkBackground
                                         ? fadedColor(0x00, 0x00, 0x00, 0.329)
                                         : fadedColor(0xff, 0xff, 0xff, 0.141)
    readonly property color compactBorder: darkBackground
                                           ? fadedColor(0xff, 0xff, 0xff, 0.165)
                                           : fadedColor(0xff, 0xff, 0xff, 0.188)
    readonly property color compactTrackFill: fadedColor(0xff, 0xff, 0xff, darkBackground ? 0.10 : 0.08)
    readonly property color compactTrackBorder: fadedColor(0xff, 0xff, 0xff, darkBackground ? 0.14 : 0.10)
    readonly property color compactInactiveSegment: fadedColor(0xff, 0xff, 0xff, darkBackground ? 0.18 : 0.14)

    width: 220
    height: compact ? (oversizedCompact ? 42 : (extraLargeCompact ? 34 : (largeCompact ? 28 : 24))) : (large ? 42 : 34)
    radius: height / 2
    color: compact ? compactFill : "#000000"
    border.width: 1
    border.color: root.muted ? "#3a2a2a" : (compact ? compactBorder : "#20ffffff")

    function fadedColor(r, g, b, baseAlpha) {
        return Qt.rgba(r / 255.0,
                       g / 255.0,
                       b / 255.0,
                       Math.max(0.0, Math.min(1.0, baseAlpha * root.backgroundOpacity)))
    }

    Item {
        anchors.fill: parent
        anchors.margins: root.contentMargin

        Row {
            anchors.fill: parent
            spacing: root.rowSpacing

            Rectangle {
                width: root.iconContainerSize
                height: root.iconContainerSize
                radius: width / 2
                anchors.verticalCenter: parent.verticalCenter
                color: root.muted ? "#4b1f1f" : "#18ffffff"
                border.width: 1
                border.color: root.muted ? "#8f3d3d" : "#1affffff"

                Image {
                    anchors.centerIn: parent
                    width: root.iconSize
                    height: root.iconSize
                    source: root.muted
                            ? "qrc:/qml/icons/microphone-muted.png"
                            : "qrc:/qml/icons/microphone.png"
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                }
            }

            Rectangle {
                width: parent.width - root.iconContainerSize - root.rowSpacing
                height: root.barHeight
                anchors.verticalCenter: parent.verticalCenter
                radius: height / 2
                color: root.compact ? root.compactTrackFill : "#101113"
                border.width: 1
                border.color: root.compact ? root.compactTrackBorder : "#1affffff"

                Item {
                    anchors.fill: parent
                    anchors.margins: 3

                    Row {
                        anchors.fill: parent
                        spacing: 2

                        Repeater {
                            model: root.segmentCount

                            delegate: Rectangle {
                                readonly property real threshold: (index + 1) / root.segmentCount

                                width: (parent.width - ((root.segmentCount - 1) * 2)) / root.segmentCount
                                height: parent.height
                                radius: 3
                                color: root.normalizedLevel >= threshold
                                       ? (threshold > 0.8 ? "#ff6262"
                                          : threshold > 0.55 ? "#ffd55a"
                                          : "#42df86")
                                       : (root.compact ? root.compactInactiveSegment : "#1d2327")

                                Behavior on color {
                                    ColorAnimation {
                                        duration: 110
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
}
