import QtQuick

Rectangle {
    id: root

    property int componentCount: 0
    property int connectionCount: 0
    property int selectedComponentCount: 0
    property int selectedConnectionCount: 0
    readonly property int selectedTotalCount: selectedComponentCount + selectedConnectionCount
    property string selectedComponentLabel: "none"
    property string selectedConnectionLabel: "none"
    property point mouseViewPos: Qt.point(0, 0)
    property point mouseWorldPos: Qt.point(0, 0)
    property real zoom: 1.0

    function formatCoord(value) {
        return Number(value).toFixed(1)
    }

    height: 26
    color: "#263238"
    opacity: 0.88

    Text {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        verticalAlignment: Text.AlignVCenter
        color: "#eceff1"
        font.pixelSize: 12
        elide: Text.ElideRight
        text: "Components: " + root.componentCount
              + " | Connections: " + root.connectionCount
              + " | Selected Components: " + root.selectedComponentCount
              + " | Selected Connections: " + root.selectedConnectionCount
              + " | Selected Total: " + root.selectedTotalCount
              + " | Selected Component: " + root.selectedComponentLabel
              + " | Selected Connection: " + root.selectedConnectionLabel
              + " | Mouse (view): [" + root.formatCoord(root.mouseViewPos.x)
              + ", " + root.formatCoord(root.mouseViewPos.y) + "]"
              + " | Mouse (world): [" + root.formatCoord(root.mouseWorldPos.x)
              + ", " + root.formatCoord(root.mouseWorldPos.y) + "]"
              + " | Zoom: " + root.zoom.toFixed(2)
    }
}
