// GraphCanvas.qml — The main editing surface.
// Displays a grid background, draws connections imperatively via a Canvas item,
// and renders components as interactive ComponentItem delegates.
import QtQuick
import ComponentMapEditor
import "GraphCanvasMath.js" as GraphCanvasMath

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
    property ConnectionModel selectedConnection: null
    property UndoStack undoStack: null

    // Temp connection points in GraphCanvas scene coordinates (Y-down).
    property point tempStart: Qt.point(0, 0)
    property point tempEnd: Qt.point(0, 0)
    property bool tempConnectionDragging: false

    property real zoom: defaultZoom
    property real minZoom: 0.35
    property real maxZoom: 3.0
    property real panX: 0
    property real panY: 0
    property bool enableBackgroundDrag: true
    property point mouseScenePos: Qt.point(0, 0)
    property point mouseWorldPos: Qt.point(0, 0)
    readonly property point worldOrigin: sceneToWorld(0, 0)

    signal componentSelected(ComponentModel component)
    signal connectionSelected(ConnectionModel connection)
    signal backgroundClicked(real x, real y)
    signal viewTransformChanged(real panX, real panY, real zoom)

    // Coordinate-space contract:
    // - world: model coordinates (Y-up), stored in ComponentModel.
    // - scene: viewport/screen coordinates in GraphCanvas (Y-down).
    function clampZoom(value) {
        return GraphCanvasMath.clamp(value, minZoom, maxZoom)
    }

    function sceneToWorld(sceneX, sceneY) {
        return GraphCanvasMath.sceneToWorld(sceneX, sceneY, panX, panY, zoom)
    }

    function worldToScene(worldX, worldY) {
        return GraphCanvasMath.worldToScene(worldX, worldY, panX, panY, zoom)
    }

    function zoomAt(screenX, screenY, zoomFactor) {
        var state = GraphCanvasMath.zoomAtCursor(screenX, screenY, panX, panY,
                                                 zoom, zoomFactor, minZoom,
                                                 maxZoom, zoomEpsilon)
        if (!state.changed)
            return

        zoom = state.zoom
        panX = state.panX
        panY = state.panY
    }

    function childToScene(childItem, childX, childY) {
        if (!childItem)
            return Qt.point(0, 0)
        return childItem.mapToItem(root, childX, childY)
    }

    function windowSceneToCanvasScene(scenePoint) {
        return root.mapFromItem(null, scenePoint.x, scenePoint.y)
    }

    function componentItemFor(component) {
        for (var i = 0; i < componentLayer.children.length; ++i) {
            var item = componentLayer.children[i]
            if (item && item.component === component)
                return item
        }
        return null
    }

    function componentAtScene(sceneX, sceneY, excludedComponent) {
        for (var i = componentLayer.children.length - 1; i >= 0; --i) {
            var item = componentLayer.children[i]
            if (!item || !item.component)
                continue
            if (excludedComponent && item.component === excludedComponent)
                continue

            var local = item.mapFromItem(root, sceneX, sceneY)
            if (local.x >= 0 && local.x <= item.width && local.y >= 0
                    && local.y <= item.height)
                return item.component
        }
        return null
    }

    function connectionEndpointsInScene(sourceComponent, targetComponent) {
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
            "source": worldToScene(endpoints.source.x, endpoints.source.y),
            "target": worldToScene(endpoints.target.x, endpoints.target.y)
        }
    }

    clip: true

    // Background fill
    Rectangle {
        anchors.fill: parent
        color: "#f8f9fa"
    }

    // Grid
    Canvas {
        id: gridCanvas
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.strokeStyle = "#e0e0e0"
            ctx.lineWidth = 0.5
            var step = GraphCanvasMath.normalizedGridStep(
                        root.baseGridStep, root.zoom, root.minGridPixelStep,
                        root.maxGridPixelStep)

            var offsetX = GraphCanvasMath.positiveModulo(root.panX, step)
            var offsetY = GraphCanvasMath.positiveModulo(root.panY, step)

            for (var gx = -step + offsetX; gx < width + step; gx += step) {
                ctx.beginPath()
                ctx.moveTo(gx, 0)
                ctx.lineTo(gx, height)
                ctx.stroke()
            }
            for (var gy = -step + offsetY; gy < height + step; gy += step) {
                ctx.beginPath()
                ctx.moveTo(0, gy)
                ctx.lineTo(width, gy)
                ctx.stroke()
            }
        }
    }

    Item {
        id: interactionLayer
        anchors.fill: parent
        z: 0

        property real startPanX: 0
        property real startPanY: 0

        HoverHandler {
            cursorShape: panDrag.active ? Qt.ClosedHandCursor : Qt.ArrowCursor
            onPointChanged: {
                root.mouseScenePos = Qt.point(point.position.x,
                                              point.position.y)
            }
        }

        DragHandler {
            id: panDrag
            enabled: root.enableBackgroundDrag
            target: null
            acceptedButtons: Qt.LeftButton
            dragThreshold: root.panStartThreshold

            onActiveChanged: {
                if (active) {
                    interactionLayer.startPanX = root.panX
                    interactionLayer.startPanY = root.panY
                }
            }

            onTranslationChanged: {
                root.panX = interactionLayer.startPanX + translation.x
                root.panY = interactionLayer.startPanY + translation.y
            }
        }

        TapHandler {
            acceptedButtons: Qt.LeftButton
            onTapped: point => {
                          // Check if the tap hit any component; if so, ignore since the component's own TapHandler will handle it.
                          var scenePos = Qt.point(point.position.x,
                                                  point.position.y)

                          var hitComponent = root.componentAtScene(scenePos.x,
                                                                   scenePos.y)
                          if (hitComponent) {
                              return
                          }

                          root.selectedComponent = null
                          root.selectedConnection = null
                          var worldPos = root.sceneToWorld(point.position.x,
                                                           point.position.y)
                          root.backgroundClicked(worldPos.x, worldPos.y)
                      }
        }

        WheelHandler {
            onWheel: event => {
                         var factor = event.angleDelta.y
                         > 0 ? root.zoomStepFactor : 1 / root.zoomStepFactor
                         root.zoomAt(event.x, event.y, factor)
                         event.accepted = true
                     }
        }
    }

    Canvas {
        id: edgeCanvas
        anchors.fill: parent
        z: 1

        function repaint() {
            requestPaint()
        }

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            if (!root.graph)
                return

            var connections = root.graph.connections
            for (var i = 0; i < connections.length; i++) {
                var connection = connections[i]
                var src = root.graph.componentById(connection.sourceId)
                var tgt = root.graph.componentById(connection.targetId)
                if (!src || !tgt)
                    continue

                var endpoints = root.connectionEndpointsInScene(src, tgt)

                var isSel = (root.selectedConnection === connection)
                GraphCanvasMath.drawConnection(ctx, endpoints.source,
                                               endpoints.target,
                                               GraphCanvasMath.CONNECTION_TYPE.real,
                                               connection.label, isSel)
            }

            if (root.tempConnectionDragging) {
                GraphCanvasMath.drawConnection(ctx, root.tempStart,
                                               root.tempEnd,
                                               GraphCanvasMath.CONNECTION_TYPE.temp,
                                               "", false)
            }
        }
    }

    Item {
        id: componentLayer
        width: parent.width
        height: parent.height
        x: root.panX
        y: root.panY
        scale: root.zoom

        // Components
        Repeater {
            model: root.graph ? root.graph.components : []

            delegate: ComponentItem {
                required property var modelData

                component: modelData
                selected: root.selectedComponent === modelData
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
                    root.tempConnectionDragging = true
                    root.tempStart = root.windowSceneToCanvasScene(startP)
                    root.tempEnd = root.windowSceneToCanvasScene(targetP)
                    edgeCanvas.repaint()
                }
                onConnectionDropped: function (sourceComponent, startP, targetP) {
                    root.tempConnectionDragging = false
                    root.tempStart = Qt.point(0, 0)
                    root.tempEnd = Qt.point(0, 0)

                    var dropPoint = root.windowSceneToCanvasScene(targetP)
                    var component = root.componentAtScene(dropPoint.x,
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

                onMoved: edgeCanvas.repaint()

                onHoverPositionChanged: function (hoverX, hoverY) {
                    root.mouseScenePos = root.childToScene(this, hoverX, hoverY)
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
        mouseScenePos: root.mouseScenePos
        mouseWorldPos: root.mouseWorldPos
        zoom: root.zoom
    }

    // React to graph-level changes
    Connections {
        target: root.graph
        enabled: root.graph !== null

        function onComponentsChanged() {
            edgeCanvas.repaint()
        }
        function onConnectionsChanged() {
            edgeCanvas.repaint()
        }
    }

    onSelectedConnectionChanged: edgeCanvas.repaint()
    onGraphChanged: edgeCanvas.repaint()
    onPanXChanged: {
        gridCanvas.requestPaint()
        edgeCanvas.repaint()
        root.viewTransformChanged(root.panX, root.panY, root.zoom)
    }
    onPanYChanged: {
        gridCanvas.requestPaint()
        edgeCanvas.repaint()
        root.viewTransformChanged(root.panX, root.panY, root.zoom)
    }
    onZoomChanged: {
        gridCanvas.requestPaint()
        edgeCanvas.repaint()
        root.viewTransformChanged(root.panX, root.panY, root.zoom)
    }
    onMouseScenePosChanged: {
        root.mouseWorldPos = root.sceneToWorld(root.mouseScenePos.x,
                                               root.mouseScenePos.y)
    }
}
