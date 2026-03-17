# Phase 0 - Baseline Execution Report (component_map_editor only)

## Purpose
Record reproducible baseline behavior/performance before refactor.

## Run Metadata
- Reviewer:
- Executor:
- Date:
- Branch:
- Commit:
- Build type:
- Qt version:
- OS:
- Notes:

## Dataset Definitions
- Small: 50 components / 80 connections
- Medium: 1000 components / 1500 connections
- Large: 5000 components / 8000 connections

---

## A) Critical Flow Repro Checklist

Mark each as `PASS` or `FAIL` and include short evidence notes.

| # | Flow | Small | Medium | Large | Evidence/Notes |
|---|------|-------|--------|-------|----------------|
| 1 | Create component from canvas add path |  |  |  |  |
| 2 | Create component from palette drag-drop path |  |  |  |  |
| 3 | Move component (drag) |  |  |  |  |
| 4 | Resize component |  |  |  |  |
| 5 | Select (single/multi/background clear) |  |  |  |  |
| 6 | Connect components (create/remove) |  |  |  |  |
| 7 | Delete component with linked connections |  |  |  |  |
| 8 | Undo/redo core flows |  |  |  |  |
| 9 | Import/export round trip |  |  |  |  |
| 10 | Validation behavior |  |  |  |  |

Failed flow count:
- Small:
- Medium:
- Large:

---

## B) Performance Baseline Metrics

Units:
- Time in milliseconds.
- Memory in KB.

| Metric | Small | Medium | Large | Notes |
|---|---:|---:|---:|---|
| Frame p95 (ms) |  |  |  |  |
| Camera update p95 (ms) |  |  |  |  |
| Drag latency p95 (ms) |  |  |  |  |
| Route rebuild p50 (ms) |  |  |  |  |
| Route rebuild p95 (ms) |  |  |  |  |
| Route rebuild sample count |  |  |  |  |
| RSS baseline (KB) |  |  |  |  |
| RSS after interaction window (KB) |  |  |  |  |
| RSS delta (KB) |  |  |  |  |

---

## C) Correctness Consistency Counters

| Counter | Small | Medium | Large | Notes |
|---|---:|---:|---:|---|
| Undo/redo mismatch count |  |  |  |  |
| Import/export mismatch count |  |  |  |  |
| Validation false-positive count |  |  |  |  |
| Validation false-negative count |  |  |  |  |

---

## D) API Freeze Confirmation

Check all that apply:
- [ ] C++ QML-facing API list from `refactor_phase0_baseline_and_freeze.md` reviewed.
- [ ] QML component interface freeze list reviewed.
- [ ] No unapproved breaking API changes found.

Exceptions (if any):
- Exception:
  - Approved by:
  - Migration note:

---

## E) Phase 0 Pass Decision

Pass criteria summary:
- [ ] Baseline report completed.
- [ ] Critical flow checklist reproducible.
- [ ] API freeze list confirmed.

Decision:
- [ ] PASS
- [ ] FAIL

Reviewer sign-off:
- Name:
- Date:
- Comment:
