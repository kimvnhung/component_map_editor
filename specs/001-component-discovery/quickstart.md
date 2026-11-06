# Quickstart: Validate Core Runtime Component Discovery

This guide validates the feature at the contract and runtime boundary. It intentionally
avoids prescribing implementation bodies.

## Prerequisites

- A build configured with the repository's existing C++/Qt and protobuf/gRPC dependencies.
- The runtime executable and test target built from the repository root.
- At least one built-in component registered by the runtime.
- A test network discovery source that can return one valid and one invalid descriptor.
- A test remote worker that accepts the declared execution contract and can return a valid
  output, structured error, timeout, and cancellation response.

## Validation scenarios

### 1. Built-in catalog

1. Start the runtime with its built-in extension enabled.
2. Call the catalog query described in [component-catalog.md](contracts/component-catalog.md).
3. Verify the response contains the built-in identity, source, version, properties,
   output properties, ports, and execution descriptor with mode `builtin`.
4. Verify the response shape matches the [component contract](contracts/component-contract.md).

Expected result: the UI can render the component without a separate component registry.

### 2. Network discovery and partial failure

1. Configure one healthy network source and one source that times out or returns an
   invalid contract.
2. Trigger discovery or wait for the configured refresh.
3. Query the catalog and consume discovery history/events.
4. Verify the valid network component is available with mode `remote_worker`, while the
   failed source has a structured diagnostic and does not remove built-in entries.
5. Verify the catalog contains no credentials or authorization material.

Expected result: source failures are isolated and observable.

### 3. Idempotent refresh

1. Publish the same network descriptor ten times.
2. Query the catalog after each refresh.
3. Verify exactly one entry exists for the identity/source/version tuple.
4. Verify refresh events do not create duplicate executable worker bindings.

Expected result: repeated discovery is safe and deterministic.

### 4. Remote worker execution

1. Discover and trust a network component with a remote worker capability reference.
2. Create a graph node with valid properties and send an input event through runtime.
3. Verify runtime sends a request with component identity/version, validated properties,
   request id, deadline, and input payload to the worker.
4. Return an output/event from the worker and verify runtime routes it through the graph.
5. Repeat with worker error, timeout, cancellation, and retryable failure.
6. Verify each result is correlated by request id and follows the descriptor's retry and
   idempotency semantics.

Expected result: core runtime, not UI, controls remote worker execution and exposes
structured outcomes without exposing credentials.

### 5. Graph validation and version preservation

1. Create a graph node referencing a catalog identity and explicit version.
2. Configure valid property values and compatible ports; validate the graph.
3. Repeat with an invalid property type, incompatible port, and unavailable component.
4. Publish a newer descriptor version and validate the original graph again.

Expected result: valid graphs pass; invalid graphs fail before execution with identity,
version, property/port path, and stable diagnostic code. The original graph is not
silently redirected to the newer version.

## Suggested automated coverage

- Contract unit tests for required fields, uniqueness, defaults, type constraints,
  unsupported versions, execution mode, and remote worker binding.
- Registry/catalog unit tests for built-in registration, network registration,
  conflict detection, idempotency, and trust gating.
- Service contract tests for catalog snapshots, filtering, pagination, and diagnostics.
- Remote execution integration tests for request construction, output/event routing,
  request correlation, timeout, cancellation, retry, and credential non-disclosure.
- Integration tests for discovery events/history, graph validation, and version-preserving
  execution resolution.

The acceptance requirements and entity invariants are defined in
[data-model.md](data-model.md); the external interface semantics are defined in
[contracts/](contracts/).
