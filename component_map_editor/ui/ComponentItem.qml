// ComponentItem.qml — A draggable component card displayed on the GraphCanvas.
import QtQuick
import ComponentMapEditor

Item {
    id: root

    property ComponentModel component: null
    property bool selected: false
    property UndoStack undoStack: null

    // Dimensions used by GraphCanvas to compute connection endpoints.
    readonly property int componentWidth:  120
    readonly property int componentHeight: 40

    signal componentClicked(ComponentModel component)
    // Emitted after a drag finishes so the canvas can redraw connections.
    signal positionChanged()

    width:  componentWidth
    height: componentHeight
    z: selected ? 2 : 1

    // Initialise position from the model; don't bind so dragging works.
    Component.onCompleted: {
        if (root.component) {
            x = root.component.x
            y = root.component.y
        }
    }

    // Keep position in sync when the model is updated externally.
    Connections {
        target: root.component
        function onXChanged() { if (!dragArea.drag.active) root.x = root.component.x }
        function onYChanged() { if (!dragArea.drag.active) root.y = root.component.y }
    }

    Rectangle {
        anchors.fill: parent
        radius: 6
        color: root.component ? root.component.color : "#4fc3f7"
        border.color: root.selected ? "#ff5722" : Qt.darker(color, 1.4)
        border.width: root.selected ? 2.5 : 1.5

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

        MouseArea {
            id: dragArea
            anchors.fill: parent
            drag.target: root
            drag.threshold: 4

            onClicked: root.componentClicked(root.component)

            onReleased: {
                if (drag.active && root.component) {
                    root.component.x = root.x
                    root.component.y = root.y
                    root.positionChanged()
                }
            }
        }
    }
}
