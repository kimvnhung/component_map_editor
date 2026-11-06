# Implementation Plan: Core Runtime Component Discovery

**Branch**: `001-component-discovery` | **Date**: 2026-11-04 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from [spec.md](spec.md)

## Summary

Move component declaration ownership from the UI into core runtime. Extend the existing
runtime handler registry with a versioned component catalog and shared descriptor contract.
Built-in extensions and network discovery publish through the same validation and catalog
path, while built-in execution uses a local handler and network execution uses a trusted
remote worker. Expose consistent catalog snapshots, structured discovery events, remote
execution outcomes, and diagnostics through the existing protobuf/gRPC query/event surfaces
so the UI can render and validate components without a separate registry or credentials.

## Technical Context

**Language/Version**: C++ with Qt, matching the existing project toolchain

**Primary Dependencies**: Existing Qt runtime, protobuf/gRPC generated interfaces,
`RuntimeHandlerRegistry`, `RuntimeGrpcServer`, current graph validation and event/history
services

**Storage**: Existing runtime persistence/history storage for discovery events and
optional graph metadata snapshots; no new persistence technology

**Testing**: Existing C++ test target and contract/integration test conventions under
`tests/`; protobuf/gRPC service contract tests

**Target Platform**: Existing desktop/runtime deployment targets supported by the project,
with network-capable core runtime

**Project Type**: C++ runtime service plus desktop graph editor integration

**Performance Goals**: Catalog snapshot for up to 500 descriptors within 1 second;
discovery failure diagnostics visible within 5 seconds; repeated refresh remains idempotent

**Constraints**: Runtime is authoritative; validation precedes execution; network
components are deny-by-default until trusted; network code is never loaded into the runtime
process; runtime controls remote worker credentials and execution; saved graph references
cannot silently change version; one failed discovery source must not invalidate other sources;
remote execution MUST expose request correlation, timeout, cancellation, retry/idempotency,
and structured error semantics.

**Remote Execution**: Network components run as remote workers. Core runtime resolves the
trusted worker capability, sends validated input/configuration, receives output/event or
structured error, and routes the result. The UI never invokes the worker directly and
never receives credentials.

**Open Implementation Detail**: The concrete remote transport is not fixed by the public
component contract; select the existing project-compatible transport during implementation
without changing the versioned descriptor semantics.

**Implementation Clarification**: No unresolved functional clarification remains. The
remote-worker model is recorded in the feature specification and design contracts.

**Scale/Scope**: Up to 500 catalog descriptors per runtime for v1; two source classes
(built-in and network); one versioned shared descriptor contract; no marketplace or
automatic binary installation

**Remote Execution**: Network components run as remote workers. Core runtime resolves the
trusted worker capability, sends validated input/configuration, receives output/event or
structured error, and routes the result. The UI never invokes the worker directly and
never receives credentials.

**Open Implementation Detail**: The concrete remote transport is not fixed by the public
component contract; select the existing project-compatible transport during implementation
without changing the versioned descriptor semantics.

**Implementation Clarification**: No unresolved functional clarification remains. The
remote-worker model is recorded in the feature specification and design contracts.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- **Graph-First Domain Model**: PASS. Component descriptors and graph references model
  workers, ports, connections, and execution metadata without moving graph ownership into UI.
- **Explicit Component and Connection Contracts**: PASS. The shared versioned contract
  explicitly covers properties, output properties, ports, and execution behavior; catalog
  and graph validation reject incompatible data.
- **Deterministic Execution Semantics**: PASS. Identity/version resolution is explicit,
  refresh is idempotent, conflict handling is deterministic, and unavailable components
  cannot execute.
- **Testable Graph Integrity**: PASS. Contract, registry, service, discovery, and graph
  integration scenarios are defined in [quickstart.md](quickstart.md).
- **Simplicity, Observability, and Compatibility**: PASS. The design extends existing
  registry and gRPC/history seams, emits structured diagnostics/events, and preserves
  graph references through explicit versioning.
- **Technical/domain constraints**: PASS. Stable ids, serializable descriptor semantics,
  pre-execution validation, versioned public contracts, and entity-specific diagnostics are
  included in the design.
- **Quality gates**: PASS pending implementation. Tasks must include unit and integration
  tests and reviewers must check compatibility, determinism, diagnostics, and persistence.

## Project Structure

### Documentation (this feature)

```text
specs/001-component-discovery/
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
│   ├── component-catalog.md
│   └── component-contract.md
└── tasks.md                 # Created by /speckit-tasks
```

### Source Code (repository root)

```text
component_map_runtime/
├── include/component_map_runtime/
│   ├── RuntimeHandlerRegistry.h       # Extend shared registration/lookup seam
│   ├── ComponentCatalog.h              # New descriptor catalog API
│   ├── ComponentContract.h             # New schema/contract value types
│   ├── ComponentDiscovery.h            # New source lifecycle/discovery API
│   ├── RemoteWorkerExecutor.h          # Execute trusted network workers
│   └── RuntimeGrpcServer.h             # Expose catalog/event operations
└── src/
    ├── RuntimeHandlerRegistry.cpp     # Shared built-in/network registration
    ├── ComponentCatalog.cpp
    ├── ComponentContract.cpp
    ├── ComponentDiscovery.cpp
    ├── RemoteWorkerExecutor.cpp       # Remote request/result lifecycle
    └── RuntimeGrpcServer.cpp           # Query/event mapping

proto/
├── component.proto                     # Versioned descriptor and catalog messages
├── execution.proto                     # Remote execution request/result messages
└── runtime.proto                       # Query/event RPC additions as appropriate

component_map_editor/
├── services/RuntimeClient.*            # Consume catalog snapshot/events
└── extensions/                         # Remove UI-owned declaration dependency

tests/
├── tst_ComponentContract.cpp
├── tst_ComponentCatalog.cpp
├── tst_ComponentDiscovery.cpp
├── tst_RemoteWorkerExecution.cpp
├── tst_RuntimeComponentCatalog.cpp
└── tst_ComponentVersionResolution.cpp
```

**Structure Decision**: Keep domain and lifecycle ownership in `component_map_runtime`,
transport contracts in `proto`, and only client-facing consumption in the editor. Extend
existing registry, runtime server, persistence, graph validation, and test targets rather
than introducing a parallel subsystem.

## Phase 0: Research

Research resolved the technology and integration unknowns in [research.md](research.md):
existing registry/service seams, shared versioned contract requirements, snapshot plus
lifecycle event delivery, network trust boundaries, identity/version preservation, remote
worker request/result semantics, and current C++/Qt/protobuf/gRPC test conventions. The
public contract remains transport-neutral; implementation uses an adapter around the
project's existing network-capable runtime transport without exposing credentials to UI.

The remote execution implementation is owned by User Story 2 because it is required only
for network-discovered components. User Story 1 defines the descriptor/catalog fields and
compatibility messages but does not execute network workers.

A single compatibility test task covers `component.proto`, `execution.proto`, and existing
field-number compatibility.

## Phase 1: Design

Design artifacts are complete:

- [data-model.md](data-model.md) defines descriptors, sources, schemas, graph references,
  lifecycle events, relationships, and invariants.
- [contracts/component-contract.md](contracts/component-contract.md) defines the common
  built-in/network descriptor contract and validation rules.
- [contracts/component-catalog.md](contracts/component-catalog.md) defines catalog query,
  descriptor response, lifecycle events, and diagnostics.
- [quickstart.md](quickstart.md) defines end-to-end validation scenarios.

## Post-Design Constitution Re-check

All gates remain **PASS**. The Phase 1 design uses one authoritative runtime path for both
component sources, keeps graph and connection validation before execution, preserves
stable versioned references, and provides structured observability. No constitution
violation or complexity exception is introduced.

## Complexity Tracking

No constitution violations require an exception. No additional complexity entry is needed.
