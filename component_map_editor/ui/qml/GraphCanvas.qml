// GraphCanvas.qml — The main editing surface.
// Displays a grid background, draws connections imperatively via a Canvas item,
// and renders components as interactive ComponentItem delegates.
import QtQuick
import ComponentMapEditor
import "GraphCanvasMath.js" as GraphCanvasMath
import "components"

Item {
    id: root

    // Camera tuning constants (world <-> screen transform behavior).
    readonly property real defaultZoom: 1.0
    readonly property real zoomStepFactor: 1.15
    readonly property real zoomEpsilon: 0.000001
    readonly property real viewportParityEpsilon: 0.0001
    property bool enableViewportParityCheck: false

    // Grid tuning constants.
    readonly property real baseGridStep: 30
    readonly property real minGridPixelStep: 16
    readonly property real maxGridPixelStep: 96

    // Interaction thresholds.
    readonly property real panStartThreshold: 3

    property GraphModel graph: null
    property ComponentModel selectedComponent: null
    property ConnectionModel selectedConnection: null
    property UndoStack undoStack: null

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
    readonly property var nodeRenderer: nodeViewport
    property point mouseViewPos: Qt.point(0, 0)
    property point mouseWorldPos: Qt.point(0, 0)
    readonly property point worldOrigin: viewToWorld(0, 0)

    // Optional telemetry hook. Set to a PerformanceTelemetry instance to
    // collect camera-update and drag-event interval samples for Phase 0 baseline.
    // Leave null (default) for zero overhead in production.
    property PerformanceTelemetry telemetry: null

    signal componentSelected(ComponentModel component)
    signal connectionSelected(ConnectionModel connection)
    signal backgroundClicked(real x, real y)
    signal viewTransformChanged(real panX, real panY, real zoom)

    // Coordinate-space contract:
    // - world: model coordinates (component centers, Y-down), stored in ComponentModel.
    // - view: viewport coordinates in GraphCanvas local space (Y-down).
    // - scene: Qt window-scene coordinates (only for mapToItem/mapFromItem use-cases).
    //
    // Current camera transform:
    //   viewX = worldX * zoom + panX
    //   viewY = worldY * zoom + panY
    //   worldX = (viewX - panX) / zoom
    //   worldY = (viewY - panY) / zoom
    function clampZoom(value) {
        return GraphCanvasMath.clamp(value, minZoom, maxZoom)
    }

    function viewToWorld(viewX, viewY) {
        return GraphCanvasMath.viewToWorld(viewX, viewY, panX, panY, zoom)
    }

    function worldToView(worldX, worldY) {
        return GraphCanvasMath.worldToView(worldX, worldY, panX, panY, zoom)
    }

    function updateMouseWorldPos() {
        root.mouseWorldPos = root.viewToWorld(root.mouseViewPos.x,
                                              root.mouseViewPos.y)
    }

    // Phase 1 guardrail: verify C++ viewport camera math parity with the
    // existing JS camera contract. If this warns, Phase 1 must be rolled back
    // or fixed before continuing migration.
    function verifyViewportParity() {
        if (!edgeViewport)
            return

        var sampleView = Qt.point(width * 0.37, height * 0.61)
        var jsWorld = GraphCanvasMath.viewToWorld(sampleView.x, sampleView.y,
                                                  panX, panY, zoom)
        var cppWorld = edgeViewport.viewToWorld(sampleView.x, sampleView.y)

        if (Math.abs(jsWorld.x - cppWorld.x) > viewportParityEpsilon
                || Math.abs(jsWorld.y - cppWorld.y) > viewportParityEpsilon) {
            console.warn("GraphViewportItem parity mismatch (viewToWorld):",
                         "js=", jsWorld.x, jsWorld.y, "cpp=", cppWorld.x,
                         cppWorld.y)
        }

        var sampleWorld = Qt.point(worldOrigin.x + 173.0, worldOrigin.y + 89.0)
        var jsView = GraphCanvasMath.worldToView(sampleWorld.x, sampleWorld.y,
                                                 panX, panY, zoom)
        var cppView = edgeViewport.worldToView(sampleWorld.x, sampleWorld.y)

        if (Math.abs(jsView.x - cppView.x) > viewportParityEpsilon
                || Math.abs(jsView.y - cppView.y) > viewportParityEpsilon) {
            console.warn("GraphViewportItem parity mismatch (worldToView):",
                         "js=", jsView.x, jsView.y, "cpp=", cppView.x,
                         cppView.y)
        }
    }

    // Zooms around a world-space anchor sampled from the cursor position.
    // If the cursor is currently over a component center in world coordinates,
    // zooming keeps that same world point under the cursor, so the visible
    // component center remains visually pinned to the mouse.
    function zoomAtCursor(zoomFactor) {
        var state = GraphCanvasMath.zoomAtCursor(mouseWorldPos.x,
                                                 mouseWorldPos.y, panX, panY,
                                                 zoom, zoomFactor, minZoom,
                                                 maxZoom, zoomEpsilon)
        if (!state.changed)
            return

        zoom = state.zoom
        panX = state.panX
        panY = state.panY
    }

    function childToView(childItem, childX, childY) {
        if (!childItem)
            return Qt.point(0, 0)
        return childItem.mapToItem(root, childX, childY)
    }

    function windowSceneToView(windowScenePoint) {
        return root.mapFromItem(null, windowScenePoint.x, windowScenePoint.y)
    }

    function componentItemFor(component) {
        for (var i = 0; i < componentLayer.children.length; ++i) {
            var host = componentLayer.children[i]
            var item = host && host.componentItem ? host.componentItem : host
            if (item && item.component === component && item.visible)
                return item
        }
        return null
    }

    function componentVisible(component) {
        if (!component)
            return false

        // Conservative culling in view space with margin to avoid flicker at edges.
        var margin = 120
        var halfW = component.width / 2
        var halfH = component.height / 2
        var left = (component.x - halfW) * zoom + panX
        var right = (component.x + halfW) * zoom + panX
        var top = (component.y - halfH) * zoom + panY
        var bottom = (component.y + halfH) * zoom + panY

        return right >= -margin && left <= root.width + margin
                && bottom >= -margin && top <= root.height + margin
    }

    function componentAtView(viewX, viewY, excludedComponent) {
        for (var i = componentLayer.children.length - 1; i >= 0; --i) {
            var host = componentLayer.children[i]
            var item = host && host.componentItem ? host.componentItem : host
            if (!item || !item.component || !item.visible)
                continue
            if (excludedComponent && item.component === excludedComponent)
                continue

            var local = item.mapFromItem(root, viewX, viewY)
            if (local.x >= 0 && local.x <= item.width && local.y >= 0
                    && local.y <= item.height)
                return item.component
        }
        return null
    }

    function connectionEndpointsInView(sourceComponent, targetComponent) {
        var sourceItem = componentItemFor(sourceComponent)
        var targetItem = componentItemFor(targetComponent)

        if (sourceItem && targetItem) {
            var sourceCenter = sourceItem.mapToItem(root, sourceItem.width / 2,
                                                    sourceItem.height / 2)
            var targetCenter = targetItem.mapToItem(root, targetItem.width / 2,
                                                    targetItem.height / 2)
            return GraphCanvasMath.connectionEndpointsBetweenRects(
                        sourceCenter, sourceItem.width, sourceItem.height,
                        targetCenter, targetItem.width, targetItem.height)
        }

        var endpoints = GraphCanvasMath.connectionEndpointsOnBounding(
                    sourceComponent, targetComponent)
        return {
            "source": worldToView(endpoints.source.x, endpoints.source.y),
            "target": worldToView(endpoints.target.x, endpoints.target.y)
        }
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
            onPointChanged: {
                // interactionLayer fills GraphCanvas at (0, 0), so the hover
                // point is already in GraphCanvas view coordinates.
                root.mouseViewPos = Qt.point(point.position.x,
                                             point.position.y)
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
            acceptedButtons: Qt.LeftButton
            onTapped: point => {
                          var viewPos = Qt.point(point.position.x,
                                                 point.position.y)

                          var hitComponent = edgeViewport.hitTestComponentAtView(viewPos.x,
                                                                                  viewPos.y)
                          if (hitComponent) {
                              root.selectedConnection = null
                              root.selectedComponent = hitComponent
                              root.componentSelected(hitComponent)
                              edgeCanvas.repaint()
                              return
                          }

                          var hitConnection = edgeViewport.hitTestConnectionAtView(viewPos.x,
                                                                                    viewPos.y,
                                                                                    10.0)
                          if (hitConnection) {
                              root.selectedComponent = null
                              root.selectedConnection = hitConnection
                              root.connectionSelected(hitConnection)
                              edgeCanvas.repaint()
                              return
                          }

                          root.selectedComponent = null
                          root.selectedConnection = null
                          var worldPos = root.viewToWorld(point.position.x,
                                                          point.position.y)
                          root.backgroundClicked(worldPos.x, worldPos.y)
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
                required property var modelData
                property Item componentItem: itemLoader.item

                Loader {
                    id: itemLoader
                    active: root.componentVisible(modelData)
                            || root.selectedComponent === modelData
                    asynchronous: false

                    sourceComponent: ComponentItem {
                        component: modelData
                        selected: root.selectedComponent === modelData
                        renderVisuals: false
                        undoStack: root.undoStack

                        onComponentClicked: clickedComponent => {
                                                root.selectedConnection = null
                                                root.selectedComponent = clickedComponent
                                                root.componentSelected(clickedComponent)
                                                edgeCanvas.repaint()
                                            }

                        onFocusedChanged: {
                            root.enableBackgroundDrag = !focused
                        }

                        onConnectionDragged: function (sourceComponent, startP, targetP) {
                            root.nodeInteractionActive = true
                            root.enableBackgroundDrag = false
                            root.tempConnectionDragging = true
                            root.tempStart = root.windowSceneToView(startP)
                            root.tempEnd = root.windowSceneToView(targetP)
                            edgeCanvas.repaint()
                        }
                        onConnectionDropped: function (sourceComponent, startP, targetP) {
                            root.nodeInteractionActive = false
                            root.enableBackgroundDrag = !focused
                            root.tempConnectionDragging = false
                            root.tempStart = Qt.point(0, 0)
                            root.tempEnd = Qt.point(0, 0)

                            var dropPoint = root.windowSceneToView(targetP)
                            var component = root.componentAtView(dropPoint.x,
                                                                 dropPoint.y,
                                                                 sourceComponent)
                            if (!component) {
                                console.log("Drop ignored: no target component")
                                edgeCanvas.repaint()
                                return
                            }

                            if (component === sourceComponent) {
                                console.log("Drop ignored: source equals target")
                                edgeCanvas.repaint()
                                return
                            }

                            var connectionId = "conn_" + sourceComponent.id + "_" + component.id
                            if (root.graph.connectionById(connectionId)) {
                                console.log("Drop ignored: connection already exists",
                                            connectionId)
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
                            root.nodeInteractionActive = true
                            root.enableBackgroundDrag = false
                            if (root.telemetry) root.telemetry.notifyDragStarted()
                        }
                        onMoved: {
                            edgeCanvas.repaint()
                            if (root.telemetry) root.telemetry.notifyDragMoved()
                        }
                        onMoveFinished: {
                            root.nodeInteractionActive = false
                            root.enableBackgroundDrag = !focused
                            if (root.telemetry) root.telemetry.notifyDragEnded()
                        }

                        onResizeStarted: {
                            root.nodeInteractionActive = true
                            root.enableBackgroundDrag = false
                        }
                        onResizeFinished: {
                            root.nodeInteractionActive = false
                            root.enableBackgroundDrag = !focused
                        }

                        onHoverPositionChanged: function (hoverX, hoverY) {
                            root.mouseViewPos = root.childToView(this, hoverX, hoverY)
                        }
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
            root.nodeInteractionActive = false
            root.enableBackgroundDrag = true
            root.pointerOverComponent = false
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

    onSelectedConnectionChanged: edgeCanvas.repaint()
    onGraphChanged: edgeCanvas.repaint()
    onPanXChanged: {
        if (root.enableViewportParityCheck)
            root.verifyViewportParity()
        if (telemetry) telemetry.notifyCameraChanged()
        root.updateMouseWorldPos()
        root.viewTransformChanged(root.panX, root.panY, root.zoom)
    }
    onPanYChanged: {
        if (root.enableViewportParityCheck)
            root.verifyViewportParity()
        root.updateMouseWorldPos()
        root.viewTransformChanged(root.panX, root.panY, root.zoom)
    }
    onZoomChanged: {
        if (root.enableViewportParityCheck)
            root.verifyViewportParity()
        if (telemetry) telemetry.notifyCameraChanged()
        root.updateMouseWorldPos()
        root.viewTransformChanged(root.panX, root.panY, root.zoom)
    }
    onMouseViewPosChanged: {
        root.pointerOverComponent = edgeViewport.hitTestComponentAtView(root.mouseViewPos.x,
                                                                        root.mouseViewPos.y) !== null
        root.updateMouseWorldPos()
    }

    Component.onCompleted: {
        if (root.enableViewportParityCheck)
            root.verifyViewportParity()
    }
}
