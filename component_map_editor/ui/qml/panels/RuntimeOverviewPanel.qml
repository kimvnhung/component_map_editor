import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ColumnLayout {
    id: root
    property var manager
    Layout.fillWidth: true
    spacing: 4

    Label { text: "Runtime Overview"; font.bold: true }
    GridLayout {
        columns: 2
        Layout.fillWidth: true
        Label { text: "Address"; font.bold: true }
        Label { text: manager ? manager.runtimeAddress : "-"; elide: Text.ElideRight; Layout.fillWidth: true }
        Label { text: "Channel"; font.bold: true }
        Label { text: manager ? manager.runtimeChannelState : "unknown"; Layout.fillWidth: true }
        Label { text: "Status"; font.bold: true }
        Label { text: manager ? manager.runtimeStatus : "unknown"; Layout.fillWidth: true }
        Label { text: "Active Graph"; font.bold: true }
        Label { text: manager && manager.runtimeActiveGraphVersion.length > 0 ? manager.runtimeActiveGraphVersion : "-"; Layout.fillWidth: true }
    }
    RowLayout {
        Layout.fillWidth: true
        Button { text: "Refresh"; onClicked: manager && manager.refreshRuntimeStatus() }
        Button { text: "Reconnect"; onClicked: manager && manager.reconnectRuntime() }
    }
}
