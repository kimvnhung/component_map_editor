# Phase E: Runtime process và gRPC polling API

## Mục tiêu

Đóng gói runtime core và persistence thành process độc lập, sau đó expose API gRPC TCP tối thiểu để client kiểm chứng mô hình.

## Transport

- gRPC trên TCP plaintext;
- mặc định bind `127.0.0.1`;
- không authentication hoặc TLS trong MVP;
- remote bind không thuộc đường chạy mặc định;
- process nhận address, database path và runtime ID qua command-line options.

## Proto layout

Các contract Phase 5 nằm trong `proto/runtime/`:

- `control.proto` cho deployment, activation, start, stop và status;
- `event.proto` cho publish event;
- `query.proto` cho component status, history, logs và rejection query.

`execution.proto` tiếp tục là compatibility contract.

## API MVP

### Deployment và control

- `DeployGraph`;
- `ActivateGraph`;
- `StartRuntime`;
- `StopRuntime`;
- `GetRuntimeStatus`.

`ActivateGraph` chỉ thành công khi runtime stopped. Runtime không tự retry event và không có dead-letter queue.

### Event

- `PublishEvent`.

Response phân biệt accepted, duplicate, validation failure, handler hoặc delivery failure và mailbox-full rejection qua machine-readable error code.

### Query bằng polling

- `GetComponentStatus`;
- `ListEventHistory`;
- `ListRuntimeLogs`;
- `ListRejections`.

## Process

Build và chạy process:

```bash
cmake --build build/phase1_runtime --target component_map_runtime_process
build/phase1_runtime/component_map_runtime/component_map_runtime_process \
  --address 127.0.0.1:50051 \
  --database component_map_runtime.sqlite \
  --runtime-id component-map-runtime
```

Process không link QML hoặc editor target. Mặc định server dùng plaintext gRPC trên localhost.

## Test

Smoke test gọi generated gRPC client stub qua TCP loopback:

```bash
cmake --build build/phase1_runtime --target tst_RuntimeProcessPhase5
ctest --test-dir build/phase1_runtime \
  -R '^tst_Runtime(ProcessPhase5|PersistencePhase4|CorePhase3|DeploymentPhase2|ServicePhase1)$' \
  --output-on-failure
```

Test kiểm tra deploy graph, activate, start, publish event, query history và stop qua gRPC.

## Trạng thái triển khai

Phase 5 đã có process độc lập, generated gRPC C++ target, control service, event service, polling history và runtime log query, cùng graceful shutdown qua `QCoreApplication`.

Các phần còn lại cần mở rộng ở phase sau gồm protocol version negotiation, timeout và message-size policy chặt hơn, restart process end-to-end, và query rejection đầy đủ.

## Mở rộng API cho composite graph

Phase F giữ nguyên API root nhưng mở rộng deployment document để mang `GraphDefinition`
và dependency closure đã pin. Server phải validate/compile toàn bộ graph tree trước khi
`ActivateGraph`; không resolve graph con theo `latest` trong lúc chạy.

### Publish và query

`PublishEvent` có thể nhận target là qualified path tới exposed port, ví dụ:

```text
root/checkout/in
```

Client không được publish trực tiếp tới `root/checkout/fraud_check/internal_score` nếu
port đó không exposed. Event response và history phải giữ `root_event_id`, `causation_id`,
execution-plan hash và qualified target để truy vết xuyên boundary.

Các query mở rộng:

- `GetComponentStatus(component_path, include_descendants)`;
- `ListEventHistory(root_event_id, component_path, graph_version)`;
- `ListRuntimeLogs(component_path)`;
- `ListRejections(component_path)`.

Status composite là aggregate của descendants; status leaf vẫn giữ semantics Phase 3.
Client cũ không hiểu hierarchy vẫn dùng được graph phẳng qua root-level instance id.

### Compatibility

`execution.proto` và các field protobuf hiện tại không bị đổi số field hoặc enum value.
Graph phẳng được biểu diễn như root scope không có composite reference. Adapter từ contract
cũ sang deployment graph mới phải tạo qualified path deterministic nhưng giữ output/error
semantics cũ cho client phẳng.

### Test Phase F

- deploy dependency closure thiếu definition/version bị reject;
- activate pin đúng graph con dù registry có version mới;
- publish tới exposed input và query output/history theo path;
- publish trực tiếp tới internal port bị reject;
- query subtree status và rejection không mất graph scope;
- client cũ vẫn deploy/publish graph phẳng.

Chi tiết model và execution-plan compiler nằm trong [phase-06-nested-graphs.md](phase-06-nested-graphs.md).
Mọi thay đổi graph con khi runtime active đều phải đi qua root deployment version mới và activation
atomic khi runtime stopped.

## Exit criteria bổ sung cho Phase F

- gRPC có thể deploy root cùng dependency closure;
- boundary port được bảo vệ bởi validation;
- history/log/rejection truy vấn được theo qualified path;
- compatibility flat client vẫn hoạt động.