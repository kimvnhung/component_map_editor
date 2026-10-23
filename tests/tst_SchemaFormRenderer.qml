import QtQuick
import QtTest
import ComponentMapEditor

TestCase {
    id: testCase
    name: "SchemaFormRenderer"
    when: windowShown

    function makeRenderer() {
        return Qt.createQmlObject(
            'import QtQuick; import ComponentMapEditor; SchemaFormRenderer {}',
            testCase);
    }

    function makeFieldModel(rows) {
        return {
            size: function() { return rows.length; },
            rowAt: function(index) { return rows[index]; }
        };
    }

    function makeSectionModel(sectionRows) {
        return {
            size: function() { return sectionRows.length; },
            rowAt: function(index) { return sectionRows[index]; }
        };
    }

    function setCurrentField(renderer, field) {
        renderer.schemaSectionModel = makeSectionModel([
            {
                "id": "test",
                "title": "Test",
                "fieldsModel": makeFieldModel([field])
            }
        ]);
    }

    function test_optionsSource_resolvesDynamicTokenList() {
        var renderer = makeRenderer();
        renderer.dynamicOptions = {
            "tokenRef": ["tok.a.b", "tok.b.c"]
        };

        var field = {
            "key": "inputAKey",
            "widget": "dropdown",
            "optionsSource": "tokenRef",
            "dropdownOptions": { "sourceKey": "tokenRef" }
        };
        setCurrentField(renderer, field);

        var options = renderer.enumModelForField(field);
        compare(options.length, 2);
        compare(options[0], "tok.a.b");
        compare(options[1], "tok.b.c");
    }

    function test_optionsSource_fallsBackToTextFieldWhenEmpty() {
        var renderer = makeRenderer();
        renderer.dynamicOptions = {
            "tokenRef": []
        };

        var field = {
            "key": "outputKey",
            "widgetEnum": renderer.widgetDropdown,
            "dropdownOptions": { "sourceKey": "tokenRef" }
        };
        setCurrentField(renderer, field);

        compare(renderer.shouldFallbackToTextField(field), true);
    }

    function test_dropdownWithoutOptionsSource_doesNotFallback() {
        var renderer = makeRenderer();

        var field = {
            "key": "shape",
            "widget": "dropdown"
        };

        compare(renderer.shouldFallbackToTextField(field), false);
    }

    function test_tokenRefDropdownUsesNormalizedSourceKey() {
        var renderer = makeRenderer();
        renderer.dynamicOptions = {
            "tokenRef": [
                { text: "edge-a::value", value: "edge-a::value" },
                { text: "edge-b::value", value: "edge-b::value" }
            ]
        };

        var field = {
            "key": "inputARef",
            "widgetEnum": renderer.widgetDropdown,
            "dropdownOptions": { "sourceKey": "tokenRef", "allowUnset": true }
        };
        setCurrentField(renderer, field);

        var options = renderer.enumModelForField(field);
        compare(options.length, 3);
        compare(options[0].value, renderer.fallbackSelectionValue);
        compare(options[0].text, "Use fallback value");
        compare(options[1].value, "edge-a::value");
        compare(renderer.shouldFallbackToTextField(field), false);
    }


    function test_autoInitializeDropdownField_persistsFirstOptionWhenValueMissing() {
        var renderer = makeRenderer();
        var edits = [];
        renderer.propertyEditRequested.connect(function(propertyName, value, sourceModelObject) {
            edits.push({ propertyName: propertyName, value: value, sourceModelObject: sourceModelObject });
        });

        var modelObject = {
            hasDynamicProperty: function() { return false; },
            dynamicPropertyValue: function() { return ""; }
        };
        renderer.modelObject = modelObject;

        var field = {
            "key": "inputARef",
            "widgetEnum": renderer.widgetDropdown,
            "dropdownOptions": { "sourceKey": "tokenRef", "allowUnset": true }
        };
        setCurrentField(renderer, field);

        var applied = renderer.autoInitializeDropdownField(field, [
            { text: "Use fallback value", value: renderer.fallbackSelectionValue },
            { text: "edge-a::value", value: "edge-a::value" },
            { text: "edge-b::value", value: "edge-b::value" }
        ]);

        compare(applied, "edge-a::value");
        compare(edits.length, 1);
        compare(edits[0].propertyName, "inputARef");
        compare(edits[0].value, "edge-a::value");
        compare(edits[0].sourceModelObject, modelObject);
    }

    function test_autoInitializeDropdownField_keepsPersistedValueUntouched() {
        var renderer = makeRenderer();
        var edits = [];
        renderer.propertyEditRequested.connect(function(propertyName, value, sourceModelObject) {
            edits.push({ propertyName: propertyName, value: value, sourceModelObject: sourceModelObject });
        });

        renderer.modelObject = {
            dynamicPropertyValue: function() { return "edge-b::value"; }
        };

        var field = {
            "key": "inputARef",
            "widgetEnum": renderer.widgetDropdown,
        };

        renderer.modelObject = {
            hasDynamicProperty: function() { return true; },
            dynamicPropertyValue: function() { return renderer.fallbackSelectionValue; }
        };

        var applied = renderer.autoInitializeDropdownField(field, [
            { text: "Use fallback value", value: renderer.fallbackSelectionValue },
            { text: "edge-a::value", value: "edge-a::value" },
            { text: "edge-b::value", value: "edge-b::value" }
        ]);

        compare(applied, undefined);
        compare(edits.length, 0);
    }

    function test_autoInitializeDropdownField_skipsFallbackOptionForNewRefField() {
        var renderer = makeRenderer();
        var edits = [];
        renderer.propertyEditRequested.connect(function(propertyName, value, sourceModelObject) {
            edits.push({ propertyName: propertyName, value: value, sourceModelObject: sourceModelObject });
        });

        var modelObject = {
            hasDynamicProperty: function() { return false; },
            dynamicPropertyValue: function() { return undefined; }
        };
        renderer.modelObject = modelObject;

        var field = {
            "key": "inputBRef",
            "widgetEnum": renderer.widgetDropdown,
            "dropdownOptions": { "sourceKey": "tokenRef", "allowUnset": true }
        };
        setCurrentField(renderer, field);

        var applied = renderer.autoInitializeDropdownField(field, [
            { text: "Use fallback value", value: renderer.fallbackSelectionValue },
            { text: "edge-a::value", value: "edge-a::value" },
            { text: "edge-b::value", value: "edge-b::value" }
        ]);

        compare(applied, "edge-a::value");
        compare(edits.length, 1);
        compare(edits[0].propertyName, "inputBRef");
        compare(edits[0].value, "edge-a::value");
        compare(edits[0].sourceModelObject, modelObject);
    }

    function test_enumIndexForValue_matchesFallbackSentinel() {
        var renderer = makeRenderer();
        var field = {
            "key": "inputARef",
            "widgetEnum": renderer.widgetDropdown,
            "dropdownOptions": { "sourceKey": "tokenRef", "allowUnset": true }
        };
        setCurrentField(renderer, field);
        renderer.dynamicOptions = {
            "tokenRef": [
                { text: "edge-a::value", value: "edge-a::value" }
            ]
        };

        var options = renderer.enumModelForField(field);
        compare(options.length, 2);
        compare(renderer.enumIndexForValue(field, options, renderer.fallbackSelectionValue), 0);
    }

    function test_isRendererStateStable_requires_matchingExpectedObject() {
        var renderer = makeRenderer();

        renderer.expectedModelObjectId = "component-b";
        renderer.modelObject = {
            id: "component-a",
            type: "start"
        };
        compare(renderer.isRendererStateStable(), false);

        renderer.modelObject = {
            id: "component-b",
            type: "start"
        };
        compare(renderer.isRendererStateStable(), true);
    }

    function test_customValueRole_isUsedForSelectionAndCommit() {
        var renderer = makeRenderer();
        renderer.modelObject = {
            dynamicPropertyValue: function() { return "b"; }
        };
        var field = {
            "key": "choice",
            "widgetEnum": renderer.widgetDropdown,
            "dropdownOptions": {
                "items": [
                    { label: "Alpha", id: "a" },
                    { label: "Beta", id: "b" }
                ],
                "textRole": "label",
                "valueRole": "id"
            }
        };
        setCurrentField(renderer, field);

        var options = renderer.enumModelForField(field);
        compare(renderer.enumIndexForValue(field, options, "b"), 1);
        compare(renderer.enumValueAt(field, options, 0), "a");
        compare(renderer.enumTextRoleForField(field), "label");
    }

    function test_fallbackToTextWhenEmpty_canDisableTextFallback() {
        var renderer = makeRenderer();
        renderer.dynamicOptions = { "tokenRef": [] };
        var field = {
            "key": "inputRef",
            "widgetEnum": renderer.widgetDropdown,
            "dropdownOptions": {
                "sourceKey": "tokenRef",
                "fallbackToTextWhenEmpty": false
            }
        };
        setCurrentField(renderer, field);

        compare(renderer.shouldFallbackToTextField(field), false);
    }

    function test_fallbackOption_usesConfiguredRoles() {
        var renderer = makeRenderer();
        var field = {
            "key": "choice",
            "widgetEnum": renderer.widgetDropdown,
            "dropdownOptions": {
                "allowUnset": true,
                "textRole": "label",
                "valueRole": "id",
                "unsetLabel": "Default"
            }
        };
        setCurrentField(renderer, field);

        var options = renderer.decorateEnumModel(field, [
            { label: "Alpha", id: "a" }
        ]);
        compare(options[0].label, "Default");
        compare(options[0].id, renderer.fallbackSelectionValue);
        compare(renderer.enumOptionValue(field, options[0]), renderer.fallbackSelectionValue);
    }

    function test_fieldHelpers_readNormalizedField() {
        var renderer = makeRenderer();
        renderer.modelObject = {
            dynamicPropertyValue: function(propertyName) {
                return propertyName === "label" ? "Internal value" : undefined;
            }
        };
        var field = { "key": "label" };

        compare(renderer.fieldKey(field), "label");
        compare(renderer.fieldText(field), "Internal value");
        compare(renderer.fieldKey({}), "");
    }

    function test_fieldValue_usesDefaultWhenModelValueEmpty() {
        var renderer = makeRenderer();
        renderer.modelObject = {
            dynamicPropertyValue: function() { return ""; }
        };
        compare(renderer.fieldText({ "key": "name", "defaultValue": "Default" }), "Default");
    }

    function test_fieldVisible_usesVisibilityRule() {
        var renderer = makeRenderer();
        renderer.modelObject = {
            dynamicPropertyValue: function(propertyName) {
                return propertyName === "enabled";
            }
        };
        compare(renderer.fieldVisible({ "visibleWhen": { "property": "enabled", "truthy": true } }), true);
        compare(renderer.fieldVisible({ "visibleWhen": { "property": "enabled", "equals": false } }), false);
    }

    function test_emptyFallbackOption_isNotDuplicated() {
        var renderer = makeRenderer();
        var field = {
            "key": "choice",
            "widgetEnum": renderer.widgetDropdown,
            "dropdownOptions": { "allowUnset": true }
        };
        setCurrentField(renderer, field);
        var options = renderer.decorateEnumModel(field, [
            { text: "", value: "" },
            { text: "Alpha", value: "a" }
        ]);
        compare(options.length, 2);
    }

    function test_typedWidgetEnum_dispatchesWithoutWidgetString() {
        var renderer = makeRenderer();

        compare(renderer.widgetEnumForField({ "widgetEnum": renderer.widgetTextField }), renderer.widgetTextField);
        compare(renderer.widgetEnumForField({ "widgetEnum": renderer.widgetTextArea }), renderer.widgetTextArea);
        compare(renderer.widgetEnumForField({ "widgetEnum": renderer.widgetDropdown }), renderer.widgetDropdown);
        compare(renderer.widgetEnumForField({ "widgetEnum": renderer.widgetCheckbox }), renderer.widgetCheckbox);
        compare(renderer.widgetEnumForField({ "widgetEnum": renderer.widgetSpinBox }), renderer.widgetSpinBox);
    }

    function test_typedSectionModel_convertsToRowsForRenderer() {
        var renderer = makeRenderer();
        var fieldsModel = makeFieldModel([
            {
                "key": "amount",
                "title": "Amount",
                "widgetEnum": renderer.widgetSpinBox,
                "widget": "spinbox",
                "defaultValue": 1
            }
        ]);
        renderer.schemaSectionModel = makeSectionModel([
            {
                "id": "behavior",
                "title": "Behavior",
                "fieldsModel": fieldsModel
            }
        ]);

        var sections = renderer.typedSectionsToRows();
        compare(sections.length, 1);
        compare(sections[0].title, "Behavior");
        compare(sections[0].fields.length, 1);
        compare(sections[0].fields[0].key, "amount");
        compare(renderer.widgetEnumForField(sections[0].fields[0]), renderer.widgetSpinBox);
    }
}
