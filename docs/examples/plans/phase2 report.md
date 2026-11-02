# Phase 2 Report: Persistence và Recovery

## 1. Thông tin xác minh

| Trường | Giá trị |
|---|---|
| Ngày xác minh | 2026-11-02 |
| Build directory | `build/Desktop_Qt_6_9_1_Debug` |
| Build type | Debug |
| Qt | 6.9.1 |
| C++ | C++17 |
| Runtime target | `component_map_runtime` |
| Runtime process | `component_map_runtime_process` |

## 2. Phạm vi đã triển khai

Phase 2 hoàn thiện nền persistence cho một runtime instance và recovery sau process restart.

### 2.1 SQLite schema

Runtime persistence hiện tạo hoặc mở các bảng:

- `runtime_instances`
- `graph_versions`
- `deployment_snapshots`
- `component_state`
- `component_state_v2`
- `event_inbox`
- `event_outbox`
- `event_history`
- `component_history`
- `event_rejections`
- `runtime_logs`

`event_inbox` có các trạng thái `accepted`, `processing`, `completed`, `failed` và `rejected`. Schema có index cho status và idempotency lookup.

### 2.2 Transaction boundary

Persistence cung cấp transaction API gồm begin, commit và rollback. Đường xử lý thành công lưu component state và đánh dấu event hoàn tất trong cùng transaction. Nếu một thao tác thất bại, transaction được rollback và không commit state một phần.

### 2.3 Inbox recovery

Runtime lưu fingerprint, payload, target, message type và trạng thái của event trong inbox. Khi restart, `recover()` nạp lại component state và các event có trạng thái `accepted` hoặc `processing` vào mailbox để tiếp tục xử lý theo policy MVP.

Handler failure được ghi là `failed`; runtime không tự retry handler failure ngoài policy hiện tại.

### 2.4 Idempotency

Request có cùng idempotency key và cùng fingerprint được nhận diện là request cũ. Request có cùng key nhưng fingerprint khác bị từ chối với mã `IDEMPOTENCY_CONFLICT`. Idempotency record được khôi phục từ inbox sau restart.

### 2.5 Runtime status và observability

Runtime status có thể lưu và truy vấn theo runtime ID. Event history, rejection và runtime log tiếp tục được lưu độc lập với execution state.

## 3. Test đã chạy

| Test | Kết quả |
|---|---|
| `tst_RuntimeDeploymentPhase2` | PASS |
| `tst_RuntimeCorePhase3` | PASS |
| `tst_RuntimePersistencePhase4` | PASS |

Lệnh xác minh:

```bash
cmake --build build/Desktop_Qt_6_9_1_Debug -j2
ctest --test-dir build/Desktop_Qt_6_9_1_Debug \
  -R 'tst_RuntimePersistencePhase4|tst_RuntimeCorePhase3|tst_RuntimeDeploymentPhase2' \
  --output-on-failure
git diff --check
```

Kết quả cuối: `3/3` test đạt và không có lỗi whitespace trong diff.

Test persistence bao phủ:

- state và event history sống qua restart;
- rejection và log sống qua restart;
- inbox event đang `processing` được phát hiện sau restart;
- fingerprint sống qua restart;
- duplicate idempotency key cùng fingerprint;
- idempotency key khác fingerprint;
- reset execution state không xóa history và log.

## 4. Acceptance status

- [x] Database riêng cho runtime instance.
- [x] State component được lưu và khôi phục.
- [x] Event inbox có lifecycle status.
- [x] Event processing được phát hiện sau crash hoặc restart.
- [x] Idempotency fingerprint được lưu bền vững.
- [x] State update và completion nằm trong transaction boundary.
- [x] History, rejection và log sống qua restart.
- [x] Test restart với cùng database đạt.
- [ ] Outbox output event chưa được nối đầy đủ vào route delivery transaction.
- [ ] Chính sách lease hoặc timeout cho event `processing` sẽ được hoàn thiện cùng actor scheduler.

## 5. Giới hạn và bước kế tiếp

Phase 2 cung cấp persistence primitive và recovery cho runtime engine hiện tại. Phase 3 sẽ tách runtime handler contract khỏi execution semantics cũ, bổ sung actor mailbox và scheduler nền, sau đó nối outbox với output event của handler.
