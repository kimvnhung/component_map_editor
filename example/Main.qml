import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import ComponentMapEditor

ApplicationWindow {
    id: window
    width: 1100
    height: 720
    visible: true
    title: "Component Map Editor — Example"

    // -----------------------------------------------------------------------
    // Data models
    // -----------------------------------------------------------------------
    GraphModel {
        id: graph

        Component.onCompleted: {
            // Seed some initial components
            var n1 = Qt.createQmlObject(
                        'import ComponentMapEditor; ComponentModel {}', graph)
            n1.id = "n1"
            n1.title = "Start"
            n1.x = 860
            n1.y = 540
            n1.color = "#26a69a"
            n1.type = "start"
            n1.inputNumber = 12
            graph.addComponent(n1)

            var n2 = Qt.createQmlObject('import ComponentMapEditor; ComponentModel {}', graph)
            n2.id = "n2"
            n2.title = "Process"
            n2.x = 1080
            n2.y = 540
            n2.color = "#5c6bc0"
            n2.type = "process"
            n2.addValue = 9
            graph.addComponent(n2)

            var n3 = Qt.createQmlObject('import ComponentMapEditor; ComponentModel {}', graph)
            n3.id = "n3"
            n3.title = "Stop"
            n3.x = 1320
            n3.y = 540
            n3.color = "#ef5350"
            n3.type = "stop"
            graph.addComponent(n3)

            var e1 = Qt.createQmlObject('import ComponentMapEditor; ConnectionModel {}', graph)
            e1.id = "e1"
            e1.sourceId = "n1"
            e1.targetId = "n2"
            e1.label = "input"
            graph.addConnection(e1)

            var e2 = Qt.createQmlObject('import ComponentMapEditor; ConnectionModel {}', graph)
            e2.id = "e2"
            e2.sourceId = "n2"
            e2.targetId = "n3"
            e2.label = "+9"
            graph.addConnection(e2)
        }
    }

    UndoStack {
        id: undoStack
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
        RowLayout {
            anchors {
                fill: parent
                leftMargin: 6
                rightMargin: 6
            }
            spacing: 4

            ToolButton {
                text: "⟵ Undo"
                enabled: undoStack.canUndo
                ToolTip.visible: hovered
                ToolTip.text: undoStack.undoText
                onClicked: undoStack.undo()
            }
            ToolButton {
                text: "Redo ⟶"
                enabled: undoStack.canRedo
                ToolTip.visible: hovered
                ToolTip.text: undoStack.redoText
                onClicked: undoStack.redo()
            }

            ToolSeparator {}

            ToolButton {
                text: "Validate"
                onClicked: {
                    var errs = validator.validationErrors(graph)
                    if (errs.length === 0) {
                        statusLabel.text = "✓ Graph is valid"
                        statusLabel.color = "#2e7d32"
                    } else {
                        statusLabel.text = "✗ " + errs.join("  |  ")
                        statusLabel.color = "#c62828"
                    }
                }
            }

            ToolButton {
                text: "Export JSON"
                onClicked: {
                    var json = exporter.exportToJson(graph)
                    exportDialog.jsonText = json
                    exportDialog.open()
                }
            }

            ToolButton {
                text: "Import JSON"
                onClicked: {
                    importDialog.errorText = ""
                    importDialog.jsonText = ""
                    importDialog.open()
                }
            }

            ToolButton {
                text: "Clear"
                onClicked: {
                    if (canvas)
                        canvas.resetAllState()
                    propertyPanel.component = null
                    propertyPanel.connection = null
                    graph.clear()
                    undoStack.clear()
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

    // Performance telemetry — disabled by default; activate via BenchmarkPanel.
    PerformanceTelemetry {
        id: perfTelemetry
    }

    // High-resolution frame interval telemetry based on QQuickWindow::frameSwapped.
    FrameSwapTelemetry {
        id: frameTelemetry
        window: window
    }

    // -----------------------------------------------------------------------
    // Main layout: Palette | Canvas | PropertyPanel  +  Benchmark footer
    // -----------------------------------------------------------------------
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

        Palette {
            id: palettePanel
            Layout.preferredWidth: 150
            Layout.fillHeight: true
            graph: graph
            undoStack: undoStack
            canvas: canvas
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
            undoStack: undoStack
            telemetry: perfTelemetry

            onComponentSelected: component => {
                                     propertyPanel.component = component
                                     propertyPanel.connection = null
                                 }
            onConnectionSelected: connection => {
                                      propertyPanel.connection = connection
                                      propertyPanel.component = null
                                  }
            onBackgroundClicked: {
                propertyPanel.component = null
                propertyPanel.connection = null
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
            Layout.preferredWidth: 210
            Layout.fillHeight: true
            undoStack: undoStack
            propertySchemaRegistry: startupPropertySchemaRegistry
        }
        }

        // Thin separator above benchmark panel
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#e0e0e0"
        }

        BenchmarkPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
            graph: graph
            appWindow: window
            canvas: canvas
            telemetry: perfTelemetry
            frameTelemetry: frameTelemetry
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
                        exportJsonTextArea.selectAll()
                        exportJsonTextArea.copy()
                        exportJsonTextArea.deselect()
                        exportDialog.copyStatusText = "Copied to clipboard"
                        copyStatusResetTimer.restart()
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
            var ok = exporter.importFromJson(graph, importDialog.jsonText)
            if (!ok) {
                importDialog.errorText = "Invalid JSON or unsupported format."
                importDialog.open()
                return
            }

            if (canvas)
                canvas.resetAllState()
            propertyPanel.component = null
            propertyPanel.connection = null
            statusLabel.text = "✓ Graph imported"
            statusLabel.color = "#2e7d32"
            canvas.edgeRenderer.repaint()
            canvas.nodeRenderer.repaint()
        }
    }
}
