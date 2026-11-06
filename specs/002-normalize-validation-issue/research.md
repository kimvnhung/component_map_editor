# Research: Chuẩn hóa structured validation issue

**Feature**: `002-normalize-validation-issue`
**Date**: 2026-11-06

Phase 0 consolidates findings từ khảo sát mã nguồn hiện tại (`proto/`, `component_map_editor/services`, `component_map_runtime`) để giải quyết các điểm chưa rõ (NEEDS CLARIFICATION) trước khi thiết kế.

## R1 — Hợp đồng issue hiện tại và sự bất nhất

**Decision**: Hợp nhất thành một shared `ValidationIssue` dùng trường `path` làm định danh ổn định, thay vì duy trì `component_id`/`connection_id` (editor) và `field_path`/`component_identity` (runtime) song song.

**Rationale**:
- Khảo sát `proto/validation.proto`: editor dùng `ValidationIssue { code, severity, message, component_id, connection_id }`.
- Khảo sát `proto/deployment.proto`: runtime dùng `DeploymentValidationIssue { code, severity, field_path, message, component_identity, component_version }`.
- `RuntimeClient::deploymentValidationIssueMap` map runtime issue về editor shape khác (`fieldPath`, `componentIdentity`, không có `componentId`/`connectionId`), nghĩa là cùng một lỗi logic có shape khác nhau giữa editor và runtime → vi phạm mục tiêu "code/severity/path tương thích".
- Dùng một trường `path` duy nhất (`components/<id>...`, `connections/<id>...`, `graph`) loại bỏ sự phân mảnh trường và là khóa ổn định để automation tham chiếu.

**Alternatives considered**:
- Giữ hai message và chỉ chuẩn hóa `code`/`severity`: bác bỏ vì `path` vẫn không thống nhất (index vs identity, field_path vs component_id) → SC-001/SC-004 không đạt.
- Dùng `component_id`+`connection_id` làm khóa thay vì `path`: bác bỏ vì không biểu diễn được property/port path và lỗi root graph một cách đồng nhất.

## R2 — Quy ước `path` (đã clarify với user)

**Decision**: `path` dùng định danh ổn định: `components/<componentId>.properties.<prop>`, `connections/<connectionId>.<field>`, và `graph` cho lỗi toàn graph. Không dùng chỉ mục mảng.

**Rationale**: Chỉ mục mảng thay đổi khi thứ tự entity thay đổi, phá vỡ tính ổn định mà acceptance yêu cầu (SC-004: hằng số qua ≥10 lần chạy và qua editor/runtime). Định danh thực thể là bất biến theo SDD (graph/component/connection có stable identifier).

**Alternatives considered**:
- Chỉ mục mảng `components[2].properties.x`: bác bỏ (không ổn định).
- Path dùng tên hiển thị/title: bác bỏ (không unique, locale-dependent).

## R3 — Đường "chuyển thành chuỗi" cần sửa

**Decision**: Mọi đường trả validation issue MUST trả structured list (`QVariantList` của issue map, hoặc proto `repeated ValidationIssue`); chỉ tầng UI format thành text. `ValidationService::validationErrors(QStringList)` và `validate()` giữ làm tiện ích UI/text nhưng không được dùng làm nguồn truyền giữa các tầng.

**Rationale**: Khảo sát `ValidationService::validationErrors` gom message thành `QStringList`, và `validate()` chỉ trả bool. Đây là "hiện trạng một số đường UI/API chuyển kết quả thành chuỗi". Giữ structured ở service/adapter/client/manager, UI-only formatting thỏa mãn FR-006/FR-010.

**Alternatives considered**: Giữ `QStringList` làm contract giữa tầng: bác bỏ (mất `code`/`path`/structured diagnostics).

## R4 — Chuẩn hóa error code

**Decision**: Áp dụng một hàm normalize thống nhất (`stableDiagnosticCode` đã có ở `RuntimeClient`, đưa thành shared util/catalog) cho cả editor và runtime để `code` ổn định (UPPER_SNAKE_CASE). Cung cấp `DiagnosticCodeCatalog` liệt kê các mã tối thiểu theo SDD F2.4.

**Rationale**: `RuntimeClient::stableDiagnosticCode` đã normalize runtime code; editor hiện phát sinh code thô (ví dụ `CORE_NULL_GRAPH`). Thống nhất qua catalog đảm bảo cùng lỗi logic → cùng `code` (SC-001).

**Alternatives considered**: Để mỗi tầng tự đặt code: bác bỏ (không tương thích editor/runtime).

## R5 — Backward compatibility trong quá trình chuyển đổi

**Decision**: Giữ `DeploymentValidationIssue` (runtime proto) nguyên hình dạng trên wire; thêm adapter `DeploymentValidationAdapter` map 2 chiều sang unified `ValidationIssue` ở editor client. Editor proto `ValidationIssue` được mở rộng thêm `path`, `details`, `remediation` (field mới, backward-compatible vì proto3 optional/tự thêm).

**Rationale**: Tránh breaking runtime contract đột ngột; cho phép editor và runtime deploy độc lập (Constitution III: runtime độc lập editor).

**Alternatives considered**: Đổi cả runtime proto cùng lúc: rủi ro cao, vi phạm independent runtime/deployment; bác bỏ.

## R6 — Severity semantics

**Decision**: Giữ enum 3 mức (`ERROR`, `WARNING`, `INFO`) cho nội bộ, nhưng contract công khai chỉ phân biệt `error` (chặn stage/activate) và `warning` (không chặn, MUST hiển thị/ghi nhận) theo FR-005/SDD D9. `INFO` không chặn và không bắt buộc hiển thị nổi bật.

**Rationale**: SDD D9 chỉ định error chặn, warning cho phép nhưng MUST ghi nhận. `INFO` hiện có trong `validation.proto` giữ làm mức thấp hơn warning, không ảnh hưởng gate.

## Open items resolved

- NEEDS CLARIFICATION về `path` convention → đã clarify (R2).
- NEEDS CLARIFICATION về backward compat → R5.
- Không còn mục NEEDS CLARIFICATION trong Technical Context.
