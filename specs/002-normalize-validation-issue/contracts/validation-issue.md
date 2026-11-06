# Contract: Unified Validation Issue

**Feature**: `002-normalize-validation-issue`
**Date**: 2026-11-06
**Scope**: Shared validation issue contract dùng chung giữa editor (`component_map_editor`), runtime (`component_map_runtime`), exporter, client và manager.

## 1. Protobuf message (source of truth)

File: `proto/validation.proto` (package `cme`).

```proto
enum ValidationSeverity {
  VALIDATION_SEVERITY_UNSPECIFIED = 0; // xử lý như ERROR (fail-closed)
  VALIDATION_SEVERITY_ERROR       = 1; // chặn stage/activate
  VALIDATION_SEVERITY_WARNING     = 2; // không chặn, MUST hiển thị/ghi nhận
  VALIDATION_SEVERITY_INFO        = 3; // không chặn, tùy chọn hiển thị
}

message ValidationIssue {
  string code      = 1; // UPPER_SNAKE_CASE, normalized qua stableDiagnosticCode
  ValidationSeverity severity = 2;
  string path      = 3; // components/<componentId>.properties.<prop> | connections/<connectionId>.<field> | graph
  string message   = 4;
  map<string,string> details = 5;   // tùy chọn
  string remediation = 6;            // tùy chọn
}

message GraphValidationResult {
  repeated ValidationIssue issues = 1;
  bool is_valid = 2; // false nếu có bất kỳ ERROR nào
}
```

Runtime wire contract giữ `proto/deployment.proto::DeploymentValidationIssue` nguyên hình dạng; adapter `DeploymentValidationAdapter` map 2 chiều:

- `DeploymentValidationIssue.field_path` → `ValidationIssue.path` (giữ nguyên định danh).
- `DeploymentValidationIssue.code` → `ValidationIssue.code` qua `stableDiagnosticCode`.
- `component_identity`/`component_version` → được gắn vào `details` (không mất data).

## 2. Editor in-process shape (QVariantMap)

`ValidationService::validationIssues()` và `RuntimeClient::*Issues()` MUST trả `QVariantList` các `QVariantMap` có khóa:

| Khóa | Nguồn | Bắt buộc |
|---|---|---|
| `code` | `ValidationIssue.code` | YES |
| `severity` | `ValidationIssue.severity` (string: `error`/`warning`/`info`) | YES |
| `path` | `ValidationIssue.path` | YES |
| `message` | `ValidationIssue.message` | YES |
| `details` | `ValidationIssue.details` | NO |
| `remediation` | `ValidationIssue.remediation` | NO |

Quy tắc: chỉ UI được chuyển map này thành chuỗi text. Mọi tầng upstream (service, client, exporter, manager) MUST truyền `QVariantList`/proto structured. Tiện ích `ValidationService::validationErrors()` (QStringList) và `validate()` (bool) CHỈ dùng cho UI/text, KHÔNG làm contract liên tầng.

## 3. Error-code catalog (shared)

`DiagnosticCodeCatalog` (static, versioned qua protocol version hiện có) MUST định nghĩa ít nhất:

| Code | Khi nào |
|---|---|
| `CORE_NULL_GRAPH` | graph null |
| `CORE_VALIDATION_PROVIDER_ERROR` | provider validation thất bại |
| `DUPLICATE_COMPONENT_ID` | trùng componentTypeId khác source/provider |
| `MISSING_DESCRIPTOR` | thiếu descriptor |
| `MISSING_SCHEMA` | thiếu property schema |
| `INVALID_PORT_METADATA` | port metadata sai |
| `MISSING_HANDLER_CAPABILITY` | thiếu handler/capability |
| `INCOMPATIBLE_COMPONENT_METADATA` | metadata không tương thích |
| `PROVIDER_DISCOVERY_REGISTRATION_FAILURE` | provider/discovery đăng ký lỗi |
| `COMPONENT_PROPERTY_REQUIRED` | thiếu property bắt buộc |

`code` MUST được normalize (`stableDiagnosticCode`: trim, upper, `[^A-Z0-9_]+` → `_`) để editor và runtime đồng dạng.

## 4. `path` convention (ổn định, dùng định danh)

| Thực thể lỗi | `path` |
|---|---|
| Component property | `components/<componentId>.properties.<prop>` |
| Component port | `components/<componentId>.ports.<portId>` |
| Connection field | `connections/<connectionId>.<field>` |
| Toàn graph | `graph` |

Quy tắc: định danh thực thể (componentId/connectionId), KHÔNG dùng chỉ mục mảng. `path` MUST không đổi khi thứ tự entity trong graph thay đổi.

## 5. Severity → deployment behavior

| Severity | Stage/Activate | UI |
|---|---|---|
| `ERROR` | Bị chặn; `is_valid=false` | Hiển thị nổi bật |
| `WARNING` | Được phép | Hiển thị + ghi nhận cùng result |
| `INFO` | Được phép | Tùy chọn |
| thiếu/không hợp lệ | Bị chặn (fail-closed) | Hiển thị nổi bật |

## 6. Non-negotiables (từ Constitution v2.0.0 V & Required Contracts)

- Issue MUST có `code`, `severity`, `path`, `message`.
- Error chặn deployment; warning không chặn nhưng MUST hiển thị/ghi nhận.
- `path` MUST ổn định để test/automation tham chiếu bằng `code` + `path`.
- Structured issue MUST giữ nguyên xuyên boundary; chỉ UI format text.
- Runtime MUST re-validate snapshot nhận được; không tin client string.
