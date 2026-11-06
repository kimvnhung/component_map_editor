# Component Catalog Contract

This contract defines the runtime-to-UI interface for component discovery. The exact
protobuf field casing follows the repository's existing protobuf conventions; the
semantics below are normative.

## Catalog query

`ListComponentCatalog(request) -> response`

### Request

- `contract_version`: supported descriptor contract version(s).
- `identity_filter`: optional stable identity filter.
- `source_filter`: optional `BUILTIN` or `NETWORK` filter.
- `status_filter`: optional lifecycle status filter.
- `include_rejected`: optional diagnostic view; rejected entries are never executable.
- `page_size` / `page_token`: optional bounded pagination.

### Response

- `catalog_version`: monotonically increasing snapshot version.
- `components[]`: descriptors with the complete shared metadata shape.
- `next_page_token`: pagination continuation.
- `diagnostics[]`: request/compatibility diagnostics.

The response MUST represent one consistent snapshot. Built-in and network descriptors
MUST use the same fields and semantics.

## Component descriptor

Each `components[]` item MUST include:

- `identity`, `version`, `source`, `source_id`, `status`, `trust_state`.
- `display`: title, category, icon, shape, default dimensions, description.
- `properties[]` and `output_properties[]`.
- `input_ports[]` and `output_ports[]`.
- `execution`: mode, capabilities, input requirements, output guarantees, timeout,
  cancellation, retry, protocol, and error behavior.
- For `remote_worker`, `worker_capability_reference` and protocol metadata are required.
- `contract_version`.
- `diagnostics[]` when status is not available.

Credentials and authorization material MUST NOT appear in the catalog response.

## Discovery lifecycle events

`ComponentDiscoveryEvent` MUST be consumable by the existing event/history mechanism.

Event kinds:

- `REGISTERED`: descriptor passed validation and entered the catalog.
- `REFRESHED`: same identity/source/version metadata was refreshed idempotently.
- `UNAVAILABLE`: source or component cannot currently be used.
- `REJECTED`: descriptor failed contract, trust, or conflict validation.
- `REMOVED`: descriptor is no longer published by its source.

Every event MUST include event id, timestamp, source id, component identity, version,
kind, resulting status, and structured diagnostics where applicable. Remote execution
failures MUST be correlated to the request id and exposed as structured execution
history/diagnostics without exposing credentials.

## Remote worker execution boundary

The UI never invokes a network worker directly. Core runtime resolves the trusted worker
binding, validates the graph and instance properties, sends input, and receives output,
event, or structured error. The execution descriptor determines timeout, cancellation,
retry, and idempotency behavior.

## Error contract

Errors MUST identify:

- stable component identity and version when available;
- source id for discovery failures;
- request id for remote execution failures;
- field/property/port path for schema failures;
- a stable error code;
- human-readable message;
- whether the caller can retry or must change input.

A catalog query may succeed with partial results when one source fails; the response
MUST include diagnostics for the failed source.
