# Phase 11: Test Scenarios Matrix  
## Detailed QA Execution Plan

**Date**: March 22, 2026  
**Scope**: Comprehensive testing across 3 pack variants × 3 graph sizes × 6 user journeys  
**Total Scenarios**: 18 core scenarios + 6 performance benchmarks + 2 stress tests = 26 test runs  

---

## 1. Test Scenario Matrix: 18 Core Combinations

### Format: [Pack Variant] × [Graph Size] × [User Journey]

#### Pack Variant 1: **Sample Workflow Pack (Modern v1)**
Standard contemporary business pack with execution semantics v1, actions, and validation.

| Scenario ID | Journey | Graph Size | Expected Duration | Pass Criteria |
|---|---|---|---|---|
| **P1-S1-J1** | Create & Connect | Small (50) | 2–3s | 5 nodes + 3 connections created; rules validated |
| **P1-S1-J2** | Component Validation | Small (50) | 1–2s | Validation schema triggers; error display functional |
| **P1-S1-J3** | Simulate (Execute) | Small (50) | 3–5s | Trace output includes adapter tracing; all fields present |
| **P1-S1-J4** | Undo/Redo Stress | Small (50) | 5–8s | 20-step history; snapshots match post-redo |
| **P1-S1-J5** | Export/Import Cycle | Small (50) | 3–5s | Round-trip JSON preserves all properties |
| **P1-S1-J6** | Extension Lifecycle | Small (50) | 4–6s | Rule hot-reload completes; registry refreshed |
| **P1-S2-J1** | Create & Connect | Medium (500) | 4–6s | 5 nodes + 3 connections; pan/zoom responsive |
| **P1-S2-J2** | Component Validation | Medium (500) | 2–3s | Validation runs on 500-node graph without timeout |
| **P1-S2-J3** | Simulate (Execute) | Medium (500) | 10–15s | All 500 nodes trace included; memory stable |
| **P1-S2-J4** | Undo/Redo Stress | Medium (500) | 8–12s | Command stack handles 20 operations; no overflow |
| **P1-S2-J5** | Export/Import Cycle | Medium (500) | 5–8s | JSON file ≤ 10MB; reimport < 2s |
| **P1-S2-J6** | Extension Lifecycle | Medium (500) | 5–8s | Hot-reload latency ≤ 100ms; rules compiled |
| **P1-S3-J1** | Create & Connect | Large (2000) | 6–10s | Node/connection operations on large graph accepted |
| **P1-S3-J2** | Component Validation | Large (2000) | 3–5s | Validation scalable; index queries responsive |
| **P1-S3-J3** | Simulate (Execute) | Large (2000) | 30–50s | Full trace generation; memory < 250MB RSS |
| **P1-S3-J4** | Undo/Redo Stress | Large (2000) | 15–25s | Undo/redo latency ≤ 500ms per operation |
| **P1-S3-J5** | Export/Import Cycle | Large (2000) | 10–20s | JSON file size acceptable; schema consistent |
| **P1-S3-J6** | Extension Lifecycle | Large (2000) | 8–12s | Hot-reload rule recompilation ≤ 150ms |

#### Pack Variant 2: **Legacy v0 Compatibility Pack**
Execution semantics v0 with adapter mediation; extension version 0.9.0.  
**Special validation**: All trace outputs must include `"adapter": "executionSemantics.v0"` injection.

| Scenario ID | Journey | Graph Size | Expected Duration | Pass Criteria |
|---|---|---|---|---|
| **P2-S1-J1** | Create & Connect | Small (50) | 2–3s | Same as P1-S1-J1; adapter transparent |
| **P2-S1-J3** | Simulate (Execute) | Small (50) | 3–5s | Trace includes v0→v1 adapter annotation |
| **P2-S2-J3** | Simulate (Execute) | Medium (500) | 10–15s | Adapter handles 500-node execution; output consistent |
| **P2-S3-J3** | Simulate (Execute) | Large (2000) | 30–50s | Adapter scales to large graph; trace fidelity preserved |

**Note**: Only execute journey J1 and J3 for Pack 2 (Create & Connect and Simulate). Other journeys are pack-agnostic and redundant.

#### Pack Variant 3: **Extended Features Pack**
All v1 contracts loaded: complex rule set (200+ rules), full validation schema, custom component types.  
**Special validation**: Rule engine performance under load; validator correctness on rich schema.

| Scenario ID | Journey | Graph Size | Expected Duration | Pass Criteria |
|---|---|---|---|---|
| **P3-S1-J1** | Create & Connect | Small (50) | 2–4s | Complex rule validation on each connection |
| **P3-S1-J2** | Component Validation | Small (50) | 2–3s | Rich schema validation triggers; errors precise |
| **P3-S2-J1** | Create & Connect | Medium (500) | 5–8s | Rule engine responsive; no O(n²) behavior manifested |
| **P3-S2-J2** | Component Validation | Medium (500) | 3–5s | Validation on medium graph with rich schema |
| **P3-S3-J1** | Create & Connect | Large (2000) | 10–15s | Rule evaluation still responsive; queries cached |
| **P3-S3-J2** | Component Validation | Large (2000) | 5–8s | Schema validation scalable |

**Note**: Only execute journeys J1 and J2 for Pack 3 (Create & Connect and Component Validation). Other journeys test the same extension/rule contract pathways.

---

## 2. Performance Benchmark Tests (Baseline + Comparison)

Run these in isolated, quiet environment (no other processes). Record p95, median, min/max.

### Benchmark Set 1: Pan/Zoom Latency (Idle, No Mutations)

| Test ID | Pack | Graph Size | Operation | Duration | Target Gate | Measurement |
|---|---|---|---|---|---|---|
| **BENCH-001** | P1 (Modern) | Small (50) | Pan 10 steps; zoom 5 steps | 10s | < 16ms p95 | BenchmarkPanel fps_pan_idle |
| **BENCH-002** | P1 (Modern) | Medium (500) | Pan 10 steps; zoom 5 steps | 10s | < 25ms p95 | BenchmarkPanel fps_pan_idle |
| **BENCH-003** | P1 (Modern) | Large (2000) | Pan 10 steps; zoom 5 steps | 10s | < 50ms p95 | BenchmarkPanel fps_pan_idle |

### Benchmark Set 2: Graph Rebuild Performance

| Test ID | Pack | Graph Size | Operation | Duration | Target Gate | Measurement |
|---|---|---|---|---|---|
| **BENCH-004** | P1 (Modern) | Small→Small | Increment by 100 connections | Varies | < 100ms | GraphModel::rebuildIndex timer |
| **BENCH-005** | P1 (Modern) | Medium→Medium | Increment by 100 connections | Varies | < 200ms | GraphModel::rebuildIndex timer |
| **BENCH-006** | P1 (Modern) | Large→Large | Increment by 100 connections | Varies | < 300ms | GraphModel::rebuildIndex timer |

### Benchmark Set 3: Execution Trace Generation

| Test ID | Pack | Graph Size | Operation | Duration | Target Gate | Measurement |
|---|---|---|---|---|---|
| **BENCH-007** | P1 (Modern) | Small (50) | Full execution + trace | < 2s | < 100ms | FrameSwapTelemetry execution stage |
| **BENCH-008** | P1 (Modern) | Medium (500) | Full execution + trace | < 5s | < 300ms | FrameSwapTelemetry execution stage |
| **BENCH-009** | P1 (Modern) | Large (2000) | Full execution + trace | < 30s | < 500ms | FrameSwapTelemetry execution stage |

### Benchmark Set 4: Memory Footprint (RSS)

| Test ID | Pack | Graph Size | Operation | Duration | Target Gate | Measurement |
|---|---|---|---|---|---|
| **BENCH-010** | P1 (Modern) | Small (50) | Create + idle 30s | 30s | < 100MB RSS | Valgrind / /proc/self/status |
| **BENCH-011** | P1 (Modern) | Medium (500) | Create + idle 30s | 30s | < 150MB RSS | Valgrind / /proc/self/status |
| **BENCH-012** | P1 (Modern) | Large (2000) | Create + idle 30s | 30s | < 250MB RSS | Valgrind / /proc/self/status |

---

## 3. Stress Tests

These tests exercise edge cases and failure modes in controlled ways.

### Stress Test 1: Multi-Journey Leak Detection

**Goal**: Verify no resource leaks accumulate across 10 consecutive journeys

**Procedure**:
1. Load P1 (Modern Pack)
2. Execute journey J1 (Create & Connect) on medium graph
3. Reset graph and UI state completely
4. Repeat steps 2–3 ten times
5. Measure initial RSS vs. final RSS

**Pass Criteria**: Final RSS – Initial RSS ≤ 10MB

**Rationale**: Catches memory leaks in command history, undo/redo stacks, and model observers.

### Stress Test 2: Extension Hot-Reload Stability

**Goal**: Verify pack reloading does not corrupt state or leak references

**Procedure**:
1. Load P1 (Modern Pack)
2. Create medium graph (500 nodes)
3. Trigger extension hot-reload (modify rule file on disk; trigger reload signal)
4. Wait for recompilation and registry refresh
5. Execute journey J1 (Create & Connect) on same graph
6. Repeat steps 3–5 five times with different rule modifications

**Pass Criteria**: 
- Zero segmentation faults or assertion failures
- Graph remains valid after each reload
- AddressSanitizer clean (no use-after-free, invalid reads)

---

## 4. Execution Sequence & Timing

All tests should complete within **4 hours** total wall-clock time (parallelizable where safe).

### Parallel Group A: Core Matrix (Can run concurrently, separate processes)
- P1-S1-J1 through P1-S1-J6 (Small graph, all journeys)
- Runtime: ~30 minutes (all 6 run in parallel)

### Parallel Group B: P1 Medium Graph
- P1-S2-J1 through P1-S2-J6
- Runtime: ~40 minutes

### Sequential Group C: Large Graph Tests
- P1-S3-J1 through P1-S3-J6 (must use single process to control RSS)
- Runtime: ~60 minutes

### Sequential Group D: Legacy Compatibility & Extended Features
- P2-S1-J1, P2-S1-J3, P2-S2-J3, P2-S3-J3
- P3-S1-J1, P3-S1-J2, P3-S2-J1, P3-S2-J2, P3-S3-J1, P3-S3-J2
- Runtime: ~40 minutes (compatibility critical; run serially)

### Sequential Group E: Performance Benchmarks
- BENCH-001 through BENCH-012
- Runtime: ~30 minutes (quiet environment required)

### Sequential Group F: Stress Tests
- Stress Test 1: Multi-Journey Leak Detection (~15 min)
- Stress Test 2: Extension Hot-Reload Stability (~20 min)
- Runtime: ~35 minutes

**Total Expected Duration**: 3.5–4.5 hours (accounting for parallel execution)

---

## 5. Pass/Fail Recording Template

For each scenario, record:

```
Scenario: [ID]
Journey: [Name]
Pack: [Variant]
Graph Size: [Size]
Status: [PASS | FAIL | SKIP]
Duration: [Actual time in seconds]
Issues Found:
  - [Issue 1: description, severity (critical/high/medium/low)]
  - [Issue 2]
Notes: [Any observations, transient behavior, environmental notes]
Timestamp: [YYYY-MM-DD HH:MM:SS UTC]
Executed By: [Tester name/agent]
```

---

## 6. Success Criteria Summary

✓ **Matrix Completion**: All 18 core scenarios executed, status recorded  
✓ **Legacy Compatibility**: P2 scenarios pass; adapter transparency verified in traces  
✓ **Performance Gates**: ≥ 80% of target on all 12 benchmarks  
✓ **Stress Resilience**: Multi-journey leak ≤ 10MB; hot-reload stable (0 crashes)  
✓ **Zero Critical Issues**: No unhandled exceptions, no data corruption detected  

---

## 7. Related Artifacts

- **Test Scenario Results**: `manual-check/phase11_test_matrix_results.json`
- **Performance Benchmark Data**: `manual-check/phase11_performance_benchmarks.csv`
- **Defect Log**: `manual-check/phase11_defect_log.md`
- **Test Runner Script**: `tests/tst_QaMatrixRunner.cpp` (CMake-integrated)

