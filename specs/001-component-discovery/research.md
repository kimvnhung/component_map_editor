# Research: Core Runtime Component Discovery

## Decision: Extend the existing runtime registry into a versioned catalog

- **Decision**: Keep the existing `RuntimeHandlerRegistry` as the built-in execution
  authority, and introduce a catalog/descriptor layer around it. Built-in registration
  and network discovery both publish into the same catalog, while network entries resolve
  to a remote-worker execution binding rather than a local handler factory.
- **Rationale**: The project already has built-in extension registration, handler factories,
  component definitions, graph validation, gRPC query services, and protobuf contracts.
  Extending those seams reduces migration risk and keeps execution and metadata aligned.
- **Alternatives considered**: A UI-owned registry was rejected because it duplicates
  runtime knowledge. A second independent execution registry was rejected because it can
  drift from the catalog and violate the authoritative-runtime requirement. Loading network
  code as a local plugin was rejected because it expands the trust boundary and conflicts
  with the clarified remote-worker requirement.

## Decision: Define the shared descriptor as a versioned wire contract

- **Decision**: Add a versioned component descriptor contract containing stable identity,
  source, lifecycle status, display metadata, property schemas, output properties, ports,
  and execution metadata. The execution section includes remote-worker endpoint/capability
  reference, input/output protocol, timeout, cancellation, retry/idempotency, and error
  semantics. Credentials and authorization material remain runtime-managed and are not UI
  metadata.
- **Rationale**: Both built-in and network components must expose the same shape to the UI,
  while graph references must remain stable across refreshes and remote execution must be
  safe and observable.
- **Alternatives considered**: Reusing only the current `ComponentType` message was rejected
  because it lacks property schemas, output schemas, source/status, trust state, and
  execution semantics. Free-form key/value metadata was rejected because it cannot provide
  reliable validation or generated UI controls.

## Decision: Use snapshot catalog queries plus lifecycle events

- **Decision**: Expose a consistent catalog snapshot query and structured discovery events
  through the existing query/event service style. Queries support identity/source/status
  filtering; events cover registered, refreshed, unavailable, rejected, and removed states.
  The event/history contract includes stable event identity, timestamp, source/component
  identity, version, resulting status, and structured diagnostics so reconnecting clients
  can rebuild state safely.
- **Rationale**: The existing runtime already exposes component status, event history, logs,
  and rejections. This keeps UI integration consistent and allows a fresh client to rebuild
  state from a snapshot while active clients consume changes.
- **Alternatives considered**: Event-only delivery was rejected because reconnecting clients
  need a complete state. Polling-only delivery was rejected because it delays lifecycle
  visibility and increases unnecessary requests.

## Decision: Network discovery is deny-by-default and isolated per source

- **Decision**: A network descriptor is registered only after schema, identity, version,
  trust, capability, and remote-worker reachability validation. Each source has independent
  timeout, retry, health, and diagnostic state; one failing source cannot remove valid
  entries from other sources. Runtime invokes remote workers and never loads their code
  into the runtime process.
- **Rationale**: Network-provided execution metadata and endpoints are an untrusted boundary.
  Explicit rejection, credential isolation, and source isolation preserve runtime safety and
  availability.
- **Alternatives considered**: Trusting all discovered descriptors was rejected because it
  could make unverified behavior executable. Failing the whole catalog on one source error
  was rejected because it harms built-in and independent network components. Returning
  credentials to the UI was rejected because execution authorization belongs to runtime.

## Decision: Preserve graph references by identity and version

- **Decision**: Graph component references resolve against stable component identity plus
  selected version. A refreshed or unavailable descriptor does not silently rewrite a saved
  reference; validation reports the exact compatibility or availability problem. A remote
  worker failure marks execution unavailable or returns a structured execution failure while
  preserving the graph reference and metadata snapshot.
- **Rationale**: Existing graph and deployment validation already treats component type and
  version as meaningful values. Explicit resolution protects reproducibility.
- **Alternatives considered**: Always resolving to latest was rejected because it changes
  execution semantics of existing graphs without user approval.

## Decision: Use current project technology and test conventions

- **Decision**: Implement in the existing C++/Qt runtime and protobuf/gRPC transport,
  with unit and integration tests in the current `tests/` target. Keep UI changes limited
  to consuming runtime catalog data after the runtime contract is available. The remote
  worker protocol is represented by a transport-neutral execution binding in the shared
  contract; its concrete network transport is an implementation detail for planning.
- **Rationale**: This matches the repository's build, service, model, and test structure
  while keeping the public component contract stable.
- **Alternatives considered**: Introducing a new transport or serialization framework into
  the catalog contract was rejected as unnecessary scope and would create a second public
  contract. Local plugin execution was rejected by the clarified feature boundary.
