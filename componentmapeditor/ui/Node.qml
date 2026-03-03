// Node.qml — A draggable node card displayed on the GraphCanvas.
import QtQuick
import ComponentMapEditor

Item {
    id: root

    property NodeModel node: null
    property bool selected: false
    property UndoStack undoStack: null

    // Dimensions used by GraphCanvas to compute edge endpoints.
    readonly property int nodeWidth:  120
    readonly property int nodeHeight: 40

    signal nodeClicked(NodeModel node)
    // Emitted after a drag finishes so the canvas can redraw edges.
    signal positionChanged()

    width:  nodeWidth
    height: nodeHeight
    z: selected ? 2 : 1

    // Initialise position from the model; don't bind so dragging works.
    Component.onCompleted: {
        if (root.node) {
            x = root.node.x
            y = root.node.y
        }
    }

    // Keep position in sync when the model is updated externally.
    Connections {
        target: root.node
        function onXChanged() { if (!dragArea.drag.active) root.x = root.node.x }
        function onYChanged() { if (!dragArea.drag.active) root.y = root.node.y }
    }

    Rectangle {
        anchors.fill: parent
        radius: 6
        color: root.node ? root.node.color : "#4fc3f7"
        border.color: root.selected ? "#ff5722" : Qt.darker(color, 1.4)
        border.width: root.selected ? 2.5 : 1.5

        Text {
            anchors.centerIn: parent
            text: root.node ? root.node.label : ""
            color: "white"
            font.bold: true
            font.pixelSize: 13
            elide: Text.ElideRight
            width: parent.width - 12
            horizontalAlignment: Text.AlignHCenter
        }

        MouseArea {
            id: dragArea
            anchors.fill: parent
            drag.target: root
            drag.threshold: 4

            onClicked: root.nodeClicked(root.node)

            onReleased: {
                if (drag.active && root.node) {
                    root.node.x = root.x
                    root.node.y = root.y
                    root.positionChanged()
                }
            }
        }
    }
}
