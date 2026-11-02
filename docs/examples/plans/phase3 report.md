# Phase 3 Report: Actor Scheduler và Runtime Handler Contract

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

## 2. Mục tiêu Phase 3

Phase 3 chuyển runtime từ đường xử lý đồng bộ phụ thuộc `processAll()` sang actor scheduler có mailbox và worker execution. Đường `processAll()` vẫn tồn tại cho test và compatibility mode, nhưng runtime process production bật background execution.

## 3. Phần đã hoàn thiện

### 3.1 Runtime handler contract

Runtime handler có thêm contract lifecycle:

- `RuntimeContext` chứa component identity, state và cancellation state.
- `RuntimeEmitter` hỗ trợ nhiều output event và delayed output abstraction.
- `onStart` được cung cấp cho lifecycle initialization.
- `onEvent` là điểm xử lý event production.
- `onStop` được cung cấp cho lifecycle shutdown.
- Handler cũ dùng `process()` vẫn hoạt động qua compatibility adapter mặc định.
- Handler failure được chuyển thành `RuntimeHandlerResult`.

### 3.2 Actor scheduler

`RuntimeActorScheduler` cung cấp:

- actor-level serialization;
- FIFO ordering trong cùng actor;
- runtime-level parallelism giữa actor khác nhau;
- bounded mailbox;
- explicit backpressure rejection;
- start và stop lifecycle;
- wait-for-idle cho test và graceful coordination;
- mặc định `maxConcurrency = 1`.

Scheduler không để handler tự tạo thread. Worker pool do scheduler sở hữu.

### 3.3 Runtime integration

`RuntimeEngine` hiện hỗ trợ background execution opt-in:

- `setBackgroundExecution(true, maxConcurrency)` bật scheduler.
- Runtime process bật background execution với concurrency mặc định bằng một.
- Event publish tự submit actor vào scheduler.
- Cùng actor không xử lý đồng thời.
- Actor khác nhau có thể chạy trên worker khác nhau khi concurrency được cấu hình lớn hơn một.
- `processAll()` chỉ chạy khi background execution tắt.
- Stop runtime dừng scheduler trước khi xóa actor mailbox.

### 3.4 Error và persistence behavior

Đường processing hiện vẫn phân biệt các lỗi chính:

- validation failure;
- handler failure;
- mailbox rejection;
- routing failure;
- persistence failure;
- runtime not running.

Event state và completion vẫn dùng persistence transaction boundary của Phase 2. Event chưa hoàn tất tiếp tục được khôi phục theo inbox policy.

## 4. Test đã chạy

| Test | Kết quả |
|---|---|
| `tst_RuntimeServicePhase1` | PASS |
| `tst_RuntimeDeploymentPhase2` | PASS |
| `tst_RuntimeCorePhase3` | PASS |
| `tst_RuntimeActorSchedulerPhase3` | PASS |
| `tst_RuntimePersistencePhase4` | PASS |

Test scheduler bao phủ:

- FIFO ordering;
- không đồng thời trong cùng actor;
- parallelism giữa hai actor độc lập;
- mailbox đầy và rejection;
- stop từ chối task mới.

Test runtime background bao phủ:

- event được xử lý mà không gọi `processAll()`;
- state được cập nhật từ worker scheduler;
- runtime stop sau background processing.

Lệnh xác minh:

```bash
cmake --build build/Desktop_Qt_6_9_1_Debug -j2
ctest --test-dir build/Desktop_Qt_6_9_1_Debug \
  --output-on-failure \
  -R 'tst_RuntimeServicePhase1|tst_RuntimeDeploymentPhase2|tst_RuntimeCorePhase3|tst_RuntimeActorSchedulerPhase3|tst_RuntimePersistencePhase4'
git diff --check
```

Kết quả: `5/5` test đạt.

## 5. Acceptance status

- [x] Runtime handler lifecycle contract được định nghĩa.
- [x] Runtime context và emitter được tách khỏi graph traversal.
- [x] Actor mailbox có giới hạn.
- [x] FIFO ordering trong actor.
- [x] Không xử lý đồng thời cùng actor.
- [x] Có worker pool cho actor độc lập.
- [x] Backpressure là explicit rejection.
- [x] Runtime process chạy background không cần request gọi `processAll()`.
- [x] `processAll()` chỉ còn cho compatibility/test khi background mode tắt.
- [x] Stop runtime không nhận event mới.
- [x] Persistence và recovery của Phase 2 vẫn pass regression.
- [ ] Output event chưa được tách thành outbox delivery worker hoàn chỉnh.
- [ ] Error object chưa có đầy đủ route ID và correlation ID ở mọi đường gRPC.
- [ ] Handler domain thật vẫn đang nằm ở static builtin registration, chưa chuyển sang extension pack.

## 6. Giới hạn và bước kế tiếp

Phase 3 đã hoàn thiện actor scheduler nền và tích hợp background execution. Phần còn lại của Phase 3 là nâng output event thành danh sách first-class trong RuntimeEngine, nối outbox delivery, chuẩn hóa error model có retryability, và bổ sung test handler failure isolation giữa các actor.
