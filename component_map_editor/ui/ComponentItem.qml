// ComponentItem.qml — A draggable component card displayed on the GraphCanvas.
import QtQuick
import ComponentMapEditor

ResizableItem {
    id: root

    property ComponentModel component: null
    property bool selected: false
    property UndoStack undoStack: null

    readonly property int defaultComponentWidth: 120
    readonly property int defaultComponentHeight: 40
    readonly property int minComponentWidth: 40
    readonly property int minComponentHeight: 24
    readonly property real connectionPointRadius: 3
    readonly property bool isRounded: !root.component
                                      || root.component.shape === "rounded"

    minItemWidth: minComponentWidth
    minItemHeight: minComponentHeight
    handleSize: 10
    handlesVisible: selected
    moveEnabled: true
    moveDragThreshold: 4

    // Model space uses Y-up, while Qt item space uses Y-down.
    function modelYToSceneTop(modelY) {
        return -modelY - (root.height / 2)
    }

    function sceneTopToModelY(sceneTop) {
        return -(sceneTop + (root.height / 2))
    }

    signal componentClicked(ComponentModel component)
    // Emitted after a drag finishes so the canvas can redraw connections.
    signal positionChanged

    function syncModelFromItemGeometry() {
        if (!root.component)
            return

        root.component.x = root.x + (root.width / 2)
        root.component.y = sceneTopToModelY(root.y)
        root.component.width = root.width
        root.component.height = root.height
        root.positionChanged()
    }

    width: defaultComponentWidth
    height: defaultComponentHeight
    z: selected ? 2 : 1

    onClicked: root.componentClicked(root.component)
    onMoveFinished: {
        if (root.component)
            syncModelFromItemGeometry()
    }
    onResized: syncModelFromItemGeometry()
    onHoveredChanged: {
        if (hovered)
            connectionHandler.arrowActivated = true
    }

    // Initialise position from the model; don't bind so dragging works.
    Component.onCompleted: {
        if (root.component) {
            root.width = root.component.width
            root.height = root.component.height
            x = root.component.x - (root.width / 2)
            y = modelYToSceneTop(root.component.y)
        }
    }

    // Keep position in sync when the model is updated externally.
    Connections {
        target: root.component
        function onXChanged() {
            if (!root.moving && !root.resizing)
                root.x = root.component.x - (root.width / 2)
        }
        function onYChanged() {
            if (!root.moving && !root.resizing)
                root.y = modelYToSceneTop(root.component.y)
        }
        function onWidthChanged() {
            if (!root.moving && !root.resizing) {
                root.width = root.component.width
                root.x = root.component.x - (root.width / 2)
            }
        }
        function onHeightChanged() {
            if (!root.moving && !root.resizing) {
                root.height = root.component.height
                root.y = modelYToSceneTop(root.component.y)
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: root.isRounded ? 6 : 0
        color: root.component ? root.component.color : "#4fc3f7"
        border.color: root.selected ? "#ff5722" : Qt.darker(color, 1.4)
        border.width: root.selected ? 2.5 : 1.5

        Rectangle {
            anchors.fill: parent
            color: "transparent"
            radius: parent.radius
            border.color: "#607d8b"
            border.width: root.selected ? 1 : 0
            opacity: 0.35
        }

        Text {
            anchors.centerIn: parent
            text: root.component ? root.component.label : ""
            color: "white"
            font.bold: true
            font.pixelSize: 13
            elide: Text.ElideRight
            width: parent.width - 12
            horizontalAlignment: Text.AlignHCenter
        }

        ConnectionHandler {
            id: connectionHandler
            anchors.fill: parent
        }
    }
}
