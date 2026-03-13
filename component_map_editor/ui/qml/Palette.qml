// Palette.qml — Sidebar listing available component types.
// Dragging an item (or clicking "Add") creates a new component on the canvas.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ComponentMapEditor

Rectangle {
    id: root

    property GraphModel graph: null
    property UndoStack  undoStack: null
    property var canvas: null

    color: "#ffffff"
    border.color: "#e0e0e0"
    border.width: 1

    readonly property var componentTypes: [
        { label: "Start",   color: "#26a69a", type: "start"   },
        { label: "Process", color: "#5c6bc0", type: "process" },
        { label: "Decision",color: "#ab47bc", type: "decision"},
        { label: "End",     color: "#ef5350", type: "end"     },
    ]

    // Tracks next auto-generated id suffix
    property int _idCounter: 1

    function _addComponent(label, color, type) {
        if (!graph) return

        if (root.canvas && root.canvas.nodeRenderer)
            root.canvas.nodeRenderer.renderNodes = true

        var component = Qt.createQmlObject(
            'import ComponentMapEditor; ComponentModel {}', graph)
        component.id    = "component_" + root._idCounter++
        component.label = label
        component.color = color
        component.type  = type

        // Place new nodes at viewport center in world space so they are always visible.
        if (root.canvas && root.canvas.viewToWorld) {
            var centerWorld = root.canvas.viewToWorld(root.canvas.width * 0.5,
                                                      root.canvas.height * 0.5)
            component.x = centerWorld.x
            component.y = centerWorld.y
        } else {
            component.x = 0
            component.y = 0
        }

        // AddComponentCommand (C++) can be used by callers wanting undoable adds.
        graph.addComponent(component)
    }

    ColumnLayout {
        anchors { fill: parent; margins: 8 }
        spacing: 6

        Label {
            text: "Component Palette"
            font.bold: true
            font.pixelSize: 13
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            bottomPadding: 4
        }

        Repeater {
            model: root.componentTypes

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

                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    onTapped: root._addComponent(modelData.label,
                                                modelData.color,
                                                modelData.type)
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
