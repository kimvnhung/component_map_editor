# Phase 2 - Routing Engine Extraction (component_map_editor only)

## Scope
- In scope: routing architecture extraction from viewport monolith into dedicated services/strategies.
- Out of scope: visual style changes, interaction redesign, unrelated model/command refactors.

## Goal
Remove routing logic from the large viewport implementation into dedicated strategy-driven services.

## Way To Do

### 1) Create Routing Abstraction And Context Object

Define a routing API that is independent of Qt Scene Graph internals.

## 1.1 Routing abstraction

Introduce interface-level contracts (architecture artifact only in this phase):

1. `IRouterStrategy`
- Responsibility: compute route polyline from immutable context.
- Input: routing context + request.
- Output: route result with status and diagnostics.

2. `RoutingEngine`
- Responsibility: orchestrate strategy chain and fallback logic.
- Input: route request from viewport layer.
- Output: final polyline route for rendering/hit-testing.

3. `RouteValidator`
- Responsibility: apply geometric validity checks consistently.
- Input: candidate route + constraints.
- Output: valid/invalid + reason.

## 1.2 Context and DTO contracts

Use plain data contracts (no scene graph types):

1. `RoutingContext`
- Component obstacle rectangles (world space).
- Component centers and boundary anchors.
- Existing connection metadata needed for side/tangent offsets.
- Tunable parameters (clearance, bend penalty, search limits).

2. `RouteRequest`
- connectionId
- source component id + side preference
- target component id + side preference
- strictness/relaxation policy flags

3. `RouteResult`
- `QVector<QPointF> polyline`
- `enum status` (Exact, Relaxed, Fallback, Failed)
- diagnostics (strategy used, node-expansion count, timing)

4. `RouteDiagnostics`
- strategy name
- fallback path (if any)
- validity checks applied

### 2) Move Orthogonal Heuristic And A* Fallback To Strategy Classes

Define target strategy split:

1. `OrthogonalHeuristicStrategy`
- Encapsulates current deterministic candidate generation and ordering.
- Produces L, dogleg, and side-constrained candidates.

2. `GridAStarStrategy`
- Encapsulates bounded local occupancy-grid A* fallback.
- Uses deterministic tie-break/order policy.

3. `CompositeRoutingPolicy` (inside `RoutingEngine`)
- Try heuristic first.
- Validate candidate.
- On failure, run A* fallback.
- If both fail, return deterministic final fallback route.

### 3) Keep Viewport As Caller/Orchestrator Only

Refined viewport responsibility after extraction:

1. Viewport inputs
- Build read-only route requests/context from model snapshot.

2. Viewport call
- Invoke `RoutingEngine::compute(request, context)`.

3. Viewport outputs
- Consume route polyline for geometry generation, arrow placement, hit-test indexing.

4. Viewport restrictions
- No embedded route-search algorithm logic.
- No strategy-specific branching in rendering code.

---

## Boundary Contract For Phase 2

### Ownership
1. Routing engine owns only temporary computation buffers.
2. Viewport owns render geometry and cache nodes.
3. Model remains owner of graph objects.

### Threading
1. Phase 2 baseline: routing execution remains on GUI/sync path for behavior parity.
2. Inputs are immutable snapshots while computing.
3. No direct model mutation from routing code.

### Mutation Rules
1. Routing layer is pure computation.
2. Same input must produce deterministic output order and shape.
3. Side-relaxation policy must be explicit and traceable in diagnostics.

---

## Migration Plan (No Behavior Change Target)

### Step A: Introduce Contracts (compile-only wiring)
1. Add routing interfaces and DTO/context types.
2. Keep old viewport routing logic active.
3. Add adapter shim that can map old inputs to new context object.

### Step B: Extract Heuristic Strategy
1. Move orthogonal heuristic logic into `OrthogonalHeuristicStrategy`.
2. Keep exact candidate ordering and tie-break behavior.
3. Diff outputs against old logic in golden tests.

### Step C: Extract A* Strategy
1. Move bounded local-grid A* into `GridAStarStrategy`.
2. Keep same bounds, limits, and neighbor-expansion ordering.
3. Verify fallback parity against baseline cases.

### Step D: Switch Viewport To Engine Orchestration
1. Replace direct route computation calls with `RoutingEngine` invocation.
2. Remove residual route-search branches from viewport.
3. Keep rendering and hit-test consumers unchanged.

### Step E: Remove Legacy Routing Code Path
1. Delete duplicated old routing internals once parity is proven.
2. Keep diagnostic hooks for route timings and strategy trace.

---

## Golden Test Plan (Route Output Parity)

### 1) Golden scenarios
1. Direct horizontal path with no obstacles.
2. Direct vertical path with no obstacles.
3. Opposite-side preferred anchors.
4. Same-side source/target conflict requiring dogleg.
5. Obstacle corridor requiring detour.
6. Dense local obstacle field requiring A* fallback.
7. Tight endpoint with side-relaxation fallback.
8. High-edge fan-in/fan-out same-side offset behavior.

### 2) Assertions per scenario
1. Route starts/ends on expected anchors.
2. Polyline is orthogonal (axis-aligned segments only).
3. Route does not violate endpoint-owner intersection rules.
4. Segment count and bend structure match expected policy.
5. Strategy/fallback marker matches expected path.

### 3) Parity criteria
1. New route equals baseline route for strict parity cases.
2. For acceptable-variance cases, route passes same validity checks and deterministic stability across runs.

---

## Independent Routing Unit Test Plan

Routing tests must run without scene graph rendering.

1. Test target
- `routing_engine_tests` (pure C++ test target).

2. Test dependencies
- Only routing DTO/context and minimal geometry helpers.
- No `QQuickItem`, no `QSG*` types.

3. Test categories
- Heuristic strategy deterministic output.
- A* strategy bounded search correctness.
- Validator correctness for invalid/self-crossing/intersection cases.
- Engine fallback sequencing and diagnostics.

4. Determinism checks
- Run same input multiple times; verify identical polyline output and status.

---

## Viewport Integration Verification

1. Visual sanity
- Edge rendering unchanged for baseline scenarios.
- Arrow orientation consistent with terminal segment direction.

2. Interaction sanity
- Connection create/delete/select flows still update routes and hit-test correctly.

3. Telemetry sanity
- Route rebuild telemetry still reports values and sample counts.

---

## Risks And Mitigation

1. Risk: Hidden viewport assumptions in old routing helper functions.
- Mitigation: Adapter layer + temporary dual-run comparison mode in debug.

2. Risk: Candidate order drift causes route shape changes.
- Mitigation: Freeze candidate ordering and add explicit order tests.

3. Risk: A* fallback behavior divergence under bounds.
- Mitigation: Port constants exactly first; optimize later in a separate phase.

4. Risk: Regression in hit-test due to route polyline differences.
- Mitigation: Rebuild spatial index parity tests with golden routes.

---

## Passed Condition

Phase 2 is considered passed only when all conditions below are met:

1. Route outputs match current behavior for golden test cases.
- Golden test suite passes with agreed parity criteria.

2. Viewport still renders correctly with extracted routing.
- Manual validation scenarios pass without visual regressions.

3. Routing unit tests run independently from scene graph rendering.
- Dedicated routing test target executes without Qt scene graph dependencies.

---

## Review Checklist

1. Abstraction and context contracts are documented and approved.
2. Strategy split (heuristic + A*) is explicitly defined.
3. Viewport responsibility is reduced to orchestration only.
4. Golden and unit test plans are complete and reproducible.
5. No implementation code movement happened in this artifact phase.

---

## Approval Block
- Reviewer:
- Date:
- Build/Commit:
- Notes:

## Execution Artifact
- Pass/fail execution checklist: `manual-check/refactor_phase2_execution_pass_fail.md`
