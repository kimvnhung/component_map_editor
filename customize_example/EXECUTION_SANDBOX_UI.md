# GraphExecutionSandbox — UI Integration Guide

How to wire `GraphExecutionSandbox` to the QML layer and present it to the user in `customize_example`.

---

## 1. Architecture Overview

```
C++ (main.cpp)
  └── GraphExecutionSandbox  ──rebuildSemanticsFromRegistry()──► ExtensionContractRegistry
          │
          │  context property: "customizeExecutionSandbox"
          ▼
QML (Main.qml)
  └── property var executionSandbox: startupExecutionSandbox
          │
          ├── Component.onCompleted: executionSandbox.graph = graph
          │
          └── Execution tab panel
                ├── Buttons:  Start · Step · Run · Reset
                ├── Status grid: status, currentTick, snapshotSummary, lastError
                ├── TextArea: executionState (full key→value map)
                ├── TextArea: componentState(selected id)  (node-level)
                └── TextArea: timeline (event log)
```

There are three layers:

| Layer | File | Role |
|-------|------|------|
| **Service** | `component_map_editor/services/GraphExecutionSandbox.h/.cpp` | Owns execution state, tick counter, timeline, breakpoints |
| **Wiring** | `customize_example/main.cpp` | Creates sandbox, injects semantics, exposes as QML context property |
| **UI** | `customize_example/qml/Main.qml` | Binds to Q_PROPERTYs, calls Q_INVOKABLEs |

---

## 2. C++ Layer — `main.cpp`

Nothing needs to change here; it is already complete:

```cpp
GraphExecutionSandbox executionSandbox;
executionSandbox.rebuildSemanticsFromRegistry(extensionContracts); // wire execution providers
engine.rootContext()->setContextProperty(
    QStringLiteral("customizeExecutionSandbox"), &executionSandbox);
```

Key point: `rebuildSemanticsFromRegistry` must be called **before** `engine.load()` so the sandbox knows about all registered component types.

---

## 3. Q_PROPERTY Bindings (read from QML)

| Property | Type | QML binding example | When it changes |
|----------|------|---------------------|-----------------|
| `graph` | `GraphModel*` | Set once: `executionSandbox.graph = graph` | On assignment |
| `status` | `QString` | `text: executionSandbox.status` | Every FSM transition |
| `currentTick` | `int` | `text: String(executionSandbox.currentTick)` | After each `step()` |
| `executionState` | `QVariantMap` | `text: prettyJson(executionSandbox.executionState)` | After each `step()` |
| `timeline` | `QVariantList` | `text: timelineText(executionSandbox.timeline)` | After each event |
| `lastError` | `QString` | `visible: lastError.length > 0` | On error |

### Status FSM

```
idle ──start()──► paused ──step()──► paused (repeats)
                         ──run()───► completed
                  paused ──pause()──► paused  (no-op while already paused)
  * ──reset()──────────────────────► idle
  * ──error══════════════════════════► error
```

---

## 4. Q_INVOKABLE Calls (buttons / gestures)

| Button | Call | Guard |
|--------|------|-------|
| **Start** | `executionSandbox.start()` → returns `bool` | None; sets status `idle→paused` |
| **Step** | `executionSandbox.step()` | Auto-start if `idle`; disabled when `completed`/`error` |
| **Run** | `executionSandbox.run()` | Auto-start if `idle`; disabled when `completed`/`error` |
| **Reset** | `executionSandbox.reset()` | Always enabled |

`start()` returns `false` if the graph has no valid start node or the graph is unset — check `lastError` in that case.

---

## 5. Q_INVOKABLE Queries (read on demand)

| Function | Returns | Use in QML |
|----------|---------|------------|
| `snapshotSummary()` | `QVariantMap` — condensed final values | Summary grid row |
| `componentState(id)` | `QVariantMap` — node-level key→value | "Selected Component State" TextArea |
| `debugSnapshot()` | `QVariantMap` — full internal state | Debugging only |
| `executionTelemetry()` | `QVariantMap` — timing data | Performance review |
| `breakpoints()` | `QVariantList` | Breakpoint panel (future) |

---

## 6. Timeline Event Schema

Each entry in `executionSandbox.timeline` is a `QVariantMap`:

| `event` value | Extra keys |
|---------------|-----------|
| `simulationStarted` | — |
| `stepExecuted` | `componentId`, `type` |
| `simulationPaused` | `tick` |
| `simulationCompleted` | `tick`, `elapsed_ms` |
| `breakpointHit` | `componentId` |
| `error` | `message`, `componentId` (if applicable) |

Helper function used in QML:

```qml
function timelineText(entries) {
    if (!entries || entries.length === 0)
        return "No execution events yet.";
    var lines = [];
    for (var i = 0; i < entries.length; ++i) {
        var entry = entries[i];
        var payload = {};
        for (var key in entry) {
            if (key === "event" || key === "tick") continue;
            payload[key] = entry[key];
        }
        var suffix = Object.keys(payload).length ? "  " + JSON.stringify(payload) : "";
        lines.push("[" + entry.tick + "] " + entry.event + suffix);
    }
    return lines.join("\n");
}
```

---

## 7. What Was Implemented in `Main.qml`

The right-side inspector panel (previously a plain `PropertyPanel`) now has two tabs:

```
┌──────────────────────────────────┐
│   Properties  │  Execution       │  ← TabBar
├──────────────────────────────────┤
│                                  │
│  [Properties tab]                │
│    PropertyPanel (unchanged)     │
│                                  │
│  [Execution tab]                 │
│    Heading: "Graph Execution"    │
│                                  │
│  ┌──────┬──────┬─────┬────────┐  │
│  │Start │ Step │ Run │ Reset  │  │
│  └──────┴──────┴─────┴────────┘  │
│                                  │
│  Status:   idle                  │
│  Tick:     0                     │
│  Summary:  {}                    │
│  Last Error: (hidden when empty) │
│                                  │
│  Execution State ▾               │
│  ┌──────────────────────────┐    │
│  │ { "n1/output": 12, … }  │    │
│  └──────────────────────────┘    │
│                                  │
│  Selected Component State ▾      │
│  ┌──────────────────────────┐    │
│  │ Select a component…      │    │
│  └──────────────────────────┘    │
│                                  │
│  Timeline ▾                      │
│  ┌──────────────────────────┐    │
│  │ [0] simulationStarted   │    │
│  │ [1] stepExecuted n1     │    │
│  └──────────────────────────┘    │
└──────────────────────────────────┘
```

New QML additions (all in `customize_example/qml/Main.qml`):

| Addition | Purpose |
|----------|---------|
| `property int inspectorTabIndex: 0` | Keeps tab state in the window scope |
| `prettyJson(value)` | JSON.stringify with indent=2, null-safe |
| `timelineText(entries)` | Formats timeline list to readable multiline |
| `selectedExecutionStateText()` | Shows per-node state for selected component |
| `TabBar` + `StackLayout` | Hosts Properties and Execution tabs side by side |
| Execution tab UI | Buttons, status grid, three TextArea viewers |

---

## 8. Step-by-Step Integration (how to replicate in another QML file)

1. **Expose the sandbox in `main.cpp`** (already done):
   ```cpp
   engine.rootContext()->setContextProperty("customizeExecutionSandbox", &executionSandbox);
   ```

2. **Alias it in the root QML**:
   ```qml
   property var executionSandbox: startupExecutionSandbox
   Component.onCompleted: { if (executionSandbox) executionSandbox.graph = graph; }
   ```

3. **Add helper functions** (`prettyJson`, `timelineText`, `selectedExecutionStateText`) to the root item.

4. **Wrap your inspector panel** in a `ColumnLayout` with `TabBar` + `StackLayout`:
   - Tab 0 = `PropertyPanel`
   - Tab 1 = `ScrollView` → `ColumnLayout` with execution controls

5. **Connect buttons**:
   - Start: `executionSandbox.start()`, check return value for error
   - Step: guard `status !== "completed" && status !== "error"`, auto-start if `idle`
   - Run: same guard, auto-start if `idle`, call `executionSandbox.run()`
   - Reset: always enabled

6. **Bind status fields** directly to Q_PROPERTYs — they emit NOTIFY signals so QML bindings stay live.

---

## 9. Manual QA Checklist

| ID | Action | Expected Result |
|----|--------|----------------|
| E01 | Launch app, switch to Execution tab | Shows "idle" status, tick=0, all TextAreas empty |
| E02 | Click **Start** with empty graph | `lastError` shown, status stays `idle` |
| E03 | Add Start→Process→Stop nodes, click **Start** | Status → `paused`, tick=0, statusLabel shows "Sandbox initialized" |
| E04 | Click **Step** once | Tick advances by 1, Timeline shows `[1] stepExecuted` |
| E05 | Click **Step** until completed | Status → `completed`, Timeline shows `simulationCompleted` |
| E06 | Select a component in canvas | "Selected Component State" updates to show that node's output values |
| E07 | Click **Reset** after completion | Status → `idle`, tick=0, all state cleared |
| E08 | Click **Run** from `idle` state | Auto-starts and runs to completion in one click |
| E09 | Execution State TextArea | Shows live `{ "n1/output": …, "n2/output": … }` after run |
| E10 | switch back to Properties tab | PropertyPanel still works; component properties editable |

---

## 10. Source Files Reference

| File | What changed / why |
|------|--------------------|
| `customize_example/qml/Main.qml` | Added `inspectorTabIndex`, 3 helper functions, replaced bare `PropertyPanel` with TabBar+StackLayout containing Execution panel |
| `customize_example/main.cpp` | No change — sandbox already created, wired, and exposed |
| `component_map_editor/services/GraphExecutionSandbox.h` | No change — public API is stable |
