# Orthogonal Routing Verification

This checklist covers Stage 1 and Stage 2 routing acceptance.

## Unit Checks (router behavior)

1. Axis alignment only
- Setup: create connections across random node placements.
- Expectation: every routed segment is horizontal or vertical.

2. Endpoint side anchoring
- Setup: set sourceSide and targetSide to each explicit side pair.
- Expectation: first segment leaves source via source side normal; last segment enters target via target side normal.

3. Simplification correctness
- Setup: route cases that create collinear bends.
- Expectation: collinear intermediate points are removed, connectivity preserved.

4. Obstacle avoidance fallback
- Setup: place blocking nodes between source and target.
- Expectation: Stage 1 candidates fail, Stage 2 local-grid route succeeds when feasible; otherwise deterministic straight fallback with warning.

## Integration Checks (UI/model/interaction)

1. Drag side capture
- Setup: drag from each arrow handle (top/right/bottom/left).
- Expectation: new connection sourceSide matches dragged handle.

2. Polyline hit testing
- Setup: click near each segment and near bends.
- Expectation: intended connection is selected consistently.

3. Undo/redo metadata and lifecycle
- Setup: create/delete connection and change source/target side in PropertyPanel.
- Expectation: undo/redo restores existence, side metadata, and rendered route.

4. Camera-only behavior
- Setup: pan/zoom without graph edits.
- Expectation: no route rebuild delta increase in benchmark route telemetry.

## Performance Gates

Use BenchmarkPanel with 1k/5k/10k datasets and validation matrix.

Required gates:
- Swap p95 <= 16.7 ms (or team-approved threshold)
- Camera p95 <= 16.7 ms (or team-approved threshold)
- Drag p95 <= 8.0 ms (or team-approved threshold)
- Route rebuild p95 <= 25.0 ms
- Pan/Zoom stages: route rebuild delta == 0

## Acceptance Checklist

- Orthogonal-only segments for all rendered connections.
- Terminal arrow direction follows source -> target.
- Backward-compatible import/export for sourceSide/targetSide.
- Hit-test accuracy on multi-segment routes.
- Camera pan/zoom remains transform-only in steady state.
- Undo/redo support for connection create/delete and side metadata edits.
