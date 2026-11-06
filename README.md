# component_map_editor

A reusable Qt/QML module (ComponentMapEditor) for building interactive graph or component-map editors.
The module exposes C++ models, services, and commands together with ready-made QML components.

## Final Interaction Architecture (PR6)

The editor now follows a boundary-first interaction architecture:

- Interaction coordination: [component_map_editor/ui/qml/InteractionStateManager.qml](component_map_editor/ui/qml/InteractionStateManager.qml) is the only state machine for pointer interaction mode transitions.
- UI orchestration: [component_map_editor/ui/qml/GraphCanvas.qml](component_map_editor/ui/qml/GraphCanvas.qml) owns gesture flow, selection behavior, and telemetry hooks.
- Mutation boundary: [component_map_editor/services/GraphEditorController.h](component_map_editor/services/GraphEditorController.h) is the default write path for interactive creates/connects/moves/resizes.
- Command persistence: [component_map_editor/commands/UndoStack.h](component_map_editor/commands/UndoStack.h) remains the undo/redo source of truth.
- Rendering and interaction telemetry sink: [component_map_editor/ui/GraphViewportItem.h](component_map_editor/ui/GraphViewportItem.h).

Legacy direct-mutation paths from QML are removed where they were superseded by controller/facade flow, or left behind an explicit off-by-default fallback toggle.

### Interaction telemetry metrics

`GraphViewportItem` now tracks:

- Transition rejects: count of rejected intent transitions.
- Intent latency: p50/p95 for intent calls.
- Action latency: p50/p95 for mutation actions.
- Route rebuild latency: existing renderer route-rebuild p50/p95.

These metrics are exposed in `GraphStatusBar` for live manual/soak validation.

### Migration toggles and safe defaults

The example app (`example/Main.qml`) ships with explicit migration toggles:

- `enableInteractionTelemetry: true`
- `enableStrictMutationGuards: true`
- `enableLegacyPaletteDropFallback: false`

Recommended production default remains legacy fallback disabled. Strict guards are safe-by-default and become effective when an invariant checker is wired into the active mutation path.

### Extension points

- Add/override component and connection policy defaults through `TypeRegistry`.
- Enforce additional mutation invariants via `GraphEditorController.invariantChecker`.
- Extend telemetry consumption by reading `GraphViewportItem` interaction and route metrics.

## Project structure

component_map_editor/
- models/
  - ComponentModel: QML element for a graph component (id, label, x, y, color, type)
  - ConnectionModel: QML element for a directed connection (id, sourceId, targetId, label)
  - GraphModel: QML element holding component + connection lists and CRUD methods
- services/
  - ValidationService: validates graph integrity
  - ExportService: JSON serialization/deserialization
- ui/
  - GraphCanvas.qml: interactive canvas (grid, draggable components, connection rendering)
  - ComponentItem.qml: draggable component card delegate
  - Connection.qml: directed-connection Shape component
  - Palette.qml: sidebar for adding new component types
  - PropertyPanel.qml: inspector panel for selected component/connection
- commands/
  - UndoStack: custom undo/redo stack (no Qt Widgets dependency)
  - GraphCommands: UI-only undo/redo commands for graph editing

example/
- standalone demo application

## Requirements

- Qt: 6.5+
- CMake: 3.21+
- C++: 17

## Build

cmake -B build -DCMAKE_PREFIX_PATH=/path/to/Qt6
cmake --build build

By default, only libraries/modules are built. Enable optional targets with:

cmake -B build \
  -DCMAKE_PREFIX_PATH=/path/to/Qt6 \
  -DCME_BUILD_EXAMPLES=ON \
  -DCME_BUILD_TESTS=ON

When examples are enabled, the example application is built as build/example/example_app.

## Routing Verification

Use the verification checklist in [manual-check/orthogonal_routing_verification.md](manual-check/orthogonal_routing_verification.md)
for Stage 1 + Stage 2 orthogonal router acceptance (unit checks, integration checks,
and benchmark performance gates).

## QML API summary

- ComponentModel: id, label, x, y, color, type
- ConnectionModel: id, sourceId, targetId, label
- GraphModel: components, connections, addComponent(), removeComponent(), addConnection(), removeConnection(), componentById(), connectionById(), clear()
- ValidationService: validate(graph), validationErrors(graph), validationIssues(graph)
- ExportService: exportToJson(graph), importFromJson(graph, json)
- UndoStack: canUndo, canRedo, undoText, redoText, undo(), redo(), clear()
- GraphCanvas: graph, undoStack, selectedComponent, selectedConnection
- ComponentItem: component, selected, undoStack
- Connection: connection, sourceX/Y, targetX/Y, selected
- Palette: graph, undoStack
- PropertyPanel: component, connection

## Validation Architecture

ValidationService is provider-backed.

- ValidationService itself is an orchestrator that builds a graph snapshot and forwards
  it to registered IValidationProvider implementations.
- If no validation providers are configured, a non-null graph is treated as valid by default.
- validate(graph) returns false only when at least one provider issue has severity "error"
  (or an empty severity).

ValidationService API:

- validate(graph): boolean pass/fail for error-level issues
- validationErrors(graph): QStringList flattened from error-level issue messages
- validationIssues(graph): raw provider issue objects (QVariantList of QVariantMap)

Use validationIssues(graph) when UI needs to distinguish warnings from errors.

## External API Contract

External integrations may query read-only provider contracts and execution state. Graph creation and editing remain UI-only.

- Policy: `TypeRegistry::canConnect(...)` and connection normalization.
- Runtime execution is handled by the independent runtime service rather than the editor UI.
- Schema: `PropertySchemaRegistry` typed schema APIs.

- Persistence: `ExportService` remains available for the existing UI flow.

No external command gateway or transport can mutate the graph.

## Naming note

- The project intentionally avoids creating a QML type named Component because QtQuick already provides a built-in Component type.
- The visual delegate is named ComponentItem to avoid ambiguity, while model naming uses ComponentModel.

## License

MIT
