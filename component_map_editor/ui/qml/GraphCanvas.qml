// GraphCanvas.qml — The main editing surface.
// Displays a grid background, draws connections imperatively via a Canvas item,
// and renders components as interactive ComponentItem delegates.
import QtQuick
import QtQuick.Controls
import ComponentMapEditor
import QmlFontAwesome
import "components"

Item {
    id: root
    focus: true
    activeFocusOnTab: true

    // Camera tuning constants (world <-> screen transform behavior).
    readonly property real defaultZoom: 1.0

    // Threshold for how many components we eagerly instantiate interaction
    // delegates for. Below this, all Loader delegates are kept active so
    // DragHandler instances see the initial press. Above this, we fall back
    // to a more selective activation strategy to avoid excessive memory/CPU.
    property int maxEagerComponentDelegates: 1500
    readonly property real zoomStepFactor: 1.15
    readonly property real zoomEpsilon: 0.000001

    // Grid tuning constants.
    readonly property real baseGridStep: 30
    readonly property real minGridPixelStep: 16
    readonly property real maxGridPixelStep: 96

    // Interaction thresholds.
    readonly property real panStartThreshold: 3

    property GraphModel graph: null
    // Selection and interaction mode are owned by interactionState below.
    // These aliases keep GraphCanvas's public API intact for external callers.
    property alias selectedComponent: interactionState.primaryComponent
    property alias selectedComponentIds: interactionState.componentIds
    property alias selectedConnection: interactionState.connection
    property alias selectedConnectionIds: interactionState.connectionIds
    property UndoStack undoStack: null
    property ComponentModel menuTargetComponent: null
    property ConnectionModel menuTargetConnection: null
    property point menuTargetWorldPos: Qt.point(0, 0)
    property string pendingConfirmAction: ""
    property string pendingConfirmMessage: ""

    // Temp connection preview — driven solely by interactionState transitions.
    readonly property alias tempStart: interactionState.tempStart
    readonly property alias tempEnd: interactionState.tempEnd
    readonly property alias tempConnectionDragging: interactionState.connectionDragging

    property real zoom: defaultZoom
    property real minZoom: 0.35
    property real maxZoom: 3.0
    property real panX: 0
    property real panY: 0
    // Interaction-mode derived state — mutated only through interactionState transitions.
    readonly property alias enableBackgroundDrag: interactionState.backgroundDragEnabled
    readonly property alias nodeInteractionActive: interactionState.nodeInteractionActive
    property bool pointerOverComponent: false
    property ComponentModel hoveredComponent: null
    // pressedComponent: transient per-pointer-event state only; not persisted interaction state.
    property ComponentModel pressedComponent: null
    readonly property alias activeInteractionComponent: interactionState.interactionTarget
    property alias suppressNextCanvasTap: interactionState.suppressNextTap
    readonly property var nodeRenderer: nodeViewport
    readonly property var edgeRenderer: edgeViewport
    property point mouseViewPos: Qt.point(0, 0)
    property point mouseWorldPos: Qt.point(0, 0)
    property int livePointerModifiers: 0
    property bool ctrlSelectionModifierActive: false
    property bool ctrlReleasedByKey: false  // Flag to prevent stale modifiers from re-enabling Ctrl
    property bool debugInputLogs: false
    // Group-move scratch state (set while dragging one selected component).
    property bool groupMoveActive: false
    property point groupMoveAnchorStart: Qt.point(0, 0)
    property var groupMoveBaseCenters: ({})
    property var moveStartPositions: ({})
    property var resizeStartGeometries: ({})

    // Optional telemetry hook. Set to a PerformanceTelemetry instance to
    // collect camera-update and drag-event interval samples for Phase 0 baseline.
    // Leave null (default) for zero overhead in production.
    property PerformanceTelemetry telemetry: null

    signal componentSelected(ComponentModel component)
    signal connectionSelected(ConnectionModel connection)
    signal backgroundClicked(real x, real y)
    signal viewTransformChanged(real panX, real panY, real zoom)

    function syncCtrlModifierState(modifiers) {
        var next = (modifiers & Qt.ControlModifier) !== 0
        // Don't allow modifiers to re-enable Ctrl immediately after keyboard release
        if (root.ctrlReleasedByKey && next && !root.ctrlSelectionModifierActive)
            return
        if (root.ctrlSelectionModifierActive === next)
            return
        root.ctrlSelectionModifierActive = next
        // End any in-progress marquee when Ctrl just turned off.
        if (!next && interactionState.marqueeSelecting)
            root.finishMarqueeSelection()
        root.debugInputLog("ctrl_state_changed")
    }

    function formatPoint(pointValue) {
        return "(" + pointValue.x.toFixed(1) + "," + pointValue.y.toFixed(1) + ")"
    }

    function debugInputLog(reason) {
        if (!root.debugInputLogs)
            return

        var rect = root.normalizedViewRect(interactionState.marqueeStart,
                                           interactionState.marqueeEnd)
        var pressedId = root.pressedComponent ? root.pressedComponent.id : "<none>"
        console.log("[GraphCanvas][input]", reason,
                    "mouseView=", root.formatPoint(root.mouseViewPos),
                    "mouseWorld=", root.formatPoint(root.mouseWorldPos),
                    "ctrl=", root.ctrlSelectionModifierActive,
                    "mods=", root.livePointerModifiers,
                    "pressed=", pressedId,
                    "marquee=", interactionState.marqueeSelecting,
                    "start=", root.formatPoint(interactionState.marqueeStart),
                    "end=", root.formatPoint(interactionState.marqueeEnd),
                    "rectX=", rect.left.toFixed(1),
                    "rectY=", rect.top.toFixed(1),
                    "rectW=", rect.width.toFixed(1),
                    "rectH=", rect.height.toFixed(1))
    }

    function componentIsSelected(component) {
        return interactionState.isSelected(component)
    }

    function removeComponentFromSelection(component) {
        if (!component || !component.id)
            return
        interactionState.removeComponentId(component.id)
    }

    function clearComponentSelection() {
        interactionState.clearComponents()
    }

    function selectSingleComponent(component) {
        interactionState.selectSingle(component)
    }

    function selectConnection(connection) {
        interactionState.selectConnectionModel(connection)
        if (connection)
            root.connectionSelected(connection)
    }

    function handleLeftComponentClick(component, modifiers) {
        interactionState.handleComponentClick(component, modifiers,
                                              root.graph, root.livePointerModifiers)
        if (interactionState.primaryComponent)
            root.componentSelected(interactionState.primaryComponent)
        else
            root.backgroundClicked(root.mouseWorldPos.x, root.mouseWorldPos.y)
        edgeCanvas.repaint()
    }

    // Resets all interaction mode and selection in one call.
    // Use this instead of directly writing to the readonly interaction properties.
    function resetAllState() {
        interactionState.resetInteraction()
        interactionState.clearAll()
    }

    function uniqueComponentId(baseId) {
        var prefix = (baseId && baseId.length > 0) ? baseId : "component"
        var candidate = prefix
        var suffix = 1
        while (root.graph && root.graph.componentById(candidate)) {
            candidate = prefix + "_" + suffix
            suffix += 1
        }
        return candidate
    }

    function uniqueConnectionId(baseId) {
        var prefix = (baseId && baseId.length > 0) ? baseId : "connection"
        var candidate = prefix
        var suffix = 1
        while (root.graph && root.graph.connectionById(candidate)) {
            candidate = prefix + "_" + suffix
            suffix += 1
        }
        return candidate
    }

    function removeConnectionById(connectionId, useUndo) {
        if (!root.graph || !connectionId)
            return

        var shouldUseUndo = useUndo === undefined ? true : useUndo
        if (shouldUseUndo && root.undoStack)
            root.undoStack.pushRemoveConnection(root.graph, connectionId)
        else
            root.graph.removeConnection(connectionId)

        if (root.selectedConnection && root.selectedConnection.id === connectionId)
            root.selectedConnection = null
        if (root.selectedConnectionIds.indexOf(connectionId) !== -1)
            interactionState.pruneStaleConnection(root.graph)
    }

    function clearComponentConnections(component, clearIncoming, clearOutgoing, useUndo) {
        if (!root.graph || !component)
            return

        var shouldUseUndo = useUndo === undefined ? true : useUndo

        var idsToRemove = []
        var currentConnections = root.graph.connections
        for (var i = 0; i < currentConnections.length; ++i) {
            var connection = currentConnections[i]
            var shouldRemove = (clearIncoming && connection.targetId === component.id)
                    || (clearOutgoing && connection.sourceId === component.id)
            if (shouldRemove)
                idsToRemove.push(connection.id)
        }

        for (var j = 0; j < idsToRemove.length; ++j)
            root.removeConnectionById(idsToRemove[j], shouldUseUndo)

        edgeCanvas.repaint()
    }

    function deleteComponent(component) {
        if (!root.graph || !component)
            return

        if (root.undoStack) {
            root.undoStack.pushRemoveComponent(root.graph, component.id)
        } else {
            root.clearComponentConnections(component, true, true, false)
            root.graph.removeComponent(component.id)
        }
        root.removeComponentFromSelection(component)
        if (root.selectedComponentIds.length > 0) {
            var lastId = root.selectedComponentIds[root.selectedComponentIds.length - 1]
            root.selectedComponent = root.graph.componentById(lastId)
        } else {
            root.selectedComponent = null
        }
        if (!root.selectedComponent)
            root.backgroundClicked(root.mouseWorldPos.x, root.mouseWorldPos.y)
        edgeCanvas.repaint()
    }

    function duplicateComponent(component) {
        if (!root.graph || !component)
            return

        var copy = Qt.createQmlObject(
                    'import ComponentMapEditor; ComponentModel {}',
                    root.graph)
        copy.id = root.uniqueComponentId(component.id + "_copy")
        copy.title = component.title + " Copy"
        copy.content = component.content
        copy.icon = component.icon
        copy.x = component.x + 40
        copy.y = component.y + 40
        copy.width = component.width
        copy.height = component.height
        copy.shape = component.shape
        copy.color = component.color
        copy.type = component.type
        if (root.undoStack)
            root.undoStack.pushAddComponent(root.graph, copy)
        else
            root.graph.addComponent(copy)
        root.selectSingleComponent(copy)
        root.selectedConnection = null
        root.componentSelected(copy)
        edgeCanvas.repaint()
    }

    function addComponentAtWorldPos(worldPos) {
        if (!root.graph)
            return

        var component = Qt.createQmlObject(
                    'import ComponentMapEditor; ComponentModel {}', root.graph)
        component.id = root.uniqueComponentId("component")
        component.title = "Component"
        component.content = ""
        component.icon = "cube"
        component.x = worldPos.x
        component.y = worldPos.y
        component.width = 96
        component.height = 96
        component.color = "#4fc3f7"
        component.shape = "rounded"
        component.type = "default"
        if (root.undoStack)
            root.undoStack.pushAddComponent(root.graph, component)
        else
            root.graph.addComponent(component)
        root.selectSingleComponent(component)
        root.selectedConnection = null
        root.componentSelected(component)
        edgeCanvas.repaint()
    }

    function addPaletteComponentAtScenePos(title, icon, color, type, scenePos) {
        if (!root.graph || !scenePos)
            return false

        var viewPos = root.mapFromItem(null, scenePos.x, scenePos.y)
        if (viewPos.x < 0 || viewPos.x > root.width || viewPos.y < 0 || viewPos.y > root.height)
            return false

        var worldPos = root.viewToWorld(viewPos.x, viewPos.y)

        var component = Qt.createQmlObject(
                    'import ComponentMapEditor; ComponentModel {}', root.graph)
        component.id = root.uniqueComponentId("component")
        component.title = title && title.length > 0 ? title : "Component"
        component.content = ""
        component.icon = icon && icon.length > 0 ? icon : "cube"
        component.x = worldPos.x
        component.y = worldPos.y
        component.width = 96
        component.height = 96
        component.color = color && color.length > 0 ? color : "#4fc3f7"
        component.shape = "rounded"
        component.type = type && type.length > 0 ? type : "default"

        if (root.undoStack)
            root.undoStack.pushAddComponent(root.graph, component)
        else
            root.graph.addComponent(component)
        root.selectSingleComponent(component)
        root.selectedConnection = null
        root.componentSelected(component)
        edgeCanvas.repaint()
        return true
    }

    function clearAllConnections() {
        if (!root.graph)
            return

        var ids = []
        var currentConnections = root.graph.connections
        for (var i = 0; i < currentConnections.length; ++i)
            ids.push(currentConnections[i].id)

        for (var j = 0; j < ids.length; ++j)
            root.removeConnectionById(ids[j], true)

        root.selectedConnection = null
        root.selectedConnectionIds = []
        edgeCanvas.repaint()
    }

    function clearAllComponents() {
        if (!root.graph)
            return

        var ids = []
        var components = root.graph.components
        for (var i = 0; i < components.length; ++i)
            ids.push(components[i].id)

        if (root.undoStack) {
            for (var j = 0; j < ids.length; ++j)
                root.undoStack.pushRemoveComponent(root.graph, ids[j])
        } else {
            root.graph.clear()
        }
        interactionState.clearAll()
        root.backgroundClicked(root.mouseWorldPos.x, root.mouseWorldPos.y)
        edgeCanvas.repaint()
    }

    function commitMoveCommands(anchorComponent) {
        if (!root.undoStack || !root.graph || !anchorComponent)
            return

        var batchedMoves = []
        if (root.groupMoveActive) {
            for (var i = 0; i < root.selectedComponentIds.length; ++i) {
                var componentId = root.selectedComponentIds[i]
                var component = root.graph.componentById(componentId)
                var base = root.groupMoveBaseCenters[componentId]
                if (component && base) {
                    batchedMoves.push({
                                          "id": componentId,
                                          "oldX": base.x,
                                          "oldY": base.y,
                                          "newX": component.x,
                                          "newY": component.y
                                      })
                }
            }
        } else {
            var start = root.moveStartPositions[anchorComponent.id]
            if (start) {
                batchedMoves.push({
                                      "id": anchorComponent.id,
                                      "oldX": start.x,
                                      "oldY": start.y,
                                      "newX": anchorComponent.x,
                                      "newY": anchorComponent.y
                                  })
            }
        }

        if (batchedMoves.length > 0)
            root.undoStack.pushMoveComponents(root.graph, batchedMoves)

        var nextMoveStart = root.moveStartPositions
        delete nextMoveStart[anchorComponent.id]
        root.moveStartPositions = nextMoveStart
    }

    function startResizeCapture(component) {
        if (!component)
            return
        var nextResize = root.resizeStartGeometries
        nextResize[component.id] = {
            "x": component.x,
            "y": component.y,
            "width": component.width,
            "height": component.height
        }
        root.resizeStartGeometries = nextResize
    }

    function commitResizeCommand(component) {
        if (!component)
            return

        var start = root.resizeStartGeometries[component.id]
        if (start && root.undoStack) {
            root.undoStack.pushSetComponentGeometry(component,
                                                    start.x,
                                                    start.y,
                                                    start.width,
                                                    start.height,
                                                    component.x,
                                                    component.y,
                                                    component.width,
                                                    component.height)
        }

        var nextResize = root.resizeStartGeometries
        delete nextResize[component.id]
        root.resizeStartGeometries = nextResize
    }

    function openConfirm(action, message) {
        root.pendingConfirmAction = action
        root.pendingConfirmMessage = message
        confirmDialog.open()
    }

    function executePendingConfirm() {
        if (root.pendingConfirmAction === "clear_connections")
            root.clearAllConnections()
        else if (root.pendingConfirmAction === "clear_components")
            root.clearAllComponents()

        root.pendingConfirmAction = ""
        root.pendingConfirmMessage = ""
    }

    function viewToWorld(viewX, viewY) {
        if (nodeViewport)
            return nodeViewport.viewToWorld(viewX, viewY)
        return Qt.point(0, 0)
    }

    function worldToView(worldX, worldY) {
        if (nodeViewport)
            return nodeViewport.worldToView(worldX, worldY)
        return Qt.point(0, 0)
    }

    function updateMouseWorldPos() {
        root.mouseWorldPos = root.viewToWorld(root.mouseViewPos.x,
                                              root.mouseViewPos.y)
    }

    // Zooms around a view-space anchor and lets C++ compute the camera state,
    // keeping one authoritative camera math path.
    function zoomAtView(viewX, viewY, zoomFactor) {
        if (!nodeViewport)
            return

        var state = nodeViewport.zoomAtViewAnchor(viewX,
                                                  viewY,
                                                  zoomFactor,
                                                  minZoom,
                                                  maxZoom,
                                                  zoomEpsilon)
        if (!state.changed)
            return

        zoom = state.zoom
        panX = state.panX
        panY = state.panY
    }

    function zoomAtCursor(zoomFactor) {
        root.zoomAtView(mouseViewPos.x, mouseViewPos.y, zoomFactor)
    }

    function childToView(childItem, childX, childY) {
        if (!childItem)
            return Qt.point(0, 0)
        return childItem.mapToItem(root, childX, childY)
    }

    function windowSceneToView(windowScenePoint) {
        return root.mapFromItem(null, windowScenePoint.x, windowScenePoint.y)
    }

    function normalizedViewRect(p1, p2) {
        var left = Math.min(p1.x, p2.x)
        var right = Math.max(p1.x, p2.x)
        var top = Math.min(p1.y, p2.y)
        var bottom = Math.max(p1.y, p2.y)
        return {
            "left": left,
            "right": right,
            "top": top,
            "bottom": bottom,
            "width": right - left,
            "height": bottom - top
        }
    }

    function pointInRect(point, rect) {
        return point.x >= rect.left && point.x <= rect.right
                && point.y >= rect.top && point.y <= rect.bottom
    }

    function segmentIntersectsSegment(a, b, c, d) {
        function cross(p, q, r) {
            return (q.x - p.x) * (r.y - p.y) - (q.y - p.y) * (r.x - p.x)
        }

        var c1 = cross(a, b, c)
        var c2 = cross(a, b, d)
        var c3 = cross(c, d, a)
        var c4 = cross(c, d, b)
        return (c1 * c2 <= 0) && (c3 * c4 <= 0)
    }

    function segmentIntersectsRect(a, b, rect) {
        if (pointInRect(a, rect) || pointInRect(b, rect))
            return true

        var tl = Qt.point(rect.left, rect.top)
        var tr = Qt.point(rect.right, rect.top)
        var br = Qt.point(rect.right, rect.bottom)
        var bl = Qt.point(rect.left, rect.bottom)

        return segmentIntersectsSegment(a, b, tl, tr)
                || segmentIntersectsSegment(a, b, tr, br)
                || segmentIntersectsSegment(a, b, br, bl)
                || segmentIntersectsSegment(a, b, bl, tl)
    }

    function componentIntersectsViewRect(component, rect) {
        var halfW = component.width / 2
        var halfH = component.height / 2
        var topLeft = root.worldToView(component.x - halfW, component.y - halfH)
        var bottomRight = root.worldToView(component.x + halfW, component.y + halfH)
        var compRect = normalizedViewRect(topLeft, bottomRight)

        return !(compRect.right < rect.left || compRect.left > rect.right
                 || compRect.bottom < rect.top || compRect.top > rect.bottom)
    }

    function connectionIntersectsViewRect(connection, rect) {
        var source = root.graph ? root.graph.componentById(connection.sourceId) : null
        var target = root.graph ? root.graph.componentById(connection.targetId) : null
        if (!source || !target)
            return false

        var s = root.worldToView(source.x, source.y)
        var t = root.worldToView(target.x, target.y)
        return pointInRect(s, rect) || pointInRect(t, rect)
                || segmentIntersectsRect(s, t, rect)
    }

    function mergeUnique(baseIds, addIds) {
        var set = {}
        var merged = []
        for (var i = 0; i < baseIds.length; ++i) {
            var existing = baseIds[i]
            if (!set[existing]) {
                set[existing] = true
                merged.push(existing)
            }
        }
        for (var j = 0; j < addIds.length; ++j) {
            var next = addIds[j]
            if (!set[next]) {
                set[next] = true
                merged.push(next)
            }
        }
        return merged
    }

    function applyMarqueeSelection(viewStart, viewEnd, additive) {
        if (!root.graph)
            return

        var rect = normalizedViewRect(viewStart, viewEnd)
        if (rect.width < 2 && rect.height < 2)
            return

        var hitComponentIds = []
        var components = root.graph.components
        for (var i = 0; i < components.length; ++i) {
            var component = components[i]
            if (componentIntersectsViewRect(component, rect))
                hitComponentIds.push(component.id)
        }

        var hitConnectionIds = []
        var connections = root.graph.connections
        for (var k = 0; k < connections.length; ++k) {
            var connection = connections[k]
            if (connectionIntersectsViewRect(connection, rect))
                hitConnectionIds.push(connection.id)
        }

        var nextComponentIds = additive
                ? mergeUnique(root.selectedComponentIds, hitComponentIds)
                : hitComponentIds
        var nextConnectionIds = additive
                ? mergeUnique(root.selectedConnectionIds, hitConnectionIds)
                : hitConnectionIds

        root.selectedComponentIds = nextComponentIds
        root.selectedConnectionIds = nextConnectionIds
        root.selectedComponent = nextComponentIds.length > 0
                ? root.graph.componentById(nextComponentIds[nextComponentIds.length - 1])
                : null
        root.selectedConnection = nextConnectionIds.length > 0
                ? root.graph.connectionById(nextConnectionIds[nextConnectionIds.length - 1])
                : null

        if (root.selectedComponent)
            root.componentSelected(root.selectedComponent)
        else if (root.selectedConnection)
            root.connectionSelected(root.selectedConnection)
        else
            root.backgroundClicked(root.mouseWorldPos.x, root.mouseWorldPos.y)

        edgeCanvas.repaint()
    }

    function beginGroupMove(anchorComponent) {
        root.groupMoveActive = false
        root.groupMoveBaseCenters = ({})

        if (!anchorComponent || !root.graph)
            return
        if (!root.componentIsSelected(anchorComponent) || root.selectedComponentIds.length < 2)
            return

        root.groupMoveAnchorStart = Qt.point(anchorComponent.x, anchorComponent.y)
        var snapshot = {}
        for (var i = 0; i < root.selectedComponentIds.length; ++i) {
            var id = root.selectedComponentIds[i]
            var component = root.graph.componentById(id)
            if (component)
                snapshot[id] = Qt.point(component.x, component.y)
        }
        root.groupMoveBaseCenters = snapshot
        root.groupMoveActive = true
    }

    function updateGroupMove(anchorComponent) {
        if (!root.groupMoveActive || !anchorComponent || !root.graph)
            return

        var dx = anchorComponent.x - root.groupMoveAnchorStart.x
        var dy = anchorComponent.y - root.groupMoveAnchorStart.y
        for (var i = 0; i < root.selectedComponentIds.length; ++i) {
            var id = root.selectedComponentIds[i]
            if (id === anchorComponent.id)
                continue
            var component = root.graph.componentById(id)
            var base = root.groupMoveBaseCenters[id]
            if (component && base) {
                component.x = base.x + dx
                component.y = base.y + dy
            }
        }
    }

    function endGroupMove() {
        root.groupMoveActive = false
        root.groupMoveBaseCenters = ({})
    }

    function finishMarqueeSelection() {
        if (!interactionState.marqueeSelecting)
            return

        var rect = root.normalizedViewRect(interactionState.marqueeStart,
                                           interactionState.marqueeEnd)
        var draggedEnough = rect.width >= 2 || rect.height >= 2
        root.debugInputLog("marquee_finish")
        if (draggedEnough) {
            root.applyMarqueeSelection(interactionState.marqueeStart,
                                       interactionState.marqueeEnd,
                                       true)
            root.suppressNextCanvasTap = true
            root.debugInputLog("marquee_apply")
        }
        interactionState.endMarqueeSelect()
        root.debugInputLog("marquee_end")
    }

    // ── Interaction state manager ────────────────────────────────────────
    // Single source of truth for selection and interaction mode.  All
    // selection writes and mode transitions route through this object.
    InteractionStateManager {
        id: interactionState
    }

    clip: true

    // Background fill
    Rectangle {
        anchors.fill: parent
        color: "#f8f9fa"
    }

    // C++ viewport layers (Phase 2)
    // - gridViewport: background grid
    // - edgeViewport: connection lines + temp drag preview
    GraphViewportItem {
        id: gridViewport
        anchors.fill: parent
        graph: root.graph
        panX: root.panX
        panY: root.panY
        zoom: root.zoom
        renderGrid: true
        renderEdges: false
        baseGridStep: root.baseGridStep
        minGridPixelStep: root.minGridPixelStep
        maxGridPixelStep: root.maxGridPixelStep
    }

    GraphViewportItem {
        id: edgeViewport
        anchors.fill: parent
        z: 1
        graph: root.graph
        panX: root.panX
        panY: root.panY
        zoom: root.zoom
        renderGrid: false
        renderEdges: true
        selectedConnection: root.selectedConnection
        selectedConnectionIds: root.selectedConnectionIds
        tempConnectionDragging: root.tempConnectionDragging
        tempStart: Qt.point(root.tempStart.x, root.tempStart.y)
        tempEnd: Qt.point(root.tempEnd.x, root.tempEnd.y)
    }

    GraphViewportItem {
        id: nodeViewport
        anchors.fill: parent
        z: 0.2
        graph: root.graph
        panX: root.panX
        panY: root.panY
        zoom: root.zoom
        renderGrid: false
        renderEdges: false
        renderNodes: true
        selectedComponent: root.selectedComponent
        selectedComponentIds: root.selectedComponentIds
    }

    // Compatibility wrappers so existing repaint call sites remain intact.
    QtObject {
        id: gridCanvas
        function requestPaint() {
            gridViewport.repaint()
        }
    }

    QtObject {
        id: edgeCanvas
        function repaint() {
            edgeViewport.repaint()
        }
    }

    Item {
        id: interactionLayer
        anchors.fill: parent
        z: 1

        property real startPanX: 0
        property real startPanY: 0
        property bool dragStartedWithCtrl: false

        HoverHandler {
            id: canvasHover
            cursorShape: interactionState.marqueeSelecting || interactionLayer.dragStartedWithCtrl
                         ? Qt.CrossCursor
                         : (canvasDrag.active
                            ? Qt.ClosedHandCursor
                            : (root.ctrlSelectionModifierActive
                               ? Qt.CrossCursor
                               : (root.pointerOverComponent || root.enableBackgroundDrag
                                  ? Qt.OpenHandCursor
                                  : Qt.ArrowCursor)))
            onPointChanged: {
                // interactionLayer fills GraphCanvas at (0, 0), so the hover
                // point is already in GraphCanvas view coordinates.
                root.mouseViewPos = Qt.point(point.position.x,
                                             point.position.y)
                root.livePointerModifiers = point.modifiers
                root.syncCtrlModifierState(point.modifiers)

                // Keep marquee endpoint in sync with pointer movement even if
                // DragHandler does not become active for this gesture.
                if (interactionState.marqueeSelecting && root.ctrlSelectionModifierActive) {
                    interactionState.updateMarqueeSelect(
                                interactionState.marqueeStart,
                                root.mouseViewPos)
                }

                if (root.ctrlSelectionModifierActive || interactionState.marqueeSelecting)
                    root.debugInputLog("mouse_move")
            }
        }

        DragHandler {
            id: canvasDrag
            enabled: root.enableBackgroundDrag
                     && !root.nodeInteractionActive
                     && !root.ctrlSelectionModifierActive
                     && !root.pressedComponent
            target: null
            acceptedButtons: Qt.LeftButton
            dragThreshold: root.panStartThreshold

            onActiveChanged: {
                if (active) {
                    interactionLayer.startPanX = root.panX
                    interactionLayer.startPanY = root.panY
                    root.debugInputLog("drag_start_pan")
                    if (root.telemetry) root.telemetry.notifyDragStarted()
                } else {
                    if (root.telemetry)
                        root.telemetry.notifyDragEnded()
                    interactionLayer.dragStartedWithCtrl = false
                    root.debugInputLog("drag_end")
                }
            }

            onTranslationChanged: {
                root.panX = interactionLayer.startPanX + translation.x
                root.panY = interactionLayer.startPanY + translation.y
                if (root.telemetry) root.telemetry.notifyDragMoved()
            }
        }

        TapHandler {
            id: canvasTap
            acceptedButtons: Qt.LeftButton

            onPressedChanged: {
                if (pressed) {
                    root.forceActiveFocus()
                    root.livePointerModifiers = point.modifiers
                    root.syncCtrlModifierState(point.modifiers)
                    root.pressedComponent = edgeViewport.hitTestComponentAtView(point.position.x,
                                                                                 point.position.y)
                    root.debugInputLog("mouse_press")
                } else {
                    root.pressedComponent = null
                    root.syncCtrlModifierState(point.modifiers)
                    interactionLayer.dragStartedWithCtrl = false
                    root.debugInputLog("mouse_release")
                }
            }

            onTapped: point => {
                          if (root.suppressNextCanvasTap) {
                              root.suppressNextCanvasTap = false
                              return
                          }

                          var viewPos = Qt.point(point.position.x,
                                                 point.position.y)

                          var hitComponent = edgeViewport.hitTestComponentAtView(viewPos.x,
                                                                                  viewPos.y)
                          if (hitComponent) {
                              root.handleLeftComponentClick(hitComponent,
                                                            point.modifiers)
                              return
                          }

                          var hitConnection = edgeViewport.hitTestConnectionAtView(viewPos.x,
                                                                                    viewPos.y,
                                                                                    10.0)
                          if (hitConnection) {
                              root.selectConnection(hitConnection)
                              edgeCanvas.repaint()
                              return
                          }

                          root.clearComponentSelection()
                          root.selectedConnection = null
                          root.selectedConnectionIds = []
                          var worldPos = root.viewToWorld(point.position.x,
                                                          point.position.y)
                          root.backgroundClicked(worldPos.x, worldPos.y)
                          edgeCanvas.repaint()
                      }
        }

        TapHandler {
            acceptedButtons: Qt.RightButton
            onTapped: point => {
                          var viewPos = Qt.point(point.position.x,
                                                 point.position.y)
                          root.menuTargetWorldPos = root.viewToWorld(viewPos.x,
                                                                     viewPos.y)

                          root.menuTargetComponent = edgeViewport.hitTestComponentAtView(viewPos.x,
                                                                                           viewPos.y)
                          if (root.menuTargetComponent) {
                              componentMenu.popup(point.position.x,
                                                  point.position.y)
                              return
                          }

                          root.menuTargetConnection = edgeViewport.hitTestConnectionAtView(viewPos.x,
                                                                                             viewPos.y,
                                                                                             10.0)
                          if (root.menuTargetConnection) {
                              connectionMenu.popup(point.position.x,
                                                   point.position.y)
                              return
                          }

                          backgroundMenu.popup(point.position.x,
                                               point.position.y)
                      }
        }

        WheelHandler {
            onWheel: event => {
                         var factor = event.angleDelta.y
                         > 0 ? root.zoomStepFactor : 1 / root.zoomStepFactor

                         root.zoomAtCursor(factor)
                         event.accepted = true
                     }
        }
    }

    Item {
        id: componentLayer
        // componentLayer local coordinates are the canonical world drawing
        // coordinates before camera transform is applied.
        //
        // That means:
        // - a world point (wx, wy) maps to componentLayer local (wx, wy)
        // - delegates place their top-left corners in this space
        // - pan and zoom are applied by this layer's x/y/scale, not by
        //   changing child coordinates
        //
        // Model note:
        // - ComponentModel.x/y store component CENTER in world coordinates.
        // - ComponentItem.x/y store TOP-LEFT in componentLayer coordinates.
        // - Conversion is center <-> top-left only.
        width: parent.width
        height: parent.height
        x: root.panX
        y: root.panY
        scale: root.zoom
        transformOrigin: Item.TopLeft
        z: 2

        // Components
        Repeater {
            model: root.graph ? root.graph.components : []

            delegate: Item {
                id: delegateRoot
                required property var modelData
                property Item componentItem: itemLoader.item
                // Keep the Loader alive while connection arrows are shown.
                // This is imperative state (not a binding) to avoid a cycle
                // between componentItem <-> itemLoader.active.
                property bool keepAlive: false

                Loader {
                    id: itemLoader
                    // Keep interaction delegates instantiated so their
                    // DragHandler sees the initial press event. Lazily
                    // creating the delegate on hover/press breaks component
                    // dragging because the handler misses pointer-down.
                    //
                    // To avoid performance issues on very large graphs, we
                    // only keep all delegates eagerly active when the
                    // component count is below maxEagerComponentDelegates.
                    // For larger graphs we only activate delegates that are
                    // selected/kept-alive.
                    active: {
                        const graph = root.graph;
                        const components = graph ? graph.components : null;
                        // Some models may not expose a length; in that case
                        // default to eager activation to preserve behavior.
                        const count = components && components.length !== undefined
                                       ? components.length
                                       : -1;
                        const eager = (count < 0) || (count <= root.maxEagerComponentDelegates);
                        if (eager)
                            return true;

                        // On very large graphs, only keep delegates active
                        // when they are selected or explicitly kept alive.
                        return delegateRoot.keepAlive
                            || root.selectedComponent === modelData
                            || root.componentIsSelected(modelData);
                    }
                    asynchronous: false

                    sourceComponent: ComponentItem {
                        component: modelData
                        viewZoom: root.zoom
                        moveEnabled: !root.ctrlSelectionModifierActive
                        resizeEnabled: !root.ctrlSelectionModifierActive
                        selected: root.selectedComponent === modelData
                                  || root.componentIsSelected(modelData)
                        renderVisuals: false
                        undoStack: root.undoStack

                        onComponentClicked: function (clickedComponent, modifiers) {
                                                root.suppressNextCanvasTap = true
                                                root.handleLeftComponentClick(clickedComponent,
                                                                              modifiers)
                                            }

                        onFocusedChanged: {
                            delegateRoot.keepAlive = focused
                        }

                        onConnectionDragged: function (sourceComponent, sourceSide, startP, targetP) {
                            var viewStart = root.windowSceneToView(startP)
                            var viewEnd = root.windowSceneToView(targetP)
                            if (interactionState.mode === InteractionStateManager.ConnectionDraw)
                                interactionState.updateConnectionDraw(viewStart, viewEnd)
                            else
                                interactionState.startConnectionDraw(sourceComponent, viewStart, viewEnd)
                            edgeCanvas.repaint()
                        }
                        onConnectionDropped: function (sourceComponent, sourceSide, startP, targetP) {
                            interactionState.endConnectionDraw()

                            var dropPoint = root.windowSceneToView(targetP)
                            var component = edgeViewport.hitTestComponentAtView(dropPoint.x,
                                                                                dropPoint.y)
                            if (!component) {
                                edgeCanvas.repaint()
                                return
                            }

                            if (component === sourceComponent) {
                                edgeCanvas.repaint()
                                return
                            }

                            var connectionId = "conn_" + sourceComponent.id + "_" + component.id
                            if (root.graph.connectionById(connectionId)) {
                                edgeCanvas.repaint()
                                return
                            }

                            // Add connection from sourceComponent to component.
                            var e1 = Qt.createQmlObject(
                                        'import ComponentMapEditor; ConnectionModel {}',
                                        root.graph)
                            e1.id = connectionId
                            e1.sourceId = sourceComponent.id
                            e1.targetId = component.id
                            e1.label = "path A"
                            e1.sourceSide = sourceSide
                            if (root.undoStack)
                                root.undoStack.pushAddConnection(root.graph, e1)
                            else
                                root.graph.addConnection(e1)
                            edgeCanvas.repaint()
                        }

                        onMoveStarted: {
                            interactionState.startNodeMove(modelData)
                            var nextMoveStart = root.moveStartPositions
                            nextMoveStart[modelData.id] = Qt.point(modelData.x,
                                                                   modelData.y)
                            root.moveStartPositions = nextMoveStart
                            root.beginGroupMove(modelData)
                            if (root.telemetry) root.telemetry.notifyDragStarted()
                        }
                        onMoved: {
                            root.updateGroupMove(modelData)
                            edgeCanvas.repaint()
                            if (root.telemetry) root.telemetry.notifyDragMoved()
                        }
                        onMoveFinished: {
                            root.commitMoveCommands(modelData)
                            root.endGroupMove()
                            interactionState.endNodeMove()
                            if (root.telemetry) root.telemetry.notifyDragEnded()
                        }

                        onResizeStarted: {
                            interactionState.startNodeResize(modelData)
                            root.startResizeCapture(modelData)
                        }
                        onResizeFinished: {
                            root.commitResizeCommand(modelData)
                            interactionState.endNodeResize()
                        }

                        onHoverPositionChanged: function (hoverX, hoverY) {
                            root.mouseViewPos = root.childToView(this, hoverX, hoverY)
                        }
                    }

                    onLoaded: {
                        if (itemLoader.item)
                            delegateRoot.keepAlive = itemLoader.item.focused
                    }
                }
            }
        }
    }

    Item {
        id: marqueeOverlay
        anchors.fill: parent
        z: 120
        visible: root.ctrlSelectionModifierActive || interactionState.marqueeSelecting

        MouseArea {
            anchors.fill: parent
            enabled: root.ctrlSelectionModifierActive
            acceptedButtons: Qt.LeftButton
            hoverEnabled: true
            preventStealing: true

            onPressed: mouse => {
                           // Clear the release flag on any mouse press to allow modifiers to work again
                           root.ctrlReleasedByKey = false
                           // Safety check: only start marquee if Ctrl is still active
                           if (!root.ctrlSelectionModifierActive)
                               return
                           var start = Qt.point(mouse.x, mouse.y)
                           interactionState.startMarqueeSelect(start, start)
                           root.debugInputLog("marquee_start_press")
                       }

            onPositionChanged: mouse => {
                                    // Safety check: only continue marquee if Ctrl is still active
                                    if (!root.ctrlSelectionModifierActive)
                                        return
                                    if (!pressed || !interactionState.marqueeSelecting)
                                        return
                                    interactionState.updateMarqueeSelect(
                                                interactionState.marqueeStart,
                                                Qt.point(mouse.x, mouse.y))
                                    root.mouseViewPos = Qt.point(mouse.x, mouse.y)
                                    root.updateMouseWorldPos()
                                    root.debugInputLog("drag_update_marquee")
                                }

            onReleased: mouse => {
                             // Safety check: only process marquee release if Ctrl is still active
                             if (!root.ctrlSelectionModifierActive)
                                 return
                             if (!interactionState.marqueeSelecting)
                                 return
                             interactionState.updateMarqueeSelect(
                                         interactionState.marqueeStart,
                                         Qt.point(mouse.x, mouse.y))
                             root.mouseViewPos = Qt.point(mouse.x, mouse.y)
                             root.updateMouseWorldPos()
                             root.finishMarqueeSelection()
                             root.debugInputLog("mouse_release")
                         }
        }

        Rectangle {
            x: Math.min(interactionState.marqueeStart.x, interactionState.marqueeEnd.x)
            y: Math.min(interactionState.marqueeStart.y, interactionState.marqueeEnd.y)
            width: Math.abs(interactionState.marqueeEnd.x - interactionState.marqueeStart.x)
            height: Math.abs(interactionState.marqueeEnd.y - interactionState.marqueeStart.y)
            color: "#3342a5f5"
            border.color: "#1e88e5"
            border.width: 1.5
            radius: 2

            Rectangle {
                anchors.fill: parent
                anchors.margins: 1
                color: "transparent"
                border.color: "#80bbdefb"
                border.width: 1
                radius: 1
            }
        }
    }

    GraphStatusBar {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        z: 200
        componentCount: root.graph ? root.graph.components.length : 0
        connectionCount: root.graph ? root.graph.connections.length : 0
        selectedComponentCount: root.selectedComponentIds.length
        selectedConnectionCount: root.selectedConnectionIds.length
        selectedComponentTitle: root.selectedComponent ? root.selectedComponent.title : "none"
        selectedConnectionLabel: root.selectedConnection ? root.selectedConnection.id : "none"
        mouseViewPos: root.mouseViewPos
        mouseWorldPos: root.mouseWorldPos
        zoom: root.zoom
    }

    // React to graph-level changes
    Connections {
        target: root.graph
        enabled: root.graph !== null

        function onComponentsChanged() {
            root.pressedComponent = null
            root.pointerOverComponent = false
            root.endGroupMove()
            interactionState.resetInteraction()
            interactionState.pruneStaleComponents(root.graph)
            edgeCanvas.repaint()
        }
        function onConnectionsChanged() {
            interactionState.pruneStaleConnection(root.graph)
            edgeCanvas.repaint()
        }
    }

    Menu {
        id: componentMenu

        MenuItem {
            text: "Duplicate"
            onTriggered: root.duplicateComponent(root.menuTargetComponent)
        }
        MenuItem {
            text: "Delete"
            onTriggered: root.deleteComponent(root.menuTargetComponent)
        }
        MenuSeparator {}
        MenuItem {
            text: "Clear Incoming Connections"
            onTriggered: root.clearComponentConnections(root.menuTargetComponent,
                                                        true,
                                                        false)
        }
        MenuItem {
            text: "Clear Outgoing Connections"
            onTriggered: root.clearComponentConnections(root.menuTargetComponent,
                                                        false,
                                                        true)
        }
    }

    Menu {
        id: connectionMenu

        MenuItem {
            text: "Delete"
            onTriggered: {
                if (root.menuTargetConnection)
                    root.removeConnectionById(root.menuTargetConnection.id)
                edgeCanvas.repaint()
            }
        }
    }

    Menu {
        id: backgroundMenu

        MenuItem {
            text: "Add New Component"
            onTriggered: root.addComponentAtWorldPos(root.menuTargetWorldPos)
        }
        MenuSeparator {}
        MenuItem {
            text: "Clear All Connections"
            onTriggered: root.openConfirm("clear_connections",
                                          "Clear all connections?")
        }
        MenuItem {
            text: "Clear All Components"
            onTriggered: root.openConfirm("clear_components",
                                          "Clear all components and related connections?")
        }
    }

    Dialog {
        id: confirmDialog
        title: "Confirm"
        modal: true
        anchors.centerIn: parent
        width: 420
        standardButtons: Dialog.Yes | Dialog.No

        contentItem: Label {
            text: root.pendingConfirmMessage
            wrapMode: Text.WordWrap
            width: 360
        }

        onAccepted: root.executePendingConfirm()
        onRejected: {
            root.pendingConfirmAction = ""
            root.pendingConfirmMessage = ""
        }
    }

    onSelectedConnectionChanged: edgeCanvas.repaint()
    onGraphChanged: edgeCanvas.repaint()
    onPanXChanged: {
        if (telemetry) telemetry.notifyCameraChanged()
        root.updateMouseWorldPos()
        root.viewTransformChanged(root.panX, root.panY, root.zoom)
    }
    onPanYChanged: {
        root.updateMouseWorldPos()
        root.viewTransformChanged(root.panX, root.panY, root.zoom)
    }
    onZoomChanged: {
        if (telemetry) telemetry.notifyCameraChanged()
        root.updateMouseWorldPos()
        root.viewTransformChanged(root.panX, root.panY, root.zoom)
    }
    onMouseViewPosChanged: {
        root.hoveredComponent = edgeViewport.hitTestComponentAtView(root.mouseViewPos.x,
                                                                     root.mouseViewPos.y)
        root.pointerOverComponent = root.hoveredComponent !== null
        root.updateMouseWorldPos()
    }
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Control) {
            root.ctrlReleasedByKey = false  // Clear the release flag when Ctrl is pressed again
            root.ctrlSelectionModifierActive = true
            root.debugInputLog("key_ctrl_pressed")
            event.accepted = false
        }
    }
    Keys.onReleased: event => {
        if (event.key === Qt.Key_Control) {
            root.ctrlSelectionModifierActive = false
            root.ctrlReleasedByKey = true  // Set flag to prevent stale modifiers from re-enabling Ctrl
            // Exit marquee mode if active when Ctrl is released
            if (interactionState.marqueeSelecting) {
                root.finishMarqueeSelection()
            }
            root.debugInputLog("key_ctrl_released")
            event.accepted = false
        }
    }
    onActiveFocusChanged: {
        if (!activeFocus) {
            if (interactionState.marqueeSelecting)
                root.finishMarqueeSelection()
            root.ctrlSelectionModifierActive = false
            root.ctrlReleasedByKey = false
            root.debugInputLog("focus_lost")
        } else {
            root.debugInputLog("focus_gained")
        }
    }

    Component.onCompleted: {
        FontAwesome.ensureLoaded()
        root.updateMouseWorldPos()
    }
}
