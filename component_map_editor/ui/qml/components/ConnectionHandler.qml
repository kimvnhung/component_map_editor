import QtQuick
import "../items"

Item {
    id: root
    property real boundingWidth: 20
    property bool arrowActivated: false

    // 0: up, 1: right, 2: down, 3: left
    signal arrowDragged(int direction, point scenePos)
    signal arrowDropped(int direction, point scenePos)
    signal hoveredPositionChanged(point scenePos)
    Item {
        id: boundingBox
        width: parent.width + boundingWidth * 2 + 2
        height: parent.height + boundingWidth * 2 + 2
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
            model: [{
                    "x": boundingBox.width / 2 - boundingWidth / 2,
                    "y": 0
                }, {
                    "x": boundingBox.width - boundingWidth,
                    "y": boundingBox.height / 2 - boundingWidth / 2
                }, {
                    "x": boundingBox.width / 2 - boundingWidth / 2,
                    "y": boundingBox.height - boundingWidth
                }, {
                    "x": 0,
                    "y": boundingBox.height / 2 - boundingWidth / 2
                }]
            delegate: Item {
                width: boundingWidth
                height: boundingWidth
                x: modelData.x
                y: modelData.y
                opacity: 0.3
                visible: root.arrowActivated
                Arrow {
                    direction: index
                    arrowColor: "#ff5722"
                }

                DragHandler {
                    id: arrowDrag
                    target: null
                    acceptedButtons: Qt.LeftButton

                    function scencePos() {
                        // mapToItem with null maps to the scene coordinates
                        // centroid.position is the center of the bounding box, which is where the arrow visually appears to be dragged from
                        return parent.mapToItem(null, centroid.position)
                    }

                    onActiveChanged: {
                        if (active)
                            root.arrowDragged(index, scencePos())
                        else
                            root.arrowDropped(index, scencePos())
                    }

                    onTranslationChanged: root.arrowDragged(index, scencePos())
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
