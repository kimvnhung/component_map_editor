# Component Contract

All built-in and network components MUST publish this same versioned contract before
becoming executable.

## Required sections

1. **Identity**: stable `identity`, `version`, `contract_version`, and source metadata.
2. **Display**: title, category, icon, shape, default dimensions, description.
3. **Properties**: input property schemas with type, required/default, constraints, and
   display metadata.
4. **Output properties**: produced values with type, constraints, and display metadata.
5. **Ports**: input/output port ids, payload types, cardinality, requiredness, and display.
6. **Execution**: mode (`builtin` or `remote_worker`), capability list, accepted inputs,
   output guarantees, worker endpoint/capability reference for remote workers, protocol,
   timeout, cancellation, retry/idempotency, and error propagation semantics.
7. **Trust/status**: lifecycle status, trust state, and diagnostics.

## Execution modes

- `builtin` resolves to a registered local handler in core runtime.
- `remote_worker` resolves to a trusted remote worker through the declared execution
  binding. Core runtime sends input and receives output/event or structured error.
- Network worker code MUST NOT be loaded or executed inside the core runtime process.
- Worker endpoint/capability reference is metadata, not a credential. Credentials and
  authorization material are managed privately by runtime and are not exposed to UI.

## Validation rules

- Identity, version, contract version, and source id are non-empty.
- Property ids, output property ids, and port ids are unique within their scopes.
- Defaults satisfy their property constraints.
- Port payload types and output property types are internally consistent.
- Execution metadata is complete enough to validate graph compatibility.
- `remote_worker` descriptors MUST include a worker capability reference and protocol
  semantics for input, output/event, structured error, timeout, cancellation, and retry.
- Unsupported contract versions are rejected or marked unavailable; they are not
  silently interpreted as another version.
- Network descriptors require an explicit trusted/authorized state before execution.
- Duplicate identity/source/version registration is idempotent only when the descriptor
  is equivalent; conflicting content is rejected deterministically.
- Remote execution requests MUST correlate results by request id, and retries MUST obey
  the component's idempotency declaration.
