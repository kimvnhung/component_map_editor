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

    property GraphModel  graph: null
    property ComponentModel selectedComponent: null
    property ConnectionModel selectedConnection: null
    property UndoStack   undoStack: null
    property real zoom: defaultZoom
    property real minZoom: 0.35
    property real maxZoom: 3.0
    property real panX: 0
    property real panY: 0
    readonly property point worldOrigin: screenToWorld(0, 0)

    signal componentSelected(ComponentModel component)
    signal connectionSelected(ConnectionModel connection)
    signal backgroundClicked(real x, real y)
    signal viewTransformChanged(real panX, real panY, real zoom)

    function clampZoom(value) {
        return GraphCanvasMath.clamp(value, minZoom, maxZoom)
    }

    function screenToWorld(screenX, screenY) {
        return GraphCanvasMath.screenToWorld(screenX, screenY, panX, panY, zoom)
    }

    function worldToScreen(worldX, worldY) {
        return GraphCanvasMath.worldToScreen(worldX, worldY, panX, panY, zoom)
    }

    function zoomAt(screenX, screenY, zoomFactor) {
        var state = GraphCanvasMath.zoomAtCursor(
            screenX,
            screenY,
            panX,
            panY,
            zoom,
            zoomFactor,
            minZoom,
            maxZoom,
            zoomEpsilon
        )
        if (!state.changed)
            return

        zoom = state.zoom
        panX = state.panX
        panY = state.panY
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
                root.baseGridStep,
                root.zoom,
                root.minGridPixelStep,
                root.maxGridPixelStep
            )

            var offsetX = GraphCanvasMath.positiveModulo(root.panX, step)
            var offsetY = GraphCanvasMath.positiveModulo(root.panY, step)

            for (var gx = -step + offsetX; gx < width + step; gx += step) {
                ctx.beginPath(); ctx.moveTo(gx, 0); ctx.lineTo(gx, height); ctx.stroke()
            }
            for (var gy = -step + offsetY; gy < height + step; gy += step) {
                ctx.beginPath(); ctx.moveTo(0, gy); ctx.lineTo(width, gy); ctx.stroke()
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
        }

        DragHandler {
            id: panDrag
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
                root.selectedComponent = null
                root.selectedConnection = null
                var worldPos = root.screenToWorld(point.position.x, point.position.y)
                root.backgroundClicked(worldPos.x, worldPos.y)
            }
        }

        WheelHandler {
            onWheel: event => {
                var factor = event.angleDelta.y > 0 ? root.zoomStepFactor : 1 / root.zoomStepFactor
                root.zoomAt(event.x, event.y, factor)
                event.accepted = true
            }
        }
    }

    Item {
        id: contentLayer
        width: parent.width
        height: parent.height
        x: root.panX
        y: root.panY
        scale: root.zoom

        // Connections — drawn imperatively so that moving a component instantly
        // updates all connected lines without requiring per-connection bindings.
        Canvas {
            id: edgeCanvas
            anchors.fill: parent

            function repaint() { requestPaint() }

            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                if (!root.graph) return

                var connections = root.graph.connections
                for (var i = 0; i < connections.length; i++) {
                    var connection = connections[i]
                    var src  = root.graph.componentById(connection.sourceId)
                    var tgt  = root.graph.componentById(connection.targetId)
                    if (!src || !tgt) continue

                    var endpoints = GraphCanvasMath.connectionEndpointsOnBounding(src, tgt)

                    var isSel = (root.selectedConnection === connection)
                    GraphCanvasMath.drawConnection(
                        ctx,
                        endpoints.source.x,
                        endpoints.source.y,
                        endpoints.target.x,
                        endpoints.target.y,
                        connection.label,
                        isSel
                    )
                }
            }
        }

        // Components
        Repeater {
            model: root.graph ? root.graph.components : []

            delegate: ComponentItem {
                required property var modelData

                component: modelData
                selected:  root.selectedComponent === modelData
                undoStack: root.undoStack

                onComponentClicked: clickedComponent => {
                    root.selectedConnection = null
                    root.selectedComponent = clickedComponent
                    root.componentSelected(clickedComponent)
                    edgeCanvas.repaint()
                }

                onPositionChanged: edgeCanvas.repaint()
            }
        }
    }

    // React to graph-level changes
    Connections {
        target: root.graph
        enabled: root.graph !== null

        function onComponentsChanged() { edgeCanvas.repaint() }
        function onConnectionsChanged() { edgeCanvas.repaint() }
    }

    onSelectedConnectionChanged: edgeCanvas.repaint()
    onGraphChanged:        edgeCanvas.repaint()
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
}
