// tst_InteractionStateManager.qml
// State-transition tests for InteractionStateManager.
// Covers click, Ctrl-click, drag (move/resize/connection), and cancel flows.
import QtQuick
import QtTest
import ComponentMapEditor

TestCase {
    id: testCase
    name: "InteractionStateManager"
    when: windowShown

    InteractionStateManager {
        id: sm
    }

    // ── Helpers ────────────────────────────────────────────────────────────

    function makeComponent(idStr) {
        var obj = Qt.createQmlObject(
            'import ComponentMapEditor; ComponentModel {}', testCase)
        obj.id = idStr
        return obj
    }

    function makeConnection(idStr, srcId, tgtId) {
        var obj = Qt.createQmlObject(
            'import ComponentMapEditor; ConnectionModel {}', testCase)
        obj.id = idStr
        obj.sourceId = srcId
        obj.targetId = tgtId
        return obj
    }

    function cleanup() {
        sm.resetInteraction()
        sm.clearAll()
    }

    // ── Initial state ──────────────────────────────────────────────────────

    function test_initialState() {
        compare(sm.primaryComponent, null)
        compare(sm.componentIds.length, 0)
        compare(sm.connection, null)
        compare(sm.connectionIds.length, 0)
        compare(sm.mode, InteractionStateManager.Idle)
        compare(sm.interactionTarget, null)
        compare(sm.componentInteractionActive, false)
        compare(sm.backgroundDragEnabled, true)
        compare(sm.connectionDragging, false)
        compare(sm.marqueeSelecting, false)
        compare(sm.suppressNextTap, false)
        compare(sm.tempStart.x, 0)
        compare(sm.tempStart.y, 0)
        compare(sm.tempEnd.x, 0)
        compare(sm.tempEnd.y, 0)
    }

    // ── selectSingle (normal click) ────────────────────────────────────────

    function test_selectSingle_setsSelection() {
        var c1 = makeComponent("c1")
        sm.selectSingle(c1)
        compare(sm.primaryComponent, c1)
        compare(sm.componentIds.length, 1)
        compare(sm.componentIds[0], "c1")
        compare(sm.connection, null)
    }

    function test_selectSingle_null_clearsSelection() {
        var c1 = makeComponent("c1")
        sm.selectSingle(c1)
        sm.selectSingle(null)
        compare(sm.primaryComponent, null)
        compare(sm.componentIds.length, 0)
    }

    function test_selectSingle_replacesMultiSelect() {
        var c1 = makeComponent("c1")
        var c2 = makeComponent("c2")
        sm.selectSingle(c1)
        sm.toggleComponent(c2, null)     // now c1+c2 selected
        compare(sm.componentIds.length, 2)
        sm.selectSingle(c1)              // normal click: replace selection
        compare(sm.componentIds.length, 1)
        compare(sm.primaryComponent, c1)
        verify(!sm.isSelected(c2))
    }

    function test_selectSingle_clearsConnection() {
        var conn = makeConnection("e1", "a", "b")
        sm.selectConnectionModel(conn)
        var c1 = makeComponent("c1")
        sm.selectSingle(c1)
        compare(sm.connection, null)
    }

    // ── toggleComponent (Ctrl-click) ───────────────────────────────────────

    function test_ctrlClick_addsToSelection() {
        var c1 = makeComponent("c1")
        var c2 = makeComponent("c2")
        sm.selectSingle(c1)
        sm.toggleComponent(c2, null)
        compare(sm.componentIds.length, 2)
        compare(sm.primaryComponent, c2)
        verify(sm.isSelected(c1))
        verify(sm.isSelected(c2))
    }

    function test_ctrlClick_deselectsFromMulti() {
        var c1 = makeComponent("c1")
        var c2 = makeComponent("c2")
        sm.selectSingle(c1)
        sm.toggleComponent(c2, null)
        // Ctrl-click c1 to deselect it; c2 should remain
        sm.toggleComponent(c1, null)
        compare(sm.componentIds.length, 1)
        compare(sm.componentIds[0], "c2")
        verify(!sm.isSelected(c1))
        verify(sm.isSelected(c2))
    }

    function test_ctrlClick_deselectsLastComponent() {
        var c1 = makeComponent("c1")
        sm.selectSingle(c1)
        sm.toggleComponent(c1, null)   // deselect the only selected component
        compare(sm.componentIds.length, 0)
        compare(sm.primaryComponent, null)
    }

    function test_ctrlClick_clearsConnection() {
        var conn = makeConnection("e1", "a", "b")
        sm.selectConnectionModel(conn)
        var c1 = makeComponent("c1")
        sm.toggleComponent(c1, null)
        compare(sm.connection, null)
        verify(sm.isSelected(c1))
    }

    // ── handleComponentClick dispatcher ────────────────────────────────────

    function test_handleClick_noCtrl_replacesSelection() {
        var c1 = makeComponent("c1")
        var c2 = makeComponent("c2")
        sm.selectSingle(c1)
        sm.handleComponentClick(c2, 0, null, 0)
        compare(sm.componentIds.length, 1)
        compare(sm.primaryComponent, c2)
        verify(!sm.isSelected(c1))
    }

    function test_handleClick_ctrl_addsToSelection() {
        var c1 = makeComponent("c1")
        var c2 = makeComponent("c2")
        sm.selectSingle(c1)
        sm.handleComponentClick(c2, Qt.ControlModifier, null, 0)
        compare(sm.componentIds.length, 2)
        verify(sm.isSelected(c1))
        verify(sm.isSelected(c2))
    }

    function test_handleClick_liveCtrl_addsToSelection() {
        // Ctrl carried in livePointerModifiers, not modifiers argument
        var c1 = makeComponent("c1")
        var c2 = makeComponent("c2")
        sm.selectSingle(c1)
        sm.handleComponentClick(c2, 0, null, Qt.ControlModifier)
        compare(sm.componentIds.length, 2)
    }

    function test_handleClick_ctrlBoth_deduplicates() {
        // Ctrl in both modifiers and liveModifiers (| gives same bit) → still Ctrl
        var c1 = makeComponent("c1")
        var c2 = makeComponent("c2")
        sm.selectSingle(c1)
        sm.handleComponentClick(c2, Qt.ControlModifier, null, Qt.ControlModifier)
        compare(sm.componentIds.length, 2)
    }

    // ── selectConnectionModel ──────────────────────────────────────────────

    function test_selectConnection_clearsComponents() {
        var c1 = makeComponent("c1")
        sm.selectSingle(c1)
        var conn = makeConnection("e1", "a", "b")
        sm.selectConnectionModel(conn)
        compare(sm.connection, conn)
        compare(sm.connectionIds.length, 1)
        compare(sm.connectionIds[0], "e1")
        compare(sm.primaryComponent, null)
        compare(sm.componentIds.length, 0)
    }

    function test_selectConnectionsByIds_setsPrimaryAndClearsComponents() {
        var g = Qt.createQmlObject('import ComponentMapEditor; GraphModel {}', testCase)
        var c1 = Qt.createQmlObject('import ComponentMapEditor; ComponentModel {}', g)
        c1.id = "c1"
        var c2 = Qt.createQmlObject('import ComponentMapEditor; ComponentModel {}', g)
        c2.id = "c2"
        g.addComponent(c1)
        g.addComponent(c2)
        var e1 = Qt.createQmlObject('import ComponentMapEditor; ConnectionModel {}', g)
        e1.id = "e1"
        e1.sourceId = "c1"
        e1.targetId = "c2"
        g.addConnection(e1)

        sm.selectSingle(c1)
        sm.selectConnectionsByIds(["e1"], g)
        compare(sm.componentIds.length, 0)
        compare(sm.primaryComponent, null)
        compare(sm.connectionIds.length, 1)
        compare(sm.connection, e1)
    }

    // ── clearAll ───────────────────────────────────────────────────────────

    function test_clearAll_zeroesEverything() {
        var c1 = makeComponent("c1")
        sm.selectSingle(c1)
        sm.connection = makeConnection("e1", "a", "b")
        sm.clearAll()
        compare(sm.primaryComponent, null)
        compare(sm.componentIds.length, 0)
        compare(sm.connection, null)
    }

    // ── isSelected ────────────────────────────────────────────────────────

    function test_isSelected_trueAndFalse() {
        var c1 = makeComponent("c1")
        var c2 = makeComponent("c2")
        sm.selectSingle(c1)
        verify(sm.isSelected(c1))
        verify(!sm.isSelected(c2))
    }

    function test_isSelected_null_returnsFalse() {
        verify(!sm.isSelected(null))
    }

    // ── Interaction mode: ComponentMove ─────────────────────────────────────────

    function test_nodeMove_startEnd() {
        var c1 = makeComponent("c1")
        compare(sm.mode, InteractionStateManager.Idle)
        sm.startComponentMove(c1)
        compare(sm.mode, InteractionStateManager.ComponentMove)
        compare(sm.interactionTarget, c1)
        compare(sm.componentInteractionActive, true)
        compare(sm.backgroundDragEnabled, false)
        sm.endComponentMove()
        compare(sm.mode, InteractionStateManager.Idle)
        compare(sm.interactionTarget, null)
        compare(sm.componentInteractionActive, false)
        compare(sm.backgroundDragEnabled, true)
    }

    // ── Interaction mode: ComponentResize ───────────────────────────────────────

    function test_nodeResize_startEnd() {
        var c1 = makeComponent("c1")
        sm.startComponentResize(c1)
        compare(sm.mode, InteractionStateManager.ComponentResize)
        compare(sm.interactionTarget, c1)
        compare(sm.componentInteractionActive, true)
        sm.endComponentResize()
        compare(sm.mode, InteractionStateManager.Idle)
        compare(sm.interactionTarget, null)
    }

    // ── Interaction mode: ConnectionDraw ───────────────────────────────────

    function test_connectionDraw_startUpdateEnd() {
        var c1 = makeComponent("c1")
        var vs = Qt.point(10, 20)
        var ve = Qt.point(30, 40)
        sm.startConnectionDraw(c1, vs, ve)
        compare(sm.mode, InteractionStateManager.ConnectionDraw)
        compare(sm.connectionDragging, true)
        compare(sm.componentInteractionActive, true)
        compare(sm.interactionTarget, c1)
        compare(sm.tempStart.x, 10)
        compare(sm.tempStart.y, 20)
        compare(sm.tempEnd.x, 30)
        compare(sm.tempEnd.y, 40)
        sm.updateConnectionDraw(Qt.point(11, 21), Qt.point(31, 41))
        compare(sm.tempStart.x, 11)
        compare(sm.tempEnd.y, 41)
        sm.endConnectionDraw()
        compare(sm.mode, InteractionStateManager.Idle)
        compare(sm.connectionDragging, false)
        compare(sm.tempStart.x, 0)
        compare(sm.tempEnd.x, 0)
        compare(sm.interactionTarget, null)
    }

    function test_marquee_startUpdateEnd() {
        sm.startMarqueeSelect(Qt.point(1, 2), Qt.point(1, 2))
        compare(sm.mode, InteractionStateManager.MarqueeSelect)
        compare(sm.marqueeSelecting, true)
        compare(sm.marqueeStart.x, 1)
        compare(sm.marqueeStart.y, 2)
        sm.updateMarqueeSelect(Qt.point(1, 2), Qt.point(5, 6))
        compare(sm.marqueeEnd.x, 5)
        compare(sm.marqueeEnd.y, 6)
        sm.endMarqueeSelect()
        compare(sm.mode, InteractionStateManager.Idle)
        compare(sm.marqueeSelecting, false)
        compare(sm.marqueeStart.x, 0)
        compare(sm.marqueeEnd.x, 0)
    }

    // ── Cancel / reset interaction ─────────────────────────────────────────

    function test_resetInteraction_preservesSelection() {
        var c1 = makeComponent("c1")
        sm.selectSingle(c1)
        sm.startComponentMove(c1)
        sm.suppressNextTap = true
        sm.resetInteraction()
        compare(sm.mode, InteractionStateManager.Idle)
        compare(sm.suppressNextTap, false)
        compare(sm.interactionTarget, null)
        // selection must be untouched after resetInteraction
        compare(sm.primaryComponent, c1)
        compare(sm.componentIds.length, 1)
    }

    function test_resetInteraction_clearsConnectionDraw() {
        var c1 = makeComponent("c1")
        sm.startConnectionDraw(c1, Qt.point(5, 5), Qt.point(10, 10))
        sm.resetInteraction()
        compare(sm.mode, InteractionStateManager.Idle)
        compare(sm.connectionDragging, false)
        compare(sm.tempStart.x, 0)
    }

    // ── Prune stale components after graph mutation ────────────────────────

    function test_pruneStaleComponents_removesDeletedId() {
        var g = Qt.createQmlObject('import ComponentMapEditor; GraphModel {}', testCase)
        var c1 = Qt.createQmlObject('import ComponentMapEditor; ComponentModel {}', g)
        c1.id = "c1"
        var c2 = Qt.createQmlObject('import ComponentMapEditor; ComponentModel {}', g)
        c2.id = "c2"
        g.addComponent(c1)
        g.addComponent(c2)

        sm.componentIds = ["c1", "c2"]
        sm.primaryComponent = c1

        g.removeComponent("c1")
        sm.pruneStaleComponents(g)

        compare(sm.componentIds.length, 1)
        compare(sm.componentIds[0], "c2")
        compare(sm.primaryComponent, null)   // c1 was primary and is now deleted
    }

    function test_pruneStaleComponents_nullGraph_isNoop() {
        var c1 = makeComponent("c1")
        sm.selectSingle(c1)
        sm.pruneStaleComponents(null)       // must not crash
        compare(sm.componentIds.length, 1)
    }

    // ── Prune stale connection ─────────────────────────────────────────────

    function test_pruneStaleConnection_nullsDeletedConnection() {
        var g = Qt.createQmlObject('import ComponentMapEditor; GraphModel {}', testCase)
        var c1 = Qt.createQmlObject('import ComponentMapEditor; ComponentModel {}', g)
        c1.id = "c1"
        var c2 = Qt.createQmlObject('import ComponentMapEditor; ComponentModel {}', g)
        c2.id = "c2"
        g.addComponent(c1)
        g.addComponent(c2)
        var conn = Qt.createQmlObject('import ComponentMapEditor; ConnectionModel {}', g)
        conn.id = "e1"
        conn.sourceId = "c1"
        conn.targetId = "c2"
        g.addConnection(conn)

        sm.connection = conn
        g.removeConnection("e1")
        sm.pruneStaleConnection(g)
        compare(sm.connection, null)
    }

    function test_pruneStaleConnection_keepsSurvivingConnection() {
        var g = Qt.createQmlObject('import ComponentMapEditor; GraphModel {}', testCase)
        var c1 = Qt.createQmlObject('import ComponentMapEditor; ComponentModel {}', g)
        c1.id = "c1"
        var c2 = Qt.createQmlObject('import ComponentMapEditor; ComponentModel {}', g)
        c2.id = "c2"
        g.addComponent(c1)
        g.addComponent(c2)
        var conn = Qt.createQmlObject('import ComponentMapEditor; ConnectionModel {}', g)
        conn.id = "e1"
        conn.sourceId = "c1"
        conn.targetId = "c2"
        g.addConnection(conn)

        sm.connection = conn
        // do NOT remove the connection from graph
        sm.pruneStaleConnection(g)
        compare(sm.connection, conn)     // still alive → unchanged
    }

    // ── removeComponentId (used by deleteComponent) ────────────────────────

    function test_removeComponentId_fromMiddleOfMultiSelect() {
        var c1 = makeComponent("c1")
        var c2 = makeComponent("c2")
        var c3 = makeComponent("c3")
        sm.selectSingle(c1)
        sm.toggleComponent(c2, null)
        sm.toggleComponent(c3, null)
        compare(sm.componentIds.length, 3)
        sm.removeComponentId("c2")
        compare(sm.componentIds.length, 2)
        verify(!sm.isSelected(c2))
        verify(sm.isSelected(c1))
        verify(sm.isSelected(c3))
    }
}
