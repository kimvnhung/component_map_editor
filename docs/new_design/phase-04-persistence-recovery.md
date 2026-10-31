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
