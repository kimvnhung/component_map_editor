// GraphCanvas.qml — The main editing surface.
// Displays a grid background, draws edges imperatively via a Canvas item,
// and renders nodes as interactive Node delegates.
import QtQuick
import ComponentMapEditor
import "GraphCanvasMath.js" as GraphCanvasMath

Item {
    id: root

    // Half-size of a node card, used to centre edge endpoints on node centres.
    readonly property int nodeHalfW: 60
    readonly property int nodeHalfH: 20

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
    property NodeModel   selectedNode: null
    property EdgeModel   selectedEdge: null
    property UndoStack   undoStack: null
    property real zoom: defaultZoom
    property real minZoom: 0.35
    property real maxZoom: 3.0
    property real panX: 0
    property real panY: 0
    readonly property point worldOrigin: screenToWorld(0, 0)

    signal nodeSelected(NodeModel node)
    signal edgeSelected(EdgeModel edge)
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

    // Background mouse area (pans and deselects)
    MouseArea {
        id: panArea
        anchors.fill: parent
        z: 0
        acceptedButtons: Qt.LeftButton

        property real pressX: 0
        property real pressY: 0
        property real startPanX: 0
        property real startPanY: 0
        property bool panning: false

        onPressed: mouse => {
            pressX = mouse.x
            pressY = mouse.y
            startPanX = root.panX
            startPanY = root.panY
            panning = false
        }

        onPositionChanged: mouse => {
            if (!(mouse.buttons & Qt.LeftButton))
                return

            var dx = mouse.x - pressX
            var dy = mouse.y - pressY

            if (!panning && (Math.abs(dx) > root.panStartThreshold || Math.abs(dy) > root.panStartThreshold))
                panning = true

            if (panning) {
                root.panX = startPanX + dx
                root.panY = startPanY + dy
            }
        }

        onReleased: mouse => {
            if (!panning) {
                root.selectedNode = null
                root.selectedEdge = null
                var worldPos = root.screenToWorld(mouse.x, mouse.y)
                root.backgroundClicked(worldPos.x, worldPos.y)
            }
        }

        onWheel: wheel => {
            var factor = wheel.angleDelta.y > 0 ? root.zoomStepFactor : 1 / root.zoomStepFactor
            root.zoomAt(wheel.x, wheel.y, factor)
            wheel.accepted = true
        }
    }

    Item {
        id: contentLayer
        anchors.fill: parent
        x: root.panX
        y: root.panY
        scale: root.zoom

        // Edges — drawn imperatively so that moving a node instantly updates all
        // connected edges without requiring per-edge property bindings.
        Canvas {
            id: edgeCanvas
            anchors.fill: parent

            function repaint() { requestPaint() }

            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                if (!root.graph) return

                var edges = root.graph.edges
                for (var i = 0; i < edges.length; i++) {
                    var edge = edges[i]
                    var src  = root.graph.nodeById(edge.sourceId)
                    var tgt  = root.graph.nodeById(edge.targetId)
                    if (!src || !tgt) continue

                    var srcCenter = GraphCanvasMath.nodeCenter(src, root.nodeHalfW, root.nodeHalfH)
                    var tgtCenter = GraphCanvasMath.nodeCenter(tgt, root.nodeHalfW, root.nodeHalfH)

                    var isSel = (root.selectedEdge === edge)
                    GraphCanvasMath.drawEdge(
                        ctx,
                        srcCenter.x,
                        srcCenter.y,
                        tgtCenter.x,
                        tgtCenter.y,
                        edge.label,
                        isSel
                    )
                }
            }
        }

        // Nodes
        Repeater {
            model: root.graph ? root.graph.nodes : []

            delegate: Node {
                required property var modelData

                node:      modelData
                selected:  root.selectedNode === modelData
                undoStack: root.undoStack

                onNodeClicked: clickedNode => {
                    root.selectedEdge = null
                    root.selectedNode = clickedNode
                    root.nodeSelected(clickedNode)
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

        function onNodesChanged() { edgeCanvas.repaint() }
        function onEdgesChanged() { edgeCanvas.repaint() }
    }

    onSelectedEdgeChanged: edgeCanvas.repaint()
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
