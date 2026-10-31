# Phase D: SQLite state và recovery tối thiểu

## Mục tiêu

Kiểm chứng runtime process có thể restart mà vẫn giữ được component state và lịch sử cơ bản. Đây là persistence MVP, không phải storage platform đầy đủ.

## Storage

Dùng SQLite với WAL, một database cho mỗi runtime instance. Retention hiện hard-code một ngày và để lại policy interface cho tương lai.

### Bảng MVP

- `runtime_instances`;
- `graph_versions`;
- `component_state`;
- `event_inbox`;
- `event_history`;
- `event_rejections`;
- `runtime_logs`.

## State

`component_state` lưu:

- component id;
- component type;
- opaque state bytes;
- state schema version;
- updated timestamp.

Runtime không diễn giải opaque bytes. Handler type tương ứng chịu trách nhiệm tạo initial state và đọc state.

## Accepted event và recovery

Event đã accepted được ghi vào inbox hoặc event history trước khi xử lý. Sau restart:

1. mở database;
2. kiểm tra runtime identity và active graph;
3. đọc component state;
4. đọc event chưa hoàn tất;
5. phục hồi runtime status;
6. không automatic retry event sau handler failure.

At-least-once recovery có nghĩa event accepted có thể được dispatch lại sau crash trước khi hoàn tất. Handler phải an toàn với duplicate dispatch nhờ idempotency key.

## Failure và rejection

- Handler failure ghi event history và structured runtime log.
- Mailbox-full rejection ghi `event_rejections` và runtime log.
- Validation hoặc idempotency conflict ghi failure record tương ứng.
- Không tạo retry schedule và không ghi dead-letter queue.

## Reset runtime

Khi component hoặc graph thay đổi:

1. stop runtime;
2. reset component state, pending inbox và pending work;
3. activate graph version mới;
4. khởi động runtime từ state sạch.

Event history và runtime logs giữ lại trong một ngày để kiểm tra lần chạy trước. Full database reset là thao tác riêng dành cho test hoặc maintenance.

## Công việc

1. Chọn SQLite wrapper và connection ownership.
2. Viết migration schema đầu tiên.
3. Implement repositories cho state, inbox, history, rejection và logs.
4. Thay in-memory state bằng persistence adapter.
5. Implement hard-coded one-day retention qua policy interface.
6. Viết crash/restart integration test.
7. Ghi audit log trước và sau reset runtime.

## Test matrix

- state được lưu sau event;
- restart đọc đúng state;
- event đang xử lý khi crash được nhận diện;
- duplicate dispatch không làm mất idempotency result;
- mailbox rejection query được;
- handler failure query được;
- reset tạo state sạch;
- event history/logs còn lại sau reset;
- retention một ngày được kiểm tra.

## Exit criteria

- Restart process giữ được component state cơ bản.
- Event history và runtime log query được.
- Rejection record được lưu bền vững.
- Reset graph không migrate state.
- Không có retry scheduler hoặc dead-letter dependency.
- Database dùng riêng cho từng runtime instance.

## Trạng thái triển khai

Phase 4 hiện có:

- `RuntimePersistence` sở hữu một SQLite connection cho từng runtime instance;
- SQLite WAL, foreign keys và migration schema tự động;
- các bảng `runtime_instances`, `graph_versions`, `component_state`, `event_inbox`, `event_history`, `event_rejections` và `runtime_logs`;
- lưu opaque component state và khôi phục state khi mở lại database;
- lưu accepted event, completion history, rejection record và structured log;
- reset execution state bằng cách xóa component state và pending inbox, đồng thời giữ history và logs;
- test restart, state recovery, history, rejection, log và reset.

Chạy kiểm thử:

```bash
cmake --build build/phase1_runtime --target tst_RuntimePersistencePhase4
ctest --test-dir build/phase1_runtime \
  -R '^tst_Runtime(PersistencePhase4|CorePhase3|DeploymentPhase2|ServicePhase1)$' --output-on-failure
```

Phase 4 chưa thêm retry scheduler, dead-letter queue hoặc IPC. Event accepted chưa hoàn tất vẫn được lưu trong `event_inbox` để các phase sau quyết định cơ chế recovery dispatch.

Retention policy một ngày và audit log trước hoặc sau reset sẽ được hoàn thiện cùng policy service ở phase tiếp theo.

## Mở rộng sau Phase 4: persistence cho graph lồng nhau

Persistence phải định danh execution instance bằng qualified path, không chỉ bằng local
`component_id`. Khi cùng một `GraphDefinition` được instantiate nhiều lần, các key sau
phải khác nhau:

```text
root/checkout/fraud_check/score
root/refund/fraud_check/score
```

Root deployment lưu dependency manifest và execution-plan hash. Manifest phải chứa chính
xác `graph_id + graph_version` của mọi graph definition đã resolve; recovery không được
resolve lại `latest` từ registry.

Các cột/bảng cần mở rộng ở Phase F:

- `graph_versions`: root graph id/version, dependency manifest, execution-plan hash;
- `component_state`: `qualified_instance_path`, graph scope, definition id/version;
- `event_inbox`, `event_history`, `event_rejections`: source/target path, root event id,
  causation id, graph scope, hop count và plan version;
- `runtime_logs`: qualified component path và boundary/connection id khi có.

Recovery dựng execution plan từ snapshot trước rồi mới restore state. Activate root graph mới
reset state, inbox và pending work của toàn bộ subtree; history/log vẫn giữ theo retention.
Không migrate state giữa những root version khác nhau trong MVP.

### Composite lifecycle

Composite không có business handler riêng. Khi root graph start, runtime tạo các scope phân cấp
rồi start leaf actor registry. Khi stop, runtime chặn ingress ở root và các boundary trước,
ap dụng cùng một drain/reset policy cho toàn bộ subtree, sau đó flush state/history/log.
Không hỗ trợ hot-swap graph con khi runtime đang chạy.

Chi tiết test và schema migration nằm trong [phase-06-nested-graphs.md](phase-06-nested-graphs.md).
Chỉ commit active manifest sau khi dependency closure và execution plan đã validate thành công,
để không tạo runtime active nửa chừng.

## Exit criteria bổ sung cho Phase F

- restart phục hồi đúng state của từng GraphInstance;
- pending event gắn với execution-plan version cũ không được route sang plan mới;
- history/rejection/log query được theo qualified path;
- activation atomic cho root graph và dependency snapshot.