# Phase 9 Operations Runbook

Tài liệu này mô tả đường chạy kiểm chứng runtime process độc lập với editor.

## 1. Build

Từ thư mục gốc project:

```bash
cmake -S . -B build/phase9 \
  -DCMAKE_PREFIX_PATH=/path/to/Qt6 \
  -DCME_BUILD_RUNTIME=ON \
  -DCME_BUILD_TESTS=ON \
  -DCME_BUILD_EXAMPLES=ON
cmake --build build/phase9 -j2
```

Nếu build directory đã được configure với Qt, chỉ cần:

```bash
cmake --build build/phase9 -j2
```

Runtime process là target `component_map_runtime_process`.

## 2. Start runtime

```bash
build/phase9/component_map_runtime/component_map_runtime_process \
  --address 127.0.0.1:50051 \
  --database ./component_map_runtime.sqlite \
  --runtime-id component-map-runtime
```

Các option:

| Option | Mặc định | Ý nghĩa |
|---|---|---|
| `--address` | `127.0.0.1:50051` | gRPC endpoint |
| `--database` | `component_map_runtime.sqlite` | SQLite database |
| `--runtime-id` | `component-map-runtime` | định danh runtime và namespace persistence |

Runtime process không load QML và không phụ thuộc editor compatibility target.

## 3. Smoke flow

Dùng test process-level để thực hiện flow đầy đủ qua gRPC:

```bash
ctest --test-dir build/phase9 --output-on-failure \
  -R 'tst_RuntimeProcessPhase5|tst_RuntimeNestedGraphPhase6'
```

Flow gồm:

1. kết nối runtime;
2. deploy graph;
3. activate graph;
4. start runtime;
5. publish event;
6. query component status;
7. query event history và logs;
8. stop process;
9. restart với cùng database;
10. xác nhận dữ liệu persistence được phục hồi.

## 4. Database và recovery

- Chọn database bằng `--database`.
- Không xóa database khi cần kiểm chứng recovery.
- Để tạo run sạch, dừng runtime và dùng một đường dẫn database mới.
- Khi database không mở được, runtime ghi lỗi ra stderr và thoát với exit code khác `0`.
- Khi runtime bị dừng, khởi động lại bằng cùng `--runtime-id` và `--database` để kiểm tra state/history đã lưu.

Các test persistence:

```bash
ctest --test-dir build/phase9 --output-on-failure \
  -R 'tst_RuntimePersistencePhase4|tst_PersistenceValidation'
```

## 5. Kiểm tra lỗi vận hành

### Endpoint không khả dụng

Kiểm tra process đang chạy và endpoint có đúng với `--address`. Runtime client phải báo lỗi kết nối, không được coi request là thành công.

### Handler failure

Kiểm tra status, history và logs sau event. Event lỗi phải có trạng thái rejection/failure tương ứng; không được làm mất event history trước đó.

### Protocol mismatch hoặc malformed request

Dùng test contract/runtime để xác nhận request bị từ chối với lỗi có cấu trúc:

```bash
ctest --test-dir build/phase9 --output-on-failure \
  -R 'tst_RuntimeClient|tst_RuntimeDeploymentPhase2|tst_RuntimeRouteContractPhase5'
```

## 6. Full verification

```bash
cmake --build build/phase9 -j2
ctest --test-dir build/phase9 --output-on-failure
git diff --check
```

Kết quả phải được ghi vào report Phase 9 cùng với build directory, Qt version, số test pass/fail và các test được loại trừ có lý do.
