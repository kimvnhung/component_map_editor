// Palette.qml — Sidebar listing available node types.
// Dragging an item (or clicking "Add") creates a new node on the canvas.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import JobDesigner

Rectangle {
    id: root

    property GraphModel graph: null
    property UndoStack  undoStack: null

    color: "#ffffff"
    border.color: "#e0e0e0"
    border.width: 1

    readonly property var nodeTypes: [
        { label: "Start",   color: "#26a69a", type: "start"   },
        { label: "Process", color: "#5c6bc0", type: "process" },
        { label: "Decision",color: "#ab47bc", type: "decision"},
        { label: "End",     color: "#ef5350", type: "end"     },
    ]

    // Tracks next auto-generated id suffix
    property int _idCounter: 1

    function _addNode(label, color, type) {
        if (!graph) return
        var node = Qt.createQmlObject(
            'import JobDesigner; NodeModel {}', graph)
        node.id    = "node_" + root._idCounter++
        node.label = label
        node.color = color
        node.type  = type
        node.x     = 150 + Math.random() * 200
        node.y     = 100 + Math.random() * 200
        // AddNodeCommand (C++) can be used by callers wanting undoable adds.
        graph.addNode(node)
    }

    ColumnLayout {
        anchors { fill: parent; margins: 8 }
        spacing: 6

        Label {
            text: "Node Palette"
            font.bold: true
            font.pixelSize: 13
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            bottomPadding: 4
        }

        Repeater {
            model: root.nodeTypes

            delegate: Rectangle {
                required property var modelData

                Layout.fillWidth: true
                height: 36
                radius: 6
                color: modelData.color

                Text {
                    anchors.centerIn: parent
                    text: modelData.label
                    color: "white"
                    font.bold: true
                    font.pixelSize: 13
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root._addNode(modelData.label,
                                            modelData.color,
                                            modelData.type)
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
