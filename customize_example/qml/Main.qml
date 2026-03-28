import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import ComponentMapEditor           // ← the library's QML module

ApplicationWindow {
    id: window
    width: 1280
    height: 720
    visible: true
    // Always maximize
    visibility: Window.Maximized
    title: "My Graph Editor"

    GraphModel {
        id: graph
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Palette {
            id: palettePanel
            Layout.preferredWidth: 150
            Layout.fillHeight: true
            graph: graph
            canvas: canvas
            componentTypeRegistry: customizeComponentTypeRegistry
        }

        // Thin separator
        Rectangle {
            width: 1
            Layout.fillHeight: true
            color: "#e0e0e0"
        }

        GraphCanvas {
            id: canvas
            Layout.fillWidth: true
            Layout.fillHeight: true
            graph: graph
            componentTypeRegistry: customizeComponentTypeRegistry
        }

        // Thin separator
        Rectangle {
            width: 1
            Layout.fillHeight: true
            color: "#e0e0e0"
        }

        PropertyPanel {
            id: propertyPanel
            Layout.preferredWidth: 320
            Layout.fillHeight: true
        }
    }
}
