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

    function test_optionsSource_resolvesDynamicTokenList() {
        var renderer = makeRenderer();
        renderer.dynamicOptions = {
            "tokenKeys": ["tok.a.b", "tok.b.c"]
        };

        var field = {
            "key": "inputAKey",
            "widget": "dropdown",
            "optionsSource": "tokenKeys"
        };

        var options = renderer.enumModelForField(field);
        compare(options.length, 2);
        compare(options[0], "tok.a.b");
        compare(options[1], "tok.b.c");
    }

    function test_optionsSource_fallsBackToTextFieldWhenEmpty() {
        var renderer = makeRenderer();
        renderer.dynamicOptions = {
            "tokenKeys": []
        };

        var field = {
            "key": "outputKey",
            "widget": "dropdown",
            "optionsSource": "tokenKeys"
        };

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
}
