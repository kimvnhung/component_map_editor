// Palette.qml — Sidebar listing available component types.
// Dragging an item (or clicking "Add") creates a new component on the canvas.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ComponentMapEditor

Rectangle {
    id: root

    property GraphModel graph: null
    property UndoStack undoStack: UndoStack {}
    property GraphEditorController graphEditorController: GraphEditorController {
        graph: root.graph
        undoStack: root.undoStack
        typeRegistry: root.componentTypeRegistry
    }
    property var canvas: null
    property var componentTypeRegistry: null
    property bool legacyDropFallbackEnabled: false
    property bool interactionTelemetryEnabled: true

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

    function _timestampMs() {
        return Date.now();
    }

    function _recordActionLatency(ms) {
        if (!root.interactionTelemetryEnabled || !root.canvas || !root.canvas.connectionRenderer || !root.canvas.connectionRenderer.recordActionLatencySample)
            return;
        root.canvas.connectionRenderer.recordActionLatencySample(ms);
    }

    function _runAction(actionFn) {
        var startMs = root._timestampMs();
        var result = actionFn();
        root._recordActionLatency(root._timestampMs() - startMs);
        return result;
    }

    // Palette gesture states for one-gesture/one-action arbitration.
    readonly property int gestureIdle: 0
    readonly property int gesturePressed: 1
    readonly property int gestureDragging: 2
    readonly property int gestureResolved: 3
    readonly property int gestureCanceled: 4

    // Gesture outcomes are resolved by a single dispatch path.
    readonly property int gestureActionNone: 0
    readonly property int gestureActionTapCreate: 1
    readonly property int gestureActionDragDropCreate: 2

    function _addComponent(descriptor) {
        if (!graph || !descriptor || !root.graphEditorController)
            return;
        var title = descriptor.title || descriptor.id || "Component";
        var icon = descriptor.icon || "";
        var color = descriptor.defaultColor || "#4fc3f7";
        var type = descriptor.id || descriptor.type || "default";
        var defaultWidth = Number(descriptor.defaultWidth || 96);
        var defaultHeight = Number(descriptor.defaultHeight || 96);

        if (root.canvas && root.canvas.componentRenderer)
            root.canvas.componentRenderer.renderComponents = true;

        var worldX = 0;
        var worldY = 0;

        // Place new nodes at viewport center in world space so they are always visible.
        if (root.canvas && root.canvas.viewToWorld) {
            var centerWorld = root.canvas.viewToWorld(root.canvas.width * 0.5, root.canvas.height * 0.5);
            worldX = centerWorld.x;
            worldY = centerWorld.y;
        }

        var componentId = root._runAction(function () {
            return root.graphEditorController.createPaletteComponent(type,
                                                                     title,
                                                                     icon,
                                                                     color,
                                                                     worldX,
                                                                     worldY,
                                                                     defaultWidth,
                                                                     defaultHeight);
        });
        if (!componentId || componentId.length === 0)
            return false;

        return true;
    }

    function _setPaletteDragLifecycle(active) {
        if (!root.canvas)
            return;
        if (active) {
            if (root.canvas.beginPaletteDrag)
                root.canvas.beginPaletteDrag();
            else if (root.canvas.paletteDragInProgress !== undefined)
                root.canvas.paletteDragInProgress = true;
            return;
        }

        if (root.canvas.endPaletteDrag)
            root.canvas.endPaletteDrag();
        else if (root.canvas.paletteDragInProgress !== undefined)
            root.canvas.paletteDragInProgress = false;
    }

    function _setPaletteStrictMode(enabled) {
        if (root.graphEditorController && root.graphEditorController.strictMode !== undefined)
            root.graphEditorController.strictMode = enabled;
    }

    function _setPaletteInvariantChecker(checker) {
        if (root.graphEditorController && root.graphEditorController.invariantChecker !== undefined)
            root.graphEditorController.invariantChecker = checker;
    }

    onCanvasChanged: {
        if (!root.canvas)
            return;
        if (root.canvas.mutationStrictMode !== undefined)
            root._setPaletteStrictMode(root.canvas.mutationStrictMode);
        if (root.canvas.mutationInvariantChecker !== undefined)
            root._setPaletteInvariantChecker(root.canvas.mutationInvariantChecker);
    }

    function _descriptorFromModel(modelData) {
        return {
            "title": modelData ? (modelData.title || modelData.id || "Component") : "Component",
            "icon": modelData ? (modelData.icon || "") : "",
            "defaultColor": modelData ? (modelData.defaultColor || "#4fc3f7") : "#4fc3f7",
            "id": modelData ? (modelData.id || modelData.type || "default") : "default",
            "type": modelData ? (modelData.type || modelData.id || "default") : "default"
        };
    }

    function _resolveDropScenePos(primaryScenePos, fallbackScenePos) {
        if (primaryScenePos && !(primaryScenePos.x === 0 && primaryScenePos.y === 0))
            return primaryScenePos;
        if (fallbackScenePos && !(fallbackScenePos.x === 0 && fallbackScenePos.y === 0))
            return fallbackScenePos;
        return Qt.point(0, 0);
    }

    function _dispatchGestureAction(descriptor, action, primaryScenePos, fallbackScenePos) {
        if (action === root.gestureActionTapCreate)
            return root._addComponent(descriptor);

        if (action === root.gestureActionDragDropCreate) {
            var resolvedPos = root._resolveDropScenePos(primaryScenePos, fallbackScenePos);
            root._handlePaletteDrop(descriptor, resolvedPos);
            return true;
        }

        return false;
    }

    function _handlePaletteDrop(descriptor, scenePos) {
        var title = descriptor ? (descriptor.title || descriptor.id || "Component") : "Component";
        var icon = descriptor ? (descriptor.icon || "") : "";
        var color = descriptor ? (descriptor.defaultColor || "#4fc3f7") : "#4fc3f7";
        var type = descriptor ? (descriptor.id || descriptor.type || "default") : "default";

        if (root.canvas && root.canvas.addPaletteComponentAtScenePos) {
            const created = root._runAction(function () {
                return root.canvas.addPaletteComponentAtScenePos(title, icon, color, type, scenePos);
            });
            if (created)
                return;
        }

        // Fallback: if drop cannot be mapped into canvas, keep previous behavior.
        // This ensures a component is always created, even if drop placement fails.
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

                // One-gesture state machine. Exactly one terminal action
                // (tap-create, drag-drop-create, or canceled) is allowed.
                property int gestureState: root.gestureIdle
                // Last known cursor position in scene coordinates for this
                // drag gesture. Cached because centroid.position may reset.
                property point lastScenePos: Qt.point(0, 0)

                function descriptor() {
                    return root._descriptorFromModel(modelData);
                }

                function resetGestureCycle() {
                    gestureState = root.gesturePressed;
                    lastScenePos = Qt.point(0, 0);
                }

                function commitActionOnce(action, primaryScenePos, fallbackScenePos) {
                    if (gestureState === root.gestureResolved || gestureState === root.gestureCanceled)
                        return;
                    root._dispatchGestureAction(descriptor(), action, primaryScenePos, fallbackScenePos);
                    gestureState = root.gestureResolved;
                }

                function cancelGestureCycle() {
                    if (gestureState === root.gestureResolved)
                        return;
                    gestureState = root.gestureCanceled;
                }

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
                        parent.lastScenePos = scenePos();
                    }

                    onActiveChanged: {
                        if (active) {
                            parent.gestureState = root.gestureDragging;
                            root._setPaletteDragLifecycle(true);
                            updateLastScenePos();
                        } else {
                            root._setPaletteDragLifecycle(false);
                            if (parent.gestureState === root.gestureDragging)
                                parent.commitActionOnce(root.gestureActionDragDropCreate, parent.lastScenePos, scenePos());
                        }
                    }

                    onTranslationChanged: {
                        if (active)
                            updateLastScenePos();
                    }

                    onCanceled: {
                        root._setPaletteDragLifecycle(false);
                        parent.cancelGestureCycle();
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    onPressedChanged: {
                        if (pressed)
                            parent.resetGestureCycle();
                    }
                    onTapped: {
                        if (parent.gestureState === root.gesturePressed)
                            parent.commitActionOnce(root.gestureActionTapCreate, Qt.point(0, 0), Qt.point(0, 0));
                    }
                    onCanceled: {
                        // DragHandler activation can cancel TapHandler; ignore that
                        // path while actively dragging to avoid suppressing drop commit.
                        if (parent.gestureState === root.gesturePressed)
                            parent.cancelGestureCycle();
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
