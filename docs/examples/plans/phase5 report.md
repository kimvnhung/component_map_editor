# Phase 5 Report: Chuẩn hóa Graph và Route Contract

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

## 2. Mục tiêu Phase 5

Chuẩn hóa deployment artifact để editor và runtime dùng cùng graph, port và route contract. Runtime chỉ đọc protobuf artifact, không đọc `GraphModel` hoặc dynamic property của editor.

## 3. Phần đã hoàn thiện

### 3.1 Graph definition

`DeploymentGraph` và `GraphDefinition` có các trường:

- `graph_id`;
- `graph_version`;
- component instances;
- component type và instance ID;
- runtime properties;
- state schema version;
- composite graph reference theo ID và version.

Runtime kiểm tra graph version, component type, handler registration, instance ID trùng lặp và graph definition dependency cycle.

### 3.2 Port descriptor

`ComponentPort` có các trường:

- `port_id`;
- direction;
- `message_type`;
- cardinality;
- required;
- `token_key` cho migration tương thích.

Validator kiểm tra port ID, duplicate port ID, direction và message type. Cardinality unspecified được coi là default single trong artifact Phase 5 và phát cảnh báo.

### 3.3 Runtime route contract

`DirectRoute` có các trường:

- connection ID;
- source instance và source port;
- target instance và target port;
- message type;
- delivery mode;
- ordering key;
- filter;
- enabled;
- `token_key` cho migration tương thích.

Validator kiểm tra endpoint tồn tại, hướng source output, hướng target input, message type tương thích, connection ID trùng lặp, disabled route và delivery mode mặc định.

### 3.4 Migration từ tokenKey

Runtime ưu tiên `message_type`. Khi artifact Phase 5 chỉ có `token_key`, validator dùng token key làm message type hiệu dụng và phát cảnh báo `TOKEN_KEY_DEPRECATED`. Artifact cũ của Phase 1 đến Phase 4 vẫn được đọc mà không phá vỡ regression test.

### 3.5 Test contract

Bổ sung `tst_RuntimeRouteContractPhase5`, bao phủ:

- typed route hợp lệ;
- duplicate instance và port;
- sai hướng port;
- message type không tương thích;
- delivery mode unspecified;
- token key migration warning.

## 4. Lệnh xác minh

```bash
cmake --build build/Desktop_Qt_6_9_1_Debug -j2
ctest --test-dir build/Desktop_Qt_6_9_1_Debug \
  --output-on-failure \
  -R 'tst_RuntimeRouteContractPhase5|tst_RuntimeProcessPhase5|tst_RuntimeServicePhase1|tst_RuntimeDeploymentPhase2|tst_RuntimeCorePhase3|tst_RuntimeActorSchedulerPhase3|tst_RuntimePersistencePhase4|tst_RuntimeExtensionPhase4'
git diff --check
```

Kết quả: build thành công, `8/8` test Phase 1 đến Phase 5 đạt, và `git diff --check` không phát hiện lỗi whitespace.

Test toàn bộ repository vẫn còn các lỗi có sẵn hoặc thuộc phase khác: `tst_DeploymentGraphExporter`, `tst_Phase2TokenRouting`, `tst_Phase5ExecutionObservability` và `tst_Phase6CutoverReadiness`. Các lỗi này không được sửa trong Phase 5 vì nằm ngoài phạm vi contract graph/route hiện tại.

## 5. Acceptance status

- [x] Graph artifact có graph ID và graph version.
- [x] Component instance có definition/type, instance ID, properties và state schema version.
- [x] Port descriptor có direction, message type, cardinality và required.
- [x] Route descriptor có connection ID, endpoints, message type, delivery mode, ordering key, filter và enabled.
- [x] Duplicate instance ID được phát hiện.
- [x] Missing component type được phát hiện.
- [x] Missing port và sai port direction được phát hiện.
- [x] Incompatible message type được phát hiện.
- [x] Disabled route được biểu diễn bằng warning.
- [x] Duplicate route ID được phát hiện.
- [x] Missing graph version được phát hiện.
- [x] Composite graph reference và dependency cycle được kiểm tra.
- [x] Runtime ưu tiên `message_type` và hỗ trợ cảnh báo `token_key` legacy.
- [x] Runtime validation không cần `GraphModel` hoặc dynamic property của editor.
- [x] Regression contract của các phase trước được giữ tương thích.

## 6. Giới hạn còn lại

- [ ] Exporter hiện vẫn lấy một số port metadata từ dynamic property của editor để tạo artifact. Đây là input của editor exporter, không phải nguồn dữ liệu của runtime.
- [ ] Mailbox và concurrency configuration chưa có field riêng trong protobuf graph contract. Runtime vẫn dùng cấu hình scheduler hiện có.
- [ ] Validation nested boundary mới kiểm tra composite reference và exposed-port uniqueness. Quy tắc mapping one-to-one đầy đủ thuộc Phase 6.
- [ ] Chưa loại bỏ `token_key`; cần một phase migration riêng để đặt thời hạn loại bỏ.

## 7. Kết luận

Phase 5 đạt mục tiêu chuẩn hóa contract ở runtime boundary và có test regression cho graph, port, route và token migration. Có thể chuyển sang Phase 6 với điều kiện giữ nguyên artifact contract này và mở rộng validation nested boundary mà không tạo execution semantics thứ hai.
