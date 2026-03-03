// GraphCanvas.qml — The main editing surface.
// Displays a grid background, draws edges imperatively via a Canvas item,
// and renders nodes as interactive Node delegates.
import QtQuick
import ComponentMapEditor

Item {
    id: root

    // Half-size of a node card, used to centre edge endpoints on node centres.
    readonly property int nodeHalfW: 60
    readonly property int nodeHalfH: 20

    property GraphModel  graph: null
    property NodeModel   selectedNode: null
    property EdgeModel   selectedEdge: null
    property UndoStack   undoStack: null

    signal nodeSelected(NodeModel node)
    signal edgeSelected(EdgeModel edge)
    signal backgroundClicked(real x, real y)

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
            var step = 30
            for (var gx = 0; gx < width; gx += step) {
                ctx.beginPath(); ctx.moveTo(gx, 0); ctx.lineTo(gx, height); ctx.stroke()
            }
            for (var gy = 0; gy < height; gy += step) {
                ctx.beginPath(); ctx.moveTo(0, gy); ctx.lineTo(width, gy); ctx.stroke()
            }
        }
    }

    // Background mouse area (deselects everything)
    MouseArea {
        anchors.fill: parent
        z: -1
        onClicked: mouse => {
            root.selectedNode = null
            root.selectedEdge = null
            root.backgroundClicked(mouse.x, mouse.y)
        }
    }

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

                var sx = src.x + root.nodeHalfW
                var sy = src.y + root.nodeHalfH
                var tx = tgt.x + root.nodeHalfW
                var ty = tgt.y + root.nodeHalfH

                var isSel = (root.selectedEdge === edge)
                ctx.strokeStyle = isSel ? "#ff5722" : "#607d8b"
                ctx.lineWidth   = isSel ? 3 : 2

                // Line
                ctx.beginPath()
                ctx.moveTo(sx, sy)
                ctx.lineTo(tx, ty)
                ctx.stroke()

                // Arrowhead
                var angle    = Math.atan2(ty - sy, tx - sx)
                var arrowLen = 12
                ctx.fillStyle = isSel ? "#ff5722" : "#607d8b"
                ctx.beginPath()
                ctx.moveTo(tx, ty)
                ctx.lineTo(tx - arrowLen * Math.cos(angle - 0.4),
                           ty - arrowLen * Math.sin(angle - 0.4))
                ctx.lineTo(tx - arrowLen * Math.cos(angle + 0.4),
                           ty - arrowLen * Math.sin(angle + 0.4))
                ctx.closePath()
                ctx.fill()

                // Label
                if (edge.label) {
                    ctx.fillStyle   = isSel ? "#ff5722" : "#607d8b"
                    ctx.font        = "11px sans-serif"
                    ctx.textAlign   = "center"
                    ctx.textBaseline = "bottom"
                    ctx.fillText(edge.label, (sx + tx) / 2, (sy + ty) / 2 - 3)
                }
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

    // React to graph-level changes
    Connections {
        target: root.graph
        enabled: root.graph !== null

        function onNodesChanged() { edgeCanvas.repaint() }
        function onEdgesChanged() { edgeCanvas.repaint() }
    }

    onSelectedEdgeChanged: edgeCanvas.repaint()
    onGraphChanged:        edgeCanvas.repaint()
}
