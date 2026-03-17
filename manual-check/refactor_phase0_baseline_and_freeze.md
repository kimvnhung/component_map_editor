# Phase 0 - Baseline And Freeze (component_map_editor only)

## Scope
- In scope: `component_map_editor/` module APIs and behaviors.
- Out of scope for this phase document: `example/`, `qml_fontawesome/` implementation details.

## Goal
Lock current behavior and performance baseline so refactor safety is measurable.

## Way To Do

### 1) Critical User Flow Checklist (reproducible)

Run each flow on three graph scales:
- Small: 50 components / 80 connections
- Medium: 1000 components / 1500 connections
- Large: 5000 components / 8000 connections

For each flow, record pass/fail and notes.

1. Create component from canvas add path
- Steps:
  1. Add one component at a specific world position.
  2. Verify title/icon/color/type defaults.
  3. Verify selection moves to the new component.
- Expected:
  - Component exists with unique id and expected defaults.
  - Canvas selection is stable.

2. Create component from palette drag-drop path
- Steps:
  1. Drag a palette item onto canvas.
  2. Drop inside canvas bounds.
  3. Repeat drop outside canvas bounds.
- Expected:
  - Inside drop creates at mapped world position.
  - Outside drop follows current fallback behavior.

3. Move component (drag)
- Steps:
  1. Drag a selected component to new location.
  2. Drag rapidly across viewport.
  3. Stop drag and verify final coordinates.
- Expected:
  - No jitter/looping.
  - Final model x/y matches rendered position.

4. Resize component
- Steps:
  1. Resize width/height from handles.
  2. Resize repeatedly with pan/zoom non-default.
- Expected:
  - Geometry remains valid (width/height > 0).
  - Visual and model geometry remain in sync.

5. Select (single, multi, background clear)
- Steps:
  1. Single-click component.
  2. Ctrl-click add/remove multi-select.
  3. Click background to clear.
- Expected:
  - `selectedComponent`, `selectedComponentIds`, and highlights are consistent.

6. Connect components (create/remove)
- Steps:
  1. Start temporary connection from source arrow.
  2. Drop on a valid target.
  3. Remove a connection.
- Expected:
  - New connection has valid source/target ids and side metadata.
  - Remove path updates selection and rendering correctly.

7. Delete component with linked connections
- Steps:
  1. Delete component with incoming/outgoing edges.
  2. Verify dependent connections are removed by current policy.
- Expected:
  - No orphan connections remain.
  - Canvas and model state remain consistent.

8. Undo/redo core flows
- Steps:
  1. Add connection -> undo -> redo.
  2. Update connection sides -> undo -> redo.
  3. Run mixed sequence and traverse full history.
- Expected:
  - History order and state restoration are deterministic.

9. Import/export round trip
- Steps:
  1. Export current graph JSON.
  2. Clear graph.
  3. Import exported JSON.
- Expected:
  - Component/connection counts and key fields match.
  - Side metadata and coordinate semantics are preserved.

10. Validation behavior
- Steps:
  1. Validate a clean graph.
  2. Inject invalid references or invalid side values.
- Expected:
  - Errors are reported accurately and consistently.

### 2) Baseline Metric Capture

Capture metrics for each scale (small/medium/large).

1. Rendering and interaction metrics
- Frame p95 (ms)
- Camera update p95 (ms)
- Drag latency p95 (ms)
- Route rebuild p50/p95 (ms)
- Route rebuild sample count per operation

2. Correctness and stability metrics
- Failed flow count (from checklist)
- Undo/redo mismatch count
- Import/export round-trip mismatch count
- Validation false-positive/false-negative notes

3. Memory/sizing snapshot
- Process RSS baseline (KB)
- RSS after large-scene interaction window (KB)
- Delta RSS (KB)

4. API surface snapshot
- QML_ELEMENT types and invokables/properties list (frozen below)
- QML component public properties/signals list (frozen below)

### 3) Public API Freeze List (during refactor)

No breaking rename/remove/signature changes unless explicitly approved.

#### 3.1 Frozen C++ QML types

1. `ComponentModel`
- Properties: id, title, content, icon, x, y, width, height, shape, color, type

2. `ConnectionModel`
- Properties: id, sourceId, targetId, label, sourceSide, targetSide
- Enum: SideAuto, SideTop, SideRight, SideBottom, SideLeft

3. `GraphModel`
- Properties: components, connections, componentCount, connectionCount
- Invokables: addComponent, removeComponent, componentById, addConnection, removeConnection, connectionById, clear, beginBatchUpdate, endBatchUpdate

4. `UndoStack`
- Properties: canUndo, canRedo, undoText, redoText, count
- Invokables: pushAddConnection, pushRemoveConnection, pushSetConnectionSides
- Slots: undo, redo, clear

5. `ExportService`
- Invokables: exportToJson, importFromJson

6. `ValidationService`
- Invokables: validate, validationErrors

7. `MemoryMonitor`
- Invokables: rssKb, vmPeakKb

8. `GraphViewportItem`
- Properties: graph, panX, panY, zoom, renderGrid, renderEdges, renderNodes, baseGridStep, minGridPixelStep, maxGridPixelStep, selectedConnection, selectedComponent, selectedComponentIds, tempConnectionDragging, tempStart, tempEnd
- Invokables: worldToView, viewToWorld, zoomAtViewAnchor, repaint, hitTestComponentAtView, hitTestConnectionAtView, purgeLabelCache, labelTextureCacheSize, rendererMemoryEstimateMb, routeRebuildLastMs, routeRebuildP50Ms, routeRebuildP95Ms, routeRebuildSampleCount

#### 3.2 Frozen QML component interface (component_map_editor/ui/qml)

1. `GraphCanvas`
- Public properties (frozen names): graph, selectedComponent, selectedComponentIds, selectedConnection, undoStack, tempStart, tempEnd, tempConnectionDragging, zoom, minZoom, maxZoom, panX, panY, telemetry
- Public signals: componentSelected, connectionSelected, backgroundClicked, viewTransformChanged

2. `ComponentItem`
- Public properties: component, selected, renderVisuals, undoStack
- Public signals: componentClicked, connectionDragged, connectionDropped

3. `Palette`
- Public properties: graph, undoStack, canvas
- Public signal: paletteDropRequested

4. `PropertyPanel`
- Public properties: component, connection, undoStack

5. `components/ConnectionHandler`
- Public properties: boundingWidth, arrowActivated
- Public signals: arrowDragged, arrowDropped, hoveredPositionChanged

## Passed Condition

Phase 0 is considered passed only when all conditions below are met:

1. Baseline report exists and is approved
- A completed metric table exists for small/medium/large scales.
- A reviewer sign-off is recorded.

2. Critical flow checklist exists and is reproducible
- All 10 flows have deterministic steps and expected outcomes.
- Re-run on same build gives consistent results.

3. Public API freeze list is documented
- C++ QML types and QML component interfaces above are agreed as frozen.
- Any exception is logged with explicit approval and migration note.

## Approval Block
- Reviewer:
- Date:
- Build/Commit:
- Notes:

## Execution Artifact
- Baseline execution report template: `manual-check/refactor_phase0_baseline_execution_report.md`
