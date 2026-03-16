# Phase 2 - Routing Extraction Execution Pass/Fail Checklist

## Scope
Validate Phase 2 routing extraction changes in `component_map_editor`.

## Current Implementation Slice
- Phase 2 Step A: Routing abstraction/context introduced (`RoutingEngine`, `IRouteStrategy`, `RouteTypes`).
- Phase 2 Step B: `OrthogonalHeuristicStrategy` extracted — full heuristic + A* algorithm moved to `routing/` module.
- Phase 2 Step B (helpers): `RoutingHelpers` module extracted — shared utility functions moved to `routing/`.
- Phase 2 Step D: Viewport cleanup — `struct ConnectionRouteMeta` and `buildConnectionRouteMeta()` removed from anonymous namespace; `LegacyCompositeRouteStrategy` replaced with `OrthogonalHeuristicStrategy` in constructor; both call sites now go through `m_routingEngine->compute()`; helper calls qualified with `RoutingHelpers::`.
- No intended route-behavior change — all algorithms identical.

## Files Touched In This Slice
- `component_map_editor/routing/RouteTypes.h` (ConnectionRouteMeta, RouteRequest, RoutingContext DTOs)
- `component_map_editor/routing/RoutingEngine.h/.cpp` (IRouteStrategy, RoutingEngine)
- `component_map_editor/routing/RoutingHelpers.h/.cpp` (shared utility functions extracted from viewport)
- `component_map_editor/routing/OrthogonalHeuristicStrategy.h/.cpp` (full routing algorithm extracted)
- `component_map_editor/ui/GraphViewportItem.h` (RoutingEngine.h included directly; m_routingEngine member)
- `component_map_editor/ui/GraphViewportItem.cpp` (see Step D above)

---

## A) Build And Static Checks

1. IDE diagnostics
- Step:
  1. Open touched files.
  2. Confirm no compile/lint diagnostics.
- Pass:
  - No new errors in touched files.

2. Build target check
- Step:
  1. Build `libComponentMapEditor` with current active kit.
- Pass:
  - Build succeeds without new warnings treated as errors.
- Fail:
  - Any compile/link error in routing engine integration path.

> **STATUS: ✅ PASS** — `ninja` build (45/45 targets) completes with zero errors after Phase 2 Steps A-D.

---

## B) Golden Route Parity Checks (Manual)

Use same camera, same graph data, same source/target side settings for baseline comparison.

### Scenario B1: Straight/L route baseline
- Steps:
  1. Place two components with clear path.
  2. Connect with default side hints.
  3. Repeat after pan/zoom changes.
- Pass:
  - > **STATUS: ✅ PASS** Polyline shape and arrow orientation match baseline behavior.

### Scenario B2: Dogleg around obstacle
- Steps:
  1. Place obstacle component between source and target.
  2. Connect source->target.
- Pass:
  - > **STATUS: ✅ PASS** Route avoids obstacle and remains orthogonal.

### Scenario B3: Side-constrained endpoints
- Steps:
  1. Set sourceSide/targetSide manually in property panel.
  2. Create and adjust route with several side combinations.
- Pass:
  - > **STATUS: ✅ PASS** Entry/exit orientation follows side rules exactly as before.

### Scenario B4: Dense area fallback behavior
- Steps:
  1. Create local dense cluster around endpoints.
  2. Connect across cluster.
- Pass:
  -**STATUS: ✅ PASS** Route still appears and remains valid (A* fallback behavior unchanged).

### Scenario B5: Determinism check
- Steps:
  1. Save graph.
  2. Clear and reload same graph 3 times.
  3. Compare route shape visually for key connections.
- Pass:
  - **STATUS: ✅ PASS** Same input produces stable route output each run.

---

## C) Integration Correctness Checks

### Scenario C1: Selection and hit-test
- Steps:
  1. Click near route segments and near nodes.
  2. Verify selection changes.
- Pass:
  -**STATUS: ✅ PASS** Connection hit-test and component hit-test behavior unchanged.

### Scenario C2: Undo/redo around routes
- Steps:
  1. Add connection, undo, redo.
  2. Move endpoint component, undo, redo.
- Pass:
  - **STATUS: ✅ PASS** Route updates and reverts correctly each operation.

### Scenario C3: Import/export with route rebuild
- Steps:
  1. Export graph.
  2. Clear + import.
  3. Validate route rendering.
- Pass:
  - **STATUS: ✅ PASS** Routes rebuild correctly post-import.

---

## D) Telemetry Regression Checks

1. Route rebuild telemetry still updates
- Steps:
  1. Trigger connection changes and component moves.
  2. Observe route rebuild counters and p50/p95.
- Pass:
  -**STATUS: ✅ PASS** Counters increase and values are non-zero when expected.

2. Camera-only interaction should not trigger reroute
- Steps:
  1. Pan and zoom without graph mutation.
  2. Observe route rebuild deltas.
- Pass:
  -**STATUS: ✅ PASS** Camera-only operations do not increase route rebuild delta.

---

## E) Pass/Fail Decision Matrix

| Check Group | Result | Notes |
|---|---|---|
| Build/static checks |  |  |
| Golden route parity |  |  |
| Integration correctness |  |  |
| Telemetry regression |  |  |

Final decision:
- [ ] PASS
- [ ] FAIL

Reviewer:
Date:
Build/Commit:

---

## If Failed - Triage Steps

1. If route shape changed:
- Compare call path between engine compute and legacy fallback.
- Verify routeMeta/source/target/context inputs are identical.

2. If hit-test changed:
- Verify route polyline used for spatial index rebuild matches rendered route path.

3. If telemetry changed:
- Confirm route rebuild sample recording still surrounds edge geometry rebuild only.

4. If build fails:
- Check include paths for `routing/*` headers and forward declarations in viewport headers.
