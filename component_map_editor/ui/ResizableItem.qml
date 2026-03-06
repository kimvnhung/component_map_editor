import QtQuick

Item {
    id: root

    property bool moveEnabled: true
    property bool resizeEnabled: true
    property bool handlesVisible: false
    property real minItemWidth: 40
    property real minItemHeight: 24
    property real handleSize: 10
    property real moveDragThreshold: 4
    property bool moving: false
    property bool resizing: false

    signal clicked()
    signal moveStarted()
    signal moved()
    signal moveFinished()
    signal resizeStarted()
    signal resized()
    signal resizeFinished()

    default property alias contentData: contentHost.data

    function applyResizedGeometry(newX, newY, newWidth, newHeight,
                                  dirX, dirY,
                                  startX, startY, startWidth, startHeight) {
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

    MouseArea {
        id: moveArea
        anchors.fill: parent
        enabled: root.moveEnabled && !root.resizing
        drag.target: root
        drag.threshold: root.moveDragThreshold
        cursorShape: pressed || drag.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor

        onPressed: {
            root.moving = false
        }

        onPositionChanged: {
            if (!root.moving && drag.active) {
                root.moving = true
                root.moveStarted()
            }

            if (root.moving)
                root.moved()
        }

        onClicked: {
            root.clicked()
        }

        function finishMove() {
            if (root.moving)
                root.moveFinished()
            root.moving = false
        }

        onReleased: finishMove()
        onCanceled: finishMove()
    }

    Item {
        anchors.fill: parent
        visible: root.resizeEnabled && root.handlesVisible
        z: 100

        Repeater {
            model: [
                { xFactor: 0.0, yFactor: 0.0, dirX: -1, dirY: -1, cursor: Qt.SizeFDiagCursor },
                { xFactor: 0.5, yFactor: 0.0, dirX:  0, dirY: -1, cursor: Qt.SizeVerCursor },
                { xFactor: 1.0, yFactor: 0.0, dirX:  1, dirY: -1, cursor: Qt.SizeBDiagCursor },
                { xFactor: 1.0, yFactor: 0.5, dirX:  1, dirY:  0, cursor: Qt.SizeHorCursor },
                { xFactor: 1.0, yFactor: 1.0, dirX:  1, dirY:  1, cursor: Qt.SizeFDiagCursor },
                { xFactor: 0.5, yFactor: 1.0, dirX:  0, dirY:  1, cursor: Qt.SizeVerCursor },
                { xFactor: 0.0, yFactor: 1.0, dirX: -1, dirY:  1, cursor: Qt.SizeBDiagCursor },
                { xFactor: 0.0, yFactor: 0.5, dirX: -1, dirY:  0, cursor: Qt.SizeHorCursor }
            ]

            delegate: MouseArea {
                required property var modelData

                id: resizeHandle
                width: root.handleSize
                height: root.handleSize
                x: modelData.xFactor * root.width - (width / 2)
                y: modelData.yFactor * root.height - (height / 2)
                cursorShape: modelData.cursor
                hoverEnabled: true

                property real startMouseX: 0
                property real startMouseY: 0
                property real startX: 0
                property real startY: 0
                property real startWidth: 0
                property real startHeight: 0

                onPressed: mouse => {
                    var p = resizeHandle.mapToItem(root, mouse.x, mouse.y)
                    startMouseX = p.x
                    startMouseY = p.y
                    startX = root.x
                    startY = root.y
                    startWidth = root.width
                    startHeight = root.height
                    root.resizing = true
                    root.resizeStarted()
                    mouse.accepted = true
                }

                onPositionChanged: mouse => {
                    if (!(mouse.buttons & Qt.LeftButton))
                        return

                    var p = resizeHandle.mapToItem(root, mouse.x, mouse.y)
                    var dx = p.x - startMouseX
                    var dy = p.y - startMouseY

                    var newX = startX
                    var newY = startY
                    var newWidth = startWidth
                    var newHeight = startHeight

                    if (modelData.dirX < 0) {
                        newX = startX + dx
                        newWidth = startWidth - dx
                    } else if (modelData.dirX > 0) {
                        newWidth = startWidth + dx
                    }

                    if (modelData.dirY < 0) {
                        newY = startY + dy
                        newHeight = startHeight - dy
                    } else if (modelData.dirY > 0) {
                        newHeight = startHeight + dy
                    }

                    root.applyResizedGeometry(
                        newX,
                        newY,
                        newWidth,
                        newHeight,
                        modelData.dirX,
                        modelData.dirY,
                        startX,
                        startY,
                        startWidth,
                        startHeight
                    )

                    mouse.accepted = true
                }

                function finishResize() {
                    if (!root.resizing)
                        return
                    root.resizing = false
                    root.resizeFinished()
                }

                onReleased: finishResize()
                onCanceled: finishResize()
            }
        }
    }
}
