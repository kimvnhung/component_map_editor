import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ColumnLayout {
    id: root
    property var manager
    property var entries: []
    Layout.fillWidth: true
    spacing: 4

    Label { text: "Rejections"; font.bold: true }
    Button { text: "Refresh Rejections"; onClicked: root.entries = manager.runtimeRejections(50) }
    ListView {
        Layout.fillWidth: true
        Layout.preferredHeight: 100
        model: root.entries
        delegate: Label { width: ListView.view.width; text: modelData.eventId + " " + modelData.reasonCode; elide: Text.ElideRight }
    }
}
