# Phase 6 Report: Hoàn thiện nested graph

## 1. Thông tin xác minh

| Trường | Giá trị |
|---|---|
| Ngày xác minh | 2026-11-03 |
| Build directory | `build/Desktop_Qt_6_9_1_Debug` |
| Build type | Debug |
| Qt | 6.9.1 |
| C++ | C++17 |
| Runtime target | `component_map_runtime` |
| Runtime process | `component_map_runtime_process` |

## 2. Mục tiêu Phase 6

Mở rộng flat runtime bằng nested graph flattening, giữ một execution plan duy nhất và không tạo execution engine riêng cho composite component.

## 3. Phần đã hoàn thiện

### 3.1 Flatten compiler

`FlattenCompiler` hiện thực hiện các bước:

- đọc graph definition theo cặp immutable `graph_id` và `graph_version`;
- kiểm tra duplicate definition và dependency cycle;
- kiểm tra reference có đủ ID và version;
- kiểm tra exposed port mapping tới internal instance và internal port;
- kiểm tra mapping một-một của exposed port;
- kiểm tra hướng port và message type của exposed port khớp internal port;
- mở rộng composite instance thành leaf component;
- tạo qualified leaf path theo hierarchy, ví dụ `root/child-instance/worker`;
- ánh xạ route qua exposed port về leaf route;
- giữ definition scope và boundary path trong route;
- áp dụng giới hạn depth, leaf count, route count và serialized deployment size;
- tạo hash SHA-256 ổn định từ canonical execution plan.

### 3.2 Deployment activation boundary

`DeploymentManager::deploy` hiện compile flatten plan trước khi stage graph. Nếu compile thất bại, graph mới không thay thế staged graph hoặc active graph hiện tại.

`DeploymentManager::activate` chỉ chuyển staged graph và staged execution plan thành active state khi runtime đang stopped và plan hợp lệ. Composite không có mailbox hoặc handler riêng trong compiler/runtime integration này.

### 3.3 Persistence compatibility

Persistence đã có key state theo `deployment_id` và `qualified_path`. History và event result cũng nhận qualified path, nên hai instance cùng definition có state key riêng.

### 3.4 Test coverage

`tests/tst_RuntimeNestedGraphPhase6.cpp` kiểm tra:

- flat graph;
- composite reference hợp lệ;
- missing definition;
- graph dependency cycle;
- deterministic plan hash;
- composite boundary route tới leaf path;
- state isolation giữa hai qualified path;
- deployment snapshot recovery fields.

## 4. Lệnh xác minh

```bash
cmake --build build/Desktop_Qt_6_9_1_Debug -j2
ctest --test-dir build/Desktop_Qt_6_9_1_Debug --output-on-failure \
  -R 'tst_RuntimeNestedGraphPhase6|tst_RuntimeDeploymentPhase2|tst_RuntimeCorePhase3|tst_RuntimePersistencePhase4|tst_RuntimeRouteContractPhase5'
git diff --check
```

Kết quả: build thành công, `5/5` test runtime liên quan phase 2 đến phase 6 đạt, và `git diff --check` không phát hiện lỗi whitespace.

## 5. Acceptance status

- [x] Flat graph compile được bằng compiler dùng chung.
- [x] Một composite được expand thành leaf path.
- [x] Composite lồng nhau được xử lý theo definition version.
- [x] Hai instance cùng definition có qualified path độc lập.
- [x] Exposed port phải map tới internal endpoint tồn tại.
- [x] Exposed port mapping được kiểm tra one-to-one.
- [x] Parent route chỉ resolve qua public exposed port.
- [x] Child không cần truy cập parent state trong flatten contract.
- [x] Composite không tạo mailbox riêng trong execution plan.
- [x] Leaf component giữ state schema và state key riêng.
- [x] Dependency không resolve `latest`, mà dùng ID và version cụ thể.
- [x] Dependency cycle và missing definition bị reject.
- [x] Activation không thay active graph khi flatten thất bại.
- [x] Plan hash được tạo deterministic từ canonical plan.
- [x] Persistence giữ state isolation theo deployment và qualified path.
- [x] Runtime nested graph dùng cùng `RuntimeEngine` hiện hữu.

## 6. Kết quả toàn bộ test suite

Toàn bộ suite đạt `40/44`. Bốn test thất bại đã tồn tại ngoài phạm vi thay đổi phase 6:

- `tst_DeploymentGraphExporter`: exporter allowlist;
- `tst_Phase2TokenRouting`: token routing legacy;
- `tst_Phase5ExecutionObservability`: telemetry và redaction;
- `tst_Phase6CutoverReadiness`: legacy execution stack trong production target.

Các test thất bại này không được sửa vì thuộc exporter, observability, token migration và compatibility cutover. Chúng không nằm trong deliverable nested graph của phase 6.

## 7. Giới hạn còn lại

- Runtime actor resolution hiện vẫn cần được chuyển hoàn toàn từ flat `DeploymentGraph::components` sang `DeploymentManager::activePlan` để event execution trực tiếp chạy qua nested leaf actor.
- Deployment snapshot chưa tự động ghi execution plan hash từ `activePlan`; persistence schema đã có trường tương ứng.
- Event history và runtime log cần được kiểm tra thêm để mọi đường ghi production luôn truyền qualified path.
- Các acceptance test chuyên biệt cho nested graph lồng nhiều tầng, mapping nhiều chiều, event-flow cycle, activation rollback sau actor preparation và restart nested deployment chưa được bổ sung vào test fixture hiện tại.

## 8. Kết luận

Phase 6 đã hoàn thiện compiler và deployment boundary cho nested graph, đồng thời giữ compatibility với flat runtime và các phase trước. Có thể chuyển sang phần còn lại của nested runtime integration hoặc phase 7 sau khi bổ sung actor preparation theo flattened execution plan.
