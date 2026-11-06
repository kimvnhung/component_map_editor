<!--
Sync Impact Report
- Version change: 1.0.0 -> 2.0.0
- Modified principles:
  - I. Graph-First Domain Model -> I. Contract-First Module Boundaries
  - II. Explicit Component and Connection Contracts -> II. Unified Component Catalog
  - III. Deterministic Execution Semantics -> III. Immutable Deployment and Independent Runtime
  - IV. Testable Graph Integrity -> IV. Reliable Runtime Semantics and Recovery
  - V. Simplicity, Observability, and Compatibility -> V. Observable, Portable, and Compatible Infrastructure
- Added sections: Scope and Product Boundaries; Required Contracts and Data Model; Development Workflow and Quality Gates
- Removed sections: None
- Follow-up TODOs: None. Ratification date inferred from the existing constitution adoption commit on 2026-11-04.
-->

# Component Map Editor Constitution

## Core Principles

### I. Contract-First Module Boundaries
The system MUST be organized around the module boundaries defined by the specification:
`component-catalog`, `editor-authoring`, `graph-validation`, `deployment-control`,
`runtime-execution`, `runtime-state`, and `runtime-observability`. Dependencies MUST
point only in the specified direction. The catalog MUST NOT depend on the editor, and
runtime modules MUST NOT depend on QML, editor UI, or the editor process. Shared behavior
MUST be expressed through stable domain or protocol contracts rather than reverse
module dependencies. This preserves independent testing and permits the runtime to
operate when the editor is disconnected.

### II. Unified Component Catalog
Built-in and remote components MUST use one registration contract and one published
catalog. Every registration MUST identify a stable `componentTypeId`, `source`,
`providerId`, type compatibility metadata, descriptor, property schema, port metadata,
validation rules, execution mode, and the applicable handler or capability. A catalog
MUST start empty as a valid state, publish only atomically consistent snapshots, and
reject incomplete, conflicting, or metadata-changing re-registration without altering
the published entry. Availability MUST remain separate from identity and metadata;
an offline remote worker MUST NOT silently remove its catalog entry. Components MUST
not appear as deployable until the required metadata and execution capability are
validated. This prevents editor and runtime from maintaining incompatible definitions.

### III. Immutable Deployment and Independent Runtime
The editor MUST author only the current graph document and MUST NOT manage graph-version
history, graph-version rollback, or in-editor test, simulation, preview, or execution.
Deployment MUST validate the graph and create an immutable, self-contained snapshot;
mutable editor models MUST NOT cross the runtime boundary. Stage MUST resolve every
execution target and record that resolution in the snapshot. Activation MUST occur only
when the runtime is `Stopped`, and the normal replacement sequence MUST be
`stop -> validate/stage -> activate -> start`. A failed stage or activation MUST leave
the previously active graph unchanged. The runtime MUST be independently operable and
MUST expose stable control, data, and query contracts. This makes deployment behavior
safe, reproducible, and independent of UI availability.

### IV. Reliable Runtime Semantics and Recovery
Runtime event delivery MUST use at-least-once semantics with a stable `eventId` used
for deduplication within the runtime scope. Idempotency records MUST distinguish
`accepted`, `processing`, `completed`, and `failed`; completed events MUST return their
stored result without re-execution, while failed events MAY be retried only under an
explicit policy. Events rejected because the runtime is stopped, a mailbox is full,
or a worker is unavailable MUST NOT be reported as accepted. Local and remote handlers
MUST implement the same `ComponentExecutionContract`, including identity, correlation,
payload, state update, emitted events, status, and structured diagnostics. Runtime
recovery MUST validate state schema compatibility before entering `Running`; an
incompatible or missing required state MUST produce a structured recovery error and
MUST NOT be reported as successful recovery. Reset MUST be explicit and MUST NOT be
reported as rollback. These rules bound duplicate effects, make failures recoverable,
and prevent false success states.

### V. Observable, Portable, and Compatible Infrastructure
All public operations MUST return machine-readable codes and human-readable diagnostics
with stable request, correlation, event, or entity identifiers where applicable.
Status, component state, event history, logs, failures, and rejections MUST be
queryable with bounded results, stable ordering, and cursor-based pagination where
history is paginated. Runtime domain code MUST access persistence only through the
vendor-neutral `RuntimeStorage` port. The port MUST cover active deployment, component
state, idempotency records, event history, logs, failures, and rejections, and MUST
expose defined behavior for not-found, conflict, unavailable, malformed, incompatible,
and transaction-failure cases. SQLite is the initial adapter, but alternate backends
MUST be replaceable through adapters and conformance tests without changing execution
domain code. Protocol and state schema compatibility MUST be explicit and versioned;
graph documents MUST remain free of graph-version requirements. This keeps operations
diagnosable and infrastructure replaceable without changing domain semantics.

## Scope and Product Boundaries

The current product MUST support authoring, validating, importing, exporting, staging,
and deploying exactly one current graph document for an editor manager and one active
graph for each target runtime. The minimum operational flow is deploy the current graph,
start the runtime, stop it, inspect runtime and component status, inspect event history
and logs, and inspect failures and rejections. Authoring is single-user and local.
Multi-tenant operation, collaborative editing, multiple simultaneously orchestrated
runtimes, graph-version history, graph-version rollback, and direct graph testing or
simulation in the editor are outside this phase and MUST NOT be introduced implicitly.
Protocol or schema versions MAY exist for compatibility, but a graph document MUST NOT
be rejected merely because it lacks or differs in a graph version.

## Required Contracts and Data Model

A deployable graph MUST contain a non-empty `graphId`, at least one component, and
exactly one valid `startComponentId`. Components and connections MUST have stable
identifiers, valid references, compatible ports, and all required properties. Import
MUST reject malformed input atomically without damaging the current graph; export MUST
remain independent of runtime state and MUST NOT invent a graph ID or start component.

A deployment snapshot MUST include the current graph ID, component identities and
compatibility metadata, routes, execution requirements, resolved targets, and a content
hash or equivalent integrity marker. It MUST NOT require a graph version or embed a
remote endpoint that belongs in runtime configuration. `ValidateDeployment` MUST return
structured issues with at least `code`, `severity`, `path`, and `message`; errors block
stage or activation, while warnings permit them but remain visible and recorded.

Runtime events MUST include an event ID, target component, message type, payload, and
correlation metadata. Remote workers MUST be addressed by logical worker type or
capability, with endpoints resolved by runtime configuration. `ComponentExecutionContract`
MUST be the canonical contract for both local and remote execution. `RuntimeStorage`
MUST provide active deployment, component state, idempotency, history, logs, failure,
rejection, reset, health, and capability operations with defined ordering, limits,
transaction, serialization, and compatibility behavior.

## Development Workflow and Quality Gates

Every feature proposal MUST identify the affected module boundary, public contract,
data model, lifecycle state, and compatibility impact. Implementation MUST include
focused unit tests for graph mutation, catalog registration, validation, state machines,
adapters, and failure paths. Contract tests MUST cover protocol compatibility,
editor-to-runtime snapshots, catalog identity and schema parity, and local-to-remote
execution parity. Integration tests MUST cover validate, stage, activate, start, stop,
event execution, deduplication, observability, persistence, recovery, and editor
reconnection or disconnection.

Each requirement in the specification MUST have a traceable test at unit, contract, or
integration level. Negative paths MUST be tested at the boundary where they occur,
including invalid graphs, unknown or incompatible components, overload, stopped runtime,
duplicate events, worker failures, incompatible state, persistence failures, and failed
deployments. No graph simulation test in the editor substitutes for runtime deployment
and integration testing. Build, test, and lint commands MUST be established from the
actual repository before implementation plans depend on them; commands MUST NOT be
invented from this constitution.

Reviewers MUST verify dependency direction, immutable deployment, active-graph
preservation on failure, structured diagnostics, runtime independence, storage
abstraction, compatibility behavior, and test coverage before approval. Any exception
MUST document its scope, rationale, risks, owner, and review or removal date.

## Governance

This constitution is the authoritative governance document for the Component Map Editor.
It takes precedence over conflicting development practices unless an approved amendment
explicitly changes the relevant rule. The specification is the source of product scope
and contract semantics; implementation plans MUST preserve those decisions or record an
approved amendment before diverging.

Amendments MUST include a written rationale, an updated Sync Impact Report, affected
principles and contracts, migration and compatibility consequences, and corresponding
test and documentation changes. The project owner MUST review and approve an amendment
before it is committed. Every specification, implementation plan, and code review MUST
check compliance with this constitution. Unresolved violations block approval unless an
explicit exception is recorded with an owner and follow-up date.

Constitution versions use semantic versioning. A MAJOR increment removes or incompatibly
redefines a principle or product boundary. A MINOR increment adds a principle or
materially expands governance requirements. A PATCH increment clarifies wording without
changing meaning. The ratification date records the original adoption date, and the
last-amended date records the approved amendment date in ISO format. Compliance MUST be
reviewed during planning and code review, and any contract or schema change MUST state
its compatibility and migration strategy.

**Version**: 2.0.0 | **Ratified**: 2026-11-04 | **Last Amended**: 2026-11-06
