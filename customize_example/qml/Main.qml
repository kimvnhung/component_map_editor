import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import ComponentMapEditor           // ← the library's QML module

ApplicationWindow {
    width: 800
    height: 600
    visible: true
    title: "My Graph Editor"

    GraphModel {
        id: graph
    }

    GraphCanvas {
        anchors.fill: parent
        graph: graph
    }
}
