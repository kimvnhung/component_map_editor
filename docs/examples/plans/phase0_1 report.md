# Phase 0 và Phase 1 Report

## 1. Thông tin xác minh

| Trường | Giá trị |
|---|---|
| Ngày xác minh | 2026-11-02 |
| Build directory | `build/Desktop_Qt_6_9_1_Debug` |
| Build type | Debug |
| Qt | 6.9.1 |
| C++ | C++17 |
| `CME_BUILD_EXAMPLES` | `ON` |
| `CME_BUILD_RUNTIME` | `ON` |
| `CME_BUILD_TESTS` | `ON` |

## 2. Phase 0: Baseline và dependency boundary

### 2.1 Target đã xác minh

Các target sau build độc lập thành công:

- `component_map_runtime`
- `libComponentMapEditor`
- `cme_runtime_client`
- `component_map_runtime_process`
- `cme_editor_compat`

### 2.2 Boundary đã kiểm tra

- Runtime library không link `libComponentMapEditor`.
- Runtime process chỉ link `component_map_runtime`.
- Runtime target không include Qt Quick hoặc QML.
- Compatibility implementation được giữ trong `cme_editor_compat`.
- Editor QML module được đóng gói với URI `ComponentMapEditor` version `1.0`.
- QML module phát sinh type information file `ComponentMapEditor.qmltypes`.
- QML import contract được kiểm tra bằng test độc lập.

### 2.3 QML module contract

Test `tst_EditorQmlModuleContract` xác nhận application có thể import module công khai:

```qml
import ComponentMapEditor
```

Test khởi tạo và sử dụng các type chính:

- `GraphModel`
- `TypeRegistry`
- `UndoStack`
- `GraphEditorController`
- `GraphCanvas`
- `Palette`
- `PropertyPanel`
- `ExecutionPanel`

Test cũng tạo một component instance thông qua `GraphEditorController` và xác nhận graph model nhận instance đó.

## 3. Phase 1: Flat runtime vertical slice

### 3.1 Graph fixture

Integration test sử dụng graph cố định:

```text
Producer → Processor → Sink
```

Graph gồm:

- ba component instance;
- port input và output;
- message type `test.event`;
- hai route ID ổn định;
- state riêng cho từng component;
- một event đầu vào;
- output được route qua cả hai connection.

### 3.2 Process boundary

Test `tst_RuntimeProcessPhase5` khởi động executable thật:

```text
component_map_runtime_process
```

Test không khởi tạo `RuntimeGrpcServer` trực tiếp trong test process. Test giao tiếp với runtime qua gRPC bằng các service:

- `RuntimeControl`;
- `RuntimeEvents`;
- `RuntimeQuery`.

### 3.3 Lifecycle đã xác minh

Chuỗi sau đã pass qua process boundary:

```text
start process
→ deploy graph
→ activate graph
→ start runtime
→ publish event
→ process event
→ query component status
→ query event history
→ query runtime logs
→ stop runtime
```

### 3.4 Kết quả runtime đã xác minh

- Event được accept qua gRPC.
- Event đi qua Producer, Processor và Sink.
- State của Sink được cập nhật thành `payload`.
- Query status trả về đúng component ID và component type.
- Event history chứa ba execution record tương ứng với ba component.
- Các execution record có trạng thái `completed`.
- Runtime logs chứa execution record cho các component đã xử lý event.
- UI không cần khởi động.

## 4. Thay đổi implementation

### Editor QML module

- Bổ sung QML type information contract trong `component_map_editor/CMakeLists.txt`.
- Bổ sung `tst_EditorQmlModuleContract.cpp`.
- Đăng ký test QML module trong `tests/CMakeLists.txt`.

### Runtime status query

`GetComponentStatus` hiện trả về `component_type` bằng cách tra cứu component trong active deployment graph.

### Runtime execution logs

Successful event processing ghi log với mã:

```text
EVENT_PROCESSED
```

### Runtime process integration test

`tests/tst_RuntimeProcessPhase5.cpp` được chuyển từ in-process gRPC server test thành process-boundary integration test.

## 5. Kết quả test Phase 0 và Phase 1

| Test | Kết quả |
|---|---|
| `tst_EditorQmlModuleContract` | PASS |
| `tst_RuntimeServicePhase1` | PASS |
| `tst_RuntimeDeploymentPhase2` | PASS |
| `tst_RuntimeCorePhase3` | PASS |
| `tst_RuntimeProcessPhase5` | PASS |

### Tổng kết

```text
5/5 Phase 0/1 tests passed
```

Lệnh chạy nhóm test:

```bash
ctest --test-dir build/Desktop_Qt_6_9_1_Debug \
  --output-on-failure \
  -R 'tst_EditorQmlModuleContract|tst_RuntimeServicePhase1|tst_RuntimeDeploymentPhase2|tst_RuntimeCorePhase3|tst_RuntimeProcessPhase5'
```

## 6. Kết quả complete suite

Complete suite có `39` test được đăng ký. Có `35` test pass và `4` test fail.

Các test fail nằm ngoài phạm vi Phase 0 và Phase 1:

- `tst_DeploymentGraphExporter`
- `tst_Phase2TokenRouting`
- `tst_Phase5ExecutionObservability`
- `tst_Phase6CutoverReadiness`

Các failure này không được sửa trong report này để tránh mở rộng phạm vi sang các phase khác.

## 7. Giới hạn còn lại

Runtime process hiện chưa có graceful shutdown qua signal hoặc shutdown RPC riêng. Integration test gọi `StopRuntime` để dừng runtime state, sau đó kết thúc process test bằng cơ chế process cleanup.

Việc bổ sung signal handling hoặc shutdown RPC nên được xử lý ở phase hardening, không thuộc vertical slice tối thiểu hiện tại.

## 8. Acceptance status

### Phase 0

- [x] Các target runtime, editor, runtime client và compatibility build độc lập.
- [x] Editor QML module có URI và version rõ ràng.
- [x] QML public import contract có automated test.
- [x] Runtime không phụ thuộc QML hoặc editor implementation.
- [x] Baseline command và test result được ghi lại.

### Phase 1

- [x] Flat graph Producer → Processor → Sink.
- [x] Deploy và activate graph qua gRPC.
- [x] Start và stop runtime qua gRPC.
- [x] Publish event qua gRPC.
- [x] Event được route qua các component.
- [x] Query component status.
- [x] Query event history.
- [x] Query runtime logs.
- [x] Integration test chạy với process runtime thật.
- [x] UI không cần khởi động.
