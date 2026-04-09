# Phase 0: Baseline and Freeze Report

## Freeze Metadata

| Field | Value |
|---|---|
| Branch | `create_example` |
| Commit | `9a1d681` |
| Frozen at | `2026-03-28T16:52:24Z` |
| Build config | `Desktop_Qt_6_9_1-Debug` |
| Qt version | 6.9.1 |
| C++ standard | 17 |

---

## Test Suite Baseline: All 15 Tests Passed

| # | Test Suite | Result | Wall Time |
|---|---|---|---|
| 1 | tst_InteractionStateManager | PASS | 0.48 s |
| 2 | tst_PersistenceValidation | PASS | 0.02 s |
| 3 | tst_GraphModelIndexing | PASS | 44.57 s |
| 4 | tst_ExtensionContractRegistry | PASS | 0.02 s |
| 5 | tst_ExtensionStartupLoader | PASS | 0.02 s |
| 6 | tst_TypeRegistry | PASS | 0.02 s |
| 7 | tst_GraphEditorController | PASS | 0.02 s |
| 8 | tst_PropertySchemaRegistry | PASS | 0.02 s |
| 9 | tst_RuleCompilerRuntime | PASS | 0.04 s |
| 10 | tst_TraversalEngine | PASS | 2.08 s |
| 11 | tst_GraphExecutionSandbox | PASS | 0.02 s |
| 12 | tst_SecurityGuardrails | PASS | 0.02 s |
| 13 | tst_CompatibilityMigrationToolkit | PASS | 0.02 s |
| 14 | tst_PerformanceScaleHardening | PASS | 4.86 s |
| 15 | tst_QaMatrixRunner | PASS | 1.10 s |

**Total: 15/15 PASS — Total wall time: 53.30 s**

---

## Performance Budget Baselines (Frozen)

These thresholds must not be relaxed in any migration phase without an explicit
performance measurement report and reviewer approval.

| Metric | Budget | Gate rule |
|---|---|---|
| Frame render p95 | ≤ 16.0 ms | Hard |
| Command latency p95 (1500 move ops) | ≤ 50.0 ms | Hard |
| Memory growth over 80 long-session iters | ≤ 64 MB | Hard |
| Validation p95 (1500 nodes) | ≤ 50.0 ms | Hard |
| Traversal BFS p95 (1500 nodes) | ≤ 50.0 ms | Hard |
| Execution sandbox p95 (1500 nodes) | ≤ 120.0 ms | Hard |
| Strict-mode overhead ratio vs non-strict | ≤ 20× | Hard |
| Strict-mode p95 command latency | ≤ 50.0 ms | Hard |

---

## Frozen Contract Points

### CommandGateway — Supported Commands (exact, ordered)

```
addComponent
removeComponent
moveComponent
addConnection
removeConnection
setComponentProperty
setConnectionProperty
```

### CommandGateway — Request Key Allowlist per Command

| Command | Allowed keys |
|---|---|
| addComponent | command, id, typeId, x, y |
| removeComponent | command, id |
| moveComponent | command, id, x, y |
| addConnection | command, id, sourceId, targetId, label |
| removeConnection | command, id |
| setComponentProperty | command, id, property, value |
| setConnectionProperty | command, id, property, value |

### GraphSchema — Canonical JSON Key Names

| Purpose | Key |
|---|---|
| Component id | `id` |
| Component type | `type` |
| Component title | `title` |
| Component x | `x` |
| Component y | `y` |
| Component width | `width` |
| Component height | `height` |
| Component color | `color` |
| Component shape | `shape` |
| Component content | `content` |
| Component icon | `icon` |
| Connection id | `id` |
| Connection source | `sourceId` |
| Connection target | `targetId` |
| Connection label | `label` |
| Connection source side | `sourceSide` |
| Connection target side | `targetSide` |
| Graph components array | `components` |
| Graph connections array | `connections` |
| Coordinate system | `coordinateSystem` |

### Extension Provider IIDs (Binary-stable identifiers)

| Interface | IID |
|---|---|
| IComponentTypeProvider | `ComponentMapEditor.Extensions.IComponentTypeProvider/1.0` |
| IConnectionPolicyProvider | `ComponentMapEditor.Extensions.IConnectionPolicyProvider/1.0` |
| IActionProvider | `ComponentMapEditor.Extensions.IActionProvider/1.0` |
| IValidationProvider | `ComponentMapEditor.Extensions.IValidationProvider/2.0` |
| IExecutionSemanticsProvider | `ComponentMapEditor.Extensions.IExecutionSemanticsProvider/1.0` |
| IPropertySchemaProvider | `ComponentMapEditor.Extensions.IPropertySchemaProvider/1.0` |

### ValidationService — Issue Map Keys (required)

```
code, severity, message, entityType, entityId
```

Sentinel null-graph issue: `code=CORE_NULL_GRAPH`, `severity=error`, `entityType=graph`

### GraphExecutionSandbox — Timeline Event Names (frozen string values)

| Event | Trigger |
|---|---|
| `simulationStarted` | `start()` called |
| `simulationCompleted` | all components executed |
| `simulationBlocked` | unresolvable dependency |
| `breakpointHit` | breakpoint component reached |
| `error` | provider execution failure |

---

## Behavioral Invariants Confirmed

1. **Capability gate enforced**: `executeRequest` with no capability returns `false` and logs `blocked=true`.
2. **Missing required field rejected**: `addComponent` without `id` returns `false`.
3. **Unknown command rejected**: unrecognized command type returns `false`.
4. **Post-invariant rollback**: graph is restored to pre-command state on post-check failure.
5. **Duplicate component rejected**: `addComponent` with existing id returns `false`.
6. **TypeRegistry open-world**: no policy providers = all connections allowed.
7. **Execution determinism**: same graph + same providers = same timeline order across multiple runs.
8. **Request log**: every `executeRequest` call appends exactly one log entry.

---

## Phase 0 Acceptance Criteria (Verified)

- [x] All 15 existing tests pass with 0 failures.
- [x] Performance budgets confirmed and frozen in `PerformanceBudgets`.
- [x] `tst_Phase0Baseline.cpp` created with 18 contract-locking tests.
- [x] `tst_Phase0Baseline` wired into `tests/CMakeLists.txt`.
- [x] Baseline report saved to `manual-check/phase0_baseline_report.md`.
- [x] Total test count after Phase 0 addition: 16 suites.

---

## How to Re-Run the Baseline

```bash
cd build/Desktop_Qt_6_9_1-Debug
cmake --build . --parallel 4
ctest --output-on-failure --timeout 120
```

The suite `tst_Phase0Baseline` must be run at the start of every subsequent phase
before any new code is merged. A failure means a regression was introduced.

---

## Recheck Update (2026-03-29)

- Re-ran `tests/tst_Phase0Baseline` after canonical validation API cutover.
- Updated Phase 0 IID expectation for `IValidationProvider` to `/2.0` in test and report.
- Recheck result target: `tst_Phase0Baseline` passes with this updated contract.

---

## Next Phase

**Phase 1: Protobuf Contract Foundation**

Entry condition: this report file and `tst_Phase0Baseline` exist and pass.

First deliverable: add `proto/` directory with `command.proto`, `graph.proto`,
`validation.proto`, `execution.proto`, and `policy.proto`; integrate protobuf
codegen into CMake; add roundtrip encode/decode tests.
