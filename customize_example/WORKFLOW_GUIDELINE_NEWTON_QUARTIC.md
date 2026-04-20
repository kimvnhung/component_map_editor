# Customize Workflow Guideline: Base Components -> Newton Sqrt -> Composite -> Quartic Solver

This guide gives a practical build order for creating reusable math workflows in the customize example.

## 0) Actual Source Map in `customize_example`

Use this section as the real implementation map for each workflow step.

Core registration and startup:
- `customize_example/main.cpp`
  - Registers factory for `customize.workflow`.
  - Loads manifests from `EXAMPLE_EXTENSION_MANIFEST_DIR`.
  - Builds `TypeRegistry` and `PropertySchemaRegistry` for QML.
- `customize_example/CMakeLists.txt`
  - Copies manifest and rule files from source into build output.

Manifest and rules:
- `customize_example/src/extensions/manifests/manifest.customize.workflow.json`
  - Declares extension ID/capabilities.
- `customize_example/src/extensions/manifests/rules.customize.workflow.json`
  - Rule-backed policy/validation file loaded at runtime.

Provider pack wiring:
- `customize_example/src/extensions/providers/customizeextensionpack.h`
- `customize_example/src/extensions/providers/customizeextensionpack.cpp`
  - Registers component types, policy, schema, validation, execution semantics.

Component types and defaults:
- `customize_example/src/extensions/providers/customizecomponenttypeprovider.h`
- `customize_example/src/extensions/providers/customizecomponenttypeprovider.cpp`
  - Add new type IDs.
  - Add descriptor entries (title/category/color/size).
  - Add default property map for each type.

Execution behavior:
- `customize_example/src/extensions/providers/customizeexecutionsanticsprovider.h`
- `customize_example/src/extensions/providers/customizeexecutionsanticsprovider.cpp`
  - Add type IDs to `supportedComponentTypes()`.
  - Implement runtime execution behavior in `executeComponent()`.
  - Reuse helpers `resolveNumber`, `resolveText`, `makeTracePayload`.

Inspector property schema:
- `customize_example/src/extensions/providers/customizepropertyschemaprovider.cpp`
  - Add schema target per component type:
    - `component/<type-id>`.
  - Keep keys consistent with execution provider property names.

Graph validation:
- `customize_example/src/extensions/providers/customizevalidationprovider.cpp`
- `customize_example/src/extensions/providers/validators/workflowvalidationprovider.cpp`
  - Add structure/domain rules if new components require constraints.

Connection policy:
- `customize_example/src/extensions/providers/customizeconnectionpolicyprovider.cpp`
  - Update rule logic when introducing branching/loop components.

Tests:
- `tests/tst_CustomizeExampleMathWorkflows.cpp`
  - Primary place for math/workflow execution tests.

Current status in source:
- Already implemented: `math/add`, `math/subtract`, `math/multiply`, `math/divide`, `control/loop`, `control/ifelse`, `system/error_handler`.
- Follow-up types to implement later for workflow progression: `workflow/sqrt_newton_composite`, quartic-specific workflow nodes.

## 1) Create Base Components

Build these as individual component types first, then test each one in isolation.

### Step 1.1: Arithmetic components

Create component types:
- `math/add`
- `math/subtract`
- `math/multiply`
- `math/divide`

Recommended common properties for all arithmetic nodes:
- `inputARef` (pick an exact incoming token field)
- `inputBRef` (pick an exact incoming token field)
- `outputKey` (default `result`)
- `errorKey` (default `error`)
- `a`, `b` fallback values when no ref is selected

Behavior rules:
- Read numeric values from exact token refs via `inputARef` and `inputBRef`.
- If a ref is not selected or cannot be resolved, use the fallback snapshot value for that operand.
- Write result to `outputKey`.
- For `math/divide`, fail when denominator is zero and write an error message to `errorKey`.

Actual source steps (already in place, use as template):
1. Type IDs + defaults:
  - `customize_example/src/extensions/providers/customizecomponenttypeprovider.h`
  - `customize_example/src/extensions/providers/customizecomponenttypeprovider.cpp`
2. Runtime execution:
  - `customize_example/src/extensions/providers/customizeexecutionsanticsprovider.cpp`
  - Follow `executeBinary(...)` pattern.
3. Inspector fields:
  - `customize_example/src/extensions/providers/customizepropertyschemaprovider.cpp`
  - Targets: `component/math/add`, `component/math/subtract`, `component/math/multiply`, `component/math/divide`.

### Step 1.2: Control-flow components

Create component types:
- `control/loop`
- `control/ifelse`

Recommended properties:
- `control/loop`:
  - `iterKey` (default `iter`)
  - `maxIterKey` (default `maxIter`)
  - `continueKey` (default `continueLoop`)
- `control/ifelse`:
  - `conditionKey` (default `condition`)
  - `trueRouteKey` (default `routeTrue`)
  - `falseRouteKey` (default `routeFalse`)

Behavior rules:
- `control/loop` controls iteration budget and loop continuation flags in context.
- `control/ifelse` writes a routing flag that downstream nodes use to decide branch execution.

Actual source steps (to implement next):
1. Declare type IDs:
  - Add `TypeLoop = "control/loop"`, `TypeIfElse = "control/ifelse"` in `customizecomponenttypeprovider.h`.
2. Register descriptors/defaults:
  - Add component templates/defaults in `customizecomponenttypeprovider.cpp`.
3. Add execution semantics:
  - Add to `supportedComponentTypes()` in `customizeexecutionsanticsprovider.cpp`.
  - Implement branches in `executeComponent()`.
4. Add inspector schemas:
  - Add `component/control/loop` and `component/control/ifelse` in `customizepropertyschemaprovider.cpp`.
5. Update graph connection constraints if needed:
  - Extend `customizePolicyStrategy(...)` in `customizeconnectionpolicyprovider.cpp`.

### Step 1.3: Validate base components

Add tests before composing workflows:
- Add/Subtract/Multiply/Divide happy paths.
- Divide-by-zero failure path.
- IfElse with both true and false conditions.
- Loop exits by convergence and by max iteration.

Actual test steps:
1. Extend `tests/tst_CustomizeExampleMathWorkflows.cpp` with:
  - `control_ifelse_routesTrueAndFalse()`.
  - `control_loop_stopsByMaxIterAndCondition()`.
2. Keep each test as single-node or minimal graph using existing helpers (`runSingleNode`, `GraphExecutionSandbox`).

## 2) Build Newton Square Root Graph Workflow

Target: compute `sqrt(S)` with Newton iteration and convergence threshold.

Newton update:
- `xNext = (x + S / x) / 2`
- Stop when `abs(xNext - x) < epsilon` or `iter >= maxIter`

### Step 2.1: Define workflow interface

Input context keys:
- `S`
- `epsilon`
- `maxIter`
- optional `initialGuess`

Output context keys:
- `sqrtS`
- `sqrt.iterations`
- `sqrt.lastDelta`
- optional `error`

Actual source alignment:
- Existing `workflow/sqrt_newton` uses keys from component properties:
  - `sKey` (default `S`), `epsilonKey` (default `epsilon`), `outputKey` (default `sqrt`), `initialGuessKey`, `maxIterations`, `errorKey`.
- Implementation location:
  - `customize_example/src/extensions/providers/customizeexecutionsanticsprovider.cpp` in `TypeSqrtNewton` branch.

### Step 2.2: Build node groups

Group A: Guard/Init
- Validate `S >= 0`, `epsilon > 0`, `maxIter > 0`.
- Initialize `x` (`initialGuess` or fallback).
- Initialize `iter = 0`.

Group B: Iteration body
- `t1 = S / x` using `math/divide`
- `t2 = x + t1` using `math/add`
- `xNext = t2 / 2` using `math/divide`
- `delta = abs(xNext - x)`
- `iter = iter + 1`

Group C: Loop decision
- If `delta < epsilon` then stop.
- Else if `iter >= maxIter` then stop with bounded result.
- Else set `x = xNext` and continue.

Group D: Finalize
- Write `sqrtS = xNext` (or `x` depending on final step ordering).
- Write iteration metadata.

### Step 2.3: Test Newton workflow

Minimum tests:
- `S=25` returns approximately `5`.
- `S=2` returns approximately `1.41421` within tolerance.
- `S<0` produces error.
- Very small epsilon still exits by `maxIter` if needed.

Actual tests already present:
- `sqrtNewton_loopStatePersistenceAndConvergence()`.
- `sqrtNegative_reportsError()`.

File:
- `tests/tst_CustomizeExampleMathWorkflows.cpp`.

## 3) Export Newton Workflow as a Composite Component

Goal: expose the graph as one reusable node, for example `workflow/sqrt_newton_composite`.

Note on current implementation:
- The project currently has a direct execution component `workflow/sqrt_newton` (not a graph-composite wrapper).
- If you still want composite behavior, implement it as a new type while keeping `workflow/sqrt_newton` for backward compatibility.

### Step 3.1: Freeze the internal graph contract

Document stable interface keys:
- Inputs: `S`, `epsilon`, `maxIter`, `initialGuess`
- Outputs: `sqrtS`, `sqrt.iterations`, `sqrt.lastDelta`, `error`

### Step 3.2: Define composite mapping

Map external composite ports to internal graph context keys:
- Input port `S` -> internal `S`
- Input port `epsilon` -> internal `epsilon`
- Input port `maxIter` -> internal `maxIter`
- Output port `sqrtS` <- internal `sqrtS`
- Output port `error` <- internal `error`

### Step 3.3: Register composite as a new component type

In extension metadata/providers:
- Add a new component type entry for the composite node.
- Provide a default property schema describing supported keys.
- Register execution semantics so the runtime invokes the composite graph.

Actual source files to change:
1. `customize_example/src/extensions/providers/customizecomponenttypeprovider.h/.cpp`
  - Add `TypeSqrtNewtonComposite`.
2. `customize_example/src/extensions/providers/customizepropertyschemaprovider.cpp`
  - Add `component/workflow/sqrt_newton_composite` target.
3. `customize_example/src/extensions/providers/customizeexecutionsanticsprovider.h/.cpp`
  - Add supported type and execution branch.
4. `tests/tst_CustomizeExampleMathWorkflows.cpp`
  - Add parity test: `sqrtComposite_matchesSqrtNewton()`.

### Step 3.4: Composite acceptance checks

Validate:
- Composite output matches raw graph output on the same inputs.
- Recursion guard works if composite is accidentally nested into itself.
- Trace/timeline still exposes useful execution diagnostics.

## 4) Reuse Components to Build a Quartic Solver Workflow

Recommended approach: use a staged Ferrari-style workflow and keep each stage as a sub-workflow.

Note on current implementation:
- Current source implements `workflow/quadratic`, not quartic.
- Quartic flow should be introduced as new workflow type(s), starting from a normalization stage and then staged expansion.

## Step 4.1: Normalize polynomial

Input polynomial:
- `A*x^4 + B*x^3 + C*x^2 + D*x + E = 0`

If `A == 0`, route to error (not quartic).

Create normalized coefficients with arithmetic nodes:
- `a = B/A`
- `b = C/A`
- `c = D/A`
- `d = E/A`

## Step 4.2: Build depressed quartic stage

Transform `x = y - a/4` and compute depressed coefficients (`p`, `q`, `r`) using arithmetic nodes.

Keep this stage as its own reusable sub-workflow:
- `workflow/quartic_depress`

## Step 4.3: Solve resolvent stage

Add a resolvent-solving sub-workflow:
- `workflow/quartic_resolvent`

Implementation options:
- Option A: exact algebraic cubic branch.
- Option B: numeric loop with `control/loop` + `control/ifelse` + arithmetic nodes.

Prefer Option B first for easier debugging and incremental delivery.

Actual source recommendation for `customize_example`:
1. Start with one orchestrator type `workflow/quartic` in:
  - `customizecomponenttypeprovider.h/.cpp`.
2. Implement staged computation in execution provider branch:
  - `customizeexecutionsanticsprovider.cpp`.
3. Write intermediate values to explicit keys (`quartic.norm.*`, `quartic.depress.*`, `quartic.resolvent.*`) for debuggability.
4. After stabilization, split to explicit sub-workflow types (`workflow/quartic_depress`, `workflow/quartic_resolvent`) if needed.

## Step 4.4: Reuse Newton sqrt composite

Call `workflow/sqrt_newton_composite` where square roots are needed in Ferrari steps.

Branching rules:
- If any required radicand is negative in real-mode solver, route to no-real-root branch.
- Otherwise continue and compute factors.

## Step 4.5: Solve final two quadratics

After factorization, solve two quadratic equations.

For each quadratic:
- Compute discriminant with arithmetic nodes.
- Reuse `workflow/sqrt_newton_composite` for `sqrt(discriminant)`.
- Compute two roots with add/subtract/divide nodes.

Aggregate up to four real roots into output keys:
- `root1`, `root2`, `root3`, `root4`
- plus `rootCount` and `status`.

## Step 4.6: Quartic workflow validation matrix

Validate at least these cases:
- Four distinct real roots.
- Two real roots (others complex).
- Repeated roots.
- No real roots.
- Degenerate input `A == 0`.

Performance checks:
- Iteration cap respected.
- No infinite loop when convergence fails.
- Stable results across repeated runs (deterministic tolerance).

## 5) Suggested Delivery Order

1. Implement and test arithmetic nodes.
2. Implement and test loop/ifelse nodes.
3. Build Newton sqrt graph and tests.
4. Export Newton graph as composite and re-verify.
5. Build quartic stages incrementally (normalize -> depress -> resolvent -> finalize).
6. Run full solver matrix and trace validation.

## 6) Concrete Checklist for This Repository

Use this checklist as the exact "what to edit" plan in `customize_example`.

1. Add new component IDs in `customizecomponenttypeprovider.h`.
2. Add descriptor/default entries in `customizecomponenttypeprovider.cpp`.
3. Add schema targets in `customizepropertyschemaprovider.cpp`.
4. Add execution branches and helper logic in `customizeexecutionsanticsprovider.cpp`.
5. If branching/loop routing changes graph constraints, update `customizeconnectionpolicyprovider.cpp`.
6. If new structural rules are needed, extend `workflowvalidationprovider.cpp`.
7. Ensure extension capabilities remain aligned in:
  - `customize_example/src/extensions/manifests/manifest.customize.workflow.json`.
8. Add/extend tests in `tests/tst_CustomizeExampleMathWorkflows.cpp`.
9. Build and run tests.

Suggested commands from workspace root:
1. `cmake --build build/Desktop_Qt_6_9_1-Debug --target customize_example_app`
2. `cmake --build build/Desktop_Qt_6_9_1-Debug --target tests`
3. `ctest --test-dir build/Desktop_Qt_6_9_1-Debug --output-on-failure -R CustomizeExampleMathWorkflows`

## 7) Manual QA Table (Control/Loop + Control/IfElse)

Run these checks in order and fill in `Actual` + `Pass/Fail`.

| ID | Area | Setup / Input | Expected Result | Actual | Pass/Fail |
|---|---|---|---|---|---|
| M01 | Build | `cmake --build build/Desktop_Qt_6_9_1-Debug --target customize_example_app` | Build succeeds with no compile/link error. |  |  |
| M02 | Type registry | Open app and component palette. | `control/loop` and `control/ifelse` are visible in palette. |  |  |
| M03 | Loop defaults | Add one `control/loop` node and inspect properties. | Keys exist: `iterKey`, `maxIterKey`, `continueKey`, `conditionKey`; fallback values exist: `iter`, `maxIter`, `condition`. |  |  |
| M04 | IfElse defaults | Add one `control/ifelse` node and inspect properties. | Keys exist: `conditionKey`, `trueRouteKey`, `falseRouteKey`; fallback `condition` exists. |  |  |
| M05 | IfElse true path | Execute one `control/ifelse` node with input `{cond:true}` and mapping `conditionKey=cond`, `trueRouteKey=routeTrue`, `falseRouteKey=routeFalse`. | Output has `routeTrue=true`, `routeFalse=false`; node execution succeeds. |  |  |
| M06 | IfElse false path | Execute one `control/ifelse` node with input `{cond:false}` using same key mapping. | Output has `routeTrue=false`, `routeFalse=true`; node execution succeeds. |  |  |
| M07 | Loop continue path | Execute one `control/loop` node with input `{iter:0,maxIter:3,condition:true}`. | Output has `iter=1`, `continueLoop=true`; node execution succeeds. |  |  |
| M08 | Loop stop by budget | Execute one `control/loop` node with input `{iter:2,maxIter:3,condition:true}`. | Output has `iter=3`, `continueLoop=false`; node execution succeeds. |  |  |
| M09 | Loop stop by condition | Execute one `control/loop` node with input `{iter:0,maxIter:5,condition:false}`. | Output has `continueLoop=false`; node execution succeeds. |  |  |
| M10 | Loop invalid maxIter | Execute one `control/loop` node with input `{iter:0,maxIter:0,condition:true}`. | Execution fails with invalid loop input error. |  |  |
| M11 | IfElse invalid condition | Execute one `control/ifelse` node with non-boolean condition input (for example `{cond:"abc"}`) and `conditionKey=cond`. | Execution fails with invalid ifelse condition error. |  |  |
| M12 | Regression test | `ctest --test-dir build/Desktop_Qt_6_9_1-Debug --output-on-failure -R CustomizeExampleMathWorkflows` | Test suite passes. |  |  |

Notes:
- Source of loop/ifelse execution behavior: `customize_example/src/extensions/providers/customizeexecutionsanticsprovider.cpp`.
- Source of type defaults: `customize_example/src/extensions/providers/customizecomponenttypeprovider.cpp`.
- Source of inspector schema: `customize_example/src/extensions/providers/customizepropertyschemaprovider.cpp`.
- Source of automated checks: `tests/tst_CustomizeExampleMathWorkflows.cpp`.

## 8) Prime Check Workflow

See full graph, component list, connection table, and QA matrix in:

- [`customize_example/PRIME_CHECK_WORKFLOW.md`](PRIME_CHECK_WORKFLOW.md)

Summary of required new component types (not yet implemented):
- `math/mod` — remainder operator, reuses `executeBinary` pattern with `std::fmod`.
- `math/less_than` — comparison, outputs bool.
- `math/less_or_equal` — comparison, outputs bool.
- `math/equal` — equality test, outputs bool.
- `logic/and` — boolean AND of two context keys, outputs bool.
- `context/set` — writes a constant value into a named context key.

## 9) Practical Tips

- Keep context keys consistent across all components.
- Prefer small, composable sub-workflows over one large graph.
- Add one integration test per stage before chaining to the next stage.
- Keep error messages explicit so the error-handler path is useful in UI and logs.
