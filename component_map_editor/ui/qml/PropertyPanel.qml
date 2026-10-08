// PropertyPanel.qml — schema-driven inspector for selected component/connection.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ComponentMapEditor

Rectangle {
    id: root

    property GraphModel graph: null
    // Internal model reference. This is used to determine which model is currently being inspected.
    property var _model: null
    property UndoStack undoStack: null
    property var providerOutputKeyHints: ({})
    property TokenKeyCatalog tokenKeyCatalog: null

    // Unified entry point. Callers can set this to either a ComponentModel,
    // a ConnectionModel, or null. The handler dispatches to component/connection
    // using duck-typing: ConnectionModel is identified by having a sourceId
    // property while ComponentModel does not.
    property var item: null

    onItemChanged: {
        if (!item) {
            root._model = root.graph;
        } else {
            root._model = item;
        }
    }

    readonly property bool isComponentModel: root._model && root._model.type !== undefined
    readonly property bool isConnectionModel: root._model && root._model.sourceId !== undefined
    readonly property bool isGraphModel: !isComponentModel && !isConnectionModel

    property PropertySchemaRegistry propertySchemaRegistry: null
    readonly property PropertySchemaRegistry effectiveSchemaRegistry: root.propertySchemaRegistry ? root.propertySchemaRegistry : fallbackSchemaRegistry
    readonly property TokenKeyCatalog effectiveTokenKeyCatalog: root.tokenKeyCatalog ? root.tokenKeyCatalog : fallbackTokenKeyCatalog

    readonly property var connectionSideModel: [
        {
            text: "Auto",
            value: ConnectionModel.SideAuto
        },
        {
            text: "Top",
            value: ConnectionModel.SideTop
        },
        {
            text: "Right",
            value: ConnectionModel.SideRight
        },
        {
            text: "Bottom",
            value: ConnectionModel.SideBottom
        },
        {
            text: "Left",
            value: ConnectionModel.SideLeft
        }
    ]

    readonly property string target: {
        if (!root._model)
            return "";

        if (root._model.sourceId !== undefined)
            return "connection/flow";

        if (root._model.type !== undefined)
            return "component/" + (root._model.type || "default");

        // Need to same with componentId in GraphPropertySchemaProvider
        return "graph";
    }

    readonly property var activeSchemaSections: root.sectionsForTarget(root.target)

    readonly property var activeSchemaSectionModel: {
        if (!root.effectiveSchemaRegistry)
            return null;
        var targetId = root.target;
        if (!targetId || !targetId.length)
            return null;
        return root.effectiveSchemaRegistry.typedSectionModelForTarget(targetId);
    }

    readonly property var dynamicFieldOptions: ({
            "tokenKeys": root.effectiveTokenKeyCatalog ? root.effectiveTokenKeyCatalog.tokenKeys : [],
            "tokenKeyOptions": root.effectiveTokenKeyCatalog ? root.effectiveTokenKeyCatalog.tokenKeyOptions : []
        })

    function updateProperty(propertyName, value) {
        if (!root._model || !propertyName)
            return;

        if (root.isComponentModel) {
            if (!root.undoStack)
                return;
            root.undoStack.pushSetComponentProperty(root._model, propertyName, value);
        } else if (root.isConnectionModel) {
            if (!root.undoStack)
                return;
            if (propertyName === "sourceSide") {
                root.undoStack.pushSetConnectionSides(root._model, value, root._model.targetSide);
                return;
            }

            if (propertyName === "targetSide") {
                root.undoStack.pushSetConnectionSides(root._model, root._model.sourceSide, value);
                return;
            }

            root.undoStack.pushSetConnectionProperty(root._model, propertyName, value);
        } else if (root.isGraphModel) {
            if (!root.undoStack)
                return;
            root.undoStack.pushSetGraphProperty(root._model, propertyName, value);
        }
    }

    function sectionsForTarget(targetId) {
        if (root.effectiveSchemaRegistry)
            return root.effectiveSchemaRegistry.sectionedSchemaForTarget(targetId);
        return [];
    }

    PropertySchemaRegistry {
        id: fallbackSchemaRegistry
    }

    TokenKeyCatalog {
        id: fallbackTokenKeyCatalog
        graph: root.graph
        providerOutputKeyHints: root.providerOutputKeyHints
        targetComponentId: root.isComponentModel ? root._model.id : ""
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

        SchemaFormRenderer {
            Layout.fillWidth: true
            schemaSections: root.activeSchemaSections
            schemaSectionModel: root.activeSchemaSectionModel
            modelObject: root._model
            expectedModelObjectId: root.isGraphModel ? "" : root._model.id
            expectedSchemaTarget: root.target
            readOnly: root.undoStack === null
            sideModel: root.connectionSideModel
            dynamicOptions: root.dynamicFieldOptions
            onPropertyEditRequested: function (propertyName, value, sourceModelObject) {
                var activeModelObject = root._model;
                if (sourceModelObject !== activeModelObject)
                    return;
                root.updateProperty(propertyName, value);
            }
        }

        Label {
            visible: (!root.isGraphModel) && root.undoStack === null
            text: "Inspector is read-only because UndoStack is not available."
            color: "#b26a00"
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
