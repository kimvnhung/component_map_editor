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

    // Unified entry point. Callers can set this to either a ComponentModel,
    // a ConnectionModel, or null. The handler dispatches to component/connection
    // using duck-typing: ConnectionModel is identified by having a sourceId
    // property while ComponentModel does not.
    property var item: null

    onItemChanged: {
        if (!item) {
            root.component = null
            root.connection = null
        } else if (item.sourceId !== undefined) {
            root.connection = item
            root.component = null
        } else {
            root.component = item
            root.connection = null
        }
    }
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
            active: root.component !== null || root.connection !== null
            Layout.fillWidth: true
            sourceComponent: unifiedInspector
        }

        Component {
            id: unifiedInspector

            SchemaFormRenderer {
                width: parent ? parent.width : 0
                schemaSections: root.component !== null
                    ? root.sectionsForTarget(root.componentTarget)
                    : root.sectionsForTarget(root.connectionTarget)
                modelObject: root.component !== null ? root.component : root.connection
                readOnly: root.undoStack === null
                sideModel: root.connectionSideModel
                onPropertyEditRequested: function(propertyName, value) {
                    if (root.component !== null)
                        root.updateComponentProperty(propertyName, value)
                    else
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
