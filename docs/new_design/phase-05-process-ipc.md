# Phase E: Runtime process và gRPC polling API

## Mục tiêu

Đóng gói runtime core và persistence thành process độc lập, sau đó expose API gRPC TCP tối thiểu để client kiểm chứng mô hình.

## Transport

- gRPC trên TCP plaintext;
- mặc định bind `127.0.0.1`;
- không authentication hoặc TLS trong MVP;
- remote bind không thuộc đường chạy mặc định;
- timeout và message size limit cơ bản vẫn cần có.

Runtime dùng plaintext cho bản MVP runnable.

} assistant to=functions.Edit  (commentary  code)  (json) {
## Proto layout

```text
proto/runtime/
  common.proto
  deployment.proto
  control.proto
  event.proto
  status.proto
```

`execution.proto` tiếp tục là compatibility contract. Chỉ thêm `query.proto` khi API query vượt quá phạm vi status/history nhỏ.

## API MVP

### Deployment

- `DeployGraph`;
- `ActivateGraph`.

`ActivateGraph` chỉ thành công khi runtime stopped. Activate graph mới reset execution state và không chuyển event pending của graph cũ.

### Control

- `StartRuntime`;
- `StopRuntime`.



### Event

- `PublishEvent`.

Response phải phân biệt accepted, duplicate, validation failure, handler/delivery failure và mailbox-full rejection.

### Query bằng polling

- `GetRuntimeStatus`;
- `GetComponentStatus`;
- `ListEventHistory`;
- `ListRuntimeLogs`.

Chưa triển khai streaming, cursor resume hoặc subscription.

## Client behavior

- Client tạo idempotency key.
- Client giữ key khi resubmit cùng request.
- Cùng key và fingerprint giống nhau trả kết quả đã biết.
- Cùng key nhưng fingerprint khác trả conflict.
- Runtime không automatic retry.
- Client tự quyết định có gọi lại `PublishEvent` hay không.

## Failure reporting

Mỗi failure phải:

- trả machine-readable code và message;
- ghi structured log;
- ghi event history hoặc rejection record;
- không đi vào retry loop hoặc dead-letter queue.

## Công việc

1. Tạo các proto file trong `proto/runtime/`.
2. Generate C++ target dùng chung cho server và test client.
3. Implement gRPC server adapter gọi runtime core.
4. Implement process config gồm TCP address, port, database path và runtime id.
5. Implement health/readiness tối thiểu nếu cần cho smoke test.
6. Implement graceful shutdown.
7. Viết process-level smoke test.
8. Viết polling client cho toàn bộ API MVP.

## Test matrix

- process chạy không cần editor;
- deploy graph hợp lệ và không hợp lệ;
- activate khi stopped;
- activate khi running bị từ chối;
- start/stop qua client;
- publish event và query history;
- handler failure query được;
- mailbox-full rejection query được;
- idempotency duplicate và conflict;
- restart process đọc lại state;
- plaintext TCP disconnect/reconnect;
- protocol version không tương thích.

## Exit criteria

- Process chạy độc lập với UI và QML.
- Client hoàn tất vertical slice Producer -> Processor -> Sink qua gRPC.
- Client query được status, history, logs và rejection.
- Restart process giữ được state cơ bản.
- Không có streaming, authentication, TLS, retry scheduler hoặc dead-letter dependency bắt buộc.
- Có tài liệu lệnh chạy smoke test.
