// Palette.qml — Sidebar listing available component types.
// Dragging an item (or clicking "Add") creates a new component on the canvas.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ComponentMapEditor

Rectangle {
    id: root

    property GraphModel graph: null
    property UndoStack undoStack: null
    property var canvas: null

    signal paletteDropRequested(string title,
                                string icon,
                                string color,
                                string type,
                                point scenePos)

    color: "#ffffff"
    border.color: "#e0e0e0"
    border.width: 1

    readonly property var componentTypes: [{
            "title": "Start",
            "icon": "play",
            "color": "#26a69a",
            "type": "start"
        }, {
            "title": "Process",
            "icon": "gears",
            "color": "#5c6bc0",
            "type": "process"
        }, {
            "title": "Stop",
            "icon": "stop",
            "color": "#ef5350",
            "type": "stop"
        }]

    // Tracks next auto-generated id suffix
    property int _idCounter: 1

    function _addComponent(title, icon, color, type) {
        if (!graph)
            return

        if (root.canvas && root.canvas.nodeRenderer)
            root.canvas.nodeRenderer.renderNodes = true

        var component = Qt.createQmlObject(
                    'import ComponentMapEditor; ComponentModel {}', graph)
        component.id = "component_" + root._idCounter++
        component.title = title
        component.content = ""
        component.icon = icon
        component.color = color
        component.type = type
        component.width = 96
        component.height = 96

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

        if (root.undoStack)
            root.undoStack.pushAddComponent(graph, component)
        else
            graph.addComponent(component)
    }

    function _handlePaletteDrop(title, icon, color, type, scenePos) {
        if (root.canvas && root.canvas.addPaletteComponentAtScenePos) {
            const created = root.canvas.addPaletteComponentAtScenePos(title,
                                                                       icon,
                                                                       color,
                                                                       type,
                                                                       scenePos)
            if (created)
                return
        }

        // Fallback: if drop cannot be mapped into canvas, keep previous behavior.
        root._addComponent(title, icon, color, type)
    }

    onPaletteDropRequested: function(title, icon, color, type, scenePos) {
        root._handlePaletteDrop(title, icon, color, type, scenePos)
    }

    ColumnLayout {
        anchors {
            fill: parent
            margins: 8
        }
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
                    text: modelData.title
                    color: "white"
                    font.bold: true
                    font.pixelSize: 13
                }

                HoverHandler {
                    cursorShape: paletteDrag.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                }

                DragHandler {
                    id: paletteDrag
                    target: null
                    acceptedButtons: Qt.LeftButton
                    dragThreshold: 4
                    cursorShape: active ? Qt.ClosedHandCursor : Qt.OpenHandCursor

                    function scenePos() {
                        return parent.mapToItem(null,
                                                centroid.position.x,
                                                centroid.position.y)
                    }

                    onActiveChanged: {
                        if (!active)
                            root.paletteDropRequested(modelData.title,
                                                      modelData.icon,
                                                      modelData.color,
                                                      modelData.type,
                                                      scenePos())
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    onTapped: root._addComponent(modelData.title,
                                                 modelData.icon,
                                                 modelData.color,
                                                 modelData.type)
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
