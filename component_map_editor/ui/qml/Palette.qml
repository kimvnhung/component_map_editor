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

    property var componentTypes: []
    onComponentTypeRegistryChanged: {
        if (componentTypeRegistry) {
            root.componentTypes = componentTypeRegistry.componentTypeDescriptors;
        } else {
            root.componentTypes = [];
        }
    }

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

                // Guard that distinguishes a drag gesture from a tap within the
                // same press cycle.  DragHandler uses target:null, so Qt Quick
                // may not always issue an exclusive grab and cancel the sibling
                // TapHandler.  Without this flag both handlers would fire on a
                // drag, creating one component at the drop position (via
                // addPaletteComponentAtScenePos) and a second one at the canvas
                // centre (via _addComponent).
                property bool _didDrag: false
                // Ensures one press gesture can produce at most one create action
                // even if DragHandler toggles active more than once.
                property bool _creationHandledInPress: false
                // Last known cursor position in scene coordinates for this
                // drag gesture. We cache this because centroid.position may be
                // stale/reset when DragHandler transitions to inactive.
                property point _lastScenePos: Qt.point(0, 0)

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
                        // Derive pointer scene position from stable drag state.
                        // centroid.position can be stale/reset at release on some
                        // grab transitions, which misplaces drop coordinates.
                        var px = centroid.pressPosition.x + translation.x;
                        var py = centroid.pressPosition.y + translation.y;
                        return parent.mapToItem(null, px, py);
                    }

                    function updateLastScenePos() {
                        parent._lastScenePos = scenePos();
                    }

                    onActiveChanged: {
                        if (active) {
                            parent._didDrag = true;
                            if (root.canvas && root.canvas.paletteDragInProgress !== undefined)
                                root.canvas.paletteDragInProgress = true;
                            updateLastScenePos();
                        } else {
                            if (parent._didDrag && !parent._creationHandledInPress) {
                                parent._creationHandledInPress = true;
                                var dropPos = parent._lastScenePos;
                                if (dropPos.x === 0 && dropPos.y === 0)
                                    dropPos = scenePos();
                                root.paletteDropRequested(modelData.title || modelData.id, modelData.icon || "", modelData.defaultColor || "#4fc3f7", modelData.id || modelData.type, dropPos);
                            }
                            if (root.canvas && root.canvas.paletteDragInProgress !== undefined)
                                root.canvas.paletteDragInProgress = false;
                        }
                    }

                    onTranslationChanged: {
                        if (active)
                            updateLastScenePos();
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    onPressedChanged: {
                        // Reset the drag flag at the start of every new press so
                        // a subsequent tap after a previous drag is not blocked.
                        if (pressed) {
                            parent._didDrag = false;
                            parent._creationHandledInPress = false;
                            parent._lastScenePos = Qt.point(0, 0);
                        }
                    }
                    onTapped: {
                        // Only create a component for genuine taps (no preceding
                        // drag in this press cycle).
                        if (!parent._didDrag && !parent._creationHandledInPress) {
                            parent._creationHandledInPress = true;
                            root._addComponent(modelData);
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
