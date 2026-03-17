# Phase 1 - Extension Contract Design Execution

## Goal of this phase
Create stable interfaces so business logic can plug in externally without modifying `component_map_editor` core source.

## Method to do

### 1) Define extension contracts in code
Contract headers are introduced under:
- `component_map_editor/extensions/contracts/INodeTypeProvider.h`
- `component_map_editor/extensions/contracts/IConnectionPolicyProvider.h`
- `component_map_editor/extensions/contracts/IPropertySchemaProvider.h`
- `component_map_editor/extensions/contracts/IValidationProvider.h`
- `component_map_editor/extensions/contracts/IActionProvider.h`

These interfaces define the plugin boundary for node types, connection rules, schema-driven UI data, validation, and command-driven actions.

### 2) Define API version and compatibility rules
Version and compatibility primitives are introduced in:
- `component_map_editor/extensions/contracts/ExtensionApiVersion.h`
- `component_map_editor/extensions/contracts/ExtensionApiVersion.cpp`

Manifest contract and validity checks are introduced in:
- `component_map_editor/extensions/contracts/ExtensionManifest.h`
- `component_map_editor/extensions/contracts/ExtensionManifest.cpp`

Compatibility evaluation rules:
- Core API must be inside extension declared range `[minCoreApi, maxCoreApi]`.
- Major-version overflow is rejected as incompatible.
- Invalid semantic versions are rejected.

### 3) Define registration, ownership, and runtime guardrails
Registry implementation is introduced in:
- `component_map_editor/extensions/contracts/ExtensionContractRegistry.h`
- `component_map_editor/extensions/contracts/ExtensionContractRegistry.cpp`

Registry responsibilities:
- Validate manifest before registration.
- Validate core/extension API compatibility before registration.
- Reject duplicate provider IDs.
- Store provider pointers as non-owning references.

Ownership/threading contract for plugin calls:
- Providers are non-owned by registry and must outlive registry usage.
- Graph mutations must not happen directly in providers.
- Action providers return command requests for core command gateway.
- Calls are intended for GUI-thread orchestration unless explicit worker-safe immutable snapshots are used.

### 4) Review and documentation workflow
- Keep architectural boundary definitions in:
  - `manual-check/refactor_phase1_boundary_and_contract_definition.md`
- Keep invariant coverage in:
  - `manual-check/refactor_phase1_invariant_test_matrix.md`
- Use this execution file as implementation and review trace.

## Condition to pass

### A) Contract review approved by architecture and QA
- [ ] Interface set reviewed and approved.
- [ ] Registry compatibility behavior reviewed and approved.
- [ ] Ownership/threading constraints reviewed and approved.

### B) No business-specific fields remain in core interfaces
- [ ] Contract interfaces contain only domain-neutral identifiers and metadata maps.
- [ ] Any business rules are provided through external providers.

### C) Version compatibility matrix is documented
- [ ] Compatibility policy is documented with semantic version rules.
- [ ] Tested matrix includes at least:
  - [ ] core == min == max (exact)
  - [ ] core inside range
  - [ ] core below min
  - [ ] core above max minor/patch
  - [ ] core above max major

## Execution record
- Reviewer:
- QA approver:
- Date:
- Branch:
- Commit:
- Notes:
