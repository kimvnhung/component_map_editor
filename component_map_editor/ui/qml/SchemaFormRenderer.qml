import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    // [ { id, title, fields:[{...}] } ]
    property var schemaSections: []
    property var modelObject: null
    property bool readOnly: false

    // Optional map for enum side values if a schema uses sourceSide/targetSide.
    // Values must match ConnectionModel::Side: SideAuto=-1, SideTop=0, SideRight=1, SideBottom=2, SideLeft=3
    property var sideModel: [
        { text: "Auto", value: -1 },
        { text: "Top", value: 0 },
        { text: "Right", value: 1 },
        { text: "Bottom", value: 2 },
        { text: "Left", value: 3 }
    ]

    signal propertyEditRequested(string propertyName, var value)

    implicitHeight: sectionsColumn.implicitHeight

    function fieldValue(field) {
        if (!root.modelObject)
            return field.defaultValue

        var key = field.key || ""
        if (!key.length)
            return field.defaultValue

        var value = root.modelObject[key]
        return value === undefined ? field.defaultValue : value
    }

    function fieldVisible(field) {
        var rule = field.visibleWhen || {}
        if (!rule || Object.keys(rule).length === 0)
            return true

        if (!root.modelObject)
            return false

        var propertyName = rule.property || ""
        var currentValue = propertyName.length ? root.modelObject[propertyName] : undefined

        if (rule.equals !== undefined)
            return currentValue === rule.equals
        if (rule.notEquals !== undefined)
            return currentValue !== rule.notEquals
        if (rule.truthy === true)
            return !!currentValue
        if (rule.in !== undefined && rule.in.length !== undefined)
            return rule.in.indexOf(currentValue) !== -1
        return true
    }

    function enumModelForField(field) {
        if (field.options && field.options.length)
            return field.options
        if (field.key === "shape")
            return ["rounded", "rectangle"]
        if (field.key === "sourceSide" || field.key === "targetSide")
            return root.sideModel
        return []
    }

    function enumIndexForValue(options, value) {
        for (var i = 0; i < options.length; ++i) {
            var option = options[i]
            if (typeof option === "object") {
                if (option.value === value)
                    return i
            } else if (option === value) {
                return i
            }
        }
        return 0
    }

    function enumValueAt(options, index) {
        if (index < 0 || index >= options.length)
            return undefined
        var option = options[index]
        if (typeof option === "object")
            return option.value
        return option
    }

    function enumTextRole(options) {
        if (!options || options.length === 0)
            return ""
        if (typeof options[0] === "object")
            return "text"
        return ""
    }

    ColumnLayout {
        id: sectionsColumn
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 10

        Repeater {
            model: root.schemaSections || []

            delegate: ColumnLayout {
                required property var modelData
                Layout.fillWidth: true
                spacing: 6

                Label {
                    text: modelData.title || "Section"
                    font.bold: true
                    font.pixelSize: 11
                    color: "#888"
                    Layout.fillWidth: true
                }

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
                                var widget = fieldData.widget || ""
                                switch (widget) {
                                case "textfield": return textFieldEditor
                                case "textarea": return textAreaEditor
                                case "dropdown": return comboBoxEditor
                                case "checkbox": return checkBoxEditor
                                case "spinbox": return spinBoxEditor
                                case "schema_error": return schemaErrorEditor
                                default: return unknownWidgetEditor
                                }
                            }
                            onLoaded: {
                                if (item && item.hasOwnProperty("fieldData"))
                                    item.fieldData = fieldData
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

    Component {
        id: textFieldEditor

        TextField {
            property var fieldData: ({})
            text: {
                var value = root.fieldValue(fieldData)
                return value === undefined || value === null ? "" : String(value)
            }
            placeholderText: fieldData.placeholder || ""
            readOnly: root.readOnly
            onEditingFinished: {
                if (root.readOnly)
                    return
                root.propertyEditRequested(fieldData.key || "", text)
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
                var value = root.fieldValue(fieldData)
                return value === undefined || value === null ? "" : String(value)
            }
            placeholderText: fieldData.placeholder || ""
            wrapMode: Text.WordWrap
            readOnly: root.readOnly
            onEditingFinished: {
                if (root.readOnly)
                    return
                root.propertyEditRequested(fieldData.key || "", text)
            }
        }
    }

    Component {
        id: comboBoxEditor

        ComboBox {
            id: combo
            property var fieldData: ({})
            property var optionsModel: root.enumModelForField(fieldData)
            model: optionsModel
            textRole: root.enumTextRole(optionsModel)
            currentIndex: root.enumIndexForValue(optionsModel, root.fieldValue(fieldData))
            enabled: !root.readOnly

            onActivated: function(index) {
                if (root.readOnly)
                    return
                var next = root.enumValueAt(optionsModel, index)
                root.propertyEditRequested(fieldData.key || "", next)
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
                    return
                root.propertyEditRequested(fieldData.key || "", checked)
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
                var value = root.fieldValue(fieldData)
                return value === undefined || value === null ? 0 : Number(value)
            }
            enabled: !root.readOnly
            onValueModified: {
                if (root.readOnly)
                    return
                root.propertyEditRequested(fieldData.key || "", value)
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
                    var value = root.fieldValue(fieldData)
                    return value === undefined || value === null ? "" : String(value)
                }
                placeholderText: fieldData.placeholder || ""
                readOnly: root.readOnly
                onEditingFinished: {
                    if (root.readOnly)
                        return
                    root.propertyEditRequested(fieldData.key || "", text)
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
