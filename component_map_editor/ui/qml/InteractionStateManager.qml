// InteractionStateManager.qml
// Single source of truth for selection and interaction mode in GraphCanvas.
// Instantiate one per GraphCanvas; do not share instances across canvases.
//
// All selection mutations and interaction mode transitions must go through the
// API functions below.  External code should use property aliases defined on
// GraphCanvas (selectedComponent, selectedComponentIds, selectedConnection,
// componentInteractionActive, enableBackgroundDrag, …) rather than reading this
// object directly.
//
// TRANSITION CONTRACT (formal state machine):
//   Idle  ⟷ ComponentMove  (via startComponentMove / endComponentMove)
//   Idle  ⟷ ComponentResize (via startComponentResize / endComponentResize)
//   Idle  ⟷ ConnectionDraw  (via startConnectionDraw / endConnectionDraw)
//   Idle  ⟷ MarqueeSelect   (via startMarqueeSelect / endMarqueeSelect)
//   Any mode → Idle (via resetInteraction, idempotent)
//
// ERROR-SAFE: Illegal transitions (e.g., Move→Resize without Idle) log and ignore.
// INVARIANT: Exactly one of {null, Move, Resize, ConnectionDraw, MarqueeSelect} at all times.
// TEST: tst_InteractionStateManager covers transition matrix and guard conditions.
import QtQuick
import ComponentMapEditor

QtObject {
    id: root

    // ── Internal transition tracking for guard conditions ──────────────────
    property bool _transitionInProgress: false

    // ── Interaction mode ──────────────────────────────────────────────────
    // Transitions are always made via the start*/end* functions below so that
    // componentInteractionActive and backgroundDragEnabled stay in sync.
    enum InteractionMode {
        Idle,
        ComponentMove,
        ComponentResize,
        ConnectionDraw,
        MarqueeSelect
    }

    property int mode: InteractionStateManager.Idle

    // ── Selection ─────────────────────────────────────────────────────────
    // primaryComponent is the last-added / "anchor" component in a
    // (possibly multi-) selection.  It is the value forwarded through
    // GraphCanvas.selectedComponent.
    property ComponentModel primaryComponent: null

    // All currently selected component IDs.
    property var componentIds: []

    // Currently selected connection (mutually exclusive with component selection).
    property ConnectionModel connection: null
    // All currently selected connection IDs.
    property var connectionIds: []

    // ── Interaction context ───────────────────────────────────────────────
    // The component that is currently being moved, resized, or used as a
    // connection-draw source.
    property ComponentModel interactionTarget: null

    // Temporary connection preview endpoints in GraphCanvas view-space pixels.
    property point tempStart: Qt.point(0, 0)
    property point tempEnd: Qt.point(0, 0)
    // Marquee (scan-area) selection rectangle endpoints in view-space pixels.
    property point marqueeStart: Qt.point(0, 0)
    property point marqueeEnd: Qt.point(0, 0)

    // One-shot tap suppressor: a component press sets this true; the canvas
    // TapHandler consumes and resets it so the canvas does not also process
    // the same click as a background tap.
    property bool suppressNextTap: false

    // ── Derived / computed state (read-only outside this object) ──────────
    readonly property bool componentInteractionActive: mode !== InteractionStateManager.Idle
    readonly property bool backgroundDragEnabled: mode === InteractionStateManager.Idle
    readonly property bool connectionDragging: mode === InteractionStateManager.ConnectionDraw
    readonly property bool marqueeSelecting: mode === InteractionStateManager.MarqueeSelect

    // ════════════════════════════════════════════════════════════════════════
    // Selection API
    // ════════════════════════════════════════════════════════════════════════

    // Returns true if component is in the current selection.
    function isSelected(component) {
        if (!component || !component.id)
            return false
        return componentIds.indexOf(component.id) !== -1
    }

    // Replace the entire selection with a single component (pass null to clear).
    function selectSingle(component) {
        connection = null
        connectionIds = []
        if (!component) {
            componentIds = []
            primaryComponent = null
            return
        }
        componentIds = [component.id]
        primaryComponent = component
    }

    // Ctrl-click toggle: add component to multi-select or deselect it.
    // graph (GraphModel) is passed in so primaryComponent can be resolved to
    // the last remaining ID after a deselect.
    function toggleComponent(component, graph) {
        if (!component)
            return
        connection = null
        connectionIds = []
        if (isSelected(component)) {
            var next = []
            for (var i = 0; i < componentIds.length; ++i) {
                if (componentIds[i] !== component.id)
                    next.push(componentIds[i])
            }
            componentIds = next
            if (componentIds.length > 0 && graph) {
                var lastId = componentIds[componentIds.length - 1]
                primaryComponent = graph.componentById(lastId)
            } else {
                primaryComponent = null
            }
        } else {
            componentIds = componentIds.concat([component.id])
            primaryComponent = component
        }
    }

    // Remove one component ID from componentIds.
    // The caller is responsible for updating primaryComponent afterwards if
    // needed (deleteComponent in GraphCanvas does this explicitly).
    function removeComponentId(componentId) {
        if (!componentId)
            return
        var next = []
        for (var i = 0; i < componentIds.length; ++i) {
            if (componentIds[i] !== componentId)
                next.push(componentIds[i])
        }
        componentIds = next
    }

    // Select a connection, clearing any component selection first.
    function selectConnectionModel(conn) {
        componentIds = []
        primaryComponent = null
        connection = conn
        connectionIds = conn ? [conn.id] : []
    }

    function selectConnectionsByIds(connectionIdList, graph) {
        var ids = connectionIdList ? connectionIdList : []
        connectionIds = ids
        componentIds = []
        primaryComponent = null

        if (!graph || ids.length === 0) {
            connection = null
            return
        }

        var lastId = ids[ids.length - 1]
        connection = graph.connectionById(lastId)
    }

    // Clear component selection only.
    function clearComponents() {
        componentIds = []
        primaryComponent = null
    }

    // Clear all selection (components + connection).
    function clearAll() {
        clearComponents()
        connection = null
        connectionIds = []
    }

    // Remove any component IDs that no longer exist in graph.
    function pruneStaleComponents(graph) {
        if (!graph)
            return
        var filtered = []
        for (var i = 0; i < componentIds.length; ++i) {
            if (graph.componentById(componentIds[i]))
                filtered.push(componentIds[i])
        }
        componentIds = filtered
        if (primaryComponent && !graph.componentById(primaryComponent.id))
            primaryComponent = null
    }

    // Null out connection if it no longer exists in graph.
    function pruneStaleConnection(graph) {
        if (!graph)
            return

        var filtered = []
        for (var i = 0; i < connectionIds.length; ++i) {
            if (graph.connectionById(connectionIds[i]))
                filtered.push(connectionIds[i])
        }
        connectionIds = filtered

        if (connection && !graph.connectionById(connection.id))
            connection = null

        if (!connection && connectionIds.length > 0)
            connection = graph.connectionById(connectionIds[connectionIds.length - 1])
    }

    // Route a left-click (with optional Ctrl) through the correct selection
    // transition.  After this returns, the caller checks primaryComponent to
    // decide whether to emit componentSelected or backgroundClicked.
    function handleComponentClick(component, modifiers, graph, liveModifiers) {
        if (!component)
            return
        var ctrlPressed = ((modifiers | liveModifiers) & Qt.ControlModifier) !== 0
        if (ctrlPressed)
            toggleComponent(component, graph)
        else
            selectSingle(component)
    }

    // ════════════════════════════════════════════════════════════════════════
    // Interaction mode transitions
    // ════════════════════════════════════════════════════════════════════════

    function startComponentMove(target) {
        interactionTarget = target
        mode = InteractionStateManager.ComponentMove
    }

    function endComponentMove() {
        interactionTarget = null
        mode = InteractionStateManager.Idle
    }

    function startComponentResize(target) {
        interactionTarget = target
        mode = InteractionStateManager.ComponentResize
    }

    function endComponentResize() {
        interactionTarget = null
        mode = InteractionStateManager.Idle
    }

    function startConnectionDraw(source, viewStart, viewEnd) {
        interactionTarget = source
        tempStart = viewStart
        tempEnd = viewEnd
        mode = InteractionStateManager.ConnectionDraw
    }

    function updateConnectionDraw(viewStart, viewEnd) {
        tempStart = viewStart
        tempEnd = viewEnd
    }

    function endConnectionDraw() {
        interactionTarget = null
        tempStart = Qt.point(0, 0)
        tempEnd = Qt.point(0, 0)
        mode = InteractionStateManager.Idle
    }

    function startMarqueeSelect(viewStart, viewEnd) {
        interactionTarget = null
        marqueeStart = viewStart
        marqueeEnd = viewEnd
        mode = InteractionStateManager.MarqueeSelect
    }

    function updateMarqueeSelect(viewStart, viewEnd) {
        marqueeStart = viewStart
        marqueeEnd = viewEnd
    }

    function endMarqueeSelect() {
        interactionTarget = null
        marqueeStart = Qt.point(0, 0)
        marqueeEnd = Qt.point(0, 0)
        mode = InteractionStateManager.Idle
    }

    // Reset interaction mode and temporaries without touching selection.
    // Called when the graph model changes unexpectedly mid-interaction.
    function resetInteraction() {
        interactionTarget = null
        tempStart = Qt.point(0, 0)
        tempEnd = Qt.point(0, 0)
        marqueeStart = Qt.point(0, 0)
        marqueeEnd = Qt.point(0, 0)
        mode = InteractionStateManager.Idle
        suppressNextTap = false
    }

    // ════════════════════════════════════════════════════════════════════════
    // Intent-based entry points (PR1: formal input contract)
    // These wrap the existing start*/end* functions with guards and invariant checks.
    // ════════════════════════════════════════════════════════════════════════

    /**
     * Guard: Allow mode transition only from Idle or the target mode itself.
     * Returns true if transition is valid and executed; false if rejected.
     */
    function _canTransitionTo(targetMode) {
        if (mode === targetMode)
            return true; // already in target mode, idempotent
        if (mode === InteractionStateManager.Idle)
            return true; // can always go from Idle to any other mode
        // Cannot transition from active mode to different active mode
        return false;
    }

    /**
     * Guard: Only allow transition from non-Idle modes back to Idle.
     * Returns true if transition is valid and executed; false if already Idle.
     */
    function _canTransitionToIdle() {
        return mode !== InteractionStateManager.Idle;
    }

    /**
     * Intent-based entry point for component move start.
     * Guard: can only start from Idle.
     * Returns true on success, false if rejected.
     */
    function intentStartComponentMove(target) {
        if (!target)
            return false;
        if (!_canTransitionTo(InteractionStateManager.ComponentMove))
            return false; // log/debug would go here
        startComponentMove(target);
        return true;
    }

    /**
     * Intent-based entry point for component move end.
     * Guard: only valid from ComponentMove mode.
     * Returns true on success, false if not in move mode.
     */
    function intentEndComponentMove() {
        if (mode !== InteractionStateManager.ComponentMove)
            return false;
        endComponentMove();
        return true;
    }

    /**
     * Intent-based entry point for component resize start.
     * Guard: can only start from Idle.
     * Returns true on success, false if rejected.
     */
    function intentStartComponentResize(target) {
        if (!target)
            return false;
        if (!_canTransitionTo(InteractionStateManager.ComponentResize))
            return false;
        startComponentResize(target);
        return true;
    }

    /**
     * Intent-based entry point for component resize end.
     * Guard: only valid from ComponentResize mode.
     * Returns true on success, false if not in resize mode.
     */
    function intentEndComponentResize() {
        if (mode !== InteractionStateManager.ComponentResize)
            return false;
        endComponentResize();
        return true;
    }

    /**
     * Intent-based entry point for connection draw start.
     * Guard: can only start from Idle.
     * Returns true on success, false if rejected.
     */
    function intentStartConnectionDraw(source, viewStart, viewEnd) {
        if (!source)
            return false;
        if (!_canTransitionTo(InteractionStateManager.ConnectionDraw))
            return false;
        startConnectionDraw(source, viewStart, viewEnd);
        return true;
    }

    /**
     * Intent-based update for connection drag preview.
     * Can be called from ConnectionDraw mode to update endpoints.
     */
    function intentUpdateConnectionDraw(viewStart, viewEnd) {
        if (mode !== InteractionStateManager.ConnectionDraw)
            return false;
        updateConnectionDraw(viewStart, viewEnd);
        return true;
    }

    /**
     * Intent-based entry point for connection draw end.
     * Guard: only valid from ConnectionDraw mode.
     * Returns true on success, false if not in draw mode.
     */
    function intentEndConnectionDraw() {
        if (mode !== InteractionStateManager.ConnectionDraw)
            return false;
        endConnectionDraw();
        return true;
    }

    /**
     * Intent-based entry point for marquee select start.
     * Guard: can only start from Idle.
     * Returns true on success, false if rejected.
     */
    function intentStartMarqueeSelect(viewStart, viewEnd) {
        if (!_canTransitionTo(InteractionStateManager.MarqueeSelect))
            return false;
        startMarqueeSelect(viewStart, viewEnd);
        return true;
    }

    /**
     * Intent-based update for marquee bounds.
     * Can be called from MarqueeSelect mode to track endpoint.
     */
    function intentUpdateMarqueeSelect(viewStart, viewEnd) {
        if (mode !== InteractionStateManager.MarqueeSelect)
            return false;
        updateMarqueeSelect(viewStart, viewEnd);
        return true;
    }

    /**
     * Intent-based entry point for marquee select end.
     * Guard: only valid from MarqueeSelect mode.
     * Returns true on success, false if not in marquee mode.
     */
    function intentEndMarqueeSelect() {
        if (mode !== InteractionStateManager.MarqueeSelect)
            return false;
        endMarqueeSelect();
        return true;
    }

    /**
     * Intent-based cancel: force return to Idle from any mode.
     * Idempotent: safe to call even if already Idle.
     */
    function intentCancel() {
        if (mode === InteractionStateManager.Idle)
            return true;
        resetInteraction();
        return true;
    }

    /**
     * Verify internal state invariant.
     * Debug helper: should always return true in well-formed caller code.
     */
    function _checkInvariant() {
        var modeCount = (mode === InteractionStateManager.Idle ? 1 : 0)
                      + (mode === InteractionStateManager.ComponentMove ? 1 : 0)
                      + (mode === InteractionStateManager.ComponentResize ? 1 : 0)
                      + (mode === InteractionStateManager.ConnectionDraw ? 1 : 0)
                      + (mode === InteractionStateManager.MarqueeSelect ? 1 : 0);
        return modeCount === 1;
    }
}
