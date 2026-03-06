// ComponentItem.qml — A draggable component card displayed on the GraphCanvas.
import QtQuick
import ComponentMapEditor

Item {
    id: root

    property ComponentModel component: null
    property bool selected: false
    property UndoStack undoStack: null

    readonly property int defaultComponentWidth: 120
    readonly property int defaultComponentHeight: 40
    readonly property real connectionPointRadius: 3
    readonly property bool isRounded: !root.component || root.component.shape === "rounded"

    signal componentClicked(ComponentModel component)
    // Emitted after a drag finishes so the canvas can redraw connections.
    signal positionChanged()

    width:  root.component ? root.component.width : defaultComponentWidth
    height: root.component ? root.component.height : defaultComponentHeight
    z: selected ? 2 : 1

    // Initialise position from the model; don't bind so dragging works.
    Component.onCompleted: {
        if (root.component) {
            x = root.component.x - (root.width / 2)
            y = root.component.y - (root.height / 2)
        }
    }

    // Keep position in sync when the model is updated externally.
    Connections {
        target: root.component
        function onXChanged() {
            if (!dragArea.drag.active)
                root.x = root.component.x - (root.width / 2)
        }
        function onYChanged() {
            if (!dragArea.drag.active)
                root.y = root.component.y - (root.height / 2)
        }
        function onWidthChanged() {
            if (!dragArea.drag.active)
                root.x = root.component.x - (root.width / 2)
        }
        function onHeightChanged() {
            if (!dragArea.drag.active)
                root.y = root.component.y - (root.height / 2)
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

        MouseArea {
            id: dragArea
            anchors.fill: parent
            drag.target: root
            drag.threshold: 4

            onClicked: root.componentClicked(root.component)

            onReleased: {
                if (drag.active && root.component) {
                    root.component.x = root.x + (root.width / 2)
                    root.component.y = root.y + (root.height / 2)
                    root.positionChanged()
                }
            }
        }

        Repeater {
            model: [
                Qt.point(width / 2, 0),
                Qt.point(width, height / 2),
                Qt.point(width / 2, height),
                Qt.point(0, height / 2)
            ]

            delegate: Rectangle {
                required property point modelData
                width: root.connectionPointRadius * 2
                height: root.connectionPointRadius * 2
                radius: root.connectionPointRadius
                x: modelData.x - root.connectionPointRadius
                y: modelData.y - root.connectionPointRadius
                color: "#ffffff"
                border.color: "#607d8b"
                border.width: 1
                visible: root.selected
            }
        }
    }
}
