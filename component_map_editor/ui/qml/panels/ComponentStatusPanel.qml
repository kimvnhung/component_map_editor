import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ColumnLayout {
    id: root
    property var manager
    property string componentId: ""
    property var status: ({})
    Layout.fillWidth: true
    spacing: 4

    Label { text: "Component Status"; font.bold: true }
    Button { text: "Refresh Component"; enabled: root.componentId.length > 0; onClicked: root.status = manager.runtimeComponentStatus(root.componentId) }
    Label { text: root.status.found ? root.status.componentId : "Component not found"; Layout.fillWidth: true }
    Label { text: root.status.found ? "Status: " + root.status.status : "-"; Layout.fillWidth: true }
    Label { text: root.status.found ? "Mailbox: " + root.status.mailboxDepth : "-"; Layout.fillWidth: true }
}
