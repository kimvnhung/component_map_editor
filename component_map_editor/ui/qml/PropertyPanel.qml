// PropertyPanel.qml — schema-driven inspector for selected component/connection.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ComponentMapEditor

Rectangle {
    id: root

    property ComponentModel component: null
    property ConnectionModel connection: null
    property UndoStack undoStack: null
    property PropertySchemaRegistry propertySchemaRegistry: null
    readonly property PropertySchemaRegistry effectiveSchemaRegistry:
        root.propertySchemaRegistry ? root.propertySchemaRegistry : fallbackSchemaRegistry

    readonly property var connectionSideModel: [
        { text: "Auto", value: ConnectionModel.SideAuto },
        { text: "Top", value: ConnectionModel.SideTop },
        { text: "Right", value: ConnectionModel.SideRight },
        { text: "Bottom", value: ConnectionModel.SideBottom },
        { text: "Left", value: ConnectionModel.SideLeft }
    ]

    readonly property string componentTarget: {
        if (!root.component)
            return ""
        var typeId = root.component.type || "default"
        return "component/" + typeId
    }

    readonly property string connectionTarget: {
        if (!root.connection)
            return ""
        return "connection/flow"
    }

    function updateComponentProperty(propertyName, value) {
        if (!root.component || !root.undoStack || !propertyName)
            return
        root.undoStack.pushSetComponentProperty(root.component, propertyName, value)
    }

    function updateConnectionProperty(propertyName, value) {
        if (!root.connection || !root.undoStack || !propertyName)
            return

        if (propertyName === "sourceSide") {
            root.undoStack.pushSetConnectionSides(root.connection, value, root.connection.targetSide)
            return
        }

        if (propertyName === "targetSide") {
            root.undoStack.pushSetConnectionSides(root.connection, root.connection.sourceSide, value)
            return
        }

        root.undoStack.pushSetConnectionProperty(root.connection, propertyName, value)
    }

    function sectionsForTarget(targetId) {
        if (root.effectiveSchemaRegistry)
            return root.effectiveSchemaRegistry.sectionedSchemaForTarget(targetId)
        return []
    }

    PropertySchemaRegistry {
        id: fallbackSchemaRegistry
    }

    color: "#ffffff"
    border.color: "#e0e0e0"
    border.width: 1

    ColumnLayout {
        anchors {
            fill: parent
            margins: 10
        }
        spacing: 8

        Label {
            text: "Properties"
            font.bold: true
            font.pixelSize: 13
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            bottomPadding: 4
        }

        Loader {
            active: root.component !== null
            Layout.fillWidth: true
            sourceComponent: componentInspector
        }

        Component {
            id: componentInspector

            SchemaFormRenderer {
                width: parent ? parent.width : 0
                schemaSections: root.sectionsForTarget(root.componentTarget)
                modelObject: root.component
                readOnly: root.undoStack === null
                onPropertyEditRequested: function(propertyName, value) {
                    root.updateComponentProperty(propertyName, value)
                }
            }
        }

        Loader {
            active: root.connection !== null
            Layout.fillWidth: true
            sourceComponent: connectionInspector
        }

        Component {
            id: connectionInspector

            SchemaFormRenderer {
                width: parent ? parent.width : 0
                schemaSections: root.sectionsForTarget(root.connectionTarget)
                modelObject: root.connection
                readOnly: root.undoStack === null
                sideModel: root.connectionSideModel
                onPropertyEditRequested: function(propertyName, value) {
                    root.updateConnectionProperty(propertyName, value)
                }
            }
        }

        Label {
            visible: root.component === null && root.connection === null
            text: "Select a component or connection\nto view its properties."
            wrapMode: Text.WordWrap
            color: "#aaa"
            font.pixelSize: 12
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        Label {
            visible: (root.component !== null || root.connection !== null) && root.undoStack === null
            text: "Inspector is read-only because UndoStack is not available."
            color: "#b26a00"
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Item { Layout.fillHeight: true }
    }
}
