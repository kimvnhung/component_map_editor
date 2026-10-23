import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    readonly property int widgetUnknown: 0
    readonly property int widgetTextField: 1
    readonly property int widgetTextArea: 2
    readonly property int widgetDropdown: 3
    readonly property int widgetCheckbox: 4
    readonly property int widgetSpinBox: 5
    readonly property int widgetSchemaError: 6

    // Mirrors cme::runtime::SchemaOptionsSource (SchemaOptionsSpec.h).
    readonly property int optionsSourceNone: 0
    readonly property int optionsSourceTokenRef: 1
    readonly property int optionsSourceDefined: 2
    readonly property int optionsSourceCustom: 3
    // Kept as an alias for QML callers using the pre-canonical name.
    readonly property int optionsSourceTokenKeys: optionsSourceTokenRef
    readonly property string fallbackSelectionValue: "__use_fallback__"
    readonly property string defaultUnsetLabel: "Use fallback value"

    // Typed model input (QAbstractListModel-based). Fields are read through
    // rowAt() so every field carries the normalized contract (dropdownOptions).
    property var schemaSectionModel: null
    property var modelObject: null
    property bool readOnly: false
    // This should be contains all the dynamic options for the schema fields. The key should match the optionsSource of the field.
    property var dynamicOptions: ({})
    property int renderContextVersion: 0
    property string expectedModelObjectId: ""
    property string expectedSchemaTarget: ""

    // Optional map for enum side values if a schema uses sourceSide/targetSide.
    // Values must match ConnectionModel::Side: SideAuto=-1, SideTop=0, SideRight=1, SideBottom=2, SideLeft=3
    property var sideModel: [
        {
            text: "Auto",
            value: -1
        },
        {
            text: "Top",
            value: 0
        },
        {
            text: "Right",
            value: 1
        },
        {
            text: "Bottom",
            value: 2
        },
        {
            text: "Left",
            value: 3
        }
    ]

    signal propertyEditRequested(string propertyName, var value, var sourceModelObject)

    implicitHeight: sectionsColumn.implicitHeight

    function advanceRenderContext(reason) {
        renderContextVersion += 1;
    }

    function requestPropertyEdit(propertyName, value) {
        root.propertyEditRequested(propertyName, value, root.modelObject);
    }

    function isEditorContextCurrent(field) {
        if (!root.isRendererStateStable())
            return false;
        return root.isFieldCurrent(field);
    }

    function modelObjectTarget() {
        if (!root.modelObject)
            return "";
        if (root.modelObject.sourceId !== undefined)
            return "connection/flow";

        var objectType = root.modelObject.type !== undefined ? String(root.modelObject.type) : "";
        if (!objectType.length)
            // Need to same with componentId in GraphPropertySchemaProvider
            return "graph";
        return "component/" + objectType;
    }

    // Returns true if the renderer's modelObject and schema target match the expected values (if any).
    // This is used to determine if a field editor is still valid for the current modelObject and schema target.
    function isRendererStateStable() {
        if (!root.modelObject)
            return false;

        if (root.expectedModelObjectId.length > 0) {
            var objectId = root.modelObject.id !== undefined ? String(root.modelObject.id) : "";
            if (objectId !== root.expectedModelObjectId)
                return false;
        }

        if (root.expectedSchemaTarget.length > 0) {
            if (root.modelObjectTarget() !== root.expectedSchemaTarget)
                return false;
        }

        return true;
    }

    function typedSectionsToRows() {
        if (!root.schemaSectionModel || root.schemaSectionModel.size === undefined || root.schemaSectionModel.rowAt === undefined)
            return [];

        var sections = [];
        var sectionCount = root.schemaSectionModel.size();
        for (var sectionIndex = 0; sectionIndex < sectionCount; ++sectionIndex) {
            var sectionRow = root.schemaSectionModel.rowAt(sectionIndex);
            var fieldsModel = sectionRow.fieldsModel;
            var fields = [];

            if (fieldsModel && fieldsModel.size !== undefined && fieldsModel.rowAt !== undefined) {
                var fieldCount = fieldsModel.size();
                for (var fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex)
                    fields.push(fieldsModel.rowAt(fieldIndex));
            }

            sections.push({
                id: sectionRow.id,
                title: sectionRow.title,
                fields: fields
            });
        }

        return sections;
    }

    function schemaContainsField(propertyName) {
        if (!propertyName || !propertyName.length)
            return false;

        var sections = root.typedSectionsToRows();
        for (var sectionIndex = 0; sectionIndex < sections.length; ++sectionIndex) {
            var section = sections[sectionIndex];
            var fields = section && section.fields ? section.fields : [];
            for (var fieldIndex = 0; fieldIndex < fields.length; ++fieldIndex) {
                var field = fields[fieldIndex];
                if (field && field.key === propertyName)
                    return true;
            }
        }

        return false;
    }

    function isFieldCurrent(field) {
        var propertyName = field && field.key ? String(field.key) : "";
        return root.schemaContainsField(propertyName);
    }

    function widgetEnumForField(field) {
        return field && field.widgetEnum !== undefined ? Number(field.widgetEnum) : root.widgetUnknown;
    }

    // The normalized spec produced by SchemaOptionsSpec (field.dropdownOptions).
    function dropdownSpecForField(field) {
        if (field && field.dropdownOptions)
            return field.dropdownOptions;
        return {};
    }

    function optionsSourceKeyForField(field) {
        var spec = root.dropdownSpecForField(field);
        return spec.sourceKey ? String(spec.sourceKey) : "";
    }

    function readModelProperty(propertyName) {
        if (!root.modelObject || !propertyName || !propertyName.length)
            return undefined;

        // ComponentModel stores schema-defined fields (for example inputNumber,
        // addValue) as QObject dynamic properties. Read through the same
        // API used by command writes so inspector values stay consistent.
        if (root.modelObject.dynamicPropertyValue !== undefined)
            return root.modelObject.dynamicPropertyValue(propertyName);

        return root.modelObject[propertyName];
    }

    function fieldValue(field) {
        if (!root.modelObject)
            return field.defaultValue;

        var key = field.key || "";
        if (!key.length)
            return field.defaultValue;

        var value = root.readModelProperty(key);
        // return defaultValue if value == undefined, or if value is string and value.length == 0
        if (value === undefined || (typeof value === "string" && value.length === 0))
            return field.defaultValue;

        return value;
    }

    function fieldVisible(field) {
        var rule = field.visibleWhen || {};
        if (!rule || Object.keys(rule).length === 0)
            return true;

        if (!root.modelObject)
            return false;

        var propertyName = rule.property || "";
        var currentValue = propertyName.length ? root.readModelProperty(propertyName) : undefined;

        if (rule.equals !== undefined)
            return currentValue === rule.equals;
        if (rule.notEquals !== undefined)
            return currentValue !== rule.notEquals;
        if (rule.truthy === true)
            return !!currentValue;
        if (rule.in !== undefined && rule.in.length !== undefined)
            return rule.in.indexOf(currentValue) !== -1;
        return true;
    }

    function enumModelForField(field) {
        if (!root.isFieldCurrent(field))
            return [];

        var spec = root.dropdownSpecForField(field);
        var items = spec.items;
        if (items && items.length !== undefined && items.length > 0)
            return root.decorateEnumModel(field, items);

        var sourceKey = root.optionsSourceKeyForField(field);
        if (sourceKey.length > 0) {
            var dynamicModel = root.dynamicOptions ? root.dynamicOptions[sourceKey] : undefined;
            if (dynamicModel && dynamicModel.length !== undefined)
                return root.decorateEnumModel(field, dynamicModel);
        }

        if (field.options && field.options.length)
            return root.decorateEnumModel(field, field.options);

        return [];
    }

    function shouldFallbackToTextField(field) {
        var optionsSource = root.optionsSourceKeyForField(field);
        if (!optionsSource.length)
            return false;
        return root.enumModelForField(field).length === 0;
    }

    function isUnsetValue(value) {
        return value === undefined || value === null || value === "" || value === root.fallbackSelectionValue;
    }

    function fieldAllowsFallbackChoice(field) {
        var spec = root.dropdownSpecForField(field);
        return spec.allowUnset !== undefined ? !!spec.allowUnset : false;
    }

    function unsetLabelForField(field) {
        var spec = root.dropdownSpecForField(field);
        if (spec.unsetLabel && String(spec.unsetLabel).length)
            return String(spec.unsetLabel);
        return root.defaultUnsetLabel;
    }

    function fallbackOptionForField(field) {
        var propertyName = field && field.key ? String(field.key) : "";
        return {
            text: root.unsetLabelForField(field),
            value: root.fallbackSelectionValue,
            key: propertyName,
            tokenId: "",
            sourceId: ""
        };
    }

    function decorateEnumModel(field, options) {
        if (!root.fieldAllowsFallbackChoice(field))
            return options;
        if (!options || options.length === undefined)
            return options;
        if (options.length > 0) {
            var firstOption = options[0];
            if (typeof firstOption === "object" && firstOption.value === "")
                return options;
        }
        return [root.fallbackOptionForField(field)].concat(options);
    }

    function modelHasPersistedProperty(propertyName) {
        if (!root.modelObject || !propertyName || !propertyName.length)
            return false;

        if (root.modelObject.hasDynamicProperty !== undefined)
            return root.modelObject.hasDynamicProperty(propertyName);

        return root.readModelProperty(propertyName) !== undefined;
    }

    function enumIndexForValue(options, value) {
        for (var i = 0; i < options.length; ++i) {
            var option = options[i];
            if (typeof option === "object") {
                if (option.value === value)
                    return i;
            } else if (option === value) {
                return i;
            }
        }
        return -1;
    }

    function enumValueAt(options, index) {
        if (index < 0 || index >= options.length)
            return undefined;
        var option = options[index];
        if (typeof option === "object")
            return option.value;
        return option;
    }

    function enumTextRole(options) {
        if (!options || options.length === 0)
            return "";
        if (typeof options[0] === "object")
            return "text";
        return "";
    }

    function enumTextRoleForField(field) {
        if (field && field.dropdownOptions && field.dropdownOptions.textRole && field.dropdownOptions.textRole.length)
            return field.dropdownOptions.textRole;

        return "text";
    }

    function preferredAutoInitializeIndex(field, options, preferredValue) {
        if (!root.isUnsetValue(preferredValue)) {
            var preferredIndex = root.enumIndexForValue(options, preferredValue);
            if (preferredIndex >= 0)
                return preferredIndex;
        }

        if (root.fieldAllowsFallbackChoice(field)) {
            for (var i = 0; i < options.length; ++i) {
                if (!root.isUnsetValue(root.enumValueAt(options, i)))
                    return i;
            }
        }

        return options.length > 0 ? 0 : -1;
    }

    function autoInitializeDropdownField(field, options) {
        if (root.readOnly || !field)
            return undefined;

        if (!root.isRendererStateStable())
            return undefined;

        var propertyName = field.key || "";
        if (!propertyName.length)
            return undefined;

        if (root.modelHasPersistedProperty(propertyName))
            return undefined;

        if (!options || options.length === undefined || options.length === 0)
            return undefined;

        var preferredValue = root.fieldValue(field);
        var preferredIndex = root.preferredAutoInitializeIndex(field, options, preferredValue);
        if (preferredIndex < 0)
            return undefined;

        var nextValue = root.enumValueAt(options, preferredIndex);
        if (nextValue === undefined)
            return undefined;

        root.requestPropertyEdit(propertyName, nextValue);
        return nextValue;
    }

    onModelObjectChanged: advanceRenderContext("modelObjectChanged")

    onSchemaSectionModelChanged: advanceRenderContext("schemaSectionModelChanged")

    ColumnLayout {
        id: sectionsColumn
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 10

        Repeater {
            model: root.typedSectionsToRows()

            delegate: ColumnLayout {
                required property var modelData
                Layout.fillWidth: true
                spacing: 4
                property bool expanded: true

                Rectangle {
                    Layout.fillWidth: true
                    radius: 6
                    color: "#fafafa"
                    border.color: "#e4e4e4"
                    border.width: 1

                    implicitHeight: groupColumn.implicitHeight + 12

                    ColumnLayout {
                        id: groupColumn
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 6

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            ToolButton {
                                Layout.preferredWidth: 24
                                text: expanded ? "▾" : "▸"
                                onClicked: expanded = !expanded
                            }

                            Label {
                                text: modelData.title || "Section"
                                font.bold: true
                                font.pixelSize: 12
                                color: "#606060"
                                Layout.fillWidth: true
                            }

                            Label {
                                text: (modelData.fields || []).length + " options"
                                color: "#9a9a9a"
                                font.pixelSize: 10
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            visible: expanded

                            Repeater {
                                model: modelData.fields || []

                                delegate: ColumnLayout {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    spacing: 3
                                    visible: root.fieldVisible(modelData)

                                    Label {
                                        text: modelData.title || modelData.key || "Field"
                                        Layout.fillWidth: true
                                    }

                                    Loader {
                                        id: editorLoader
                                        Layout.fillWidth: true
                                        property var fieldData: modelData
                                        sourceComponent: {
                                            var widgetEnum = root.widgetEnumForField(fieldData);
                                            switch (widgetEnum) {
                                            case root.widgetTextField:
                                                return textFieldEditor;
                                            case root.widgetTextArea:
                                                return textAreaEditor;
                                            case root.widgetDropdown:
                                                return root.shouldFallbackToTextField(fieldData) ? textFieldEditor : comboBoxEditor;
                                            case root.widgetCheckbox:
                                                return checkBoxEditor;
                                            case root.widgetSpinBox:
                                                return spinBoxEditor;
                                            case root.widgetSchemaError:
                                                return schemaErrorEditor;
                                            default:
                                                return unknownWidgetEditor;
                                            }
                                        }
                                        onLoaded: {
                                            if (item && item.hasOwnProperty("fieldData"))
                                                item.fieldData = fieldData;
                                        }
                                    }

                                    Label {
                                        visible: (modelData.hint || "").length > 0
                                        text: modelData.hint || ""
                                        color: "#8a8a8a"
                                        font.pixelSize: 11
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: textFieldEditor

        TextField {
            property var fieldData: ({})
            text: {
                var value = root.fieldValue(fieldData);
                return value === undefined || value === null ? "" : String(value);
            }
            onTextChanged: {
                if (text === fieldData.defaultValue) {
                    // On first loading default value
                    root.requestPropertyEdit(fieldData.key || "", text);
                }
            }
            placeholderText: fieldData.placeholder || ""
            readOnly: root.readOnly
            onEditingFinished: {
                if (root.readOnly)
                    return;
                root.requestPropertyEdit(fieldData.key || "", text);
            }
        }
    }

    Component {
        id: textAreaEditor

        TextArea {
            property var fieldData: ({})
            Layout.fillWidth: true
            Layout.preferredHeight: 64
            text: {
                var value = root.fieldValue(fieldData);
                return value === undefined || value === null ? "" : String(value);
            }
            placeholderText: fieldData.placeholder || ""
            wrapMode: Text.WordWrap
            readOnly: root.readOnly
            onEditingFinished: {
                if (root.readOnly)
                    return;
                root.requestPropertyEdit(fieldData.key || "", text);
            }
        }
    }

    Component {
        id: comboBoxEditor

        ComboBox {
            id: combo
            property var fieldData: ({})
            property var optionsModel: root.enumModelForField(fieldData)
            property int syncRequestVersion: 0
            property bool suppressCurrentIndexCommit: false

            function applyDefaultSelectionIfNeeded() {
                if (!root.isEditorContextCurrent(fieldData))
                    return;
                root.autoInitializeDropdownField(fieldData, optionsModel);
            }

            function syncCurrentSelection() {
                if (!root.isEditorContextCurrent(fieldData)) {
                    combo.currentIndex = -1;
                    return;
                }
                var fieldValue = root.fieldValue(fieldData);
                var nextIndex = root.enumIndexForValue(optionsModel, fieldValue);
                suppressCurrentIndexCommit = true;
                combo.currentIndex = nextIndex;
                suppressCurrentIndexCommit = false;
            }

            function scheduleSelectionSync() {
                syncRequestVersion += 1;
                var requestVersion = syncRequestVersion;
                Qt.callLater(function () {
                    if (requestVersion !== syncRequestVersion)
                        return;
                    combo.syncCurrentSelection();
                });
            }

            model: optionsModel
            textRole: root.enumTextRoleForField(fieldData)
            currentIndex: -1
            enabled: !root.readOnly

            Component.onCompleted: {
                applyDefaultSelectionIfNeeded();
                scheduleSelectionSync();
            }
            onFieldDataChanged: scheduleSelectionSync()
            onOptionsModelChanged: {
                applyDefaultSelectionIfNeeded();
                scheduleSelectionSync();
            }

            onCurrentIndexChanged: {
                if (suppressCurrentIndexCommit)
                    return;
                if (root.readOnly)
                    return;
                if (!root.isEditorContextCurrent(fieldData))
                    return;
                if (currentIndex < 0)
                    return;
                var propertyName = fieldData && fieldData.key ? String(fieldData.key) : "";
                if (!propertyName.length)
                    return;
                var next = root.enumValueAt(optionsModel, currentIndex);
                var rawModelValue = root.readModelProperty(propertyName);
                if (rawModelValue === next)
                    return;
                root.requestPropertyEdit(propertyName, next);
            }

            onActivated: function (index) {
                if (root.readOnly)
                    return;
                if (!root.isEditorContextCurrent(fieldData))
                    return;
            }
        }
    }

    Component {
        id: checkBoxEditor

        CheckBox {
            property var fieldData: ({})
            checked: !!root.fieldValue(fieldData)
            enabled: !root.readOnly
            text: fieldData.checkboxText || ""
            onToggled: {
                if (root.readOnly)
                    return;
                root.requestPropertyEdit(fieldData.key || "", checked);
            }
        }
    }

    Component {
        id: spinBoxEditor

        SpinBox {
            property var fieldData: ({})
            from: (fieldData.validation || {}).min !== undefined ? Number(fieldData.validation.min) : -999999
            to: (fieldData.validation || {}).max !== undefined ? Number(fieldData.validation.max) : 999999
            value: {
                var value = root.fieldValue(fieldData);
                return value === undefined || value === null ? 0 : Number(value);
            }
            enabled: !root.readOnly
            onValueModified: {
                if (root.readOnly)
                    return;
                root.requestPropertyEdit(fieldData.key || "", value);
            }
        }
    }

    Component {
        id: unknownWidgetEditor

        ColumnLayout {
            property var fieldData: ({})
            spacing: 4

            TextField {
                Layout.fillWidth: true
                text: {
                    var value = root.fieldValue(fieldData);
                    return value === undefined || value === null ? "" : String(value);
                }
                placeholderText: fieldData.placeholder || ""
                readOnly: root.readOnly
                onEditingFinished: {
                    if (root.readOnly)
                        return;
                    root.requestPropertyEdit(fieldData.key || "", text);
                }
            }

            Label {
                Layout.fillWidth: true
                text: "Unknown widget '" + (fieldData.widget || "") + "'. Fallback text editor is used."
                color: "#b26a00"
                wrapMode: Text.WordWrap
                font.pixelSize: 11
            }
        }
    }

    Component {
        id: schemaErrorEditor

        Label {
            property var fieldData: ({})
            text: (fieldData.schemaError || "Schema error")
            color: "#c62828"
            wrapMode: Text.WordWrap
            font.pixelSize: 11
        }
    }
}
