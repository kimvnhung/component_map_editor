# Phase C: Runtime core và vertical slice

## Mục tiêu

Chạy được graph Producer -> Processor -> Sink trong test process với actor, mailbox, routing, state và failure logging.

## Thành phần MVP

- `RuntimeEngine`;
- `EventRouter` cho direct route;
- `ActorRegistry`;
- `ActorScheduler`;
- bounded FIFO `Mailbox`;
- handler factory/registry;
- opaque `ComponentStateStore` in-memory;
- structured failure logger;
- idempotency store in-memory.

## Actor semantics

- Mỗi component instance có mailbox và state riêng.
- Mặc định `max_concurrency = 1`.
- Actor khác nhau có thể chạy song song.
- Public status: `idle`, `busy`, `failed`, `stopped`.
- Event chỉ complete sau khi handler result được quyết định.
- Handler không tự tạo thread và không tự truy cập mailbox.

## Routing MVP

- direct route;
- nhiều direct routes cho fan-out cơ bản;
- enabled/disabled route;
- target port và message type;
- delivery failure reporting.

Không làm filter, ordering key, delivery mode hoặc advanced backpressure.

## Backpressure

Mailbox bounded. Khi đầy, runtime reject event và ghi rejection record gồm:

- event id;
- idempotency key;
- target component;
- route;
- timestamp;
- reason code;
- mailbox depth.

Runtime không block producer, không spill-to-disk và không automatic retry.

## Failure contract

Mọi failure phải:

- trả machine-readable failure code;
- ghi structured log;
- ghi event history hoặc rejection record;
- không bị silently dropped.

Các failure tối thiểu:

- validation failure;
- handler failure;
- delivery failure;
- mailbox-full rejection;
- idempotency conflict.

Runtime không có retry loop, backoff scheduler hoặc dead-letter queue. Client tự quyết định resubmit.

## Idempotency

- Client tạo idempotency key.
- Cùng key và cùng request fingerprint trả lại kết quả đã biết.
- Cùng key nhưng fingerprint khác trả conflict.
- Key đang xử lý không được dispatch đồng thời lần hai.
- Runtime không tự chạy lại event sau failure.

## Công việc

1. Tách actor/mailbox primitive khỏi execution engine cũ.
2. Định nghĩa runtime event và handler result.
3. Viết fake Producer, Processor và Sink.
4. Resolve route table từ deployment document.
5. Implement scheduler shutdown.
6. Implement in-memory state, idempotency và rejection store.
7. Implement structured failure logging.
8. Thêm status counters cơ bản.

## Test matrix

- Producer -> Processor -> Sink;
- nhiều instance cùng type có state riêng;
- actor FIFO và `max_concurrency = 1`;
- actor khác nhau chạy song song;
- mailbox đầy tạo rejection;
- handler failure có log và failure code;
- delivery failure không automatic retry;
- idempotency duplicate và conflict;
- graceful stop không nhận event mới.

## Exit criteria

- Vertical slice chạy hoàn chỉnh trong test process.
- Không cần process hoặc QML để chạy runtime core test.
- Không có data race trên state của cùng actor.
- Failure và rejection không bị silently dropped.
- Không có retry scheduler hoặc dead-letter dependency.
- Compatibility engine cũ vẫn build và test pass.

## Trạng thái triển khai

Phase 3 hiện có:

- `RuntimeEngine` synchronous, transport-neutral và không phụ thuộc QML;
- actor registry nội bộ với mailbox FIFO bounded và `max_concurrency = 1`;
- direct route từ deployment graph, gồm route enabled/disabled và fan-out cơ bản;
- `ComponentStateStore` in-memory với state riêng theo instance;
- failure log có machine-readable code và rejection record cho delivery hoặc mailbox-full;
- idempotency store in-memory cho duplicate, conflict và kết quả handler;
- graceful stop không nhận event mới;
- test vertical slice Producer -> Processor, FIFO, bounded mailbox, handler failure, route disabled, idempotency và stop.

Chạy kiểm thử:

```bash
cmake --build build/phase1_runtime --target tst_RuntimeCorePhase3
ctest --test-dir build/phase1_runtime \
  -R '^tst_Runtime(CorePhase3|DeploymentPhase2|ServicePhase1)$' --output-on-failure
```

Phase 3 cố ý chưa có thread scheduler, retry, dead-letter, SQLite hoặc IPC. Các phần đó thuộc các phase sau.

Mọi event được xử lý tuần tự trong `processAll()`. Duplicate idempotency đang chờ sẽ nhận lỗi `IDEMPOTENCY_IN_FLIGHT` và không được dispatch lần hai.

Test Phase 3 không link QML hoặc editor target.

Runtime core hiện dùng scheduler synchronous deterministic cho vertical slice trong test process. Actor boundary vẫn tách riêng để các phase sau có thể thay scheduler mà không đổi handler API.

## Mở rộng sau Phase 3: execution plan cho composite graph

Runtime không nên xử lý composite graph như một handler lá đơn. Deployment compiler sẽ
resolve graph hierarchy trước khi tạo actor:

```text
DeploymentGraph
  -> resolve graph definitions và exposed ports
  -> validate dependency/cycle/expansion limits
  -> compile execution plan
  -> create leaf actors và boundary routes
```

Execution plan giữ source path của node để trace, nhưng actor vẫn chỉ được tạo cho leaf
component. Composite instance là một execution scope/container; nó không có business
mailbox thứ hai. Event đi qua composite bằng boundary route từ exposed port tới endpoint
nội bộ và ngược lại, tránh việc một event bị tính backpressure nhiều lần.

Các invariant cần giữ:

- leaf actor có mailbox, state và `max_concurrency = 1` như graph phẳng;
- cùng graph definition được dùng nhiều lần nhưng mỗi qualified path có actor/state riêng;
- route ngoài chỉ trỏ tới exposed port của composite;
- `root_event_id`, `causation_id`, idempotency key và failure path được giữ qua boundary;
- structural dependency cycle bị reject lúc compile, còn event-flow cycle chỉ được hỗ trợ
  khi có hop limit rõ ràng.

Phase 3 hiện chưa triển khai compiler này; đây là contract runtime cho Phase F.

## Status và failure theo hierarchy

Failure phải giữ `qualified_component_path` và `graph_scope` thay vì chỉ trả local
`instance_id`. Status của composite là aggregate status của descendants; status leaf vẫn
là actor status hiện có. Không khóa toàn bộ composite khi một child xử lý, để tránh làm
mất concurrency giữa các leaf actor và tránh deadlock trên route cycle.

Mở rộng này không thêm retry, dead-letter hoặc dynamic graph update.

## Exit criteria bổ sung cho Phase F

- execution plan deterministic với cùng root manifest;
- boundary route không làm mất event metadata;
- failure tại child truy vấn được bằng qualified path;
- hai composite instance không dùng chung state hoặc mailbox.

Chi tiết model, persistence và API nằm trong [phase-06-nested-graphs.md](phase-06-nested-graphs.md).
Nếu triển khai graph lồng nhau, phải hoàn thành validation và execution-plan compilation trước
khi tạo bất kỳ actor nào.
