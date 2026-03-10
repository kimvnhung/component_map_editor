import QtQuick

Item {
    id: root

    property bool moveEnabled: true
    property bool resizeEnabled: true
    // Whether to show the resize handles.
    // Resize can still be enabled without showing the handles, but it won't be discoverable to users without other visual affordances.
    property bool handlesVisible: false
    property real minItemWidth: 40
    property real minItemHeight: 24
    property real handleSize: 10
    property real moveDragThreshold: 4
    property bool moving: false
    // Set to true while an item is actively being resized.
    // This is smaller case of handlesVisible, which is true for the entire duration that resize handles are shown, while resizing is only true during the active drag.
    property bool resizing: false
    property bool hovered: false
    // True when the cursor is hovering over any resize handle, independent of whether a drag is active.
    readonly property bool handleHovered: _handleHoverCount > 0
    property int _handleHoverCount: 0

    signal clicked
    signal moveStarted
    signal moved
    signal moveFinished
    signal resizeStarted
    signal resized
    signal resizeFinished

    default property alias contentData: contentHost.data

    function applyResizedGeometry(newX, newY, newWidth, newHeight, dirX, dirY, startX, startY, startWidth, startHeight) {
        var clampedWidth = Math.max(minItemWidth, newWidth)
        var clampedHeight = Math.max(minItemHeight, newHeight)
        var finalX = newX
        var finalY = newY

        // Keep the opposite edge fixed when min-size clamping occurs.
        if (dirX < 0 && clampedWidth !== newWidth)
            finalX = startX + (startWidth - clampedWidth)

        if (dirY < 0 && clampedHeight !== newHeight)
            finalY = startY + (startHeight - clampedHeight)

        root.x = finalX
        root.y = finalY
        root.width = clampedWidth
        root.height = clampedHeight

        root.resized()
    }

    Item {
        id: contentHost
        anchors.fill: parent
    }

    HoverHandler {
        id: moveHover
        enabled: root.moveEnabled
        cursorShape: moveDrag.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor
        onHoveredChanged: root.hovered = hovered
    }

    DragHandler {
        id: moveDrag
        enabled: root.moveEnabled && !root.resizing
        target: root
        acceptedButtons: Qt.LeftButton
        dragThreshold: root.moveDragThreshold

        onActiveChanged: {
            if (active) {
                root.moving = true
                root.moveStarted()
            } else {
                if (root.moving)
                    root.moveFinished()
                root.moving = false
            }
        }

        onTranslationChanged: {
            if (active)
                root.moved()
        }
    }

    TapHandler {
        enabled: root.moveEnabled && !root.resizing
        acceptedButtons: Qt.LeftButton
        onTapped: root.clicked()
    }

    Item {
        anchors.fill: parent
        visible: root.resizeEnabled && root.handlesVisible
        z: 100

        Repeater {
            model: [{
                    "xFactor": 0.0,
                    "yFactor": 0.0,
                    "dirX": -1,
                    "dirY": -1,
                    "cursor": Qt.SizeFDiagCursor
                }, {
                    "xFactor": 0.5,
                    "yFactor": 0.0,
                    "dirX": 0,
                    "dirY": -1,
                    "cursor": Qt.SizeVerCursor
                }, {
                    "xFactor": 1.0,
                    "yFactor": 0.0,
                    "dirX": 1,
                    "dirY": -1,
                    "cursor": Qt.SizeBDiagCursor
                }, {
                    "xFactor": 1.0,
                    "yFactor": 0.5,
                    "dirX": 1,
                    "dirY": 0,
                    "cursor": Qt.SizeHorCursor
                }, {
                    "xFactor": 1.0,
                    "yFactor": 1.0,
                    "dirX": 1,
                    "dirY": 1,
                    "cursor": Qt.SizeFDiagCursor
                }, {
                    "xFactor": 0.5,
                    "yFactor": 1.0,
                    "dirX": 0,
                    "dirY": 1,
                    "cursor": Qt.SizeVerCursor
                }, {
                    "xFactor": 0.0,
                    "yFactor": 1.0,
                    "dirX": -1,
                    "dirY": 1,
                    "cursor": Qt.SizeBDiagCursor
                }, {
                    "xFactor": 0.0,
                    "yFactor": 0.5,
                    "dirX": -1,
                    "dirY": 0,
                    "cursor": Qt.SizeHorCursor
                }]

            delegate: Item {
                required property var modelData

                id: resizeHandle
                width: root.handleSize
                height: root.handleSize
                x: modelData.xFactor * root.width - (width / 2)
                y: modelData.yFactor * root.height - (height / 2)

                property real startX: 0
                property real startY: 0
                property real startWidth: 0
                property real startHeight: 0

                HoverHandler {
                    cursorShape: resizeHandle.modelData.cursor
                    onHoveredChanged: root._handleHoverCount = Math.max(0, root._handleHoverCount + (hovered ? 1 : -1))
                }

                DragHandler {
                    id: resizeDrag
                    target: null
                    acceptedButtons: Qt.LeftButton

                    onActiveChanged: {
                        if (active) {
                            resizeHandle.startX = root.x
                            resizeHandle.startY = root.y
                            resizeHandle.startWidth = root.width
                            resizeHandle.startHeight = root.height
                            root.resizing = true
                            root.resizeStarted()
                        } else {
                            if (!root.resizing)
                                return
                            root.resizing = false
                            root.resizeFinished()
                        }
                    }

                    onTranslationChanged: {
                        if (!active)
                            return

                        var dx = translation.x
                        var dy = translation.y

                        var newX = resizeHandle.startX
                        var newY = resizeHandle.startY
                        var newWidth = resizeHandle.startWidth
                        var newHeight = resizeHandle.startHeight

                        if (resizeHandle.modelData.dirX < 0) {
                            newX = resizeHandle.startX + dx
                            newWidth = resizeHandle.startWidth - dx
                        } else if (resizeHandle.modelData.dirX > 0) {
                            newWidth = resizeHandle.startWidth + dx
                        }

                        if (resizeHandle.modelData.dirY < 0) {
                            newY = resizeHandle.startY + dy
                            newHeight = resizeHandle.startHeight - dy
                        } else if (resizeHandle.modelData.dirY > 0) {
                            newHeight = resizeHandle.startHeight + dy
                        }

                        root.applyResizedGeometry(newX, newY, newWidth,
                                                  newHeight,
                                                  resizeHandle.modelData.dirX,
                                                  resizeHandle.modelData.dirY,
                                                  resizeHandle.startX,
                                                  resizeHandle.startY,
                                                  resizeHandle.startWidth,
                                                  resizeHandle.startHeight)
                    }
                }
            }
        }
    }
}
