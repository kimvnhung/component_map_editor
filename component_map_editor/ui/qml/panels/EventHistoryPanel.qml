import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ColumnLayout {
    id: root
    property var manager
    property var entries: []
    Layout.fillWidth: true
    spacing: 4

    Label { text: "Event History"; font.bold: true }
    Button { text: "Refresh History"; onClicked: root.entries = manager.runtimeEventHistory(50) }
    ListView {
        Layout.fillWidth: true
        Layout.preferredHeight: 100
        model: root.entries
        delegate: Label { width: ListView.view.width; text: modelData.eventId + "  " + modelData.status; elide: Text.ElideRight }
    }
}
