# Quickstart: Chuẩn hóa structured validation issue

**Feature**: `002-normalize-validation-issue`
**Date**: 2026-11-06

Hướng dẫn xác minh end-to-end rằng validation issue đã được chuẩn hóa (structured, cùng `code`/`severity`/`path` giữa editor và runtime, warning không làm mất data). Không chứa implementation; tham chiếu `contracts/validation-issue.md` và `data-model.md`.

## Prerequisites

- Repo build được (Qt 6.9, CMake, Conan) — command build cụ thể lấy từ project root, không tự giả định.
- Runtime và editor dùng chung `proto/` (package `cme`, `cme.runtime.v1`).
- Có sẵn test harness Qt Test (`tests/`) và harness gRPC cho runtime.

## Validation scenarios

### S1 — Cùng lỗi logic sinh code/severity/path tương thích (editor vs runtime) [SC-001]

1. Chuẩn bị một graph cố định có lỗi: thiếu property bắt buộc trên component `C1` (ví dụ `inputARef`).
2. Chạy editor validation: `ValidationService::validationIssues(graph)` →期望 issue `{ code: COMPONENT_PROPERTY_REQUIRED, severity: error, path: components/C1.properties.inputARef }`.
3. Export graph → deployment snapshot → gọi runtime `ValidateDeployment(snapshot)` → expectation issue `{ code: COMPONENT_PROPERTY_REQUIRED, severity: error, path: components/C1.properties.inputARef }`.
4. **Assert**: `code`, `severity`, `path` khớp chính xác giữa (2) và (3) trên bộ test cases đã định nghĩa (100%).

### S2 — Structured issue giữ nguyên xuyên boundary [SC-002]

1. Từ graph lỗi ở S1, đẩy qua chuỗi: exporter → gRPC client → runtime `ValidateDeployment` → manager nhận result.
2. Ở mỗi boundary, serialize/deserialize issue và so sánh `code`/`severity`/`path`/`details`/`remediation`.
3. **Assert**: không boundary nào chuyển issue thành chuỗi không khôi phục được; structured shape bảo toàn 100%.

### S3 — Warning được hiển thị, không làm mất data [SC-003]

1. Graph có một warning (ví dụ component có thể deprecated nhưng hợp lệ) và không có error.
2. Gọi validation; **Assert**: `is_valid == true`, result chứa issue `severity=warning`, và `details`/`remediation` (nếu có) vẫn nguyên. UI hiển thị warning cùng result.
3. Graph có cả warning và error: **Assert** deployment bị chặn bởi error, nhưng warning vẫn nằm trong `issues` và hiển thị.

### S4 — `path` ổn định qua lần chạy và reorder [SC-004]

1. Graph cố định, chạy validation ≥10 lần; **Assert** `path` của mỗi issue logic là hằng số.
2. Đổi thứ tự component trong graph (không đổi id); **Assert** `path` không đổi (vì dùng định danh, không dùng chỉ mục).

### S5 — Automation tham chiếu issue bằng code + path [SC-005]

1. Viết test tham chiếu trực tiếp một issue bằng `code=COMPONENT_PROPERTY_REQUIRED` + `path=components/C1.properties.inputARef`.
2. **Assert**: test ổn định qua nhiều lần chạy và qua cả editor/runtime (dùng catalog `DiagnosticCodeCatalog`).

### S6 — Runtime không tin client string [FR-004, edge case]

1. Client gửi snapshot kèm `ValidationResult` đã được format thành chuỗi/tự claim valid.
2. **Assert**: runtime bỏ qua và tự re-validate, sinh structured issue riêng; không trả lại string của client.

### S7 — Fail-closed khi severity thiếu [edge case]

1. Issue có `severity` thiếu/không hợp lệ.
2. **Assert**: hệ thống xử lý như `ERROR` (chặn stage/activate), không bỏ qua.

## Expected outcomes

- Tất cả S1–S7 pass trên bộ test cases định nghĩa.
- Không còn đường nào trả validation result dưới dạng `QStringList` chuỗi làm contract liên tầng (chỉ UI text).
- `DiagnosticCodeCatalog` cover đủ các mã tối thiểu SDD F2.4.

## Notes

- Command build/test chính xác lấy từ project (xem `README.md` / instructions nội bộ); không tự giả định.
- Nếu runtime chưa expose `ValidateDeployment` trực tiếp cho test, dùng integration test qua control plane đã có.
