import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import ActorMailboxExample 1.0

Rectangle {
    id: root

    property string title: "JSON"
    property var value: ({})
    property bool expanded: true

    color: "#202124"
    radius: 4
    border.color: "#3c4043"
    border.width: 1

    implicitHeight: expanded ? contentColumn.implicitHeight : header.height

    function isPrimitive(value) {
        return value === null || typeof value !== "object";
    }

    function isObject(value) {
        return value !== null && typeof value === "object" && !Array.isArray(value);
    }

    function isArray(value) {
        return Array.isArray(value);
    }

    Column {
        id: contentColumn

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top

        spacing: 0

        // Header
        Rectangle {
            id: header

            width: parent.width
            height: 40

            color: "#292a2d"

            RowLayout {
                anchors.fill: parent

                anchors.leftMargin: 12
                anchors.rightMargin: 8

                Label {
                    Layout.fillWidth: true

                    text: root.title

                    color: "white"
                    font.bold: true
                    font.pixelSize: 14
                }

                ToolButton {
                    text: root.expanded ? "▼" : "▶"

                    onClicked: {
                        root.expanded = !root.expanded;
                    }
                }
            }
        }

        // Object content
        Rectangle {
            id: objectContent

            width: 300

            implicitHeight: content.implicitHeight + 20

            visible: root.expanded

            Text {
                id: content
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top

                anchors.margins: 10
                wrapMode: Text.WordWrap
                text: JSON.stringify(root.value, null, 2)
            }
        }
    }
}
