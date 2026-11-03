# Phase 4 Report: Runtime Extension Pack

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

## 2. Khả năng bắt đầu Phase 4

Có thể thực hiện Phase 4 vì Phase 3 đã có actor scheduler, runtime handler lifecycle contract và runtime process background execution. Phase 4 được triển khai theo lựa chọn static registration cho MVP, đúng kế hoạch. Dynamic shared-library loading chưa được đưa vào phase này.

## 3. Phần đã hoàn thiện

### 3.1 Runtime extension contract

Bổ sung các descriptor độc lập với editor và QML:

- component type;
- handler ID;
- extension version;
- component version;
- state schema version;
- input message types;
- output message types;
- runtime handler factory.

Runtime extension contract chỉ dùng Qt Core value types và runtime handler interfaces. Runtime không include QObject, QML hoặc editor model.

### 3.2 Runtime handler registry

`RuntimeHandlerRegistry` hiện hỗ trợ:

- đăng ký factory đơn lẻ cho compatibility;
- đăng ký một `RuntimeExtensionPack`;
- kiểm tra component type trùng lặp;
- kiểm tra factory còn thiếu;
- lưu component definition;
- truy vấn extension owner;
- tạo handler từ component type;
- truy vấn danh sách component type.

Đăng ký extension thất bại nguyên tử ở cấp validation trước khi thêm definition vào registry.

### 3.3 Static factory extension pack

Tạo builtin factory extension pack với các component type:

- `factory/manager`;
- `factory/fruit_producer`;
- `factory/store`;
- `factory/wash_machine`;
- `factory/seller`.

Mỗi component có handler ID, version, state schema version và factory runtime tương ứng. Runtime process không còn định nghĩa handler business trực tiếp trong `main.cpp`; `main.cpp` chỉ đăng ký extension pack.

### 3.4 Boundary

Editor provider và runtime handler được giữ tách biệt. Runtime extension pack không dùng `FactoryComponentTypeProvider`, QML type hoặc `IExecutionSemanticsProvider` làm production runtime contract.

## 4. Test đã chạy

| Test | Kết quả |
|---|---|
| `tst_RuntimeServicePhase1` | PASS |
| `tst_RuntimeDeploymentPhase2` | PASS |
| `tst_RuntimeCorePhase3` | PASS |
| `tst_RuntimeActorSchedulerPhase3` | PASS |
| `tst_RuntimePersistencePhase4` | PASS |
| `tst_RuntimeExtensionPhase4` | PASS |

Test extension bao phủ:

- static factory pack đăng ký đủ component definitions;
- handler ID và state schema version;
- extension ownership lookup;
- tạo handler từ registry;
- duplicate registration rejection;
- thiếu factory rejection;
- registry không cần editor type.

Lệnh xác minh:

```bash
cmake --build build/Desktop_Qt_6_9_1_Debug -j2
ctest --test-dir build/Desktop_Qt_6_9_1_Debug \
  --output-on-failure \
  -R 'tst_RuntimeExtensionPhase4|tst_RuntimeServicePhase1|tst_RuntimeDeploymentPhase2|tst_RuntimeCorePhase3|tst_RuntimeActorSchedulerPhase3|tst_RuntimePersistencePhase4'
git diff --check
```

Kết quả: `6/6` test đạt.

## 5. Acceptance status

- [x] Runtime extension contract độc lập với editor và QML.
- [x] Component definition có handler ID và version.
- [x] State schema version được lưu trong definition.
- [x] Runtime handler factory được registry quản lý.
- [x] Supported input và output message types được mô tả.
- [x] Duplicate component registration bị từ chối.
- [x] Missing factory bị từ chối.
- [x] Static registration hoạt động cho MVP.
- [x] Runtime process sử dụng extension pack thay vì business handler trong `main.cpp`.
- [x] Factory domain component types đã được đăng ký.
- [ ] Handler behavior chi tiết của từng fruit factory component chưa thay thế hoàn toàn bằng semantics nghiệp vụ trong example extension.
- [ ] Dynamic extension loading chưa triển khai.
- [ ] Extension compatibility migration giữa state schema versions chưa triển khai.

## 6. Giới hạn và bước kế tiếp

Phase 4 đạt static extension registration MVP. Bước tiếp theo theo kế hoạch là Phase 5, chuẩn hóa graph definition, port descriptor và runtime route contract. Dynamic loading, state schema migration và domain-specific failure semantics có thể được bổ sung sau khi graph contract ổn định.
