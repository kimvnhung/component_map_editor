# Phase 11: QA Matrix and Release Readiness
## Testing Strategy & Pass/Fail Gates

**Status**: Release Candidate Preparation  
**Target**: v1.0.0 release readiness  
**Date**: March 22, 2026  

---

## 1. QA Matrix Dimensions

### 1.1 Business Pack Variants (3x)
Testing across representative extension pack configurations to validate extensibility and contract adherence:

| Pack Type | Description | Purpose |
|-----------|-------------|---------|
| **Sample Workflow Pack (Current)** | Modern v1 execution semantics, actions, validation rules | Validates core contract alignment |
| **Legacy v0 Compat Pack** | Execution semantics v0 with adapter mediation; component types 0.9.0 | Validates migration path; adapter correctness |
| **Extended Features Pack** | All v1 contracts (rules, schemas, validation); complex rule set | Validates extensibility ceiling; performance under full feature load |

### 1.2 Graph Sizes & Complexity (3x)
Validating performance, memory, and interaction stability across scale dimensions:

| Size Class | Node Count | Connection Count | Graph Density | Purpose |
|-----------|-----------|-----------------|--------------|---------|
| **Small** | 50 | 75 | 0.06 | Interaction responsiveness; baseline latency |
| **Medium** | 500 | 1500 | 0.012 | Typical business workflows; rendering performance |
| **Large** | 2000 | 8000 | 0.004 | Stress test; memory and search performance |

### 1.3 Key User Journeys (6x)

Each journey exercises critical workflows and validates state consistency, undo/redo correctness, and extensibility integration:

| Journey | Steps | Validates |
|---------|-------|-----------|
| **Create & Connect** | Add 5 nodes; connect with rules; validate connections accepted | UI responsiveness; rule engine correctness; persistence checkpoint |
| **Component Validation** | Load pack with validators; add invalid component; trigger validation; check error display | Validation contract; error propagation; extension integration |
| **Simulate (Execute)** | Run execution on graph; check trace output; verify component state mutations | Execution semantics correctness; adapter neutrality (v0/v1); trace fidelity |
| **Undo/Redo Stress** | Create 20-step workflow; undo 10; redo 5; verify graph state matches snapshot | Command history integrity; state machine correctness; redaction handling |
| **Export/Import Cycle** | Export to JSON; mangle internal format; reimport; validate schema recovery | Persistence service; schema evolution tolerance; corruption detection |
| **Extension Lifecycle** | Load pack; trigger rule hot-reload; change rule file; verify recompilation without restart | Extension hot-reload; rule compiler error recovery; registry refresh |

---

## 2. Pass/Fail Gates (Measurable Criteria)

### 2.1 Functional Pass Gates

| Gate ID | Criterion | Expected Result | How to Verify |
|---------|-----------|-----------------|--------------|
| **FUNC-001** | All 6 user journeys pass on all pack variants | 100% journey × pack combinations succeed | Manual script execution; zero crashes/hangs |
| **FUNC-002** | Undo/redo history consistency | 20-step redo after undo matches original | Graph snapshot byte-compare; component state checksum |
| **FUNC-003** | Extension adapter transparency | Legacy v0 pack executes equivalently to modern | Trace comparison; output map matching |
| **FUNC-004** | Validation error capture completeness | All expected schema violations reported | Error count ≥ seed violations injected |
| **FUNC-005** | Export/import roundtrip fidelity | Re-imported graph matches pre-export snapshot | Structural equality + schema version recovery |

### 2.2 Performance Pass Gates

| Gate ID | Criterion | Target | How to Measure |
|---------|-----------|--------|-----------------|
| **PERF-001** | Pan/zoom latency (small graph, idle) | < 16ms p95 | BenchmarkPanel; fps_pan_idle ≥ 60 |
| **PERF-002** | Pan/zoom latency (large graph, idle) | < 50ms p95 | BenchmarkPanel; fps_pan_large ≥ 20 |
| **PERF-003** | Graph rebuild (500→1500 connections) | < 200ms | GraphModel::rebuildIndex timer |
| **PERF-004** | Execution trace generation (large graph) | < 500ms | FrameSwapTelemetry; execution stage timing |
| **PERF-005** | Rule compilation (complex rule set) | < 100ms | RuleHotReloadService; compile stage timer |
| **PERF-006** | Memory footprint (large graph, idle) | < 250MB RSS | Valgrind/RSS monitor on 2000-node scenario |

### 2.3 Stability & Reliability Gates

| Gate ID | Criterion | Target | How to Measure |
|---------|-----------|--------|-----------------|
| **STAB-001** | Zero unhandled exceptions in any journey | 0 crashes | LLDB exception hook; exit codes all 0 |
| **STAB-002** | No resource leaks in multi-journey runs | Δ RSS ≤ 10MB over 10-journey cycle | Valgrind --leak-check=full; full/definitely_lost |
| **STAB-003** | Command stack capacity never exceeded | Stack never > 1000 commands | UndoStack::maxStackSize guard; no silent drops |
| **STAB-004** | Model invariant violations: zero | No assertion failures in production paths | QVERIFY(invariant()) in all mutating paths |
| **STAB-005** | Extension registry mutation safety | No dangling pointers after pack reload | AddressSanitizer on hot-reload scenario |

### 2.4 Defect Priority & Closure Criteria

**Critical Defects** (Release Blocker)
- Unhandled crash/SEGV in any user journey
- Data loss or corruption (graph state, undo history, export file)
- Extension adapter failure blocking legacy pack execution
- Memory leak > 50MB accumulation in single journey

**High Defects** (Must Close Before Release)
- Performance gate miss > 50% tolerance (e.g., PERF-001 > 24ms)
- Validation error incomplete or incorrect classification
- Undo/redo state mismatch detectable by snapshot diff
- Rule hot-reload failure requiring restart

**Medium Defects** (Should Close; Release Gate Optional)
- UI responsiveness degradation perceived but not measurable
- Minor memory optimization opportunities
- Documentation gaps in extension guide
- Non-blocking deprecation warnings

**Low Defects** (Enhancement/Future)
- UX polish (label alignment, tooltip timing)
- Code style inconsistencies
- Test coverage gaps in non-critical paths

---

## 3. Release Readiness Checkpoints

### 3.1 Automated CI Gates
- All unit tests passing (48 + suite)
- All integration tests passing (16 test files)
- Code coverage ≥ 75% on modified code paths (target: core, routing, extension contracts)
- Static analysis clean (clang-tidy, Qt Creator analyzer)
- Memory sanitizer green on 10-journey cycle
- Performance benchmark all gates ≥ 80% of target

### 3.2 Manual Validation Gates
- All 6 user journeys executed on all 3 pack variants (18 scenarios)
- Critical and high defects: count = 0 (closed)
- Performance gates audited by independent reviewer
- Extension migration guide peer-reviewed and signed
- Release notes drafted with breaking/deprecated API changes

### 3.3 Sign-Off Checklist
Must receive documented approval from:
- **Architecture Lead**: System design stability, extensibility ceiling validated, no technical debt blockers
- **QA Lead**: All gates green, test matrix coverage adequate, defect burndown plan in place
- **Product Owner**: Feature completeness vs. roadmap, compatibility with business pack portfolio, migration path acceptable

---

## 4. Timeline & Execution Plan

### Phase 11a: Setup & Automation (Day 1)
- [ ] Create QA matrix test runner (automated)
- [ ] Create manual validation script framework
- [ ] Set up performance monitoring and gate thresholds
- [ ] Record baseline metrics on current main branch

### Phase 11b: Execution & Validation (Day 2–3)
- [ ] Run full QA matrix (all dimensions)
- [ ] Execute 6 user journey scripts on all pack variants
- [ ] Collect performance metrics and validate gates
- [ ] Capture and triage any defects found

### Phase 11c: Cleanup & Release Prep (Day 4)
- [ ] Close critical/high defects (or document exclusions with justification)
- [ ] Generate release readiness report
- [ ] Prepare sign-off artifacts
- [ ] Coordinate peer reviews and final approvals

---

## 5. Success Criteria (Phase 11 Pass)

✓ **Functional**: All 6 user journeys pass on all pack variants (18/18 scenarios green)  
✓ **Performance**: All measurable gates achieve ≥ 80% of target (6/6 PERF gates)  
✓ **Stability**: Zero unhandled exceptions; zero resource leaks > 10MB (STAB-001 + STAB-002)  
✓ **Defect**: All critical and high defects closed (count = 0)  
✓ **Sign-Off**: Architecture, QA, and product leads signed on release readiness checklist

---

## 6. Related Artifacts

- **Acceptance Tests**: `tests/tst_QaMatrixRunner.cpp` (automated matrix executor)
- **Manual Scripts**: `manual-check/phase11_user_journeys.sh` (6 journeys)
- **Performance Baseline**: `manual-check/phase11_performance_gates.json` (measured vs. target)
- **Defect Log**: `manual-check/phase11_defect_log.md` (triage & closure status)
- **Release Report**: `manual-check/phase11_release_readiness_report.md` (final gate summary)
- **Sign-Off Template**: `manual-check/phase11_sign_off_checklist.md` (stakeholder sign-offs)

