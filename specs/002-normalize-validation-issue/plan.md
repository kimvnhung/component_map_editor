# Implementation Plan: Chuẩn hóa structured validation issue

**Branch**: `002-normalize-validation-issue` | **Date**: 2026-11-06 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/002-normalize-validation-issue/spec.md`

## Summary

Runtime đã sinh `ValidationIssue` (proto) nhưng editor và runtime dùng hai hợp đồng issue khác nhau: editor dùng `ValidationIssue` (`code`, `severity`, `message`, `component_id`, `connection_id`), runtime trả `DeploymentValidationIssue` (`code`, `severity`, `field_path`, `component_identity`, `component_version`). Nhiều đường UI/API lại gom issue thành `QStringList` chuỗi (`ValidationService::validationErrors`, `validate`). Mục tiêu: chuẩn hóa một shared validation issue contract gồm `code`, `severity`, `path`, `message`, `details?`, `remediation?`; phân biệt error (chặn deployment) và warning; giữ structured issue xuyên exporter/client/runtime/manager; ổn định `path` bằng định danh thực thể để automation/test tham chiếu.

Cách tiếp cận (từ research): hợp nhất hai proto message thành một `ValidationIssue` dùng `path` làm khóa định danh ổn định (thay cho `component_id`/`connection_id`/`field_path`), duy trì adapter map 2 chiều tạm thời để không phá giao tiếp hiện tại; đưa mọi đường trả issue về structured list thay vì chuỗi; chuẩn hóa `code` qua một hàm dùng chung (`stableDiagnosticCode` đã có ở runtime, áp dụng thống nhất cho cả editor); và cung cấp error-code catalog dùng chung (header/enum) cho các mã lỗi tối thiểu theo SDD F2.4.

## Technical Context

**Language/Version**: C++ (Qt 6.9 / QML), Python chỉ cho tooling/test nếu cần.

**Primary Dependencies**: Qt 6, gRPC + protobuf (proto/), CMake, Conan (conanfile.txt). Runtime/editor dùng cùng `proto/` package `cme` / `cme.runtime.v1`.

**Storage**: Không lưu validation issue (runtime-time only). SQLite chỉ cho runtime state, không liên quan trực tiếp feature này.

**Testing**: Qt Test (`tests/`, `tst_*.cpp`), có sẵn `tests/tst_Phase5ValidationMigration.cpp`, `tests/tst_PersistenceValidation.cpp`. Contract test và integration test sẽ bổ sung.

**Target Platform**: Desktop (Qt/QML client) + standalone runtime process/service; editor-runtime giao tiếp gRPC/protobuf.

**Project Type**: Desktop application (editor) + runtime service (library/process), có public protobuf API.

**Performance Goals**: Validation là tương tác người dùng; không có SLO cứng. Yêu cầu duy nhất mang tính độ ổn định: `path` và `code` phải ổn định trên bộ test cases (SC-001, SC-004, SC-005).

**Constraints**: Không đưa graph-version requirement vào graph document (SDD/Constitution v2.0.0). Contract/protocol version có thể version nhưng không version hóa graph. Structured issue phải giữ nguyên xuyên boundary; chỉ UI format text.

**Scale/Scope**: Một current graph document ở editor, một active graph ở mỗi runtime. Bộ error-code catalog bao phủ ít nhất các mã theo SDD F2.4.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

Dựa trên `.specify/memory/constitution.md` v2.0.0:

1. **I. Contract-First Module Boundaries** — Feature thay đổi hợp đồng chung (`ValidationIssue`) dùng bởi editor-runtime. Phải giữ dependency direction: editor và runtime cùng reference shared `proto/` contract, không reverse dependency. ✅ (Thiết kế dùng `proto/` chung, không đưa logic editor vào runtime.)
2. **II. Unified Component Catalog** — Error-code catalog dùng chung liên quan đến registration errors (duplicate component ID, missing descriptor, missing schema, invalid port metadata, missing handler/capability, incompatible metadata, provider/discovery failure). Phải nhất quán với catalog semantics. ✅ (FR-002 ánh xạ đúng SDD F2.4.)
3. **III. Immutable Deployment and Independent Runtime** — Runtime MUST re-validate snapshot nhận được; không tin client string. ✅ (FR-004 đã capture; runtime giữ `ValidateDeployment` riêng.)
4. **IV. Reliable Runtime Semantics and Recovery** — Structured issue là một phần của observability/diagnostics; không đổi delivery semantics. ✅ (Feature chỉ chuẩn hóa diagnostics, không đổi event delivery.)
5. **V. Observable, Portable, and Compatible Infrastructure** — Required Contracts: validation issue MUST có `code`, `severity`, `path`, `message`; errors chặn, warnings không chặn nhưng MUST hiển thị/ghi nhận; path MUST ổn định. ✅ (Đây là nguồn trực tiếp của FR-001/FR-003/FR-005/FR-008/FR-009.)

**Gate result**: PASS (không vi phạm; thiết kế tuân thủ cả 5 nguyên tắc). Không cần Complexity Tracking.

## Project Structure

### Documentation (this feature)

```text
specs/002-normalize-validation-issue/
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
├── contracts/           # Phase 1 output
│   └── validation-issue.md
└── tasks.md             # Phase 2 output (/speckit-tasks)
```

### Source Code (repository root)

```text
proto/
├── validation.proto         # Unified ValidationIssue (path-based) + GraphValidationResult
└── deployment.proto         # DeploymentValidationIssue (mapped to unified via adapter)

component_map_editor/
├── adapters/
│   ├── ValidationAdapter.{h,cpp}   # proto <-> QVariantMap, thêm path mapping
│   └── DeploymentValidationAdapter.{h,cpp}  # DeploymentValidationIssue <-> unified
├── services/
│   ├── ValidationService.{h,cpp}   # trả QVariantList issues (structured), giữ validate()
│   ├── RuntimeClient.{h,cpp}       # map runtime issue -> unified editor issue
│   └── DiagnosticCodeCatalog.h     # shared error-code catalog (enum/constants)
└── ui/                            # chỉ tầng này format issue thành text

component_map_runtime/
└── src/                           # runtime ValidateDeployment dùng unified issue semantics
```

**Structure Decision**: Giữ cấu trúc module map hiện có (editor-authoring, graph-validation, deployment-control, runtime-execution). Feature chỉ sửa `proto/` chung, adapter layer và service layer ở editor, cùng runtime validation path. Không tạo module mới; adapter là nơi duy nhất thực hiện mapping backward-compatible.

## Complexity Tracking

Không có vi phạm constitution cần biện minh. (Bảng để trống theo template.)
