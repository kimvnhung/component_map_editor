# Customize Workflow Guideline: Base Components -> Newton Sqrt -> Composite -> Quartic Solver

This guide gives a practical build order for creating reusable math workflows in the customize example.

## 1) Create Base Components

Build these as individual component types first, then test each one in isolation.

### Step 1.1: Arithmetic components

Create component types:
- `math/add`
- `math/subtract`
- `math/multiply`
- `math/divide`

Recommended common properties for all arithmetic nodes:
- `inputAKey` (default `a`)
- `inputBKey` (default `b`)
- `outputKey` (default `result`)
- `errorKey` (default `error`)

Behavior rules:
- Read numeric values from execution context via `inputAKey` and `inputBKey`.
- Write result to `outputKey`.
- For `math/divide`, fail when denominator is zero and write an error message to `errorKey`.

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

### Step 1.3: Validate base components

Add tests before composing workflows:
- Add/Subtract/Multiply/Divide happy paths.
- Divide-by-zero failure path.
- IfElse with both true and false conditions.
- Loop exits by convergence and by max iteration.

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

## 3) Export Newton Workflow as a Composite Component

Goal: expose the graph as one reusable node, for example `workflow/sqrt_newton_composite`.

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

### Step 3.4: Composite acceptance checks

Validate:
- Composite output matches raw graph output on the same inputs.
- Recursion guard works if composite is accidentally nested into itself.
- Trace/timeline still exposes useful execution diagnostics.

## 4) Reuse Components to Build a Quartic Solver Workflow

Recommended approach: use a staged Ferrari-style workflow and keep each stage as a sub-workflow.

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

## 6) Practical Tips

- Keep context keys consistent across all components.
- Prefer small, composable sub-workflows over one large graph.
- Add one integration test per stage before chaining to the next stage.
- Keep error messages explicit so the error-handler path is useful in UI and logs.
