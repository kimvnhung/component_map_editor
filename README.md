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
  - GraphCommands: GraphCommand subclasses (Add/Remove/Move component & connection)

example/
- standalone demo application

## Requirements

- Qt: 6.5+
- CMake: 3.21+
- C++: 17

## Build

cmake -B build -DCMAKE_PREFIX_PATH=/path/to/Qt6
cmake --build build

The example application is built as build/example/example_app.

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

## GraphExecutionSandbox behavior

GraphExecutionSandbox is a deterministic runtime simulator for a GraphModel.
It does not mutate the graph structure itself. Instead, it snapshots the graph,
executes component-by-component, and produces runtime outputs such as
timeline events, executionState, componentState, and status.

### How it finds the correct workflow to run

The sandbox resolves the workflow in two layers:

- Layer 1: detect executable order from the connection graph.
- Layer 2: execute each ready component using the provider registered for that component type.

The important point is that GraphExecutionSandbox does not try to infer workflow from labels,
UI position, or component titles. It derives workflow only from:

- component ids and types
- connection sourceId -> targetId relationships
- registered execution semantics providers

```plantuml
@startuml
start

:start(inputSnapshot);

if (graph set?) then (yes)
else (no)
  :mark error;
  stop
endif

:read all components from GraphModel;
repeat
  :skip null or empty-id component;
  :build ComponentSnapshot;
  :copy built-in attributes;
  :copy dynamic properties;
  :store snapshot by component id;
repeat while (more components?) is (yes)

:initialize pendingInDegree[id] = 0
for every snapshot component;

:read all connections from GraphModel;
repeat
  :build ConnectionSnapshot;
  if (sourceId and targetId both exist in snapshot?) then (yes)
    :append to outgoingBySource[sourceId];
    :pendingInDegree[targetId] += 1;
  else (no)
    :ignore dangling connection;
  endif
repeat while (more connections?) is (yes)

:sort outgoing connections by targetId, then connection id;
:sort component ids;

repeat
  if (pendingInDegree[id] == 0) then (ready)
    :enqueueReadyComponent(id);
  endif
repeat while (more component ids?) is (yes)

while (readyQueue not empty?) is (yes)
  :take first ready component id;
  :load ComponentSnapshot;
  :find provider by component.type;

  if (provider found?) then (yes)
    :executeComponent(type, id, snapshot, executionState);
  else (no)
    :use default fallback execution;
  endif

  :mark component executed;
  :emit stepExecuted timeline event;

  repeat
    :for each outgoing connection of executed component;
    :pendingInDegree[targetId] -= 1;
    if (pendingInDegree[targetId] == 0) then (yes)
      :enqueueReadyComponent(targetId);
    endif
  repeat while (more outgoing connections?) is (yes)
endwhile (no)

if (executed count == component count?) then (completed)
  :emit simulationCompleted;
else (blocked)
  :emit simulationBlocked;
endif

stop
@enduml
```

1. Execution order (which component runs next)
- On start(), the sandbox first snapshots every valid component into `m_componentsById`.
- It then initializes `m_pendingInDegree` for every snapshotted component to `0`.
- Next, it scans every connection and treats it as a dependency edge:
  `sourceId -> targetId`.
- If either endpoint is missing from the component snapshot, that connection is ignored.
- For each valid connection:
  - it is stored in `m_outgoingBySource[sourceId]`
  - `m_pendingInDegree[targetId]` is incremented
- After scanning all connections, any component with `pendingInDegree == 0` is an entry point.
- Those entry points are inserted into the ready queue in sorted id order.
- Each execution step removes the first ready component, executes it, then walks all of its
  outgoing connections.
- Every outgoing connection reduces the target component's pending in-degree by `1`.
- A target becomes runnable only when all of its incoming dependencies have been satisfied,
  meaning its pending in-degree reaches `0`.
- This is effectively a deterministic topological traversal of the component graph.
- If the ready queue becomes empty before all components are executed, the remaining graph is
  treated as blocked. In practice this means a cycle, a dead dependency chain, or an invalid
  graph shape that never exposes another zero-in-degree component.

2. Execution semantics (how a component is executed)
- Providers are registered by supported component type.
- Internally, the sandbox builds a map: componentType -> provider.
- When a component is dequeued, it looks up provider by component.type.
- If found, executeComponent(...) is called with:
  - component snapshot attributes
  - current executionState input
  - outputState and trace outputs
- If no provider is registered for that type, a default fallback path is used
  (adds trace note and updates lastExecutedComponentId).
- Connection labels do not choose the provider. Provider selection is based on
  component.type, while connections only control execution readiness/order.

### Practical example

For a graph like this:

- `InputA -> Add1`
- `InputB -> Add1`
- `Add1 -> Output1`

the sandbox detects the workflow as:

1. `InputA` and `InputB` start with in-degree `0`, so both are ready immediately.
2. `Add1` starts with in-degree `2`, so it cannot run yet.
3. After `InputA` executes, `Add1` drops to in-degree `1`.
4. After `InputB` executes, `Add1` drops to in-degree `0` and becomes ready.
5. After `Add1` executes, `Output1` drops to in-degree `0` and becomes ready.

So the sandbox finds workflow order from dependency satisfaction, not from a hard-coded
workflow definition file.

This means "correct workflow" is selected by both data dependencies
(connection topology) and type-specific semantics providers.

### Determinism and debugging

- Ready queue insertion is id-sorted, so same graph + same providers produce
  stable execution order.
- Breakpoints can pause before a specific component id executes.
- run(maxSteps) allows bounded execution for inspection and tests.
- Timeline events include simulationStarted, stepExecuted, breakpointHit,
  simulationCompleted, simulationBlocked, and error.

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

## External API Contract (Typed First)

For reuse outside this library, prefer typed protobuf entrypoints.

- Command: `CommandGateway::executeTypedRequest(...)`, `executeTypedSystemCommand(...)`
- Policy: `TypeRegistry::canConnect(const cme::ConnectionPolicyContext&, ...)`
  and `normalizeConnectionProperties(const cme::ConnectionPolicyContext&, ...)`
- Execution: `GraphExecutionSandbox::startTyped(...)`,
  `componentStateTyped(...)`, `executionSnapshotTyped()`
- Schema: `PropertySchemaRegistry::schemaForTargetTyped(...)`,
  `componentSchemaTyped(...)`, `connectionSchemaTyped(...)`
- Action invocation: `ActionInvocationService::invokeActionTyped(...)`

The QVariant/QVariantMap entrypoints remain as compatibility wrappers for
existing integrations and internal glue code.

## Naming note

- The project intentionally avoids creating a QML type named Component because QtQuick already provides a built-in Component type.
- The visual delegate is named ComponentItem to avoid ambiguity, while model naming uses ComponentModel.

## License

MIT
