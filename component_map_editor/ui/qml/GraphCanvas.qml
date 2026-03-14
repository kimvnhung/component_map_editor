// GraphCanvas.qml — The main editing surface.
// Displays a grid background, draws connections imperatively via a Canvas item,
// and renders components as interactive ComponentItem delegates.
import QtQuick
import QtQuick.Controls
import ComponentMapEditor
import "components"

Item {
    id: root

    // Camera tuning constants (world <-> screen transform behavior).
    readonly property real defaultZoom: 1.0
    readonly property real zoomStepFactor: 1.15
    readonly property real zoomEpsilon: 0.000001

    // Grid tuning constants.
    readonly property real baseGridStep: 30
    readonly property real minGridPixelStep: 16
    readonly property real maxGridPixelStep: 96

    // Interaction thresholds.
    readonly property real panStartThreshold: 3

    property GraphModel graph: null
    property ComponentModel selectedComponent: null
    property var selectedComponentIds: []
    property ConnectionModel selectedConnection: null
    property UndoStack undoStack: null
    property ComponentModel menuTargetComponent: null
    property ConnectionModel menuTargetConnection: null
    property point menuTargetWorldPos: Qt.point(0, 0)
    property string pendingConfirmAction: ""
    property string pendingConfirmMessage: ""

    // Temp connection points in GraphCanvas view coordinates (Y-down).
    property point tempStart: Qt.point(0, 0)
    property point tempEnd: Qt.point(0, 0)
    property bool tempConnectionDragging: false

    property real zoom: defaultZoom
    property real minZoom: 0.35
    property real maxZoom: 3.0
    property real panX: 0
    property real panY: 0
    property bool enableBackgroundDrag: true
    property bool nodeInteractionActive: false
    property bool pointerOverComponent: false
    property ComponentModel hoveredComponent: null
    property ComponentModel pressedComponent: null
    property ComponentModel activeInteractionComponent: null
    property bool suppressNextCanvasTap: false
    readonly property var nodeRenderer: nodeViewport
    property point mouseViewPos: Qt.point(0, 0)
    property point mouseWorldPos: Qt.point(0, 0)
    property int livePointerModifiers: 0

    // Optional telemetry hook. Set to a PerformanceTelemetry instance to
    // collect camera-update and drag-event interval samples for Phase 0 baseline.
    // Leave null (default) for zero overhead in production.
    property PerformanceTelemetry telemetry: null

    signal componentSelected(ComponentModel component)
    signal connectionSelected(ConnectionModel connection)
    signal backgroundClicked(real x, real y)
    signal viewTransformChanged(real panX, real panY, real zoom)

    function componentIsSelected(component) {
        if (!component || !component.id)
            return false

        return root.selectedComponentIds.indexOf(component.id) !== -1
    }

    function removeComponentFromSelection(component) {
        if (!component || !component.id)
            return

        var nextSelection = []
        for (var i = 0; i < root.selectedComponentIds.length; ++i) {
            if (root.selectedComponentIds[i] !== component.id)
                nextSelection.push(root.selectedComponentIds[i])
        }
        root.selectedComponentIds = nextSelection
    }

    function clearComponentSelection() {
        root.selectedComponentIds = []
        root.selectedComponent = null
    }

    function selectSingleComponent(component) {
        if (!component) {
            clearComponentSelection()
            return
        }

        root.selectedComponentIds = [component.id]
        root.selectedComponent = component
    }

    function selectConnection(connection) {
        root.clearComponentSelection()
        root.selectedConnection = connection
        if (connection)
            root.connectionSelected(connection)
    }

    function handleLeftComponentClick(component, modifiers) {
        var eventModifiers = modifiers ? modifiers : 0
        var effectiveModifiers = eventModifiers | root.livePointerModifiers
        var ctrlPressed = (effectiveModifiers & Qt.ControlModifier) !== 0

        root.selectedConnection = null
        if (ctrlPressed) {
            if (root.componentIsSelected(component)) {
                root.removeComponentFromSelection(component)
                if (root.selectedComponentIds.length > 0) {
                    var lastSelectedId = root.selectedComponentIds[root.selectedComponentIds.length - 1]
                    root.selectedComponent = root.graph ? root.graph.componentById(lastSelectedId) : null
                } else {
                    root.selectedComponent = null
                }
                if (root.selectedComponent)
                    root.componentSelected(root.selectedComponent)
                else
                    root.backgroundClicked(root.mouseWorldPos.x,
                                           root.mouseWorldPos.y)
            } else {
                root.selectedComponentIds = root.selectedComponentIds.concat([component.id])
                root.selectedComponent = component
                root.componentSelected(component)
            }
        } else {
            root.selectSingleComponent(component)
            root.componentSelected(component)
        }

        edgeCanvas.repaint()
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

    function removeConnectionById(connectionId) {
        if (!root.graph || !connectionId)
            return

        root.graph.removeConnection(connectionId)
        if (root.selectedConnection && root.selectedConnection.id === connectionId)
            root.selectedConnection = null
    }

    function clearComponentConnections(component, clearIncoming, clearOutgoing) {
        if (!root.graph || !component)
            return

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
            root.removeConnectionById(idsToRemove[j])

        edgeCanvas.repaint()
    }

    function deleteComponent(component) {
        if (!root.graph || !component)
            return

        root.clearComponentConnections(component, true, true)
        root.graph.removeComponent(component.id)
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
        copy.label = component.label + " Copy"
        copy.x = component.x + 40
        copy.y = component.y + 40
        copy.width = component.width
        copy.height = component.height
        copy.shape = component.shape
        copy.color = component.color
        copy.type = component.type
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
        component.label = "Component"
        component.x = worldPos.x
        component.y = worldPos.y
        component.width = 120
        component.height = 40
        component.color = "#4fc3f7"
        component.shape = "rounded"
        component.type = "default"
        root.graph.addComponent(component)
        root.selectSingleComponent(component)
        root.selectedConnection = null
        root.componentSelected(component)
        edgeCanvas.repaint()
    }

    function clearAllConnections() {
        if (!root.graph)
            return

        var ids = []
        var currentConnections = root.graph.connections
        for (var i = 0; i < currentConnections.length; ++i)
            ids.push(currentConnections[i].id)

        for (var j = 0; j < ids.length; ++j)
            root.graph.removeConnection(ids[j])

        root.selectedConnection = null
        edgeCanvas.repaint()
    }

    function clearAllComponents() {
        if (!root.graph)
            return

        root.graph.clear()
        root.clearComponentSelection()
        root.selectedConnection = null
        root.backgroundClicked(root.mouseWorldPos.x, root.mouseWorldPos.y)
        edgeCanvas.repaint()
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

        HoverHandler {
            id: canvasHover
            cursorShape: panDrag.active
                         ? Qt.ClosedHandCursor
                         : (root.pointerOverComponent || root.enableBackgroundDrag
                            ? Qt.OpenHandCursor
                            : Qt.ArrowCursor)
            onPointChanged: {
                // interactionLayer fills GraphCanvas at (0, 0), so the hover
                // point is already in GraphCanvas view coordinates.
                root.mouseViewPos = Qt.point(point.position.x,
                                             point.position.y)
                root.livePointerModifiers = point.modifiers
            }
        }

        DragHandler {
            id: panDrag
            enabled: root.enableBackgroundDrag
                     && !root.nodeInteractionActive
                     && !root.pointerOverComponent
            target: null
            acceptedButtons: Qt.LeftButton
            dragThreshold: root.panStartThreshold

            onActiveChanged: {
                if (active) {
                    interactionLayer.startPanX = root.panX
                    interactionLayer.startPanY = root.panY
                    if (root.telemetry) root.telemetry.notifyDragStarted()
                } else {
                    if (root.telemetry) root.telemetry.notifyDragEnded()
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
                    root.livePointerModifiers = point.modifiers
                    root.pressedComponent = edgeViewport.hitTestComponentAtView(point.position.x,
                                                                                 point.position.y)
                } else {
                    root.pressedComponent = null
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
                    // Phase 4: keep only interaction overlays in QML.
                    // C++ hit-testing handles picking for all nodes.
                    active: root.selectedComponent === modelData
                            || root.componentIsSelected(modelData)
                            || root.hoveredComponent === modelData
                            || root.pressedComponent === modelData
                            || root.activeInteractionComponent === modelData
                            || delegateRoot.keepAlive
                    asynchronous: false

                    sourceComponent: ComponentItem {
                        component: modelData
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
                            root.enableBackgroundDrag = !focused
                        }

                        onConnectionDragged: function (sourceComponent, startP, targetP) {
                            root.activeInteractionComponent = sourceComponent
                            root.nodeInteractionActive = true
                            root.enableBackgroundDrag = false
                            root.tempConnectionDragging = true
                            root.tempStart = root.windowSceneToView(startP)
                            root.tempEnd = root.windowSceneToView(targetP)
                            edgeCanvas.repaint()
                        }
                        onConnectionDropped: function (sourceComponent, startP, targetP) {
                            root.activeInteractionComponent = null
                            root.nodeInteractionActive = false
                            root.enableBackgroundDrag = !focused
                            root.tempConnectionDragging = false
                            root.tempStart = Qt.point(0, 0)
                            root.tempEnd = Qt.point(0, 0)

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
                            root.graph.addConnection(e1)
                            edgeCanvas.repaint()
                        }

                        onMoveStarted: {
                            root.activeInteractionComponent = modelData
                            root.nodeInteractionActive = true
                            root.enableBackgroundDrag = false
                            if (root.telemetry) root.telemetry.notifyDragStarted()
                        }
                        onMoved: {
                            edgeCanvas.repaint()
                            if (root.telemetry) root.telemetry.notifyDragMoved()
                        }
                        onMoveFinished: {
                            root.activeInteractionComponent = null
                            root.nodeInteractionActive = false
                            root.enableBackgroundDrag = !focused
                            if (root.telemetry) root.telemetry.notifyDragEnded()
                        }

                        onResizeStarted: {
                            root.activeInteractionComponent = modelData
                            root.nodeInteractionActive = true
                            root.enableBackgroundDrag = false
                        }
                        onResizeFinished: {
                            root.activeInteractionComponent = null
                            root.nodeInteractionActive = false
                            root.enableBackgroundDrag = !focused
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

    GraphStatusBar {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        z: 200
        componentCount: root.graph ? root.graph.components.length : 0
        connectionCount: root.graph ? root.graph.connections.length : 0
        selectedComponentCount: root.selectedComponentIds.length
        selectedConnectionCount: root.selectedConnection ? 1 : 0
        selectedComponentLabel: root.selectedComponent ? root.selectedComponent.label : "none"
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
            root.activeInteractionComponent = null
            root.nodeInteractionActive = false
            root.enableBackgroundDrag = true
            root.pointerOverComponent = false
            var filteredIds = []
            for (var i = 0; i < root.selectedComponentIds.length; ++i) {
                if (root.graph.componentById(root.selectedComponentIds[i]))
                    filteredIds.push(root.selectedComponentIds[i])
            }
            root.selectedComponentIds = filteredIds
            if (root.selectedComponent && !root.graph.componentById(root.selectedComponent.id))
                root.selectedComponent = null
            edgeCanvas.repaint()
        }
        function onConnectionsChanged() {
            if (root.selectedConnection && !root.graph.connectionById(root.selectedConnection.id))
                root.selectedConnection = null
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

    Component.onCompleted: root.updateMouseWorldPos()
}
