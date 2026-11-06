import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ColumnLayout {
    id: root
    property var manager
    property var entries: []
    Layout.fillWidth: true
    spacing: 4

    Label { text: "Runtime Logs"; font.bold: true }
    Button { text: "Refresh Logs"; onClicked: root.entries = manager.runtimeLogs(50) }
    ListView {
        Layout.fillWidth: true
        Layout.preferredHeight: 100
        model: root.entries
        delegate: Label { width: ListView.view.width; text: modelData.level + " " + modelData.code; elide: Text.ElideRight }
    }
}
