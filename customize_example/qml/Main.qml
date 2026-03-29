import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import ComponentMapEditor           // ← the library's QML module

ApplicationWindow {
    id: window
    width: 1280
    height: 720
    visible: true
    // Always maximize
    visibility: Window.Maximized
    title: "My Graph Editor"

    GraphModel {
        id: graph
    }

    ValidationService {
        id: validator
    }

    ExportService {
        id: exporter
    }

    // -----------------------------------------------------------------------
    // Toolbar
    // -----------------------------------------------------------------------
    header: ToolBar {
        id: toolBar
        property UndoStack undoStack: canvas ? canvas.undoStack : {}

        RowLayout {
            anchors {
                fill: parent
                leftMargin: 6
                rightMargin: 6
            }
            spacing: 4

            ToolButton {
                text: "⟵ Undo"
                enabled: toolBar.undoStack.canUndo
                ToolTip.visible: hovered
                ToolTip.text: toolBar.undoStack.undoText
                onClicked: toolBar.undoStack.undo()
            }
            ToolButton {
                text: "Redo ⟶"
                enabled: toolBar.undoStack.canRedo
                ToolTip.visible: hovered
                ToolTip.text: toolBar.undoStack.redoText
                onClicked: toolBar.undoStack.redo()
            }

            ToolSeparator {}

            ToolButton {
                text: "Validate"
                onClicked: {
                    var errs = validator.validationErrors(graph);
                    if (errs.length === 0) {
                        statusLabel.text = "✓ Graph is valid";
                        statusLabel.color = "#2e7d32";
                    } else {
                        statusLabel.text = "✗ " + errs.join("  |  ");
                        statusLabel.color = "#c62828";
                    }
                }
            }

            ToolButton {
                text: "Export JSON"
                onClicked: {
                    var json = exporter.exportToJson(graph);
                    exportDialog.jsonText = json;
                    exportDialog.open();
                }
            }

            ToolButton {
                text: "Import JSON"
                onClicked: {
                    importDialog.errorText = "";
                    importDialog.jsonText = "";
                    importDialog.open();
                }
            }

            ToolButton {
                text: "Clear"
                onClicked: {
                    if (canvas)
                        canvas.resetAllState();
                    propertyPanel.item = null;
                    graph.clear();
                    undoStack.clear();
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                id: statusLabel
                text: "Ready"
                color: "#555"
                rightPadding: 4
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Palette {
            id: palettePanel
            Layout.preferredWidth: 150
            Layout.fillHeight: true
            graph: graph
            canvas: canvas
            componentTypeRegistry: customizeComponentTypeRegistry
        }

        // Thin separator
        Rectangle {
            width: 1
            Layout.fillHeight: true
            color: "#e0e0e0"
        }

        GraphCanvas {
            id: canvas
            Layout.fillWidth: true
            Layout.fillHeight: true
            graph: graph
            componentTypeRegistry: customizeComponentTypeRegistry

            onComponentSelected: component => {
                propertyPanel.item = component;
            }
            onConnectionSelected: connection => {
                propertyPanel.item = connection;
            }
            onBackgroundClicked: {
                propertyPanel.item = null;
            }
        }

        // Thin separator
        Rectangle {
            width: 1
            Layout.fillHeight: true
            color: "#e0e0e0"
        }

        PropertyPanel {
            id: propertyPanel
            Layout.preferredWidth: 320
            Layout.fillHeight: true
            undoStack: canvas ? canvas.undoStack : null
            propertySchemaRegistry: customizePropertySchemaRegistry
        }
    }

    // -----------------------------------------------------------------------
    // Export JSON dialog
    // -----------------------------------------------------------------------
    Dialog {
        id: exportDialog
        title: "Exported JSON"
        width: 600
        height: 450
        anchors.centerIn: parent

        property string jsonText: ""
        property string copyStatusText: ""

        contentItem: ColumnLayout {
            spacing: 8

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                TextArea {
                    id: exportJsonTextArea
                    text: exportDialog.jsonText
                    readOnly: true
                    font.family: "monospace"
                    font.pixelSize: 12
                    wrapMode: TextArea.Wrap
                }
            }

            RowLayout {
                Layout.fillWidth: true

                Label {
                    text: exportDialog.copyStatusText
                    color: "#2e7d32"
                    visible: text.length > 0
                }

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    text: "Copy"
                    onClicked: {
                        exportJsonTextArea.selectAll();
                        exportJsonTextArea.copy();
                        exportJsonTextArea.deselect();
                        exportDialog.copyStatusText = "Copied to clipboard";
                        copyStatusResetTimer.restart();
                    }
                }

                Button {
                    text: "Close"
                    onClicked: exportDialog.close()
                }
            }
        }

        onOpened: exportDialog.copyStatusText = ""

        Timer {
            id: copyStatusResetTimer
            interval: 1500
            repeat: false
            onTriggered: exportDialog.copyStatusText = ""
        }
    }

    Dialog {
        id: importDialog
        title: "Import Graph JSON"
        width: 600
        height: 500
        anchors.centerIn: parent

        property string jsonText: ""
        property string errorText: ""

        contentItem: ColumnLayout {
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: "Paste exported graph JSON below and click Import."
                color: "#555"
                wrapMode: Text.WordWrap
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true

                TextArea {
                    id: importTextArea
                    text: importDialog.jsonText
                    placeholderText: "{\n  \"coordinateSystem\": \"world-center-y-down-v3\",\n  \"components\": [],\n  \"connections\": []\n}"
                    font.family: "monospace"
                    font.pixelSize: 12
                    wrapMode: TextArea.Wrap
                    onTextChanged: importDialog.jsonText = text
                }
            }

            Label {
                Layout.fillWidth: true
                visible: importDialog.errorText.length > 0
                text: importDialog.errorText
                color: "#c62828"
                wrapMode: Text.WordWrap
            }
        }

        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            var ok = exporter.importFromJson(graph, importDialog.jsonText);
            if (!ok) {
                importDialog.errorText = "Invalid JSON or unsupported format.";
                importDialog.open();
                return;
            }

            if (canvas)
                canvas.resetAllState();
            propertyPanel.item = null;
            statusLabel.text = "✓ Graph imported";
            statusLabel.color = "#2e7d32";
            canvas.connectionRenderer.repaint();
            canvas.componentRenderer.repaint();
        }
    }
}
