import QtQuick
import "items"

Item {
    id: root
    property real boundingWidth: 20
    property bool contentHovered: false
    property bool arrowActivated: false

    // 0: up, 1: right, 2: down, 3: left
    signal arrowClicked(int direction)
    Item {
        id: boundingBox
        width: parent.width + boundingWidth * 2 + 2
        height: parent.height + boundingWidth * 2 + 2
        anchors.centerIn: parent

        HoverHandler {
            id: boundingHandler
            onHoveredChanged: {
                if (!hovered) {
                    root.arrowActivated = false
                }
            }
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

                TapHandler {
                    onTapped: {
                        root.arrowClicked(index)
                    }
                }

                HoverHandler {
                    onHoveredChanged: {
                        parent.opacity = hovered ? 1 : 0.3
                    }
                }
            }
        }
    }

    HoverHandler {
        id: contentHandler
        onHoveredChanged: {
            if (hovered) {
                root.arrowActivated = true
            }
        }
    }
}
