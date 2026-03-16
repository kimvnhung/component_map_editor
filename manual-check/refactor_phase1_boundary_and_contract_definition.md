# Phase 1 - Boundary And Contract Definition (component_map_editor only)

## Scope
- In scope: `component_map_editor/` architecture contracts only.
- Out of scope: implementation/code movement, optimization, behavioral changes.

## Goal
Make responsibilities explicit before code movement.

## Way To Do

### 1) Target Boundary Map

Define explicit boundaries for seven architecture areas.

1. Model boundary
- Owns: graph entities and domain state (`ComponentModel`, `ConnectionModel`, `GraphModel`).
- Exposes: stable data properties and graph CRUD operations.
- Does not own: rendering decisions, routing algorithm details, UI interaction mode.

2. Command boundary
- Owns: reversible user/domain mutations and command history (`UndoStack`, `GraphCommands`).
- Exposes: push/undo/redo/clear and command metadata.
- Does not own: direct rendering updates or persistence formatting.

3. Rendering boundary
- Owns: view-space/world-space drawing, scene graph node lifecycle, hit-test geometry/index cache.
- Exposes: camera transform API, repaint/hit-test/read-only telemetry.
- Does not own: domain mutations or graph business rules.

4. Routing boundary
- Owns: edge path computation from graph geometry and side hints.
- Exposes: route request/route result contract only.
- Does not own: scene graph node construction or command state.

5. State boundary
- Owns: interaction/session state for selection and temporary gestures (selected ids, temp connection drag states).
- Exposes: deterministic transition rules for selection and interaction modes.
- Does not own: persistence schema or route algorithm internals.

6. Persistence boundary
- Owns: import/export formats, version compatibility, migration rules.
- Exposes: serialize/deserialize graph and compatibility policy.
- Does not own: UI interactions or render cache lifecycle.

7. Validation boundary
- Owns: graph consistency checks and reporting of invalid states.
- Exposes: boolean validity + detailed error list.
- Does not own: mutation application or command history.

---

### 2) Input/Output Contracts By Boundary

## 2.1 Model Contract
Inputs:
- Property updates on component/connection objects.
- Graph add/remove/clear requests.

Outputs:
- Change notifications (`componentAdded`, `componentRemoved`, `connectionAdded`, `connectionRemoved`, `componentsChanged`, `connectionsChanged`).
- Query responses (`componentById`, `connectionById`, list variants).

Ownership rules:
- `GraphModel` is owner of model objects once added (QObject parent ownership).
- Removed objects are detached from parent and become caller/command responsibility.

Threading rules:
- All model mutation occurs on GUI thread.
- No cross-thread model writes allowed.

Mutation rules:
- Graph mutation must preserve id uniqueness constraints.
- Batch mode may suppress per-item changed signals but must emit consolidated final signals at end.

## 2.2 Command Contract
Inputs:
- Command push requests from UI/service layers.
- Undo/redo requests.

Outputs:
- Deterministic state changes through command `redo/undo`.
- Stack state signals (`canUndoChanged`, `canRedoChanged`, `undoTextChanged`, `redoTextChanged`, `countChanged`).

Ownership rules:
- `UndoStack` owns command objects after push.
- Commands own temporary detached model objects when needed for undo/redo lifecycle.

Threading rules:
- Command execution only on GUI thread.

Mutation rules:
- Every command must be idempotent per `redo`/`undo` cycle.
- Mergeable commands must only merge same target and compatible intent.

## 2.3 Rendering Contract
Inputs:
- Read-only graph snapshot + camera + selection/temp-gesture state.

Outputs:
- Scene graph geometry updates.
- Hit-test result references (read-only usage by caller).
- Performance telemetry values (read-only, observational).

Ownership rules:
- Rendering layer owns scene graph nodes and render caches.
- Rendering layer does not take ownership of domain model objects.

Threading rules:
- API invocation and state writes from GUI thread.
- Scene graph sync/render internals follow Qt render-thread model.

Mutation rules:
- Rendering must not mutate graph domain objects.
- Rendering dirty-flag transitions must be monotonic per frame tick.

## 2.4 Routing Contract
Inputs:
- Source/target endpoints, component obstacle geometry, side constraints, tunable routing parameters.

Outputs:
- Polyline route in world coordinates.
- Route metadata (success/fallback path, optional diagnostics).

Ownership rules:
- Routing owns temporary route computation data only.

Threading rules:
- Same thread as caller unless explicitly isolated with safe immutable inputs.

Mutation rules:
- Pure computation: no direct graph mutation.
- Fallback must be deterministic for same input set.

## 2.5 State Contract
Inputs:
- User interaction events and modifier keys.
- Graph membership updates (for selection pruning).

Outputs:
- Selected component/connection state.
- Interaction mode state transitions.

Ownership rules:
- State owner controls authoritative selection/interaction values.
- Other layers treat these values as inputs, not independent authorities.

Threading rules:
- GUI thread only.

Mutation rules:
- Selection updates are atomic per event handling pass.
- State must be pruned when backing graph objects disappear.

## 2.6 Persistence Contract
Inputs:
- Graph snapshot for export.
- JSON payload for import.

Outputs:
- JSON representation with documented coordinate/version semantics.
- Imported graph with backward compatibility handling.

Ownership rules:
- Persistence does not own graph lifetime; it only reads/writes graph content.

Threading rules:
- Current contract assumes GUI-thread graph mutation.

Mutation rules:
- Import must produce valid graph object structure even with partial/legacy fields.
- Export/import round-trip must preserve defined fields.

## 2.7 Validation Contract
Inputs:
- Graph snapshot.

Outputs:
- `validate()` boolean.
- `validationErrors()` stable, human-readable errors.

Ownership rules:
- Validation owns no graph objects.

Threading rules:
- GUI thread unless immutable snapshot is introduced.

Mutation rules:
- Validation is side-effect free (no graph writes).

---

### 3) Cross-Boundary Rules

1. Dependency direction
- UI/State -> Command/Model/Rendering
- Rendering -> Model (read-only)
- Persistence/Validation -> Model
- Command -> Model
- Routing -> Model geometry snapshots (read-only)

2. Prohibited direct coupling
- Rendering must not invoke persistence/validation.
- Persistence must not alter UI state directly.
- Validation must not mutate graph.

3. Event and signal discipline
- Narrow signal emissions preferred over broad global invalidation.
- Batch mutation operations must provide consolidated completion signaling.

4. Identity and lifecycle
- Component/connection ids are immutable identity for references in connection and selection contracts.
- Deletion must clear dependent references (selection and connection references) within same logical operation.

---

### 4) Invariants (Agreed And Testable)

## 4.1 Graph State Invariants
1. Every component id is unique and non-empty.
2. Every connection id is unique and non-empty.
3. Every connection sourceId and targetId references an existing component id.
4. Connection self-loop policy is explicit: if disallowed, validator must always reject it.
5. Component geometry constraints hold: width > 0 and height > 0.
6. Connection side values are valid enum values only.
7. After `GraphModel::clear()`, component and connection counts are both zero.
8. Import of valid payload preserves graph cardinality and required fields.

## 4.2 Selection/Interaction State Invariants
1. If `selectedComponent` is non-null, its id must be in `selectedComponentIds`.
2. Every id in `selectedComponentIds` must exist in graph.
3. `selectedConnection` and non-empty `selectedComponentIds` may coexist only if explicitly allowed by state policy; otherwise they are mutually exclusive.
4. Ending temporary connection drag clears drag-active state and leaves valid selection state.
5. Deleting a selected component removes it from selection in same operation.
6. Removing a selected connection clears `selectedConnection` in same operation.

## 4.3 Command Invariants
1. For any command, `redo(); undo();` returns to prior graph state.
2. Command stack index is always within `[-1, count-1]`.
3. Pushing a new command after undo drops obsolete redo branch.

---

### 5) Review Checklist (Phase 1 artifact review)

1. Boundary map reviewed by maintainers.
2. Each boundary has explicit input/output/ownership/threading/mutation rules.
3. Graph and selection invariants are accepted.
4. Every invariant has at least one planned test case.
5. No implementation edits are introduced in this phase.

---

## Passed Condition

Phase 1 is considered passed only when all conditions below are met:

1. Boundary map and interface contracts are written and reviewed.
- This document is completed and approved.
- Reviewer sign-off recorded.

2. Invariants are agreed and testable.
- Graph and selection invariant list is accepted.
- Test planning references exist for each invariant.

3. No implementation changes yet.
- Only architecture contract artifacts are changed.

---

## Approval Block
- Reviewer:
- Date:
- Build/Commit:
- Notes:

## Execution Artifact
- Invariant test matrix template: `manual-check/refactor_phase1_invariant_test_matrix.md`
