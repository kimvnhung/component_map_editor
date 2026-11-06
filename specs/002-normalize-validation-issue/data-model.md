# Data Model: Chuẩn hóa structured validation issue

**Feature**: `002-normalize-validation-issue`
**Date**: 2026-11-06

Trích xuất từ spec (FR-001…FR-010, Key Entities) và research (R1–R6). Mô hình này là contract chung, dùng bởi editor (`proto/validation.proto`), runtime (`proto/deployment.proto` qua adapter), exporter, client và manager. Chỉ UI format thành text.

## Entities

### ValidationIssue (unified, shared contract)

Đại diện một phát hiện của validation. Là message proto duy nhất dùng chung cho editor và runtime (runtime map qua adapter).

| Trường | Kiểu | Bắt buộc | Mô tả |
|---|---|---|---|
| `code` | string | YES | Machine-readable, UPPER_SNAKE_CASE đã normalize qua `stableDiagnosticCode`. Ví dụ `CORE_NULL_GRAPH`, `COMPONENT_PROPERTY_REQUIRED`, `DUPLICATE_COMPONENT_ID`. |
| `severity` | enum | YES | `ERROR` (chặn stage/activate), `WARNING` (không chặn, MUST hiển thị/ghi nhận), `INFO` (không chặn, tùy chọn hiển thị). Thiếu → xử lý như `ERROR` (fail-closed, FR edge case). |
| `path` | string | YES | Định danh ổn định của thực thể bị lỗi. `components/<componentId>.properties.<prop>`, `connections/<connectionId>.<field>`, hoặc `graph` cho lỗi toàn graph. KHÔNG dùng chỉ mục mảng. |
| `message` | string | YES | Human-readable, locale-independent nếu có thể. |
| `details` | map<string,string> hoặc string | NO | Ngữ cảnh bổ sung (ví dụ giá trị thực tế, expected). Giữ nguyên xuyên boundary nếu có. |
| `remediation` | string | NO | Gợi ý sửa (ví dụ "set startComponentId"). Giữ nguyên xuyên boundary nếu có. |

**Quan hệ**: `ValidationResult` chứa `repeated ValidationIssue`. Mỗi issue tham chiếu một thực thể trong `GraphDocument`/`DeploymentSnapshot` qua `path`.

### ValidationResult (GraphValidationResult / DeploymentValidationResult)

| Trường | Kiểu | Mô tả |
|---|---|---|
| `is_valid` / `valid` | bool | `false` nếu có bất kỳ issue `ERROR` nào. Warning không làm `is_valid=false`. |
| `issues` | repeated ValidationIssue | Danh sách issue, giữ cả error và warning. |

### DiagnosticCodeCatalog

Không phải entity truyền tải, mà là nguồn danh mục mã lỗi dùng chung (header/enum tĩnh). Phải cover ít nhất (SDD F2.4):

- `DUPLICATE_COMPONENT_ID`
- `MISSING_DESCRIPTOR`
- `MISSING_SCHEMA`
- `INVALID_PORT_METADATA`
- `MISSING_HANDLER_CAPABILITY`
- `INCOMPATIBLE_COMPONENT_METADATA`
- `PROVIDER_DISCOVERY_REGISTRATION_FAILURE`
- `COMPONENT_PROPERTY_REQUIRED` (property/required-field)
- `CORE_NULL_GRAPH`, `CORE_VALIDATION_PROVIDER_ERROR` (editor-internal)

## Validation Rules (từ requirements)

- `code`: không được rỗng; MUST qua `stableDiagnosticCode` normalize để editor/runtime đồng dạng.
- `severity`: MUST là một trong `ERROR`/`WARNING`/`INFO`; thiếu hoặc không hợp lệ → `ERROR`.
- `path`: MUST theo quy ước định danh ổn định; lỗi root → `graph`; không được rỗng khi issue thuộc entity cụ thể.
- `message`: không được rỗng.
- Error (`severity=ERROR`) trong result MUST đặt `is_valid=false` và chặn stage/activate.
- Warning MUST được giữ trong `issues` và hiển thị/ghi nhận, KHÔNG làm mất structured data của bất kỳ issue nào (FR-009).
- Cùng một lỗi logic (cùng `code` + `path`) MUST sinh `code`/`severity`/`path` tương thích giữa editor validation và runtime `ValidateDeployment`.

## State / Mapping (không phải state machine)

- Editor `ValidationIssue` (proto) hiện tại: `code, severity, message, component_id, connection_id`.
  → Map sang unified: `path = components/<component_id>...` hoặc `connections/<connection_id>...` hoặc `graph` khi cả hai rỗng.
- Runtime `DeploymentValidationIssue`: `code, severity, field_path, message, component_identity, component_version`.
  → Map sang unified: `path = <field_path>` (đã dùng định danh, giữ nguyên), `code` qua `stableDiagnosticCode`.
- Adapter MUST là nơi duy nhất thực hiện mapping; service/client/manager giữ unified shape.

## Key relationships

- `GraphDocument` / `DeploymentSnapshot` → được validate → sinh `ValidationResult` → chứa `ValidationIssue[]` tham chiếu entity qua `path`.
- `DiagnosticCodeCatalog` → cung cấp giá trị hợp lệ cho `ValidationIssue.code`.
- UI layer → nhận `ValidationResult` structured, format `message` (và `path`) thành text để hiển thị; không truyền text ngược lại làm contract.
