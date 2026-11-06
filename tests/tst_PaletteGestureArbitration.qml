import QtQuick
import QtTest
import ComponentMapEditor

TestCase {
    id: testCase
    name: "PaletteGestureArbitration"
    when: windowShown

    function makePaletteWithGraph() {
        var graph = Qt.createQmlObject('import ComponentMapEditor; GraphModel {}', testCase);
        var palette = Qt.createQmlObject('import QtQuick; import ComponentMapEditor; Palette {}', testCase);
        palette.graph = graph;
        return {
            "graph": graph,
            "palette": palette
        };
    }

    function makeComponent(parentObj, idValue, titleValue, xValue, yValue) {
        var component = Qt.createQmlObject('import ComponentMapEditor; ComponentModel {}', parentObj);
        component.id = idValue;
        component.title = titleValue;
        component.x = xValue;
        component.y = yValue;
        component.width = 96;
        component.height = 96;
        component.color = "#4fc3f7";
        component.type = "default";
        return component;
    }

    function makeCanvasMock(parentObj, graph, opts) {
        var createOnDrop = opts && opts.createOnDrop !== undefined ? opts.createOnDrop : true;
        var acceptDrop = opts && opts.acceptDrop !== undefined ? opts.acceptDrop : true;
        var centerX = opts && opts.centerX !== undefined ? opts.centerX : 10;
        var centerY = opts && opts.centerY !== undefined ? opts.centerY : 20;

        var canvas = Qt.createQmlObject('import QtQuick; QtObject {'
            + 'property bool paletteDragInProgress: false;'
            + 'property int beginCount: 0;'
            + 'property int endCount: 0;'
            + 'property int dropCount: 0;'
            + 'property int fallbackCreateCount: 0;'
            + 'property int lastDropX: -1;'
            + 'property int lastDropY: -1;'
            + 'function beginPaletteDrag() { beginCount += 1; paletteDragInProgress = true; }'
            + 'function endPaletteDrag() { endCount += 1; paletteDragInProgress = false; }'
            + 'function addPaletteComponentAtScenePos(title, icon, color, type, scenePos) {'
            + '  dropCount += 1; lastDropX = Math.round(scenePos.x); lastDropY = Math.round(scenePos.y);'
            + '  if (!' + acceptDrop + ') return false;'
            + '  if (' + createOnDrop + ') {'
            + '    var c = Qt.createQmlObject("import ComponentMapEditor; ComponentModel {}", graphRef);'
            + '    c.id = "drop_" + dropCount; c.title = title; c.x = scenePos.x; c.y = scenePos.y;'
            + '    graphRef.addComponent(c);'
            + '  }'
            + '  return true;'
            + '}'
            + 'function viewToWorld(x, y) { return Qt.point(' + centerX + ', ' + centerY + '); }'
            + 'property var componentRenderer: QtObject { property bool renderComponents: false }'
            + 'property var graphRef: null;'
            + '}', parentObj);
        canvas.graphRef = graph;
        return canvas;
    }

    function descriptor() {
        return {
            "title": "Task",
            "icon": "cube",
            "defaultColor": "#4fc3f7",
            "id": "default",
            "type": "default"
        };
    }

    function cleanup() {
    }

    function test_tap_creates_exactly_one_component() {
        var ctx = makePaletteWithGraph();
        var palette = ctx.palette;
        var graph = ctx.graph;

        palette._dispatchGestureAction(descriptor(), palette.gestureActionTapCreate, Qt.point(0, 0), Qt.point(0, 0));

        compare(graph.componentCount, 1);
        var created = graph.components[0];
        compare(created.title, "Task");
    }

    function test_drag_release_creates_exactly_one_at_drop_location() {
        var ctx = makePaletteWithGraph();
        var palette = ctx.palette;
        var graph = ctx.graph;
        var canvas = makeCanvasMock(palette, graph, {
            "createOnDrop": true,
            "acceptDrop": true
        });
        palette.canvas = canvas;

        palette._dispatchGestureAction(descriptor(), palette.gestureActionDragDropCreate, Qt.point(123, 77), Qt.point(0, 0));

        compare(canvas.dropCount, 1);
        compare(graph.componentCount, 1);
        compare(Math.round(graph.components[0].x), 123);
        compare(Math.round(graph.components[0].y), 77);
    }

    function test_drag_cancel_creates_none() {
        var ctx = makePaletteWithGraph();
        var palette = ctx.palette;
        var graph = ctx.graph;
        var canvas = makeCanvasMock(palette, graph, {
            "createOnDrop": true,
            "acceptDrop": true
        });
        palette.canvas = canvas;

        // Cancel path resolves to no action dispatch.
        palette._dispatchGestureAction(descriptor(), palette.gestureActionNone, Qt.point(50, 50), Qt.point(0, 0));

        compare(canvas.dropCount, 0);
        compare(graph.componentCount, 0);
    }

    function test_drop_outside_canvas_fallback_happens_once() {
        var ctx = makePaletteWithGraph();
        var palette = ctx.palette;
        var graph = ctx.graph;
        var canvas = makeCanvasMock(palette, graph, {
            "createOnDrop": false,
            "acceptDrop": false,
            "centerX": 44,
            "centerY": 55
        });
        palette.canvas = canvas;

        palette._dispatchGestureAction(descriptor(), palette.gestureActionDragDropCreate, Qt.point(9999, 9999), Qt.point(0, 0));

        compare(canvas.dropCount, 1);
        compare(graph.componentCount, 1);
        compare(Math.round(graph.components[0].x), 44);
        compare(Math.round(graph.components[0].y), 55);
    }

    function test_palette_drag_lifecycle_updates_single_channel() {
        var ctx = makePaletteWithGraph();
        var palette = ctx.palette;
        var graph = ctx.graph;
        var canvas = makeCanvasMock(palette, graph, {
            "createOnDrop": true,
            "acceptDrop": true
        });
        palette.canvas = canvas;

        palette._setPaletteDragLifecycle(true);
        compare(canvas.beginCount, 1);
        verify(canvas.paletteDragInProgress);

        palette._setPaletteDragLifecycle(false);
        compare(canvas.endCount, 1);
        verify(!canvas.paletteDragInProgress);
    }

    function test_drop_resolver_prefers_primary_then_fallback() {
        var ctx = makePaletteWithGraph();
        var palette = ctx.palette;

        var primary = palette._resolveDropScenePos(Qt.point(7, 9), Qt.point(100, 100));
        compare(Math.round(primary.x), 7);
        compare(Math.round(primary.y), 9);

        var fallback = palette._resolveDropScenePos(Qt.point(0, 0), Qt.point(11, 13));
        compare(Math.round(fallback.x), 11);
        compare(Math.round(fallback.y), 13);
    }
}
