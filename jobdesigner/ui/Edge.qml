// Edge.qml — A standalone directed-edge component drawn with QtQuick.Shapes.
// Place it inside any Item and set the four coordinate properties.
import QtQuick
import QtQuick.Shapes
import JobDesigner

Item {
    id: root

    property EdgeModel edge: null
    property real sourceX: 0
    property real sourceY: 0
    property real targetX: 100
    property real targetY: 100
    property bool selected: false

    signal clicked()

    // Computed angle from source to target (used for the arrowhead).
    readonly property real _angle: Math.atan2(targetY - sourceY, targetX - sourceX)

    // The shape fills whatever parent is given; coordinates are absolute
    // within the parent's coordinate system.
    anchors.fill: parent

    Shape {
        anchors.fill: parent

        // Main line
        ShapePath {
            strokeColor: root.selected ? "#ff5722" : "#607d8b"
            strokeWidth: root.selected ? 3 : 2
            fillColor: "transparent"
            startX: root.sourceX
            startY: root.sourceY
            PathLine { x: root.targetX; y: root.targetY }
        }

        // Arrowhead at the target end
        ShapePath {
            strokeColor: "transparent"
            fillColor: root.selected ? "#ff5722" : "#607d8b"
            startX: root.targetX
            startY: root.targetY
            PathLine {
                x: root.targetX - 12 * Math.cos(root._angle - 0.4)
                y: root.targetY - 12 * Math.sin(root._angle - 0.4)
            }
            PathLine {
                x: root.targetX - 12 * Math.cos(root._angle + 0.4)
                y: root.targetY - 12 * Math.sin(root._angle + 0.4)
            }
            PathLine { x: root.targetX; y: root.targetY }
        }
    }

    // Edge label
    Text {
        visible: root.edge && root.edge.label.length > 0
        text: root.edge ? root.edge.label : ""
        x: (root.sourceX + root.targetX) / 2 - width / 2
        y: (root.sourceY + root.targetY) / 2 - height - 3
        color: root.selected ? "#ff5722" : "#607d8b"
        font.pixelSize: 11
    }
}
