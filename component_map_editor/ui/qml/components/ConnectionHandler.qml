import QtQuick
import "../items"

Item {
    id: root
    property real boundingWidth: 20
    property bool arrowActivated: false
    readonly property real effectiveBoundingWidth: boundingWidth

    // 0: up, 1: right, 2: down, 3: left
    signal arrowDragged(int direction, point scenePos)
    signal arrowDropped(int direction, point scenePos)
    signal hoveredPositionChanged(point scenePos)

    Item {
        id: boundingBox
        width: parent.width + root.effectiveBoundingWidth * 2 + 2
        height: parent.height + root.effectiveBoundingWidth * 2 + 2
        anchors.centerIn: parent

        HoverHandler {
            id: boundingHandler
            // Activate or deactivate arrows based on bounding-box hover.
            // This covers the case where the pointer enters the arrow region
            // from outside the component body.
            onHoveredChanged: root.arrowActivated = hovered
            onPointChanged: root.hoveredPositionChanged(
                                parent.mapToItem(root, point.position))
        }

        Repeater {
            model: 4
            delegate: Item {
                readonly property int direction: index

                width: root.effectiveBoundingWidth
                height: root.effectiveBoundingWidth
                x: {
                    if (direction === 1)
                        return boundingBox.width - width
                    if (direction === 0 || direction === 2)
                        return (boundingBox.width - width) / 2
                    return 0
                }
                y: {
                    if (direction === 2)
                        return boundingBox.height - height
                    if (direction === 1 || direction === 3)
                        return (boundingBox.height - height) / 2
                    return 0
                }
                opacity: 0.3
                visible: root.arrowActivated
                Arrow {
                    anchors.fill: parent
                    arrowSize: parent.width
                    arrowWidth: 2.0
                    direction: parent.direction
                    arrowColor: "#ff5722"
                }

                DragHandler {
                    id: arrowDrag
                    target: null
                    acceptedButtons: Qt.LeftButton
                    dragThreshold: 0

                    function scencePos() {
                        // mapToItem with null maps to the scene coordinates
                        // centroid.position is the center of the bounding box, which is where the arrow visually appears to be dragged from
                        return parent.mapToItem(null, centroid.position)
                    }

                    onActiveChanged: {
                        if (active)
                            root.arrowDragged(parent.direction, scencePos())
                        else
                            root.arrowDropped(parent.direction, scencePos())
                    }

                    onTranslationChanged: root.arrowDragged(parent.direction, scencePos())
                }

                HoverHandler {
                    onHoveredChanged: {
                        parent.opacity = hovered ? 1 : 0.3
                    }
                    onPointChanged: root.hoveredPositionChanged(
                                        parent.mapToItem(root, point.position))
                }
            }
        }
    }
}
