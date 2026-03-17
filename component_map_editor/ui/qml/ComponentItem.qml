// ComponentItem.qml — A draggable component card displayed on the GraphCanvas.
import QtQuick
import ComponentMapEditor
import QmlFontAwesome

ResizableItem {
    id: root

    property ComponentModel component: null
    property bool selected: false
    property bool renderVisuals: true
    property UndoStack undoStack: null
    property real viewZoom: 1.0
    readonly property int defaultComponentWidth: 96
    readonly property int defaultComponentHeight: 96
    readonly property int minComponentWidth: 40
    readonly property int minComponentHeight: 24
    readonly property bool isRounded: !root.component
                                      || root.component.shape === "rounded"

    property bool focused: false

    minItemWidth: minComponentWidth
    minItemHeight: minComponentHeight
    handleSize: 10
    handlesVisible: selected
    moveEnabled: true
    moveDragThreshold: 4
    interactionZoom: viewZoom

    // Coordinate contract:
    // - ComponentModel.x/y store the COMPONENT CENTER in world coordinates.
    // - ComponentItem.x/y store the TOP-LEFT corner in componentLayer coordinates.
    // - Both spaces use Y-down, so only center<->top-left conversion is needed.
    //
    // Center Y -> top-left Y:
    //   itemTopY = modelCenterY - height/2
    function modelYToItemTop(modelY) {
        return modelY - root.height / 2
    }

    // Top-left Y -> center Y:
    //   modelCenterY = itemTopY + height/2
    function itemTopToModelY(itemTopY) {
        return itemTopY + root.height / 2
    }

    function directionToScenePoint(direction) {
        switch (direction) {
        case 0:
            // up
            return root.mapToItem(null, root.width / 2, 0)
        case 1:
            // right
            return root.mapToItem(null, root.width, root.height / 2)
        case 2:
            // down
            return root.mapToItem(null, root.width / 2, root.height)
        case 3:
            // left
            return root.mapToItem(null, 0, root.height / 2)
        }

        return root.mapToItem(null, root.width / 2, root.height / 2)
    }

    function refreshFocused() {
        focused = selected || connectionHandler.arrowActivated
    }

    // Writes item geometry back to the model.
    // Item x/y are top-left in componentLayer coordinates.
    // Model x/y store the component center in world coordinates.
    // Therefore:
    //   modelCenterX = itemX + width/2
    //   modelCenterY = itemY + height/2
    // Width/height are copied as-is.
    function syncModelFromItemGeometry() {
        if (!root.component)
            return
        root.component.x = root.x + root.width / 2
        root.component.y = itemTopToModelY(root.y)
        root.component.width = root.width
        root.component.height = root.height
    }

    function syncItemFromModelGeometry() {
        if (!root.component || root.moving || root.resizing)
            return

        root.width = root.component.width
        root.height = root.component.height
        root.x = root.component.x - root.width / 2
        root.y = modelYToItemTop(root.component.y)
    }

    onMoved: syncModelFromItemGeometry()
    onMoveFinished: syncModelFromItemGeometry()
    onResized: syncModelFromItemGeometry()
    onResizeFinished: syncModelFromItemGeometry()

    Component.onCompleted: syncItemFromModelGeometry()

    signal componentClicked(ComponentModel component, int modifiers)

    // startP and targetP are in scene coordinates relative to the top-left of the view.
    signal connectionDragged(ComponentModel sourceComponent, int sourceSide, point startP, point targetP)
    signal connectionDropped(ComponentModel sourceComponent, int sourceSide, point startP, point targetP)

    width: defaultComponentWidth
    height: defaultComponentHeight
    z: selected ? 2 : 1

    onClicked: modifiers => {
                   root.componentClicked(root.component, modifiers)
               }

    onHoveredChanged: {
        if (hovered)
            connectionHandler.arrowActivated = true
    }
    onSelectedChanged: root.refreshFocused()
    onComponentChanged: syncItemFromModelGeometry()

    // Keep position in sync when the model is updated externally.
    Connections {
        target: root.component
        function onXChanged() {
            if (!root.moving && !root.resizing)
                root.x = root.component.x - root.width / 2
        }
        function onYChanged() {
            if (!root.moving && !root.resizing)
                root.y = modelYToItemTop(root.component.y)
        }
        function onWidthChanged() {
            if (!root.moving && !root.resizing) {
                root.width = root.component.width
                root.x = root.component.x - root.width / 2
            }
        }
        function onHeightChanged() {
            if (!root.moving && !root.resizing) {
                root.height = root.component.height
                root.y = modelYToItemTop(root.component.y)
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: root.isRounded ? 6 : 0
        color: root.renderVisuals && root.component ? root.component.color : "transparent"
        border.color: root.renderVisuals
                      ? (root.selected ? "#ff5722" : Qt.darker(root.component ? root.component.color : "#4fc3f7", 1.4))
                      : "transparent"
        border.width: root.renderVisuals ? (root.selected ? 2.5 : 1.5) : 0

        // Overlay to indicate selection with a semi-transparent border, since the main border can be hard to see against some colors.
        Rectangle {
            anchors.fill: parent
            visible: root.renderVisuals
            color: "transparent"
            radius: parent.radius
            border.color: "#607d8b"
            border.width: root.selected ? 1 : 0
            opacity: 0.35
        }

        readonly property int headerHeight: 26

        Rectangle {
            id: headerBar
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: parent.headerHeight
            radius: root.isRounded ? parent.radius : 0
            color: Qt.rgba(0.0, 0.0, 0.0, 0.18)
            visible: root.renderVisuals

            Row {
                anchors.fill: parent
                anchors.leftMargin: 6
                anchors.rightMargin: 6
                spacing: 6

                FaIcon {
                    anchors.verticalCenter: parent.verticalCenter
                    iconName: root.component && root.component.icon.length > 0
                              ? root.component.icon
                              : "cube"
                    iconStyle: FontAwesome.Solid
                    font.pixelSize: 12
                    color: "white"
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - 26
                    text: root.component ? root.component.title : ""
                    color: "white"
                    font.bold: true
                    font.pixelSize: 11
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        Item {
            id: contentArea
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: headerBar.bottom
            anchors.bottom: parent.bottom
            anchors.margins: 8
            visible: root.renderVisuals

            Text {
                anchors.centerIn: parent
                width: parent.width
                text: root.component ? root.component.content : ""
                color: "#e8f1f6"
                font.pixelSize: 10
                maximumLineCount: 3
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                visible: text.length > 0
            }
        }

        ConnectionHandler {
            id: connectionHandler
            anchors.fill: parent
            onArrowDragged: function (direction, scenePos) {
                root.connectionDragged(root.component,
                                       direction,
                                       root.directionToScenePoint(direction),
                                       scenePos)
            }
            onArrowDropped: function (direction, scenePos) {
                root.connectionDropped(root.component,
                                       direction,
                                       root.directionToScenePoint(direction),
                                       scenePos)
            }

            onArrowActivatedChanged: root.refreshFocused()
            onHoveredPositionChanged: function (internalPos) {
                var rootPos = connectionHandler.mapToItem(root, internalPos)
                root.hoverPositionChanged(rootPos.x, rootPos.y)
            }
        }
    }
}
