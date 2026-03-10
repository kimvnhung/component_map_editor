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
            n1.label = "Start"
            n1.x = 80
            n1.y = -220
            n1.color = "#26a69a"
            n1.type = "start"
            graph.addComponent(n1)

            // var n2 = Qt.createQmlObject('import ComponentMapEditor; ComponentModel {}', graph)
            // n2.id = "n2"; n2.label = "Process A"; n2.x = 280; n2.y = 140
            // n2.color = "#5c6bc0"; n2.type = "process"
            // graph.addComponent(n2)

            // var n3 = Qt.createQmlObject('import ComponentMapEditor; ComponentModel {}', graph)
            // n3.id = "n3"; n3.label = "Process B"; n3.x = 280; n3.y = 300
            // n3.color = "#5c6bc0"; n3.type = "process"
            // graph.addComponent(n3)

            // var n4 = Qt.createQmlObject('import ComponentMapEditor; ComponentModel {}', graph)
            // n4.id = "n4"; n4.label = "Decide";  n4.x = 480; n4.y = 220
            // n4.color = "#ab47bc"; n4.type = "decision"
            // graph.addComponent(n4)

            // var n5 = Qt.createQmlObject('import ComponentMapEditor; ComponentModel {}', graph)
            // n5.id = "n5"; n5.label = "End";     n5.x = 680; n5.y = 220
            // n5.color = "#ef5350"; n5.type = "end"
            // graph.addComponent(n5)

            // // Seed some connections
            // var e1 = Qt.createQmlObject('import ComponentMapEditor; ConnectionModel {}', graph)
            // e1.id = "e1"; e1.sourceId = "n1"; e1.targetId = "n2"; e1.label = "path A"
            // graph.addConnection(e1)

            // var e2 = Qt.createQmlObject('import ComponentMapEditor; ConnectionModel {}', graph)
            // e2.id = "e2"; e2.sourceId = "n1"; e2.targetId = "n3"; e2.label = "path B"
            // graph.addConnection(e2)

            // var e3 = Qt.createQmlObject('import ComponentMapEditor; ConnectionModel {}', graph)
            // e3.id = "e3"; e3.sourceId = "n2"; e3.targetId = "n4"
            // graph.addConnection(e3)

            // var e4 = Qt.createQmlObject('import ComponentMapEditor; ConnectionModel {}', graph)
            // e4.id = "e4"; e4.sourceId = "n3"; e4.targetId = "n4"
            // graph.addConnection(e4)

            // var e5 = Qt.createQmlObject('import ComponentMapEditor; ConnectionModel {}', graph)
            // e5.id = "e5"; e5.sourceId = "n4"; e5.targetId = "n5"; e5.label = "done"
            // graph.addConnection(e5)
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
                text: "Clear"
                onClicked: {
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

    // -----------------------------------------------------------------------
    // Main layout: Palette | Canvas | PropertyPanel
    // -----------------------------------------------------------------------
    RowLayout {
        anchors.fill: parent
        spacing: 0

        Palette {
            id: palettePanel
            Layout.preferredWidth: 150
            Layout.fillHeight: true
            graph: graph
            undoStack: undoStack
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

        ScrollView {
            anchors.fill: parent
            TextArea {
                text: exportDialog.jsonText
                readOnly: true
                font.family: "monospace"
                font.pixelSize: 12
                wrapMode: TextArea.Wrap
            }
        }

        standardButtons: Dialog.Close
    }
}
