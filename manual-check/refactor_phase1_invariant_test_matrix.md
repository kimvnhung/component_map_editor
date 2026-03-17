# Phase 1 - Invariant Test Matrix (component_map_editor only)

## Purpose
Map each agreed invariant from Phase 1 to explicit test scenarios and acceptance checks.

## Run Metadata
- Reviewer:
- Executor:
- Date:
- Branch:
- Commit:
- Notes:

---

## A) Graph State Invariants

| Invariant ID | Invariant | Test Type | Test Scenario | Expected Result | Status |
|---|---|---|---|---|---|
| G1 | Component id unique and non-empty | Unit/Integration | Add duplicates and empty id attempts | Duplicate/empty rejected or reported by validation |  |
| G2 | Connection id unique and non-empty | Unit/Integration | Add duplicate/empty connection id | Duplicate/empty rejected or reported by validation |  |
| G3 | Connection references existing components | Integration | Create connection with unknown source/target ids | Validation reports missing references |  |
| G4 | Self-loop policy enforced | Integration | Create sourceId == targetId connection | Behavior matches policy (reject or report) |  |
| G5 | Component width/height > 0 | Unit/Integration | Resize/set invalid dimensions | Invalid geometry prevented or validation error reported |  |
| G6 | Connection side enums valid | Unit/Integration | Inject invalid side values | Validation reports invalid side values |  |
| G7 | `GraphModel::clear()` empties graph | Unit | Populate then clear | componentCount == 0 and connectionCount == 0 |  |
| G8 | Valid import preserves cardinality/required fields | Integration | Export -> clear -> import | Counts and required fields preserved |  |

---

## B) Selection/Interaction Invariants

| Invariant ID | Invariant | Test Type | Test Scenario | Expected Result | Status |
|---|---|---|---|---|---|
| S1 | selectedComponent id exists in selectedComponentIds | Integration/UI | Select single component | selectedComponent id appears in selectedComponentIds |  |
| S2 | Every selectedComponentIds entry exists in graph | Integration/UI | Delete selected component(s) | Removed ids are pruned from selection list |  |
| S3 | selectedConnection and component selection coexist policy is respected | Integration/UI | Select component then connection and inverse order | State respects documented exclusivity/coexistence policy |  |
| S4 | Ending temp connection drag clears drag-active state | Integration/UI | Start drag then cancel/drop | tempConnectionDragging false and selection remains valid |  |
| S5 | Deleting selected component clears its selection atomically | Integration/UI | Select then delete component | No stale selected component id remains |  |
| S6 | Removing selected connection clears selectedConnection | Integration/UI | Select then remove connection | selectedConnection becomes null in same logical op |  |

---

## C) Command Invariants

| Invariant ID | Invariant | Test Type | Test Scenario | Expected Result | Status |
|---|---|---|---|---|---|
| C1 | redo + undo returns prior graph state | Unit/Integration | Execute each command, then undo | Full state parity with pre-command snapshot |  |
| C2 | Stack index bounds valid (`[-1, count-1]`) | Unit | Sequence push/undo/redo/clear | Index always within bounds |  |
| C3 | New push after undo drops redo branch | Unit | Push A,B then undo then push C | Redo for B is unavailable |  |

---

## D) Evidence Links

Attach or reference:
- Test logs:
- Screenshots/gifs for UI invariants:
- Benchmark logs (if relevant):

---

## E) Phase 1 Pass Decision

Pass criteria summary:
- [ ] All invariants mapped to test scenarios.
- [ ] Test outcomes recorded.
- [ ] Reviewer agrees invariants are testable and complete.

Decision:
- [ ] PASS
- [ ] FAIL

Reviewer sign-off:
- Name:
- Date:
- Comment:
