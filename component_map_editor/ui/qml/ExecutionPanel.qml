import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import ComponentMapEditor

ColumnLayout {
    id: root
    property string selectedComponentId: ""
    property var selectedStatus: ({})
    property var history: []
    property var logs: []
    property var rejections: []

    Layout.fillWidth: true
    spacing: 10
    Layout.margins: 10

    function runtimeLabel() {
        if (!editorManager || !editorManager.runtimeAddress || editorManager.runtimeAddress.length === 0)
            return "not configured";
        return editorManager.runtimeAddress;
    }

    Label {
        Layout.fillWidth: true
        text: "Runtime Execution"
        font.bold: true
        font.pixelSize: 13
        horizontalAlignment: Text.AlignHCenter
        topPadding: 10
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        color: "#666"
        text: "Execution is owned by the runtime process. Use these controls to inspect and control that runtime."
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 4

        Button {
            text: "Refresh"
            onClicked: refreshRequested()
        }

        Button {
            text: "Start"
            onClicked: startRequested()
        }

        Button {
            text: "Stop"
            onClicked: stopRequested()
        }
    }

    function refreshRequested() {
        if (!editorManager)
            return;
        editorManager.refreshRuntimeStatus();
        history = editorManager.runtimeEventHistory(50);
        logs = editorManager.runtimeLogs(50);
        rejections = editorManager.runtimeRejections(50);
        if (selectedComponentId.length > 0)
            selectedStatus = editorManager.runtimeComponentStatus(selectedComponentId);
        else
            selectedStatus = ({});
    }

    function startRequested() {
        if (editorManager)
            editorManager.startRuntime();
    }

    function stopRequested() {
        if (editorManager)
            editorManager.stopRuntime();
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 2
        columnSpacing: 8
        rowSpacing: 4

        Label {
            text: "Address"
            font.bold: true
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: runtimeLabel()
        }

        Label {
            text: "Status"
            font.bold: true
        }
        Label {
            text: editorManager ? editorManager.runtimeStatus : "unavailable"
        }

        Label {
            text: "Active Graph"
            font.bold: true
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: {
                if (!editorManager || !editorManager.runtimeActiveGraphVersion)
                    return "-";
                return editorManager.runtimeActiveGraphVersion.length === 0
                    ? "-"
                    : editorManager.runtimeActiveGraphVersion;
            }
        }

        Label {
            text: "Last Error"
            font.bold: true
            visible: editorManager && editorManager.runtimeLastError && editorManager.runtimeLastError.length > 0
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: "#c62828"
            visible: editorManager && editorManager.runtimeLastError && editorManager.runtimeLastError.length > 0
            text: editorManager ? editorManager.runtimeLastError : ""
        }

        Label {
            text: "Selected Component"
            font.bold: true
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: selectedComponentId.length > 0 ? selectedComponentId : "Select a component to inspect."
        }

        Label {
            text: "Component Status"
            font.bold: true
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: selectedComponentId.length === 0
                ? "-"
                : (selectedStatus.found ? selectedStatus.status : "Component not found")
        }

        Label {
            text: "Mailbox Depth"
            font.bold: true
        }
        Label {
            Layout.fillWidth: true
            text: selectedComponentId.length > 0 && selectedStatus.found
                ? String(selectedStatus.mailboxDepth)
                : "-"
        }

        Label {
            text: "Event History"
            font.bold: true
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: history.length > 0 ? String(history.length) + " events" : "No events"
        }

        Label {
            text: "Runtime Logs"
            font.bold: true
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: logs.length > 0 ? String(logs.length) + " entries" : "No logs"
        }

        Label {
            text: "Rejections"
            font.bold: true
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: rejections.length > 0 ? String(rejections.length) : "None"
        }
    }
}
