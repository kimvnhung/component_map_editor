// PropertyPanel.qml — Shows and edits properties of the currently selected
// node or edge.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ComponentMapEditor

Rectangle {
    id: root

    property NodeModel node: null
    property EdgeModel edge: null

    color: "#ffffff"
    border.color: "#e0e0e0"
    border.width: 1

    ColumnLayout {
        anchors { fill: parent; margins: 10 }
        spacing: 8

        Label {
            text: "Properties"
            font.bold: true
            font.pixelSize: 13
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            bottomPadding: 4
        }

        // ---------- Node properties ----------
        Loader {
            active: root.node !== null
            Layout.fillWidth: true
            sourceComponent: nodeProps
        }

        Component {
            id: nodeProps

            ColumnLayout {
                spacing: 6
                width: parent ? parent.width : 0

                Label { text: "Node"; font.bold: true; font.pixelSize: 11; color: "#888" }

                Label { text: "ID" }
                TextField {
                    Layout.fillWidth: true
                    text: root.node ? root.node.id : ""
                    onEditingFinished: if (root.node) root.node.id = text
                }

                Label { text: "Label" }
                TextField {
                    Layout.fillWidth: true
                    text: root.node ? root.node.label : ""
                    onEditingFinished: if (root.node) root.node.label = text
                }

                Label { text: "Color" }
                TextField {
                    Layout.fillWidth: true
                    text: root.node ? root.node.color : ""
                    onEditingFinished: if (root.node) root.node.color = text
                }

                Label { text: "Type" }
                TextField {
                    Layout.fillWidth: true
                    text: root.node ? root.node.type : ""
                    onEditingFinished: if (root.node) root.node.type = text
                }

                Label { text: "X" }
                SpinBox {
                    Layout.fillWidth: true
                    from: -9999; to: 9999
                    value: root.node ? Math.round(root.node.x) : 0
                    onValueModified: if (root.node) root.node.x = value
                }

                Label { text: "Y" }
                SpinBox {
                    Layout.fillWidth: true
                    from: -9999; to: 9999
                    value: root.node ? Math.round(root.node.y) : 0
                    onValueModified: if (root.node) root.node.y = value
                }
            }
        }

        // ---------- Edge properties ----------
        Loader {
            active: root.edge !== null
            Layout.fillWidth: true
            sourceComponent: edgeProps
        }

        Component {
            id: edgeProps

            ColumnLayout {
                spacing: 6
                width: parent ? parent.width : 0

                Label { text: "Edge"; font.bold: true; font.pixelSize: 11; color: "#888" }

                Label { text: "ID" }
                TextField {
                    Layout.fillWidth: true
                    text: root.edge ? root.edge.id : ""
                    onEditingFinished: if (root.edge) root.edge.id = text
                }

                Label { text: "Source" }
                TextField {
                    Layout.fillWidth: true
                    text: root.edge ? root.edge.sourceId : ""
                    onEditingFinished: if (root.edge) root.edge.sourceId = text
                }

                Label { text: "Target" }
                TextField {
                    Layout.fillWidth: true
                    text: root.edge ? root.edge.targetId : ""
                    onEditingFinished: if (root.edge) root.edge.targetId = text
                }

                Label { text: "Label" }
                TextField {
                    Layout.fillWidth: true
                    text: root.edge ? root.edge.label : ""
                    onEditingFinished: if (root.edge) root.edge.label = text
                }
            }
        }

        // Placeholder when nothing is selected
        Label {
            visible: root.node === null && root.edge === null
            text: "Select a node or edge\nto view its properties."
            wrapMode: Text.WordWrap
            color: "#aaa"
            font.pixelSize: 12
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        Item { Layout.fillHeight: true }
    }
}
