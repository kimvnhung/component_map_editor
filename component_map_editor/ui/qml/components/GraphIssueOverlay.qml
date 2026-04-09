import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Rectangle {
    id: root

    property var messages: []
    property bool expanded: false
    property bool canClearRuntime: false
    readonly property real availableWidth: {
        if (root.parent && root.parent.parent && root.parent.parent.width > 0)
            return root.parent.parent.width;
        if (root.Window.window)
            return root.Window.window.width;
        return 1000;
    }

    signal clearRequested()

    width: root.expanded
           ? Math.min(520, Math.max(340, root.availableWidth * 0.42))
           : Math.min(360, Math.max(260, root.availableWidth * 0.3))
    implicitHeight: contentLayout.implicitHeight + 16
    height: implicitHeight
    color: "#fff7f7"
    border.color: "#ef9a9a"
    border.width: 1
    radius: 8

    ColumnLayout {
        id: contentLayout
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: "Issues"
                font.bold: true
                color: "#b71c1c"
            }

            Label {
                text: "(" + root.messages.length + ")"
                color: "#c62828"
            }

            Item {
                Layout.fillWidth: true
            }

            ToolButton {
                text: root.expanded ? "Collapse" : "Expand"
                onClicked: root.expanded = !root.expanded
            }

            ToolButton {
                text: "Clear"
                enabled: root.canClearRuntime
                onClicked: root.clearRequested()
            }
        }

        Item {
            Layout.fillWidth: true
            implicitHeight: root.expanded ? Math.min(260, issueList.contentHeight + 8) : 28

            ListView {
                id: issueList
                anchors.fill: parent
                clip: true
                spacing: 4
                model: root.messages
                interactive: root.expanded

                delegate: Rectangle {
                    required property string modelData
                    width: issueList.width
                    height: issueText.implicitHeight + 6
                    color: "transparent"

                    Row {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        spacing: 6

                        Text {
                            text: "•"
                            color: "#b71c1c"
                            font.bold: true
                        }

                        Text {
                            id: issueText
                            width: issueList.width - 20
                            text: modelData
                            color: "#4e342e"
                            wrapMode: root.expanded ? Text.WordWrap : Text.NoWrap
                            maximumLineCount: root.expanded ? 0 : 1
                            elide: Text.ElideRight
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }
    }
}
