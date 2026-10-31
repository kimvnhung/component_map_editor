# Kế hoạch triển khai MVP runtime

## Mục tiêu

Xây một runtime process runnable để kiểm chứng mô hình:

```text
Producer -> Processor -> Sink
```

Runtime phải chạy độc lập với UI, nhận deployment document, route event, giữ state riêng cho từng component instance, ghi failure/rejection và phục hồi state cơ bản sau restart.

## Phạm vi cố định

### Có trong MVP

- runtime library và runtime process;
- graph deployment document tối thiểu;
- một active graph version;
- component type, instance, port và direct route;
- actor với bounded FIFO mailbox;
- `max_concurrency = 1` cho từng actor;
- opaque state bytes có schema version;
- idempotency key và request fingerprint;
- mailbox-full rejection;
- failure code và structured log;
- SQLite WAL cho state, inbox, history, rejection và log;
- gRPC TCP plaintext trên `127.0.0.1`;
- API polling;
- restart và recovery cơ bản;
- fake handlers Producer, Processor và Sink.

## Thứ tự triển khai

| Bước | Kết quả | Phụ thuộc |
|---|---|---|
| 1 | Runtime boundary và test harness | Không |
| 2 | Deployment contract và validation | Bước 1 |
| 3 | Runtime core vertical slice | Bước 1, 2 |
| 4 | SQLite persistence và recovery | Bước 3 |
| 5 | gRPC server, process và smoke client | Bước 4 |
| 6 | Graph lồng nhau / composite component | Bước 5 |
| 7 | Hoàn thiện kiểm chứng và tài liệu chạy | Bước 6 |

Bước 6 là phần mở rộng sau MVP phẳng; không làm thay đổi contract execution đã được
kiểm chứng ở các bước 1-5. Chi tiết nằm trong [phase-06-nested-graphs.md](phase-06-nested-graphs.md).

Các bước 1-5 vẫn là baseline bắt buộc để cô lập lỗi giữa runtime cơ bản và execution
phân cấp.

### Bước 6: Graph lồng nhau / composite component

- thêm graph registry và immutable version resolver;
- thêm composite component reference, exposed port và hierarchical component identity;
- validate dependency closure, missing version, exposed port, duplicate path và cycle;
- compile graph tree thành execution plan có boundary adapter;
- route event qua exposed ports, giữ state/mailbox riêng cho từng GraphInstance;
- mở rộng persistence, status, history và gRPC query theo hierarchical path;
- test hai instance của cùng một graph con không chia sẻ state, pin version và restart.

### Hoàn thành khi

- root graph deploy được cùng toàn bộ dependency closure;
- graph con được instantiate như component bình thường;
- event vào/ra boundary đúng port và lỗi boundary không bị mất;
- graph con được dùng hai lần với state độc lập;
- cycle và dependency thiếu bị từ chối trước khi tạo actor;
- restart giữ đúng state theo graph instance và không đổi version đã pin.


## Bước 1: Runtime boundary và test harness

### Công việc

- Giữ target `component_map_runtime` độc lập với QML.
- Giữ executable `component_map_runtime_process`.
- Chuẩn hóa `RuntimeDescriptor` và lifecycle `stopped`, `starting`, `running`, `stopping`, `failed`.
- Tạo runtime event, component identity và failure result nội bộ.
- Tạo fake handler Producer, Processor và Sink.
- Tạo fixture graph có ít nhất hai instance cùng type nhưng state riêng.

### Đầu ra

- runtime library build được;
- process skeleton build được;
- test harness chạy được không cần UI;
- fake handlers được đăng ký bằng handler registry.

### Hoàn thành khi

- Runtime không include QML hoặc editor QObject.
- Test chứng minh hai instance cùng type không chia sẻ state.
- Lifecycle test pass.

## Bước 2: Deployment contract và validation

### Công việc

- Tạo `proto/runtime/common.proto`.
- Tạo `proto/runtime/deployment.proto`.
- Mô tả graph version, component instance, port, route và message type.
- Viết adapter từ editor graph sang deployment document.
- Viết validator cho duplicate id, port, direction, message type và handler.
- Chỉ cho phép một active graph version.
- Chỉ cho phép activate khi runtime stopped.
- Activate graph mới reset execution state và không chuyển event pending.

### Đầu ra

- deployment document serialize/deserialize được;
- valid fixture Producer -> Processor -> Sink;
- invalid fixtures có validation code ổn định;
- round-trip test.

### Hoàn thành khi

- Runtime nhận deployment document mà không link `GraphModel`.
- Graph invalid bị từ chối trước khi tạo actor.
- Activate khi runtime đang chạy trả lỗi rõ ràng.

## Bước 3: Runtime core vertical slice

### Công việc

- Tách mailbox và actor primitive khỏi execution engine cũ.
- Implement `RuntimeEngine`, `ActorRegistry`, `ActorScheduler` và direct `EventRouter`.
- Implement bounded FIFO mailbox.
- Implement `max_concurrency = 1`.
- Implement opaque in-memory component state.
- Implement idempotency store và request fingerprint.
- Implement failure result, event history interface và rejection interface.
- Implement structured logging.
- Không implement retry loop, backoff scheduler hoặc dead-letter.

### Luồng bắt buộc

1. Deploy graph.
2. Activate graph.
3. Start runtime.
4. Publish event tới Producer.
5. Producer phát event tới Processor.
6. Processor phát event tới Sink.
7. Query status và history.
8. Stop runtime.

### Hoàn thành khi

- Vertical slice chạy pass trong test process.
- Actor cùng instance xử lý tuần tự.
- Actor khác instance có thể chạy song song.
- Mailbox đầy trả rejection và ghi record.
- Handler failure trả failure code và structured log.
- Duplicate idempotency request không dispatch lần hai.

## Bước 4: SQLite persistence và recovery

### Công việc

- Chọn SQLite wrapper và connection ownership.
- Tạo migration schema đầu tiên với WAL.
- Implement repository cho:
  - `runtime_instances`;
  - `graph_versions`;
  - `component_state`;
  - `event_inbox`;
  - `event_history`;
  - `event_rejections`;
  - `runtime_logs`.
- Persist state và accepted event.
- Persist failure và rejection trước khi trả kết quả tương ứng.
- Implement hard-coded retention một ngày qua policy interface.
- Implement stop/reset/start cho graph update.
- Không migrate state giữa graph versions.

### Recovery behavior

Sau process restart:

1. mở database;
2. đọc runtime identity và active graph;
3. phục hồi component state;
4. nhận diện event chưa hoàn tất;
5. khởi động runtime ở trạng thái ổn định.

Runtime không automatic retry sau handler failure. Event accepted có thể được dispatch lại sau crash; handler dùng idempotency key để xử lý duplicate.

### Hoàn thành khi

- Restart giữ được state cơ bản.
- History, rejection và logs query được.
- Reset tạo state sạch nhưng giữ history/log trong một ngày.
- Crash/restart integration test pass.

## Bước 5: gRPC server, process và smoke client

### Proto files

Tạo:

```text
proto/runtime/common.proto
proto/runtime/deployment.proto
proto/runtime/control.proto
proto/runtime/event.proto
proto/runtime/status.proto
```

### API

- `DeployGraph`;
- `ActivateGraph`;
- `StartRuntime`;
- `StopRuntime`;
- `PublishEvent`;
- `GetRuntimeStatus`;
- `GetComponentStatus`;
- `ListEventHistory`;
- `ListRuntimeLogs`.

### Công việc

- Generate C++ protobuf target dùng chung.
- Implement gRPC server adapter gọi runtime core.
- Bind mặc định `127.0.0.1` với TCP plaintext.
- Thêm timeout và message size limit cơ bản.
- Implement graceful shutdown.
- Viết smoke client cho vertical slice.
- Viết process-level test cho disconnect/reconnect.

### Hoàn thành khi

- Runtime process chạy không cần editor hoặc QML.
- Smoke client hoàn tất Producer -> Processor -> Sink qua gRPC.
- Client query được status, history, rejection và logs.
- Activate khi runtime running bị từ chối.
- Không cần authentication, TLS hoặc streaming để chạy MVP.

## Bước 7: Hoàn thiện kiểm chứng

### Công việc

- Viết hướng dẫn build và chạy process.
- Viết hướng dẫn chạy smoke client.
- Thêm test matrix cho valid, invalid, failure, rejection, restart, reset và nested graph.
- Đo thời gian xử lý và độ ổn định cơ bản của vertical slice.
- Ghi lại kết luận: mô hình phù hợp, cần điều chỉnh hoặc không phù hợp.

### Definition of done

MVP phẳng và nested-graph extension được coi là hoàn thành khi một người khác có thể:

1. build project;
2. khởi động runtime process;
3. deploy và activate root graph cùng dependency closure;
4. start runtime;
5. publish event bằng smoke client;
6. query status, history và logs theo hierarchical component path;
7. stop, restart process;
8. xác nhận state của từng GraphInstance được phục hồi độc lập.

## Tệp dự kiến triển khai

- `component_map_runtime/include/component_map_runtime/`;
- `component_map_runtime/src/`;
- `component_map_runtime/main.cpp`;
- `proto/runtime/`;
- `tests/tst_RuntimeCoreMvp.cpp`;
- `tests/tst_RuntimePersistenceMvp.cpp`;
- `tests/tst_RuntimeGrpcMvp.cpp`;
- `tests/tst_RuntimeNestedGraphPhase6.cpp`.

Tên test có thể thay đổi theo cấu trúc hiện tại, nhưng mỗi bước phải có test tự động tương ứng.

Chi tiết Phase 6 nằm trong [phase-06-nested-graphs.md](phase-06-nested-graphs.md).

Ở mọi trường hợp, không tự động nâng version graph con khi root graph đang active; phải tạo
deployment root version mới và activate atomic.

## Tệp dự kiến triển khai (MVP phẳng)

Các tệp Phase 1-5 giữ nguyên; Phase 6 bổ sung contract, resolver, execution plan,
persistence migration và test nested graph theo tài liệu riêng.

Tên test có thể thay đổi theo cấu trúc hiện tại, nhưng mỗi bước phải có test tự động tương ứng.

## Bước 6: Graph lồng nhau / composite component

Xem [phase-06-nested-graphs.md](phase-06-nested-graphs.md).

## Bước 7: Hoàn thiện kiểm chứng

Các công việc kiểm chứng và tài liệu chạy nằm ở phần Bước 7 bên trên.

### Definition of done

MVP được coi là hoàn thành khi một người khác có thể:

1. build project;
2. khởi động runtime process;
3. deploy và activate fixture;
4. start runtime;
5. publish event bằng smoke client;
6. query được status, history và logs;
7. stop, restart process;
8. xác nhận state được phục hồi.

## Tệp dự kiến triển khai

- `component_map_runtime/include/component_map_runtime/`;
- `component_map_runtime/src/`;
- `component_map_runtime/main.cpp`;
- `proto/runtime/`;
- `tests/tst_RuntimeCoreMvp.cpp`;
- `tests/tst_RuntimePersistenceMvp.cpp`;
- `tests/tst_RuntimeGrpcMvp.cpp`;

Tên test có thể thay đổi theo cấu trúc hiện tại, nhưng mỗi bước phải có test tự động tương ứng.
