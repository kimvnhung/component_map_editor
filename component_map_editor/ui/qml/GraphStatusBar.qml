import QtQuick
import QtQuick.Layouts
import QmlFontAwesome

Rectangle {
    id: root

    property int componentCount: 0
    property int connectionCount: 0
    property int selectedComponentCount: 0
    property int selectedConnectionCount: 0
    readonly property int selectedTotalCount: selectedComponentCount + selectedConnectionCount
    property string selectedComponentTitle: "none"
    property string selectedConnectionLabel: "none"
    property point mouseViewPos: Qt.point(0, 0)
    property point mouseWorldPos: Qt.point(0, 0)
    property real zoom: 1.0

    function formatCoord(value) {
        return Number(value).toFixed(1)
    }

    height: 34
    color: "#1e252b"
    opacity: 0.94

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.color: "#2f3b45"
        border.width: 1
    }

    component StatPill: Rectangle {
        id: pill
        required property string iconName
        required property string valueText
        required property color accent

        radius: 9
        color: Qt.rgba(pill.accent.r, pill.accent.g, pill.accent.b, 0.16)
        border.color: Qt.rgba(pill.accent.r, pill.accent.g, pill.accent.b, 0.42)
        border.width: 1
        implicitHeight: 24
        implicitWidth: content.implicitWidth + 16

        RowLayout {
            id: content
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            spacing: 6

            FaIcon {
                iconName: pill.iconName
                iconStyle: FontAwesome.Solid
                font.pixelSize: 11
                color: pill.accent
                Layout.alignment: Qt.AlignVCenter
            }

            Text {
                text: pill.valueText
                color: "#e8edf2"
                font.pixelSize: 11
                font.bold: true
                verticalAlignment: Text.AlignVCenter
                Layout.alignment: Qt.AlignVCenter
            }
        }
    }

    Flickable {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        clip: true
        contentWidth: metricsRow.implicitWidth
        contentHeight: metricsRow.implicitHeight
        interactive: contentWidth > width
        boundsBehavior: Flickable.StopAtBounds

        Row {
            id: metricsRow
            spacing: 8
            anchors.verticalCenter: parent.verticalCenter

            StatPill {
                iconName: "cubes"
                valueText: "Components " + root.componentCount
                accent: "#4fc3f7"
            }

            StatPill {
                iconName: "share-nodes"
                valueText: "Connections " + root.connectionCount
                accent: "#80cbc4"
            }

            StatPill {
                iconName: "check-double"
                valueText: "Selected " + root.selectedTotalCount
                accent: "#ffd166"
            }

            StatPill {
                iconName: "tag"
                valueText: "Node " + root.selectedComponentTitle
                accent: "#ff9f7f"
            }

            StatPill {
                iconName: "link"
                valueText: "Edge " + root.selectedConnectionLabel
                accent: "#d4a5ff"
            }

            StatPill {
                iconName: "arrows-up-down-left-right"
                valueText: "View [" + root.formatCoord(root.mouseViewPos.x)
                           + ", " + root.formatCoord(root.mouseViewPos.y) + "]"
                accent: "#9ad1ff"
            }

            StatPill {
                iconName: "crosshairs"
                valueText: "World [" + root.formatCoord(root.mouseWorldPos.x)
                           + ", " + root.formatCoord(root.mouseWorldPos.y) + "]"
                accent: "#b3e5fc"
            }

            StatPill {
                iconName: "magnifying-glass"
                valueText: "Zoom " + root.zoom.toFixed(2)
                accent: "#ffcc80"
            }
        }
    }
}
