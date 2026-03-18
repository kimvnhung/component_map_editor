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
    property var sideModel: [
        { text: "Auto", value: 0 },
        { text: "Top", value: 1 },
        { text: "Right", value: 2 },
        { text: "Bottom", value: 3 },
        { text: "Left", value: 4 }
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
                            sourceComponent: {
                                var widget = modelData.widget || ""
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
            text: {
                var value = root.fieldValue(modelData)
                return value === undefined || value === null ? "" : String(value)
            }
            placeholderText: modelData.placeholder || ""
            readOnly: root.readOnly
            onEditingFinished: {
                if (root.readOnly)
                    return
                root.propertyEditRequested(modelData.key || "", text)
            }
        }
    }

    Component {
        id: textAreaEditor

        TextArea {
            Layout.fillWidth: true
            Layout.preferredHeight: 64
            text: {
                var value = root.fieldValue(modelData)
                return value === undefined || value === null ? "" : String(value)
            }
            placeholderText: modelData.placeholder || ""
            wrapMode: Text.WordWrap
            readOnly: root.readOnly
            onEditingFinished: {
                if (root.readOnly)
                    return
                root.propertyEditRequested(modelData.key || "", text)
            }
        }
    }

    Component {
        id: comboBoxEditor

        ComboBox {
            id: combo
            property var optionsModel: root.enumModelForField(modelData)
            model: optionsModel
            textRole: root.enumTextRole(optionsModel)
            currentIndex: root.enumIndexForValue(optionsModel, root.fieldValue(modelData))
            enabled: !root.readOnly

            onActivated: function(index) {
                if (root.readOnly)
                    return
                var next = root.enumValueAt(optionsModel, index)
                root.propertyEditRequested(modelData.key || "", next)
            }
        }
    }

    Component {
        id: checkBoxEditor

        CheckBox {
            checked: !!root.fieldValue(modelData)
            enabled: !root.readOnly
            text: modelData.checkboxText || ""
            onToggled: {
                if (root.readOnly)
                    return
                root.propertyEditRequested(modelData.key || "", checked)
            }
        }
    }

    Component {
        id: spinBoxEditor

        SpinBox {
            from: (modelData.validation || {}).min !== undefined ? Number(modelData.validation.min) : -999999
            to: (modelData.validation || {}).max !== undefined ? Number(modelData.validation.max) : 999999
            value: {
                var value = root.fieldValue(modelData)
                return value === undefined || value === null ? 0 : Number(value)
            }
            enabled: !root.readOnly
            onValueModified: {
                if (root.readOnly)
                    return
                root.propertyEditRequested(modelData.key || "", value)
            }
        }
    }

    Component {
        id: unknownWidgetEditor

        ColumnLayout {
            spacing: 4

            TextField {
                Layout.fillWidth: true
                text: {
                    var value = root.fieldValue(modelData)
                    return value === undefined || value === null ? "" : String(value)
                }
                placeholderText: modelData.placeholder || ""
                readOnly: root.readOnly
                onEditingFinished: {
                    if (root.readOnly)
                        return
                    root.propertyEditRequested(modelData.key || "", text)
                }
            }

            Label {
                Layout.fillWidth: true
                text: "Unknown widget '" + (modelData.widget || "") + "'. Fallback text editor is used."
                color: "#b26a00"
                wrapMode: Text.WordWrap
                font.pixelSize: 11
            }
        }
    }

    Component {
        id: schemaErrorEditor

        Label {
            text: (modelData.schemaError || "Schema error")
            color: "#c62828"
            wrapMode: Text.WordWrap
            font.pixelSize: 11
        }
    }
}
