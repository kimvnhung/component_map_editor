import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import ComponentMapEditor

ColumnLayout {
    property var executionSandbox: editorManager.executionSandbox

    Layout.fillWidth: true
    spacing: 10
    Layout.margins: 10

    Label {
        Layout.fillWidth: true
        text: "Graph Execution"
        font.bold: true
        font.pixelSize: 13
        horizontalAlignment: Text.AlignHCenter
        topPadding: 10
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        color: "#666"
        text: "Step through the graph using the loaded execution semantics.\nStart → Step nodes one by one, or Run to complete all at once."
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 4

        Button {
            text: "Start"
            onClicked: {
                if (!executionSandbox)
                    return;
                if (executionSandbox.start()) {
                    statusLabel.text = "Sandbox initialized";
                    statusLabel.color = "#1565c0";
                } else {
                    statusLabel.text = "Start failed: " + executionSandbox.lastError;
                    statusLabel.color = "#c62828";
                }
            }
        }

        Button {
            text: "Step"
            enabled: executionSandbox && executionSandbox.status !== "completed" && executionSandbox.status !== "error"
            onClicked: {
                if (!executionSandbox)
                    return;
                if (executionSandbox.status === "idle")
                    executionSandbox.start();
                executionSandbox.step();
            }
        }

        Button {
            text: "Run"
            enabled: executionSandbox && executionSandbox.status !== "completed" && executionSandbox.status !== "error"
            onClicked: {
                if (!executionSandbox)
                    return;
                if (executionSandbox.status === "idle") {
                    if (!executionSandbox.start())
                        return;
                }
                executionSandbox.run();
            }
        }

        Button {
            text: "Reset"
            onClicked: {
                if (executionSandbox)
                    executionSandbox.reset();
            }
        }
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 2
        columnSpacing: 8
        rowSpacing: 4

        Label {
            text: "Status"
            font.bold: true
        }
        Label {
            text: executionSandbox ? executionSandbox.status : "unavailable"
        }

        Label {
            text: "Tick"
            font.bold: true
        }
        Label {
            text: executionSandbox ? String(executionSandbox.currentTick) : "0"
        }

        Label {
            text: "Summary"
            font.bold: true
        }
        ReadOnlyPlainText {
            preferredHeight: 96
            panelText: executionSandbox ? prettyJson(executionSandbox.snapshotSummary()) : "{}"
        }

        Label {
            text: "Last Error"
            font.bold: true
            visible: executionSandbox && executionSandbox.lastError.length > 0
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: "#c62828"
            visible: executionSandbox && executionSandbox.lastError.length > 0
            text: executionSandbox ? executionSandbox.lastError : ""
        }
    }

    Label {
        Layout.fillWidth: true
        text: "Execution State"
        font.bold: true
    }

    ReadOnlyPlainText {
        preferredHeight: 140
        panelText: executionSandbox ? prettyJson(executionSandbox.executionState) : "{}"
    }

    Label {
        Layout.fillWidth: true
        text: "Selected Component State"
        font.bold: true
    }

    ReadOnlyPlainText {
        preferredHeight: 140
        panelText: selectedExecutionStateText()
    }

    TimelinePanel {
        Layout.fillWidth: true
        Layout.preferredHeight: 250
        model: executionSandbox ? executionSandbox.timeline : []
    }
}
