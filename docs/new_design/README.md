# Thiết kế và kế hoạch triển khai MVP runtime

## Mục tiêu

Xây một runtime process runnable để kiểm chứng mô hình execution độc lập với editor:

```text
Producer -> Processor -> Sink
```

Bản MVP phải cho phép build, khởi động runtime, deploy graph, publish event, quan sát kết quả, restart process và xác nhận state được phục hồi.

## Kiến trúc MVP

```text
┌──────────────────────┐
│ Test client          │
│ hoặc UI client       │
└──────────┬───────────┘
           │ gRPC TCP plaintext
           ▼
┌────────────────────────────────────┐
│ component_map_runtime              │
│                                    │
│ Runtime API                        │
│ Deployment Manager                 │
│ Event Router                       │
│ Actor Scheduler                    │
│                                    │
│ ┌──────────┬───────────┬────────┐  │
│ │ Producer │ Processor │ Sink   │  │
│ │ mailbox  │ mailbox   │ mailbox│  │
│ └──────────┴───────────┴────────┘  │
│                                    │
│ SQLite state, history và logs      │
└────────────────────────────────────┘
```

Runtime không include QML. UI hoặc test client chỉ gọi gRPC, không sở hữu actor, mailbox hoặc runtime state. Runtime tiếp tục chạy khi client đóng.

## Phạm vi MVP

### Runtime

- runtime library `component_map_runtime`;
- runtime process `component_map_runtime_process`;
- lifecycle `stopped`, `starting`, `running`, `stopping`, `failed`;
- một graph version active;
- activate graph chỉ khi runtime stopped;
- activate graph mới reset execution state;
- không chuyển event pending sang graph mới.

### Graph

- component type;
- component instance id;
- configured properties;
- input/output port;
- message type;
- direct route;
- graph version;
- validation code và field path.

### Actor và event

- mỗi component instance có actor riêng;
- bounded FIFO mailbox;
- `max_concurrency = 1`;
- actor khác nhau có thể chạy song song;
- direct routing;
- mailbox-full rejection;
- opaque state bytes có schema version;
- idempotency key và request fingerprint.

### Persistence

SQLite với WAL, một database cho mỗi runtime instance:

```text
runtime_instances
graph_versions
component_state
event_inbox
event_history
event_rejections
runtime_logs
```

Retention record cố định một ngày trong MVP. State và execution work được reset khi activate graph mới. Event history và runtime logs vẫn giữ lại sau reset.

### Failure

Mọi failure phải:

- trả machine-readable code và message;
- ghi structured log;
- ghi event history hoặc rejection record;
- không bị silently dropped.

Runtime không automatic retry, không backoff scheduler và không dead-letter queue. Client tự resubmit bằng `PublishEvent` nếu cần. Cùng idempotency key và request fingerprint sẽ nhận lại kết quả đã biết.

## API MVP

### Deployment

```text
DeployGraph
ActivateGraph
```

`ActivateGraph` trả lỗi nếu runtime đang chạy. Graph mới không dùng state của graph cũ.

### Control

```text
StartRuntime
StopRuntime
```

### Event

```text
PublishEvent
```

Response phải phân biệt tối thiểu:

```text
accepted
duplicate
validation_failure
handler_failure
delivery_failure
rejected_mailbox_full
idempotency_conflict
```

### Polling query

```text
GetRuntimeStatus
GetComponentStatus
ListEventHistory
ListRuntimeLogs
```

MVP dùng polling. Không cần streaming hoặc subscription.

## Transport

- gRPC trên TCP plaintext;
- mặc định bind `127.0.0.1`;
- không cần authentication hoặc TLS;
- không cần remote deployment;
- có timeout và message size limit cơ bản.

## Proto layout

```text
proto/runtime/
  common.proto
  deployment.proto
  control.proto
  event.proto
  status.proto
```

`execution.proto` vẫn tồn tại cho compatibility nhưng không làm runtime protocol mới.

## Worker model

Mỗi node trong graph là một component instance. Các instance cùng type dùng chung handler code nhưng có state và mailbox riêng.

```text
ComponentTypeDefinition
        │
        ├── handler factory
        ├── port definitions
        └── property schema

ComponentInstance
        │
        ├── instance id
        ├── configured properties
        ├── mailbox
        └── opaque state
```

Handler không tự tạo thread và không tự truy cập mailbox. Scheduler sở hữu lifecycle và concurrency.

## Vertical slice kiểm chứng

```text
Producer -> Processor -> Sink
```

Slice phải chứng minh:

- event đi đúng direct route;
- hai instance cùng type có state riêng;
- một actor xử lý tuần tự;
- actor khác nhau có thể chạy song song;
- mailbox đầy tạo rejection;
- handler failure có log và failure code;
- duplicate idempotency request không dispatch hai lần;
- process restart phục hồi state cơ bản.

## Kế hoạch triển khai

Chi tiết thực hiện nằm trong [00-implementation-roadmap.md](00-implementation-roadmap.md).

| Bước | Kết quả |
|---|---|
| 1 | Runtime boundary và test harness |
| 2 | Deployment contract và validation |
| 3 | Runtime core vertical slice |
| 4 | SQLite persistence và recovery |
| 5 | gRPC server, runtime process và smoke client |
| 6 | Kiểm chứng end-to-end và tài liệu chạy |

## Thành phần giữ lại

- `GraphModel` và `ComponentModel` cho editor;
- `ConnectionModel` với port và message metadata tối thiểu;
- `TypeRegistry` và property schema registry;
- extension registry và handler registry;
- `GraphExecutionSandbox` cho compatibility/preview;
- execution engine cũ cho compatibility test;
- actor/mailbox primitives để refactor vào runtime core.

## Tiêu chí hoàn thành MVP

Một người khác phải có thể:

1. build project;
2. khởi động runtime process;
3. deploy và activate graph fixture;
4. start runtime;
5. publish event bằng smoke client;
6. query status, history và logs;
7. stop và restart process;
8. xác nhận component state được phục hồi.
