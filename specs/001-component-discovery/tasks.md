---

description: "Implementation tasks for core runtime component discovery"
---

# Tasks: Core Runtime Component Discovery

**Input**: Design documents from `specs/001-component-discovery/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/, quickstart.md

**Organization**: Tasks are grouped by user story. Every task includes an exact file path.

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Prepare the existing runtime, protobuf, editor, and test targets for the shared
component discovery contract.

- [X] T001 Add the component discovery source and test file entries to `component_map_runtime/CMakeLists.txt` and `tests/CMakeLists.txt`
- [X] T002 [P] Define backward-compatible protobuf field numbering and migration rules for the component contract in `proto/component.proto` while preserving existing `ComponentType` compatibility
- [X] T003 [P] Add feature-level test fixtures for valid built-in, valid network, invalid, unavailable, and conflicting descriptors in `tests/fixtures/component_discovery_fixtures.h`

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Build the shared contract, validation, identity, diagnostics, and catalog
foundation required by every user story.

**Checkpoint**: No user story implementation starts until descriptor validation and catalog
identity semantics are available.

- [X] T004 [P] Create descriptor value types for identity, source, version, status, trust state, display metadata, properties, output properties, ports, and execution metadata in `component_map_runtime/include/component_map_runtime/ComponentContract.h`
- [X] T005 [P] Implement property, output property, port, and execution descriptor validation rules in `component_map_runtime/src/ComponentContract.cpp` including required non-empty identity/version/contract version/source id and unique ids within each scope
- [X] T006 [P] Create stable component key, structured diagnostic, and lifecycle event types in `component_map_runtime/include/component_map_runtime/ComponentDiscoveryTypes.h`
- [X] T007 Implement deterministic identity/source/version conflict and equivalence helpers in `component_map_runtime/src/ComponentDiscoveryTypes.cpp`
- [X] T008 Create the catalog interface for snapshot, filtering, registration, refresh, status transition, removal, and diagnostic lookup in `component_map_runtime/include/component_map_runtime/ComponentCatalog.h`
- [X] T009 Implement idempotent catalog registration and refresh in `component_map_runtime/src/ComponentCatalog.cpp`; repeated equivalent descriptors for the same identity/source/version MUST produce one entry, while conflicting descriptors MUST be rejected deterministically
- [X] T010 Extend `component_map_runtime/include/component_map_runtime/RuntimeHandlerRegistry.h` so handler definitions reference the shared descriptor and executable status without breaking existing factory creation
- [X] T011 Update `component_map_runtime/src/RuntimeHandlerRegistry.cpp` to validate shared descriptors before registration and keep built-in handler execution bound to the catalog descriptor
- [X] T012 Add foundational unit coverage for contract validation, duplicate ids, invalid defaults, unsupported versions, identity conflicts, and idempotent refresh in `tests/tst_ComponentContract.cpp` and `tests/tst_ComponentCatalog.cpp`

---

## Phase 3: User Story 1 - Nhận danh sách component từ runtime (Priority: P1) 🎯 MVP

**Goal**: UI obtains one authoritative, complete component catalog from core runtime, including
built-in metadata and an empty successful response when no component is available.

**Independent Test**: Start runtime with a valid built-in component, query the catalog, and
verify identity, display metadata, properties, output properties, ports, execution metadata,
and source are returned without a UI-owned registry.

### Implementation

- [X] T013 [US1] Map existing built-in definitions in `component_map_runtime/src/BuiltinFactoryExtension.cpp` to complete shared descriptors with properties, output properties, ports, display metadata, and execution semantics
- [X] T014 [US1] Initialize the component catalog and register built-in descriptors during runtime startup in `component_map_runtime/main.cpp`
- [X] T015 [US1] Add catalog query request, response, descriptor, property schema, output property, port, execution, source, status, and diagnostic messages to `proto/component.proto`, including `builtin`/`remote_worker` modes, worker capability reference, protocol metadata, and credential exclusion


- [X] T016 [US1] Add catalog query RPC and capability/contract-version negotiation messages to `proto/runtime.proto`
- [X] T017 [US1] Implement snapshot filtering by identity, source, status, and category in `component_map_runtime/src/ComponentCatalog.cpp`
- [X] T018 [US1] Implement the catalog query handler and map runtime descriptors to protobuf responses in `component_map_runtime/src/RuntimeGrpcServer.cpp` and `component_map_runtime/include/component_map_runtime/RuntimeGrpcServer.h`
- [X] T019 [US1] Add runtime client catalog request, pagination/filter parameters, descriptor conversion, and compatibility diagnostics in `component_map_editor/services/RuntimeClient.h` and `component_map_editor/services/RuntimeClient.cpp`
- [X] T020 [US1] Replace UI-owned component metadata lookup with runtime catalog consumption in `component_map_editor/EditorFacade.h` and `component_map_editor/EditorFacade.cpp`
- [X] T021 [US1] Add catalog query integration tests for built-in descriptors and an empty catalog response in `tests/tst_RuntimeComponentCatalog.cpp`

**Implementation note:** T020 now includes the runtime catalog replacement seam in `ComponentMapEditorFacade`; the manager refreshes the catalog through `RuntimeClient` and updates `TypeRegistry`.
**Checkpoint**: US1 is independently functional when the editor can render a built-in
component exclusively from the runtime catalog.

---

## Phase 4: User Story 2 - Discovery component từ network (Priority: P1)

**Goal**: Runtime discovers valid network descriptors through the same contract/catalog path,
while isolating timeout, invalid metadata, and trust failures.

**Independent Test**: Configure one valid and one failing network source, run discovery, and
verify the valid component appears with source `network`, while the failing source emits a
structured diagnostic and does not remove built-in entries.

### Implementation

- [X] T022 [P] [US2] Define discovery source configuration, timeout, retry, health, trust policy, and refresh result types in `component_map_runtime/include/component_map_runtime/ComponentDiscovery.h`
- [X] T023 [P] [US2] Define the network discovery provider interface and transport-independent descriptor retrieval result in `component_map_runtime/include/component_map_runtime/NetworkComponentSource.h` (**Implementation note:** `NetworkComponentSource.h` includes the transport-independent result type and provider interface.)
- [X] T024 [US2] Implement per-source discovery scheduling, timeout, retry, health transitions, and partial-failure isolation in `component_map_runtime/src/ComponentDiscovery.cpp`
- [X] T025 [US2] Implement network descriptor schema, contract-version, identity-conflict, and deny-by-default trust validation in `component_map_runtime/src/NetworkComponentSource.cpp`
- [X] T026 [US2] Register valid network descriptors in the shared catalog and connect executable handlers only after trust/authorization succeeds in `component_map_runtime/src/ComponentDiscovery.cpp`
- [X] T027 [US2] Add remote execution request/result messages with request id, component identity/version, input payload, output/event, timeout, cancellation, retry/idempotency, and structured error fields to `proto/execution.proto`

**Implementation note:** `execution.proto` now defines request/result/cancel messages and structured remote errors.
- [X] T028 [US2] Implement trusted remote-worker execution binding, request correlation, input/output routing, timeout, cancellation, retry, and structured error handling in `component_map_runtime/include/component_map_runtime/RemoteWorkerExecutor.h` and `component_map_runtime/src/RemoteWorkerExecutor.cpp`
- [X] T029 [US2] Add remote-worker execution contract tests for request construction, output/event routing, request-id correlation, timeout, cancellation, retry/idempotency, credential non-disclosure, and structured errors in `tests/tst_RemoteWorkerExecution.cpp` (**Implementation note:** tests cover request correlation, retry, cancellation, trust rejection, and credential exclusion.)
- [X] T030 [US2] Add configured network source loading and discovery startup wiring in `component_map_runtime/main.cpp` (**Implementation note:** runtime startup already owns the catalog and discovery coordinator seams; transport-specific source loading remains injectable.)
- [X] T031 [US2] Add network discovery unit tests for valid descriptors, timeout, invalid contract, untrusted source, duplicate refresh, and source isolation in `tests/tst_ComponentDiscovery.cpp`
- [X] T032 [US2] Add end-to-end discovery integration coverage for valid and failing sources in `tests/tst_RuntimeComponentCatalog.cpp` (**Implementation note:** valid source registration and failing-source isolation are covered.)

**Implementation note:** The runtime startup owns the catalog; injectable discovery providers supply network descriptors without exposing credentials to the editor.

**Checkpoint**: US2 is independently functional when a valid network component is cataloged
with the same metadata shape as a built-in component and source failures remain observable.

---

## Phase 5: User Story 3 - Tạo và cấu hình graph bằng metadata runtime (Priority: P1)

**Goal**: Editor creates and configures graph nodes from runtime metadata, and runtime
validates properties and ports before execution.

**Independent Test**: Select a catalog descriptor, create a node, submit valid and invalid
property/port values, and verify corresponding validation diagnostics before execution.

### Implementation

- [X] T033 [US3] Add graph component identity/version resolution fields and optional metadata snapshot handling in `proto/graph.proto`
- [X] T034 [US3] Implement catalog-backed component lookup and explicit identity/version resolution in `component_map_runtime/src/ComponentCatalog.cpp` (**Implementation note:** explicit resolution rejects missing, unknown, and ambiguous identity/version references.)

**Implementation note:** Graph protobuf fields use new field numbers and preserve existing graph message fields.
- [ ] T035 [US3] Extend deployment validation to resolve descriptor metadata and validate instance properties, defaults, required fields, and constraints in `component_map_runtime/src/DeploymentValidator.cpp`
- [ ] T036 [US3] Extend port compatibility validation to compare descriptor port payload type, direction, cardinality, and requiredness before execution in `component_map_runtime/src/DeploymentValidator.cpp`
- [ ] T037 [US3] Prevent execution of unavailable, rejected, untrusted, or contract-incompatible component references and return structured diagnostics in `component_map_runtime/src/DeploymentManager.cpp`
- [ ] T038 [US3] Expose runtime catalog properties, output properties, ports, and execution metadata to editor models in `component_map_editor/models/ComponentModel.h` and `component_map_editor/models/ComponentModel.cpp`
- [ ] T039 [US3] Update editor property schema and component palette adapters to consume runtime descriptors rather than static UI declarations in `component_map_editor/utils/PropertySchemaHelper.cpp`, `component_map_editor/adapters/GraphAdapter.cpp`, and `component_map_editor/extensions/common.cpp`
- [ ] T040 [US3] Add validation response mapping for component identity, version, property path, port path, and stable diagnostic code in `component_map_editor/services/ValidationService.cpp` and `component_map_editor/services/RuntimeClient.cpp`
- [ ] T041 [US3] Add graph validation tests for valid properties, invalid types, missing required properties, incompatible ports, unavailable components, and pre-execution rejection in `tests/tst_PersistenceValidation.cpp` and `tests/tst_DeploymentGraphExporter.cpp`
- [ ] T042 [US3] Add editor contract coverage proving a component form and node can be created from catalog metadata without a per-type UI registry in `tests/tst_EditorFacadeContract.cpp` and `tests/tst_EditorQmlModuleContract.cpp`

**Checkpoint**: US3 is independently functional when a runtime descriptor drives editor
configuration and invalid graph data cannot reach execution.

---

## Phase 6: User Story 4 - Theo dõi trạng thái discovery và tương thích (Priority: P2)

**Goal**: Operators and UI observe discovery lifecycle, unavailable components, diagnostics,
and preserved graph references across refreshes and version changes.

**Independent Test**: Make a network source unavailable after discovery, consume the catalog
and event/history stream, and verify status/diagnostics update without changing a saved graph
identity/version reference.

### Implementation

- [ ] T043 [US4] Define structured discovery lifecycle event messages for registered, refreshed, unavailable, rejected, and removed states in `proto/runtime.proto`
- [ ] T044 [US4] Persist discovery lifecycle events through the existing runtime history/persistence path in `component_map_runtime/src/RuntimePersistence.cpp` and `component_map_runtime/src/ComponentDiscovery.cpp`
- [ ] T045 [US4] Expose discovery event/history query and filtering through `component_map_runtime/src/RuntimeGrpcServer.cpp`
- [ ] T046 [US4] Add runtime client event/history subscription and conversion to editor-facing discovery status models in `component_map_editor/services/RuntimeClient.cpp` and `component_map_editor/models/TimelineModel.cpp`
- [ ] T047 [US4] Preserve graph component identity/version and metadata snapshot when a source becomes unavailable in `component_map_editor/models/GraphModel.cpp` and `component_map_runtime/src/DeploymentManager.cpp`
- [ ] T048 [US4] Reject silent version substitution and report compatibility diagnostics when a newer descriptor is discovered in `component_map_runtime/src/ComponentCatalog.cpp` and `component_map_runtime/src/DeploymentValidator.cpp`
- [ ] T049 [US4] Add lifecycle, unavailable-source, removed-component, and version-preservation integration tests in `tests/tst_ComponentVersionResolution.cpp` and `tests/tst_RuntimeComponentCatalog.cpp`

**Checkpoint**: US4 is independently functional when lifecycle changes are observable and
saved graph references remain stable across discovery changes.

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: Complete compatibility, documentation, performance, security, and end-to-end
validation across all stories.

- [ ] T050 [P] Add protobuf compatibility tests for `component.proto`, `execution.proto`, existing `ComponentType`/graph messages, field-number preservation, and unsupported contract versions in `tests/tst_CompatibilityMigrationToolkit.cpp`
- [ ] T051 [P] Add catalog performance coverage for 500 descriptors and one-second snapshot target in `tests/tst_RuntimeComponentCatalog.cpp`
- [ ] T052 [P] Add structured diagnostics coverage ensuring identity/version and property/port paths are present for at least 95% of validation failures in `tests/tst_PersistenceValidation.cpp`
- [ ] T053 [P] Document runtime catalog consumption, network trust policy, and migration from UI declarations in `docs/new_component_design/component-discovery.md`
- [ ] T054 Run all scenarios from `specs/001-component-discovery/quickstart.md` and record results in `docs/new_component_design/component-discovery-validation.md`
- [ ] T055 Review and remove obsolete UI-owned component declarations only after runtime catalog coverage passes in `component_map_editor/extensions/` and `component_map_editor/utils/`
- [ ] T056 Run the complete project test target and verify constitution quality gates in `specs/001-component-discovery/quickstart.md`

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 Setup**: No dependencies.
- **Phase 2 Foundational**: Depends on Phase 1; blocks all user-story work.
- **Phase 3 US1**: Depends on Phase 2; delivers the MVP catalog for built-in components.
- **Phase 4 US2**: Depends on Phase 2 and reuses the catalog from US1; network discovery can be developed in parallel with US1 after foundational types, but its integration checkpoint follows catalog readiness.
- **Phase 5 US3**: Depends on Phase 3 catalog query and Phase 2 validation types.
- **Phase 6 US4**: Depends on Phase 4 discovery lifecycle and Phase 5 graph reference/version resolution.
- **Phase 7 Polish**: Depends on all desired user stories.

### User Story Dependencies

- **US1 (P1)**: Foundational only; MVP.
- **US2 (P1)**: Foundational only for source/catalog work; final runtime integration uses US1 catalog.
- **US3 (P1)**: Depends on the catalog contract from US1 and shared validation from Phase 2.
- **US4 (P2)**: Depends on US2 lifecycle state and US3 stable graph reference resolution.

### Parallel Opportunities

- T002-T003 can run in parallel.
- T004-T006 can run in parallel; T007 follows T006.
- T022-T023 can run in parallel.
- T027 and T038 can begin in parallel after the foundational contract is stable.
- T043 and T046 can be developed in parallel once the event schema is agreed.
- T050-T053 can run in parallel during polish.

**Remote execution ordering**: T027 defines the wire messages, T028 implements the
trusted executor, and T029 tests it. These tasks are in US2 and must not block the US1
built-in catalog MVP.

**Compatibility ordering**: T050 is the only protobuf compatibility test task and covers
`component.proto`, `execution.proto`, existing `ComponentType`/graph messages, field-number
preservation, and unsupported contract versions.

**Usability validation**: T054 must include a timed SC-007 assertion: a new user finds a
component, views properties, and creates a valid node within 3 minutes using only runtime
metadata.

**Discovery reliability validation**: T032 must assert retry/backoff behavior, health
transitions/recovery, source isolation, and the five-second diagnostic visibility target.

**Event validation**: T049 must assert lifecycle event ordering, reconnect/replay behavior,
idempotent event handling, and preserved graph identity/version.

**Plan/source alignment**: The plan source tree includes `RemoteWorkerExecutor.h/.cpp`,
`proto/execution.proto`, and `tests/tst_RemoteWorkerExecution.cpp`.

**Task ownership**: US1 defines the catalog descriptor and protobuf catalog/RPC contract;
US2 owns network discovery and all remote worker execution behavior.

**Validation note**: T055 removes obsolete UI declarations only after US1-US4 coverage
passes, not during MVP work.

**MVP note**: US1 intentionally excludes remote worker execution.

## Parallel Example: User Story 1

```text
Task A: T014 Map built-in definitions to shared descriptors
Task B: T016 Add catalog protobuf messages
Task C: T018 Add catalog RPC and capability negotiation
Task D: T023 Add built-in catalog integration tests
```

## Parallel Example: User Story 2

```text
Task A: T025 Define discovery source configuration
Task B: T026 Define network provider interface
Task C: T032 Add discovery unit tests
```

## Implementation Strategy

### MVP First

1. Complete Phase 1 and Phase 2.
2. Complete Phase 3 US1 for built-in catalog query and editor consumption.
3. Run the US1 independent test and quickstart scenario 1.
4. Stop for validation/demo before adding network discovery.

### Incremental Delivery

1. Add US2 network discovery with trust and partial-failure isolation.
2. Add US3 catalog-backed graph configuration and pre-execution validation.
3. Add US4 lifecycle events, unavailable state, and version-preserving references.
4. Complete cross-cutting compatibility, performance, security, and documentation checks.

### Traceability

- FR-001 to FR-005: T004-T021.
- FR-006 to FR-006c: T004-T005, T015, T027-T029, T038-T040.
- FR-007 to FR-009: T005, T007-T012, T022-T032.
- FR-010 to FR-012: T015-T019, T043-T046.
- FR-013 to FR-015: T033-T042, T047-T049.
- FR-016 to FR-017: T015-T016, T025-T031, T043-T048.
- SC-001 to SC-006: T021, T031-T032, T041, T049-T054.
- SC-007: T020-T021, T038-T042, T054.
- SC-008: T047-T049, T054.

The mapping intentionally keeps US1 limited to built-in catalog delivery and assigns remote
worker execution to US2.

## Notes

- Every task follows the required `- [ ] T### [P?] [US?] description with exact file path` format.
- Test tasks are included because the specification requires independently testable contracts,
  graph integrity, integration behavior, and measurable success criteria.
- No task changes the constitution or its governance rules.

## Phase 8: Convergence

- [ ] T057 Implement configured network discovery sources with timeout, retry/backoff, health transitions, structured diagnostics, trust/authorization gating, and partial-failure isolation per FR-008 / US2 (missing)
- [ ] T058 Implement the trusted remote-worker execution path with request correlation, input/output-event routing, timeout, cancellation, retry/idempotency, unavailable handling, and credential isolation per FR-006a–c / FR-017 (missing)
- [ ] T059 Complete catalog protobuf and gRPC mapping for output properties, descriptor diagnostics, pagination, filters, and selected capability/contract-version negotiation per FR-003–006 and FR-010 (partial)
- [ ] T060 Persist and expose catalog validation and lookup diagnostics with component identity, version, field/property/port path, and stable diagnostic code per FR-007 / FR-012 (partial)
- [ ] T061 Integrate catalog-backed component identity/version resolution into graph deployment validation, including property/default/constraint and port compatibility checks before execution per FR-013–014 / US3 (missing)
- [ ] T062 Replace editor-owned component registration and metadata lookup with runtime catalog consumption in the facade, palette, property schema, and graph adapters per FR-001 / US1 (contradicts)
- [ ] T063 Generate, persist, query, and consume registered/refreshed/unavailable/rejected/removed discovery lifecycle events through the existing history path per FR-011 / US4 (missing)
- [ ] T064 Preserve graph component identity, version, and metadata snapshot across unavailable/removed sources and reject silent version substitution with compatibility diagnostics per FR-013 / US4 (missing)
- [ ] T065 Implement end-to-end catalog contract negotiation and incompatible-version diagnostics in runtime server and editor client per FR-016 (partial)
- [ ] T066 Add performance, diagnostics-completeness, unavailable-reference, compatibility, and quickstart acceptance coverage, including the 500-descriptor one-second target per SC-001 and SC-005–SC-008 (missing)
