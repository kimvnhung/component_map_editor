# Runtime Component Discovery

The runtime owns the authoritative component catalog. The editor consumes catalog snapshots and lifecycle events and does not register component metadata locally.

## Catalog contract

Each descriptor carries a stable identity, semantic version, source, status, trust state, display metadata, property and output schemas, ports, and execution metadata. Graph nodes persist the selected identity and version. A metadata snapshot remains attached to the node when the source is unavailable.

Catalog resolution is exact. A newer descriptor does not replace the requested version automatically. The runtime returns a compatibility diagnostic so the graph remains reproducible.

## Network trust policy

Network descriptors use `remote_worker` execution and are deny-by-default. The runtime accepts them only after contract validation and explicit trust authorization. Worker capability references are metadata, not credentials. Credentials remain runtime-managed and never appear in catalog responses or editor models.

Discovery failures are isolated per source. A timeout, invalid descriptor, or rejected trust decision produces a structured lifecycle diagnostic without removing valid built-in or independent network entries.

## Migration from UI declarations

Existing editor code may continue to render legacy component fields for compatibility, but new component creation should apply the runtime descriptor. Identity, version, and metadata snapshot must be persisted with the graph. Runtime validation resolves the exact descriptor before checking properties, defaults, ports, availability, and trust.

Lifecycle events are exposed through the runtime history path and can be appended to the editor timeline. The event payload includes event identity, source, component identity, version, status, and diagnostics.
