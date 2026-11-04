import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import ComponentMapEditor

ScrollView {
    id: root
    property string selectedComponentId: ""
    property var graph
    property var selectedStatus: ({})
    property var history: []
    property var logs: []
    property var rejections: []
    clip: true

    ColumnLayout {
        width: root.availableWidth
        spacing: 12
        anchors.margins: 10
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        RuntimeOverviewPanel { manager: editorManager }
        DeploymentPanel { manager: editorManager; graph: root.graph; graphVersion: "v1" }
        ComponentStatusPanel {
            manager: editorManager
            componentId: root.selectedComponentId
            status: root.selectedStatus
        }
        EventHistoryPanel { manager: editorManager; entries: root.history }
        RuntimeLogPanel { manager: editorManager; entries: root.logs }
        RejectionPanel { manager: editorManager; entries: root.rejections }

        Label {
            Layout.fillWidth: true
            visible: editorManager && editorManager.runtimeLastError.length > 0
            color: "#c62828"
            wrapMode: Text.WordWrap
            text: editorManager ? editorManager.runtimeLastError : ""
        }
    }

    Timer {
        interval: editorManager ? editorManager.runtimePollIntervalMs : 2000
        repeat: true
        running: visible && editorManager !== null
        onTriggered: {
            if (!editorManager)
                return;
            editorManager.refreshRuntimeStatus();
            root.history = editorManager.runtimeEventHistory(50);
            root.logs = editorManager.runtimeLogs(50);
            root.rejections = editorManager.runtimeRejections(50);
            if (root.selectedComponentId.length > 0)
                root.selectedStatus = editorManager.runtimeComponentStatus(root.selectedComponentId);
        }
    }
}
