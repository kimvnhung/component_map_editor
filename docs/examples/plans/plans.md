Tôi sẽ chuyển các khoảng trống đã xác định thành roadmap có thứ tự phụ thuộc, deliverable, test bắt buộc và tiêu chí nghiệm thu cho từng phase.# Plan thực hiện kiến trúc mới

## Mục tiêu cuối

Đưa project về mô hình:

```text
customize_example_app
  ├── Graph Editor
  └── Runtime Monitor
          │
          │ RuntimeClient + gRPC
          ▼
component_map_runtime_process
  ├── Deployment Manager
  ├── Event Router
  ├── Actor Scheduler
  ├── Runtime Handlers
  ├── SQLite Persistence
  ├── Recovery
  └── Status, History, Logs
```

Các nguyên tắc cần giữ xuyên suốt:

- Editor không sở hữu execution state.
- Runtime không include QML hoặc editor model.
- `GraphExecutionSandbox` chỉ còn cho preview và compatibility.
- Runtime artifact là protobuf độc lập với QObject.
- Một component instance có mailbox và state riêng.
- Runtime process vẫn chạy khi UI đóng.
- Mọi behavior production phải kiểm chứng qua process boundary.

---

# Phase 0. Chốt baseline và dựng hàng rào kiến trúc

## Mục tiêu

Đảm bảo mọi phase sau đều bắt đầu từ trạng thái build được và không làm lẫn hai execution stack.

## Công việc

### 0.1 Chốt target CMake

Duy trì các target sau:

```text
libComponentMapEditor
cme_runtime_client
component_map_runtime
component_map_runtime_process
cme_editor_compat
customize_example_app
```

Kiểm tra dependency bắt buộc:

```text
libComponentMapEditor
  → graph contracts, Qt UI support

cme_runtime_client
  → runtime protobuf, gRPC

component_map_runtime
  → runtime protobuf, SQLite, gRPC, logging

component_map_runtime_process
  → component_map_runtime

cme_editor_compat
  → legacy execution implementation
```

Không cho phép:

```text
component_map_runtime → libComponentMapEditor
component_map_runtime → QML
cme_runtime_client → GraphModel
libComponentMapEditor → execution_internal
```

### 0.2 Phân loại test hiện tại

Chia test thành ba nhóm:

| Nhóm | Ví dụ | Target |
|---|---|---|
| Editor authoring | graph model, property schema, validation | `libComponentMapEditor` |
| Runtime production | deployment, persistence, gRPC, routing | `component_map_runtime` |
| Preview compatibility | sandbox, sequential, actor simulator | `cme_editor_compat` |

### 0.3 Tạo baseline report

Ghi lại:

- command build;
- command chạy unit test;
- runtime executable;
- runtime endpoint mặc định;
- database path mặc định;
- test nào đang pass;
- test nào còn phụ thuộc sandbox.

## Deliverable

- CMake dependency graph ổn định.
- Test suite được phân nhóm.
- Một tài liệu build và test thống nhất.
- Không có production target nào link `cme_editor_compat`.

## Tiêu chí nghiệm thu

Có thể build riêng:

```text
runtime library
runtime process
editor library
runtime client
compatibility layer
```

và build runtime không cần Qt Quick hoặc QML.

---

# Phase 1. Đóng flat runtime vertical slice

Đây là phase ưu tiên cao nhất. Không nên triển khai nested graph hoặc dọn legacy trước khi phase này chạy hoàn chỉnh.

## Mục tiêu

Chạy được chuỗi sau qua process thật:

```text
start process
→ deploy graph
→ activate graph
→ start runtime
→ publish event
→ process event
→ query component status
→ query history
→ query logs
→ stop runtime
```

## 1.1 Chuẩn hóa test graph

Tạo một graph phẳng cố định:

```text
Producer → Processor → Sink
```

Graph cần có:

- ba component instance;
- port input/output rõ ràng;
- message type rõ ràng;
- route ID ổn định;
- state riêng cho từng instance;
- một event đầu vào;
- ít nhất một output event.

Không dùng `BuiltinHandler` làm tiêu chuẩn cuối cùng. Có thể dùng nó làm test fixture tạm thời.

## 1.2 Hoàn thiện deployment lifecycle

Xác định state machine:

```text
NoDeployment
  → Staged
  → Active
  → Running
  → Stopped
```

Các transition không hợp lệ phải bị từ chối:

- activate khi chưa deploy;
- deploy khi runtime đang chạy;
- start khi chưa activate;
- activate graph version khác với staged version;
- deploy graph không hợp lệ;
- stop khi runtime đã stopped.

Mỗi rejection cần có:

```text
machine-readable code
human-readable message
runtime state
```

## 1.3 Hoàn thiện event flow

Runtime cần thể hiện rõ:

```text
publish
  → validate target
  → accept vào inbox
  → enqueue mailbox
  → dispatch handler
  → update state
  → emit output
  → route output
  → record history
```

Tạm thời có thể giữ execution đồng bộ để đóng vertical slice. Tuy nhiên phải đặt interface để thay scheduler ở phase sau.

## 1.4 Bổ sung smoke client

Tạo một client hoặc test executable có thể:

- kết nối gRPC;
- gửi deployment;
- activate;
- start;
- publish event;
- đọc status;
- đọc history;
- đọc logs;
- stop.

## Test bắt buộc

Tạo hoặc hoàn thiện test tương đương:

- `tst_RuntimeProcessPhase5.cpp`;
- `tst_RuntimeGrpcMvp.cpp`;
- `tst_RuntimeDeploymentPhase2.cpp`;
- `tst_RuntimeCorePhase3.cpp`.

## Tiêu chí nghiệm thu

Một test integration khởi động `component_map_runtime_process`, giao tiếp qua gRPC, rồi xác nhận:

- event được xử lý;
- state thay đổi;
- output event được route;
- history có event;
- log có execution record;
- UI không cần khởi động.

---

# Phase 2. Hoàn thiện persistence và recovery

## Mục tiêu

Đảm bảo process restart không làm mất state hoặc tạo execution không kiểm soát.

## 2.1 Chuẩn hóa schema SQLite

Giữ database riêng cho một runtime instance.

Các bảng cần có:

```text
runtime_instances
graph_versions
deployment_snapshots
component_state
event_inbox
event_outbox
event_history
component_history
event_rejections
runtime_logs
scheduled_jobs
```

Mỗi bảng cần có:

- primary key ổn định;
- timestamp;
- schema version nếu dữ liệu có thể migrate;
- index cho query thường xuyên;
- foreign key hoặc quy tắc consistency tương đương.

## 2.2 Thiết kế transaction boundary

Một lần xử lý event cần xác định rõ transaction:

```text
begin transaction
  load component state
  process event
  save component state
  insert history
  insert output events into outbox
  mark inbox completed
commit
```

Nếu transaction thất bại:

```text
rollback
record failure
không phát output event
```

Không ghi state thành công nhưng bỏ mất output event.

## 2.3 Xử lý event inbox

`event_inbox` cần phân biệt ít nhất:

```text
accepted
processing
completed
failed
rejected
```

Khi process crash:

- event `processing` phải được phát hiện;
- runtime phải có chính sách rõ ràng;
- không tự retry handler failure nếu MVP chưa hỗ trợ retry;
- event chưa bắt đầu xử lý có thể tiếp tục;
- event đã commit phải không xử lý lại ngoài policy.

## 2.4 Idempotency

Lưu:

```text
idempotency_key
request_fingerprint
result status
failure
```

Hai request cùng key nhưng fingerprint khác phải bị từ chối.

Hai request cùng key và fingerprint giống nhau phải trả lại kết quả cũ.

## 2.5 Recovery flow

`recover()` cần mở rộng từ việc chỉ nạp state thành:

```text
load active deployment
→ recreate actors
→ load qualified component states
→ load incomplete inbox events
→ restore runtime counters
→ restore status
→ rebuild ready queue
```

## Test bắt buộc

- restart khi chưa có event;
- restart sau khi state đã commit;
- restart khi event đang ở inbox;
- restart sau failure;
- duplicate idempotency key;
- cùng key khác fingerprint;
- hai graph instance dùng chung definition nhưng state độc lập;
- database cũ được mở lại sau process restart;
- không route event của graph version cũ sang graph version mới.

## Tiêu chí nghiệm thu

Test phải thực hiện:

```text
process A
  deploy graph
  publish event
  stop hoặc kill process

process B
  mở cùng database
  recover
  query state
  tiếp tục xử lý event phù hợp
```

Kết quả phải deterministic.

---

# Phase 3. Thay runtime engine đồng bộ bằng actor scheduler

## Mục tiêu

Đưa runtime từ “server gọi `processAll()`” thành runtime worker thật.

## 3.1 Định nghĩa handler contract mới

Thêm contract runtime riêng, không dùng `IExecutionSemanticsProvider` làm production contract.

Ví dụ:

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

Các object hỗ trợ cần có:

```text
RuntimeContext
  component identity
  state access
  clock
  cancellation
  logging

RuntimeEmitter
  output event
  delayed job
  state transition
```

## 3.2 Tách actor runtime khỏi actor simulator

Tạo runtime actor riêng hoặc refactor actor primitives thành abstraction dùng được cho production:

```text
RuntimeActor
  ├── instance identity
  ├── component type
  ├── bounded mailbox
  ├── lifecycle status
  ├── state
  ├── current job
  └── metrics
```

Không để runtime actor phụ thuộc:

- graph traversal;
- in-degree;
- join gate;
- execution tick;
- breakpoint.

## 3.3 Scheduler

Scheduler cần hỗ trợ:

```text
actor-level serialization
runtime-level parallelism
bounded mailbox
FIFO ordering
backpressure
busy/idle transitions
```

Mặc định:

```text
maxConcurrency = 1
mailboxCapacity = configured
backpressure = explicit rejection
```

Các actor khác nhau có thể chạy trên worker pool.

Handler không tự tạo thread.

## 3.4 Output event

Một handler có thể phát:

- không có output;
- một output;
- nhiều output;
- output đến nhiều route;
- delayed output nếu runtime hỗ trợ.

Không giới hạn `RuntimeEvent *output` thành một output duy nhất.

## 3.5 Error model

Phân biệt:

```text
validation failure
handler failure
mailbox rejection
routing failure
persistence failure
runtime lifecycle failure
```

Mỗi lỗi cần có:

```text
error code
event ID
component ID
route ID nếu có
correlation ID
retryability
```

## Test bắt buộc

- hai actor độc lập chạy song song;
- cùng actor không xử lý đồng thời;
- mailbox đầy;
- FIFO ordering;
- handler phát nhiều output;
- actor chuyển `idle → busy → idle`;
- handler failure không làm hỏng actor khác;
- stop runtime không nhận event mới;
- actor stop gọi lifecycle đúng một lần.

## Tiêu chí nghiệm thu

Runtime có thể chạy nền mà không cần gRPC request gọi `processAll()`.

`processAll()` nếu còn tồn tại chỉ được dùng trong test hoặc compatibility mode.

---

# Phase 4. Kết nối extension pack runtime

## Mục tiêu

Thay các handler mẫu trong [main.cpp](component_map_runtime/main.cpp) bằng domain handler có thể load từ extension system.

## 4.1 Định nghĩa runtime extension contract

Extension pack cần cung cấp:

```text
component type definition
port definitions
property schema
runtime handler factory
state schema
event schema
validation
```

Cần phân biệt rõ:

```text
editor provider
runtime handler
```

Một extension có thể cung cấp cả hai, nhưng runtime không được phụ thuộc vào QObject hoặc QML provider.

## 4.2 Runtime handler registry

Registry cần hỗ trợ:

- handler factory;
- handler ID;
- supported component types;
- version;
- state schema version;
- compatibility check;
- duplicate registration rejection.

## 4.3 Migrate fruit factory

Triển khai các handler:

```text
Manager
FruitProducer
Store
WashMachine
Seller
```

Mỗi handler cần:

- lifecycle;
- input events;
- output events;
- state serialization;
- failure cases;
- unit test.

## 4.4 Extension loading

Quyết định một cơ chế rõ ràng:

- static registration cho MVP;
- dynamic extension pack loading sau đó.

Trong phase đầu, có thể đăng ký statically nhưng vẫn dùng đúng runtime contract. Không nên để `main.cpp` chứa business handler implementation.

## Tiêu chí nghiệm thu

Một deployment artifact chứa component type domain thật có thể chạy trong runtime process mà không cần editor process.

---

# Phase 5. Chuẩn hóa graph và route contract

## Mục tiêu

Loại bỏ dần việc dùng dynamic properties như nguồn sự thật cho routing.

## 5.1 Graph definition

Bổ sung hoặc chuẩn hóa:

```text
graph_id
graph_version
component instances
definition ID
instance ID
configured properties
state schema version
```

## 5.2 Port descriptor

Mỗi port cần có:

```text
port_id
direction
message_type
cardinality
required/optional
```

## 5.3 Runtime route

Mỗi route cần có:

```text
connection_id
source_component_id
source_port
target_component_id
target_port
message_type
delivery_mode
ordering_key
enabled
filter
```

## 5.4 Validation

Bổ sung validation cho:

- duplicate instance ID;
- missing component type;
- missing port;
- wrong port direction;
- incompatible message type;
- disabled route;
- duplicate route ID;
- invalid mailbox configuration;
- invalid concurrency configuration;
- missing graph version;
- invalid nested boundary.

## 5.5 Migration từ tokenKey

Trong migration:

```text
tokenKey → messageType
```

Nhưng cần:

- export cả hai nếu cần compatibility;
- runtime ưu tiên `messageType`;
- cảnh báo khi chỉ có `tokenKey`;
- đặt thời hạn loại bỏ `tokenKey`.

## Tiêu chí nghiệm thu

Hai bên editor và runtime đọc cùng một deployment artifact và đưa ra cùng kết quả validation.

Không cần đọc `GraphModel` hoặc dynamic property từ runtime.

---

# Phase 6. Hoàn thiện nested graph

Chỉ bắt đầu sau khi flat runtime và recovery đã ổn định.

## Mục tiêu

Bảo đảm nested graph là một extension của flat runtime, không tạo execution semantics thứ hai.

## 6.1 Flatten compiler

Compiler cần:

```text
resolve graph definition version
→ validate dependency closure
→ expand composite instances
→ map public ports
→ produce qualified leaf paths
→ produce leaf routes
→ calculate deterministic plan hash
```

## 6.2 Boundary rules

Bắt buộc kiểm tra:

- public port mapping one-to-one;
- parent chỉ gọi public port;
- child không truy cập parent internal state;
- composite không có mailbox riêng trong MVP;
- composite không có business state riêng;
- leaf actor có state riêng;
- graph dependency pin theo version;
- không resolve `latest`.

## 6.3 Failure behavior

Nếu flatten hoặc actor preparation thất bại:

```text
activation fail
→ không thay active deployment
→ giữ deployment trước đó
→ ghi failure rõ ràng
```

## 6.4 Persistence

State key phải dùng:

```text
deployment_id + qualified_path
```

History và log cũng phải lưu qualified path.

## Test bắt buộc

- flat graph;
- một composite;
- composite lồng nhau;
- hai instance cùng definition;
- exposed port không tồn tại;
- mapping nhiều chiều;
- graph dependency cycle;
- event-flow cycle;
- activation rollback;
- deterministic hash;
- restart nested deployment;
- state isolation giữa hai graph instance.

## Tiêu chí nghiệm thu

Nested graph chạy cùng `RuntimeEngine`, không tạo engine riêng cho composite.

---

# Phase 7. Hoàn thiện RuntimeClient và UI monitoring

## Mục tiêu

Biến `customize_example_app` thành editor và monitoring client, không còn là execution host.

## 7.1 RuntimeClient

Bổ sung:

- timeout mặc định;
- reconnect;
- channel state;
- protocol version check;
- application error mapping;
- cancellation;
- endpoint configuration;
- local process discovery nếu cần.

## 7.2 Manager facade

`ComponentMapEditorManager` chỉ nên làm:

```text
export editor graph
call RuntimeClient
map protobuf to QML-friendly QVariant
emit status changes
```

Không giữ:

- actor state;
- runtime mailbox;
- execution queue;
- runtime handler;
- runtime persistence.

## 7.3 UI panels

Tách UI thành các vùng:

```text
RuntimeOverviewPanel
DeploymentPanel
ComponentStatusPanel
EventHistoryPanel
RuntimeLogPanel
RejectionPanel
```

Các command chính:

```text
Deploy
Activate
Start
Stop
Refresh
Publish Event
Reconnect
```

## 7.4 Snapshot và live updates

Khi UI mở:

```text
connect runtime
→ query current runtime status
→ query active deployment
→ query component statuses
→ query latest history/logs
→ bắt đầu polling
```

Khi UI đóng:

```text
không stop runtime
```

MVP có thể dùng polling. Streaming để phase sau.

## 7.5 Loại bỏ execution debugger khỏi production UI

Không để production UI hiển thị:

- `step`;
- `run(maxSteps)`;
- current tick;
- ready queue;
- breakpoint;
- engine mode;
- sequential/actor implementation mode.

`TimelineModel` nếu giữ lại phải trở thành event history model.

## Tiêu chí nghiệm thu

Khởi động runtime process, đóng app, chờ runtime xử lý event, mở app lại và thấy state mới qua query snapshot.

---

# Phase 8. Compatibility cutover và retirement

## Mục tiêu

Giữ backward compatibility đủ lâu nhưng không để legacy trở thành đường chạy mặc định.

## 8.1 Đổi tên và vị trí logic

Chuyển dần về cấu trúc:

```text
component_map_editor/preview/
component_map_editor/compatibility/
```

Các implementation cần chuyển:

- `GraphExecutionSandbox`;
- `SequentialExecutionEngine`;
- `ActorMailboxExecutionEngine`;
- `ExecutionMigrationFlags`;
- `ExecutionPlanBuilder`.

## 8.2 Quy tắc sử dụng

Legacy chỉ được dùng cho:

- preview;
- unit test DAG cũ;
- migration;
- backward compatibility.

Không được dùng cho:

- runtime process;
- production monitoring;
- deploy flow;
- domain worker execution.

## 8.3 Xóa migration flags

Chỉ xóa sau khi:

- không còn production caller;
- test cũ được phân loại;
- runtime client flow đã thay thế;
- preview có API riêng;
- documentation đã cập nhật.

## Tiêu chí nghiệm thu

Có thể build production editor mà không link compatibility target.

Compatibility target vẫn build độc lập và có test riêng.

---

# Phase 9. Hardening và Definition of Done

## 9.1 Test matrix cuối

### Deployment

- valid graph;
- invalid graph;
- duplicate instance;
- invalid port;
- invalid message type;
- invalid version;
- activate rollback.

### Runtime

- start/stop;
- publish;
- route;
- fan-out;
- mailbox full;
- actor failure;
- output event;
- state update;
- idempotency.

### Persistence

- state restore;
- event restore;
- history;
- logs;
- rejection;
- crash recovery;
- database migration.

### IPC

- process start;
- process unavailable;
- timeout;
- reconnect;
- malformed request;
- protocol mismatch;
- graceful shutdown.

### Nested graph

- flatten;
- qualified paths;
- state isolation;
- dependency pinning;
- cycle rejection;
- recovery.

### UI

- deploy;
- activate;
- runtime status;
- component status;
- history;
- logs;
- reconnect;
- UI close không dừng runtime.

## 9.2 Operational checklist

Cần có tài liệu cho:

- build;
- start runtime;
- chọn database;
- chọn endpoint;
- deploy graph;
- activate graph;
- publish event;
- xem status;
- restart;
- recovery;
- xử lý database lỗi;
- xử lý handler failure.

## Definition of Done

Target mới được coi là hoàn thành khi một người mới vào project có thể:

1. build runtime process;
2. start runtime bằng command line;
3. deploy graph bằng smoke client;
4. activate graph;
5. start runtime;
6. publish event;
7. kiểm tra status, history và logs;
8. stop process;
9. restart bằng cùng database;
10. xác nhận state được phục hồi;
11. chạy nested graph;
12. mở editor để quan sát runtime;
13. đóng editor mà runtime vẫn tiếp tục chạy.

---

# Thứ tự commit đề xuất

Nên chia commit theo boundary có thể review độc lập:

```text
1. baseline CMake and test target isolation
2. flat runtime smoke fixture
3. deployment lifecycle hardening
4. SQLite transaction and inbox recovery
5. outbox and idempotency
6. runtime handler lifecycle contract
7. actor scheduler and worker pool
8. runtime extension registration
9. graph port and route contract
10. nested graph validation and recovery
11. RuntimeClient reconnect and timeout
12. monitoring UI cutover
13. compatibility cleanup
14. full end-to-end test and documentation
```

## Ưu tiên thực tế

Nếu cần triển khai theo từng milestone, nên chia như sau:

| Milestone | Kết quả |
|---|---|
| M1 | Runtime process xử lý được flat graph qua gRPC |
| M2 | Restart khôi phục state và event đúng policy |
| M3 | Worker handler lifecycle và scheduler hoạt động |
| M4 | Fruit factory chạy bằng runtime extension thật |
| M5 | Nested graph chạy và cô lập state |
| M6 | UI hoàn toàn là editor/monitoring client |
| M7 | Legacy execution stack chỉ còn compatibility |

Điểm cần tránh là bắt đầu bằng việc xóa `GraphExecutionSandbox` hoặc viết thêm execution engine mới. Trình tự an toàn là **đóng flat vertical slice, chứng minh persistence, thay semantics worker, rồi mới dọn compatibility**.