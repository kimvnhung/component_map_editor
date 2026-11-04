# Data Model: Core Runtime Component Discovery

## ComponentDescriptor

A catalog entry representing one component version available to the runtime.

| Field | Description | Rules |
|---|---|---|
| `identity` | Stable component identifier | Required, non-empty, unique within source/version resolution |
| `version` | Component contract/component version | Required, valid semantic version; graph references it explicitly |
| `source` | `builtin` or `network` | Required; determines provenance, not response shape |
| `source_id` | Built-in extension or network source identity | Required for diagnostics and conflict resolution |
| `status` | `registered`, `available`, `unavailable`, `rejected`, or `removed` | Only available/trusted entries are executable |
| `trust_state` | Trust/authorization result | Network execution is deny-by-default |
| `display` | Title, category, icon, shape, default size, description | UI-facing metadata |
| `properties` | Input property schemas | Each property has id, type, required, default, constraints, display metadata |
| `output_properties` | Produced value schemas | Each output has id, type, constraints, display metadata |
| `ports` | Input/output port contracts | Port ids unique per component; payload type/cardinality are explicit |
| `execution` | Execute capabilities and semantics | Includes local handler or remote worker binding, inputs, outputs, timeout, cancellation, retry, and error behavior |
| `contract_version` | Descriptor schema version | Unsupported versions are rejected or marked unavailable |
| `diagnostics` | Structured registration/discovery issues | Must identify component/version when possible |

## DiscoverySource

A configured source that produces component descriptors.

| Field | Description | Rules |
|---|---|---|
| `source_id` | Stable source identity | Required and unique |
| `kind` | Built-in or network | Network sources require trust policy |
| `endpoint` | Network location when applicable | Not present for built-in source; worker endpoint is separate execution metadata |
| `health` | Healthy, degraded, timeout, unavailable | Independent source state |
| `timeout` | Maximum discovery wait | Bounded and observable |
| `retry_policy` | Retry count/backoff | Must not block catalog availability indefinitely |
| `last_refresh` | Last attempt/result timestamp | Used for status and diagnostics |
| `trust_policy` | Authorization decision inputs | Deny-by-default for untrusted network data |

## PropertySchema

A property that can be configured or displayed by the UI.

- `id`: stable property key, required and unique within its component scope.
- `type`: supported scalar/structured data type.
- `required`: whether a value is needed before execution.
- `default`: optional valid default value.
- `constraints`: range, enum, pattern, length, or structure constraints.
- `display`: label, description, grouping, ordering, and editor hints.

## OutputPropertySchema

A named result produced by execution. Its type and constraints must be compatible
with the component's output ports and downstream connections.

## PortContract

A graph connection endpoint.

- `id`: stable port key, unique per component.
- `direction`: input or output.
- `payload_type`: declared event/data type.
- `cardinality`: single, optional, or repeated as supported.
- `required`: whether the graph must connect/configure the port.
- `display`: label, description, and ordering metadata.

## ExecutionDescriptor

The declarative execution contract exposed to UI and used by runtime validation.

- `mode`: `builtin` local handler or `remote_worker` network service.
- `capabilities`: supported operation/event modes.
- `input_requirements`: accepted event and property requirements.
- `output_guarantees`: possible outputs and their schemas.
- `worker_endpoint` or `capability_reference`: remote worker execution target when mode is `remote_worker`; it is not a credential.
- `protocol`: input, output/event, and structured error semantics.
- `timeout`: supported/default execution limit.
- `cancellation`: whether cancellation is supported and its outcome.
- `retry`: retryability and idempotency information.
- `error_behavior`: failure categories and propagation semantics.

Runtime-managed credentials and authorization material are not part of the UI-facing
execution metadata.

## GraphComponentReference

The persisted reference from a graph node to a component descriptor.

- `identity`: stable component identity.
- `version`: selected version or resolution constraint.
- `metadata_snapshot`: optional persisted display/schema snapshot for unavailable sources.
- `properties`: instance values validated against the resolved property schemas.

A graph reference MUST NOT be silently redirected to a different version.

## DiscoveryEvent

A structured history/event record.

- `event_id`, `timestamp`, `source_id`, `component_identity`, `version`.
- `kind`: registered, refreshed, unavailable, rejected, removed.
- `status`, `diagnostics`, and optional descriptor summary.

## RemoteExecutionRequest

A runtime-to-worker request for a network component.

- `request_id`: stable correlation id for retries and diagnostics.
- `component_identity`, `version`, and `worker_capability_reference`.
- `graph_instance_id` and input event/data payload.
- `configuration`: validated instance properties, excluding runtime credentials.
- `deadline` and cancellation token/reference.
- `attempt`: retry attempt number.

## RemoteExecutionResult

A worker-to-runtime result.

- `request_id` and component identity/version.
- `outputs[]`: output properties/events matching declared schemas.
- `status`: succeeded, failed, cancelled, or timed_out.
- `error`: structured error code/message/retryability when not succeeded.
- `worker_metadata`: non-sensitive execution metadata for diagnostics.

## Relationships and invariants

- One `DiscoverySource` publishes zero or more `ComponentDescriptor` versions.
- A catalog contains only descriptors that pass contract validation; rejected entries
  remain observable through diagnostics/events but are not executable.
- `ComponentDescriptor.properties` and `output_properties` are referenced by the
  graph instance property values and port compatibility checks.
- A graph node references exactly one identity/version resolution.
- Duplicate identity/source/version descriptors are idempotently merged/refreshed;
  conflicting descriptors are rejected with a deterministic diagnostic.
- Built-in execution resolves to a local registered handler; network execution resolves
  only to a trusted remote worker binding and never loads worker code into the runtime
  process.
- Remote execution requests and results correlate by `request_id`; retry behavior MUST
  respect the descriptor's idempotency declaration.
- Graph validation completes before execution and requires an available, trusted,
  compatible descriptor.
