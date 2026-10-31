# Đánh giá kiến trúc mục tiêu

Yêu cầu mới phù hợp hơn với mô hình:

```text
Graph Editor UI
        │
        │ IPC / gRPC
        ▼
Runtime Process
        │
        ├── Event Router
        ├── Durable Event Store
        ├── Actor Mailboxes
        ├── Worker Pool
        ├── State Store
        └── Event/Status Log
```

UI chỉ là **client quản lý graph và quan sát runtime**. Runtime phải chạy độc lập, không phụ thuộc vào việc UI có đang mở hay không.

Kiến trúc hiện tại của dự án lại đang là:

```text
QML UI
  └── ComponentMapEditorManager
        └── GraphExecutionSandbox
              └── chạy graph trong cùng process
```

Vì vậy cần tách **Editor Runtime** và **Execution Runtime**, thay vì tiếp tục mở rộng `GraphExecutionSandbox`.

---

# 1. Những phần nên giữ lại

## 1.1 Graph editor và QML UI

Nên giữ gần như toàn bộ:

- [GraphModel.h](component_map_editor/models/GraphModel.h)
- [ComponentModel.h](component_map_editor/models/ComponentModel.h)
- [ConnectionModel.h](component_map_editor/models/ConnectionModel.h)
- [GraphEditorController.h](component_map_editor/services/GraphEditorController.h)
- thư mục [component_map_editor/ui/](component_map_editor/ui/)
- `Palette`
- `GraphCanvas`
- `PropertyPanel`
- undo/redo
- import/export graph

Các phần này đã phù hợp với yêu cầu:

- chỉnh sửa graph;
- clone component;
- tạo connection;
- cấu hình dynamic properties;
- quan sát graph trực quan.

Tuy nhiên `GraphModel` hiện chỉ nên được xem là **editor model**, không phải runtime state.

---

## 1.2 Component type registry và property schema

Nên giữ:

- [TypeRegistry.h](component_map_editor/extensions/runtime/TypeRegistry.h)
- [PropertySchemaRegistry.h](component_map_editor/extensions/runtime/PropertySchemaRegistry.h)
- [DynamicOptionsRegistry.h](component_map_editor/extensions/runtime/DynamicOptionsRegistry.h)
- `IComponentTypeProvider`
- `IPropertySchemaProvider`

Các API này phù hợp để mô tả:

```text
component type
ports
properties
default values
configuration options
```

Ví dụ một `FruitProducer` có thể có:

```text
fruitType
capacity
unitCost
processingDelay
```

Một component được clone nên có:

```text
definitionId = fruit_producer
instanceId = apple_producer_01
```

Không nên tạo một type riêng cho từng instance như `AppleProducer`, `BananaProducer`. Nên dùng một type chung với properties khác nhau.

---

## 1.3 Connection model, nhưng cần mở rộng

Nên giữ các trường hiện tại:

- `sourceId`
- `targetId`
- `label`
- `tokenKey`
- source/target side cho UI

Nhưng `ConnectionModel` cần thêm semantics runtime:

```text
sourcePort
targetPort
messageType
deliveryMode
orderingKey
enabled
```

Đề xuất:

```cpp
Q_PROPERTY(QString sourcePort ...)
Q_PROPERTY(QString targetPort ...)
Q_PROPERTY(QString messageType ...)
Q_PROPERTY(QString orderingKey ...)
```

`tokenKey` có thể giữ trong giai đoạn migration, nhưng về lâu dài nên đổi tên hoặc bổ sung `messageType`. `tokenKey` hiện mang ý nghĩa dữ liệu nội bộ, chưa thể hiện rõ routing event.

---

## 1.4 Extension system

Nên giữ:

- `IComponentTypeProvider`
- `IPropertySchemaProvider`
- `IConnectionPolicyProvider`
- `IValidationProvider`
- `ExtensionContractRegistry`
- `ExtensionStartupLoader`
- manifest và extension pack system

Extension system phù hợp để cung cấp:

```text
component definitions
runtime handler definitions
property schemas
connection validation
port metadata
```

Tuy nhiên cần bổ sung một contract mới, không nên ép runtime worker vào `IExecutionSemanticsProvider` hiện tại.

---

# 2. Những phần nên giữ nhưng phải thay đổi

## 2.1 `IExecutionSemanticsProvider`

Contract hiện tại trong [IExecutionSemanticsProvider.h](component_map_editor/extensions/contracts/IExecutionSemanticsProvider.h) có dạng đồng bộ:

```text
executeComponent(...) -> bool
```

Contract này phù hợp với:

```text
input payload -> output payload
```

Nhưng không phù hợp với worker lâu dài vì worker cần:

- nhận nhiều event;
- giữ state;
- tạo nhiều output event;
- tạo job xử lý trễ;
- báo busy/idle/error;
- có thể được kích hoạt lại nhiều lần.

Nên giữ `IExecutionSemanticsProvider` cho:

- one-shot preview;
- validation;
- compatibility với workflow DAG cũ.

Đồng thời thêm contract mới, ví dụ:

```cpp
class IRuntimeComponentHandler
{
public:
    virtual ~IRuntimeComponentHandler() = default;

    virtual QString handlerId() const = 0;
    virtual QStringList supportedComponentTypes() const = 0;

    virtual void onStart(RuntimeContext &context) = 0;

    virtual void onEvent(const RuntimeEvent &event,
                         RuntimeContext &context,
                         RuntimeEmitter &emitter) = 0;

    virtual void onStop(RuntimeContext &context) = 0;
};
```

Điểm quan trọng: handler không tự tạo thread. Runtime scheduler sở hữu lifecycle và mailbox.

---

## 2.2 Actor mailbox engine

Nên giữ lại các phần có giá trị:

- `Mailbox`
- `Actor`
- `ActorRegistry`
- một phần `ActorScheduler`

Hiện các file nằm trong:

- [Actor.h](component_map_editor/services/execution_internal/actor_mailbox/Actor.h)
- [Mailbox.h](component_map_editor/services/execution_internal/actor_mailbox/Mailbox.h)
- [ActorRegistry.h](component_map_editor/services/execution_internal/actor_mailbox/ActorRegistry.h)
- [ActorScheduler.h](component_map_editor/services/execution_internal/actor_mailbox/ActorScheduler.h)

Nhưng không nên giữ `ActorMailboxExecutionEngine` như runtime cuối cùng. Hiện nó vẫn:

- khởi tạo từ graph snapshot;
- dùng in-degree/join gate;
- xử lý component theo graph traversal;
- mỗi component được coi là một execution step;
- kết thúc khi graph không còn work.

Nó là một **DAG adapter có mailbox**, không phải process runtime.

Nên refactor mailbox thành nền tảng cho:

```text
RuntimeActor
  ├── mailbox
  ├── lifecycle
  ├── current status
  ├── in-flight jobs
  └── durable checkpoint
```

---

## 2.3 `GraphExecutionSandbox`

Không nên xóa ngay, nhưng nên hạ xuống thành:

```text
LegacyGraphExecutionSandbox
```

hoặc:

```text
PreviewExecutionSandbox
```

Nó vẫn hữu ích cho:

- preview workflow;
- test provider;
- chạy graph nhỏ trong unit test;
- giữ backward compatibility;
- kiểm tra payload trước khi deploy.

Nhưng không nên dùng nó để chạy hệ thống worker thật.

Các API sau không còn phù hợp làm API runtime chính:

```text
start()
step()
run()
pause()
reset()
executedCount
currentTick
resolvedStartComponentId
```

Runtime mới cần API khác:

```text
startRuntime()
stopRuntime()
pauseRuntime()
resumeRuntime()
publishEvent()
queryComponentStatus()
queryRuntimeState()
queryEventHistory()
```

---

# 3. Những phần có thể loại bỏ hoặc chuyển sang compatibility layer

## 3.1 Sequential execution engine

[SequentialExecutionEngine.h](component_map_editor/services/execution_internal/SequentialExecutionEngine.h) không phù hợp với runtime worker lâu dài.

Nên:

- giữ trong `legacy/` hoặc `preview/`;
- tiếp tục dùng cho các test DAG cũ;
- không dùng trong runtime process mới;
- loại bỏ dần sau khi migration hoàn tất.

## 3.2 Execution migration flags

[ExecutionMigrationFlags.h](component_map_editor/services/ExecutionMigrationFlags.h) chủ yếu phục vụ quá trình chuyển đổi sequential/actor.

Sau khi runtime mới ổn định, có thể loại bỏ:

```text
actorEngineEnabled
tokenTransportEnabled
```

hoặc chuyển chúng thành cấu hình runtime thật:

```text
scheduler.workerCount
scheduler.deliveryMode
runtime.persistenceMode
```

Không nên tiếp tục duy trì feature flag tạm thời trong domain architecture.

## 3.3 Breakpoint/debug APIs

Người dùng không cần debug engine, nên có thể loại bỏ khỏi sản phẩm runtime:

- breakpoint;
- breakpoint-hit event;
- debug snapshot;
- execution telemetry chi tiết;
- internal ready queue;
- internal engine snapshot;
- implementation-specific actor/sequential mode.

Tuy nhiên cần giữ **operational observability**:

- log nghiệp vụ;
- trạng thái component;
- event history;
- job history;
- lỗi;
- thời điểm bắt đầu/kết thúc;
- số lượng message xử lý;
- trạng thái `idle`, `busy`, `blocked`, `error`, `stopped`.

Đây là monitoring, không phải debugger.

---

# 4. Kiến trúc runtime đề xuất

## 4.1 Runtime process độc lập

Tạo executable mới, ví dụ:

```text
component_runtime
```

Hoặc:

```text
component_map_runtime
```

Process này:

- load graph đã deploy;
- load extension packs;
- tạo actor instance;
- nhận event;
- route event;
- chạy worker;
- ghi state và history;
- phục hồi sau restart;
- expose API cho UI.

UI không được là owner của runtime.

```text
customize_example_app
    └── chỉ là editor + monitoring client

component_map_runtime
    └── process chạy nền
```

---

## 4.2 Giao tiếp UI/runtime

Dự án đã có protobuf:

- [execution.proto](proto/execution.proto)
- [public_api.proto](proto/public_api.proto)

Có thể tái sử dụng protobuf nhưng cần mở rộng thành API runtime.

Khuyến nghị sử dụng **gRPC** cho:

- control API;
- query API;
- streaming event/status;
- giao tiếp local hoặc remote;
- versioning contract.

API nên gồm:

### Deployment

```text
DeployGraph
ActivateGraph
DeactivateGraph
GetGraph
GetRuntimeVersion
```

### Runtime control

```text
StartRuntime
StopRuntime
PauseRuntime
ResumeRuntime
```

### Event

```text
PublishEvent
GetEvent
ListEvents
SubscribeEvents
```

### Status

```text
GetRuntimeStatus
ListComponentStatuses
GetComponentStatus
GetComponentHistory
GetRuntimeLogs
SubscribeStatusChanges
```

UI có thể mở sau khi runtime đã chạy và gọi:

```text
connect
query current snapshot
subscribe live updates
```

Do đó việc UI đóng/mở không ảnh hưởng execution.

Nếu chưa muốn triển khai gRPC ngay, có thể dùng Unix domain socket cho bản local, nhưng vẫn nên giữ protobuf làm contract. Không nên để UI gọi trực tiếp QObject hoặc chia sẻ memory với runtime process.

---

## 4.3 Durable storage

Runtime cần persistence vì process có thể restart.

Khuyến nghị dùng SQLite với WAL:

```text
runtime.db
```

Các bảng chính:

```text
graphs
graph_versions
runtime_instances
component_instances
event_inbox
event_outbox
scheduled_jobs
component_state
event_history
component_history
runtime_logs
```

### Event inbox

Dùng để chống mất message khi process crash.

```text
event_id
message_type
target_component_id
payload
status
attempt_count
created_at
received_at
```

### Event outbox

Dùng transactionally để bảo đảm event phát ra không bị mất cùng với state update.

```text
outbox_id
source_component_id
target_component_id
event_type
payload
available_at
published
```

### State

Mỗi component có một state riêng:

```text
component_id
status
current_job_id
state_json
updated_at
version
```

---

# 5. Mô hình worker

## 5.1 Worker là component instance

Mỗi node trong graph đại diện cho một instance worker:

```text
appleProducer01
bananaProducer01
washMachine01
washMachine02
seller01
```

Nhiều node có cùng type dùng chung handler code nhưng có state riêng.

```text
ComponentTypeDefinition
        │
        ├── handler factory
        ├── port definitions
        └── property schema

ComponentInstance
        │
        ├── instanceId
        ├── configured properties
        ├── mailbox
        ├── state
        └── history
```

## 5.2 Chỉ nhận event khi rảnh

Mỗi actor nên có:

```text
idle
busy
waiting
failed
stopped
```

Scheduler không gọi handler nếu actor đang chạy job không cho phép concurrent execution.

Các policy nên cấu hình được:

```text
maxConcurrency = 1
mailboxCapacity
backpressurePolicy
orderingPolicy
```

Mặc định với domain worker nên là:

```text
maxConcurrency = 1
FIFO mailbox
bounded queue
explicit rejection/backpressure
```

Điều này đảm bảo một Store không cập nhật inventory đồng thời từ hai message theo cách không nhất quán.

## 5.3 Concurrency

Có hai lớp đồng bộ:

1. Actor-level serialization:
   - cùng một component instance chỉ xử lý một event tại một thời điểm.
2. Runtime-level parallelism:
   - các actor khác nhau có thể chạy song song trong worker pool.

Không nên cho nhiều thread cùng sửa state của một component.

---

# 6. Connection và routing

Connection không nên chỉ là dependency edge nữa. Cần phân biệt hai loại:

```text
GraphConnection
    dùng để mô tả topology/editor

RuntimeRoute
    dùng để route message/event
```

Một connection nên mô tả:

```text
connectionId
sourceComponentId
sourcePort
targetComponentId
targetPort
messageType
filter
deliveryMode
orderingKey
enabled
```

Ví dụ:

```text
Manager.purchase_out
    -> AppleProducer.purchase_in
    messageType = PurchaseRequested
```

```text
Seller.sale_completed
    -> Manager.sale_in
    messageType = SaleCompleted
```

```text
Store.wash_out
    -> WashMachine01.request_in
    messageType = WashRequested
```

Một source có thể broadcast đến nhiều target. Runtime router phải hỗ trợ:

- fan-out;
- filtering;
- target port;
- event type;
- retry;
- dead-letter queue;
- backpressure.

`tokenKey` có thể được map tạm thời:

```text
tokenKey = messageType
```

nhưng không nên dùng nó làm toàn bộ routing model về lâu dài.

---

# 7. State và lịch sử cần hiển thị

Không cần debug snapshot, nhưng UI cần operational view.

## Runtime overview

```text
runtime status
uptime
active graph version
total events
failed events
queued events
busy workers
last error
```

## Component status

```text
component id
type
status
current job
mailbox depth
last event
last success
last error
processed count
failed count
updated time
```

## Component history

```text
event received
job started
job completed
event emitted
job failed
state changed
```

## Log

Log nên có structured fields:

```text
timestamp
level
runtimeId
componentId
eventId
correlationId
messageType
message
payload summary
```

Không nên lưu toàn bộ payload nhạy cảm mặc định. Cần redaction theo schema hoặc field policy.

---

# 8. Những phần trong proto nên thay đổi

[execution.proto](proto/execution.proto) hiện thiên về snapshot của `GraphExecutionSandbox`:

```text
ExecutionSnapshot
TimelineEvent
ComponentExecutionState
```

Nên chuyển thành các message nền tảng hơn:

```proto
message RuntimeStatus {
  string runtime_id = 1;
  string graph_version = 2;
  string status = 3;
  int64 logical_time_ms = 4;
  int64 queued_event_count = 5;
  int64 active_job_count = 6;
}

message ComponentStatus {
  string component_id = 1;
  string component_type = 2;
  string status = 3;
  string current_job_id = 4;
  int64 mailbox_depth = 5;
  google.protobuf.Struct state = 6;
  google.protobuf.Timestamp updated_at = 7;
}

message RuntimeEvent {
  string event_id = 1;
  string event_type = 2;
  string source_component_id = 3;
  string target_component_id = 4;
  string correlation_id = 5;
  google.protobuf.Struct payload = 6;
  int64 sequence = 7;
  google.protobuf.Timestamp created_at = 8;
}

message RuntimeLogEntry {
  string level = 1;
  string component_id = 2;
  string event_id = 3;
  string message = 4;
  google.protobuf.Struct fields = 5;
  google.protobuf.Timestamp timestamp = 6;
}
```

Có thể giữ `ExecutionSnapshot` cho backward compatibility, nhưng không nên dùng nó làm runtime protocol mới.

---

# 9. Những thứ nên xóa hoặc hoãn

## Có thể loại bỏ khỏi sản phẩm mới

- `Breakpoint` runtime;
- debug snapshot;
- engine-specific telemetry;
- sequential/actor mode hiển thị cho người dùng;
- ready queue exposure;
- `resolvedStartComponentId`;
- one-shot `run(maxSteps)` trong UI runtime;
- composite execution nếu chỉ phục vụ math workflow;
- rule hot reload nếu không còn dùng cho business validation.

## Nên giữ tạm thời

- `GraphExecutionSandbox`;
- `SequentialExecutionEngine`;
- `ActorMailboxExecutionEngine`;
- `ExecutionMigrationFlags`;
- `TimelineModel`.

Các phần này nên nằm trong compatibility/preview layer cho đến khi:

- các workflow cũ được migrate;
- test cũ được thay thế;
- runtime mới có đầy đủ persistence và monitoring.

Không nên xóa chúng trong cùng phase với việc xây runtime mới.

---

# 10. Kế hoạch triển khai

## Phase 1 — Tách boundary

Tạo target mới:

```text
libComponentMapEditor
component_map_runtime
customize_example_app
```

Quy tắc:

- `libComponentMapEditor`: model, UI support, extension registry, graph validation.
- `component_map_runtime`: runtime, event router, scheduler, persistence, IPC.
- `customize_example_app`: editor/monitoring client.

Không để runtime include QML.

## Phase 2 — Chuẩn hóa graph contract

Bổ sung:

- component ports;
- message types;
- runtime route metadata;
- component instance configuration;
- graph version;
- deployment metadata.

Bổ sung validation cho:

- port tồn tại;
- message type hợp lệ;
- route source/target tương thích;
- duplicate instance ID;
- invalid worker configuration.

## Phase 3 — Xây runtime core in-process trước

Trước khi làm IPC, viết runtime có thể chạy trong test:

```text
RuntimeEngine
EventRouter
ActorScheduler
Mailbox
ComponentStateStore
JobScheduler
```

Test được:

- nhiều instance;
- event fan-out;
- worker busy/idle;
- backpressure;
- retry;
- completion event;
- actor restart.

## Phase 4 — Thêm persistence

Tích hợp SQLite:

- inbox/outbox;
- component state;
- scheduled jobs;
- event history;
- logs;
- graph version.

Đảm bảo crash recovery:

1. process restart;
2. đọc event chưa hoàn tất;
3. khôi phục component state;
4. tiếp tục job;
5. không phát duplicate event ngoài policy.

## Phase 5 — Tạo runtime process và IPC

Thêm gRPC service:

```text
RuntimeControlService
RuntimeQueryService
RuntimeEventStreamService
```

UI kết nối bằng runtime endpoint:

```text
localhost:port
unix:///path/component-runtime.sock
```

Có thể hỗ trợ cả:

```text
local embedded mode
remote process mode
```

Nhưng production/default nên là process riêng.

## Phase 6 — Tích hợp UI monitoring

Giữ graph editor hiện tại, thay `ExecutionPanel` bằng:

- Runtime status panel;
- Component status panel;
- Event history panel;
- Log panel;
- Start/stop/deploy controls.

UI khi mở sẽ:

1. kết nối runtime;
2. lấy snapshot hiện tại;
3. subscribe event/status stream;
4. hiển thị trạng thái hiện tại.

## Phase 7 — Migrate customize example

Đưa các component domain vào extension pack runtime:

```text
Manager
FruitProducer
Store
WashMachine
Seller
```

Mỗi component cần:

- type descriptor;
- port descriptor;
- property schema;
- runtime handler;
- state schema;
- event schema;
- validation;
- tests.

---

# 11. Kiến trúc đích

```text
                         ┌────────────────────────┐
                         │ customize_example_app │
                         │                        │
                         │ Graph Editor           │
                         │ Property Panel        │
                         │ Runtime Monitor        │
                         └───────────┬────────────┘
                                     │ gRPC / IPC
                                     ▼
┌────────────────────────────────────────────────────────┐
│ component_map_runtime                                  │
│                                                        │
│  Runtime API                                           │
│      │                                                 │
│  Deployment Manager                                    │
│      │                                                 │
│  Event Router ─── Durable Inbox/Outbox                 │
│      │                                                 │
│  Actor Scheduler                                       │
│      │                                                 │
│  ┌────────────┬────────────┬────────────┬───────────┐  │
│  │ Manager    │ Producer   │ Store      │ Seller    │  │
│  │ mailbox    │ mailbox    │ mailbox    │ mailbox   │  │
│  └────────────┴────────────┴────────────┴───────────┘  │
│      │                                                 │
│  State Store + Event History + Structured Logs         │
│                                                        │
└────────────────────────────────────────────────────────┘
```

# Kết luận

Nên giữ dự án hiện tại làm **graph editor và extension platform**, nhưng không nên tiếp tục biến `GraphExecutionSandbox` thành runtime process.

Phân loại cuối cùng:

| Thành phần | Quyết định |
|---|---|
| GraphModel, ComponentModel | Giữ |
| GraphCanvas, PropertyPanel, UndoStack | Giữ |
| TypeRegistry, PropertySchemaRegistry | Giữ |
| Extension contracts/loader | Giữ và mở rộng |
| ConnectionModel | Giữ, bổ sung port/message metadata |
| GraphExecutionSandbox | Giữ cho preview/compatibility |
| SequentialExecutionEngine | Chuyển legacy/preview |
| ActorMailbox primitives | Giữ, refactor thành runtime core |
| ActorMailboxExecutionEngine | Không dùng làm runtime cuối |
| TimelineModel | Giữ, đổi thành event/history view |
| Debug snapshot/breakpoint APIs | Loại bỏ khỏi sản phẩm mới |
| ExecutionMigrationFlags | Loại bỏ sau migration |
| `execution.proto` snapshot-only API | Giữ compatibility, mở rộng thành runtime API |
| ComponentMapEditorManager | Tách thành editor manager và runtime client |
| Runtime process | Bổ sung mới |
| Persistence/event journal | Bổ sung mới |
| IPC/gRPC server | Bổ sung mới |

Thiết kế này đáp ứng được cả năm yêu cầu chính: UI graph hiện tại, worker component, clone instance, event routing và runtime chạy độc lập không phụ thuộc UI.