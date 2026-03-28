import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import ComponentMapEditor           // ← the library's QML module

ApplicationWindow {
    width: 1280
    height: 720
    visible: true
    title: "My Graph Editor"

    GraphModel {
        id: graph
    }

    GraphCanvas {
        id: graphCanvas
        anchors.fill: parent
        graph: graph
    }

    Palette {
        id: palette
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 200
        graph: graph
        canvas: graphCanvas
        componentTypeRegistry: customizeComponentTypeRegistry
    }

    PropertyPanel {
        id: propertyPanel
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 200
    }
}
