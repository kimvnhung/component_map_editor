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
    property var componentTypeRegistry: null

    signal paletteDropRequested(string title, string icon, string color, string type, point scenePos)

    color: "#ffffff"
    border.color: "#e0e0e0"
    border.width: 1

    readonly property var componentTypes: componentTypeRegistry ? componentTypeRegistry.componentTypeDescriptors : []

    // Tracks next auto-generated id suffix
    property int _idCounter: 1

    function _applyDefaultProperties(component, type) {
        if (!componentTypeRegistry || !component)
            return;
        var defaults = componentTypeRegistry.defaultComponentProperties(type);
        for (var key in defaults)
            component.setDynamicProperty(key, defaults[key]);
    }

    function _addComponent(descriptor) {
        if (!graph || !descriptor)
            return;
        var title = descriptor.title || descriptor.id || "Component";
        var icon = descriptor.icon || "";
        var color = descriptor.defaultColor || "#4fc3f7";
        var type = descriptor.id || descriptor.type || "default";
        var defaultWidth = Number(descriptor.defaultWidth || 96);
        var defaultHeight = Number(descriptor.defaultHeight || 96);

        if (root.canvas && root.canvas.componentRenderer)
            root.canvas.componentRenderer.renderComponents = true;

        var component = Qt.createQmlObject('import ComponentMapEditor; ComponentModel {}', graph);
        component.id = "component_" + root._idCounter++;
        component.title = title;
        component.content = "";
        component.icon = icon;
        component.color = color;
        component.type = type;
        component.width = defaultWidth;
        component.height = defaultHeight;
        root._applyDefaultProperties(component, type);

        // Place new nodes at viewport center in world space so they are always visible.
        if (root.canvas && root.canvas.viewToWorld) {
            var centerWorld = root.canvas.viewToWorld(root.canvas.width * 0.5, root.canvas.height * 0.5);
            component.x = centerWorld.x;
            component.y = centerWorld.y;
        } else {
            component.x = 0;
            component.y = 0;
        }

        if (root.undoStack)
            root.undoStack.pushAddComponent(graph, component);
        else
            graph.addComponent(component);
    }

    function _handlePaletteDrop(descriptor, scenePos) {
        var title = descriptor ? (descriptor.title || descriptor.id || "Component") : "Component";
        var icon = descriptor ? (descriptor.icon || "") : "";
        var color = descriptor ? (descriptor.defaultColor || "#4fc3f7") : "#4fc3f7";
        var type = descriptor ? (descriptor.id || descriptor.type || "default") : "default";

        if (root.canvas && root.canvas.addPaletteComponentAtScenePos) {
            const created = root.canvas.addPaletteComponentAtScenePos(title, icon, color, type, scenePos);
            if (created)
                return;
        }

        // Fallback: if drop cannot be mapped into canvas, keep previous behavior.
        root._addComponent(descriptor);
    }

    onPaletteDropRequested: function (title, icon, color, type, scenePos) {
        root._handlePaletteDrop({
            "title": title,
            "icon": icon,
            "defaultColor": color,
            "id": type
        }, scenePos);
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
                color: modelData.defaultColor || "#4fc3f7"

                Text {
                    anchors.centerIn: parent
                    text: modelData.title || modelData.id
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
                        return parent.mapToItem(null, centroid.position.x, centroid.position.y);
                    }

                    onActiveChanged: {
                        if (!active)
                            root.paletteDropRequested(modelData.title || modelData.id, modelData.icon || "", modelData.defaultColor || "#4fc3f7", modelData.id || modelData.type, scenePos());
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    onTapped: root._addComponent(modelData)
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
