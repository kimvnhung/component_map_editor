import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ColumnLayout {
    id: root
    property var manager
    property var graph
    property string graphVersion: "v1"
    Layout.fillWidth: true
    spacing: 4

    Label { text: "Deployment"; font.bold: true }
    RowLayout {
        Layout.fillWidth: true
        TextField { id: version; text: root.graphVersion; Layout.fillWidth: true; placeholderText: "Graph version" }
        Button { text: "Deploy"; onClicked: manager && manager.deployAndActivate(root.graph, version.text) }
    }
    RowLayout {
        Button { text: "Start"; onClicked: manager && manager.startRuntime() }
        Button { text: "Stop"; onClicked: manager && manager.stopRuntime() }
    }
}
