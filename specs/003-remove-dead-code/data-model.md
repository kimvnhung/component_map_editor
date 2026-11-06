# Data Model: Loại bỏ dead code và module ngoài phạm vi SDD

**Feature**: `003-remove-dead-code`
**Date**: 2026-11-06

Feature này không thay đổi domain data model của ứng dụng; nó thay đổi **build graph** (CMake target ownership) và **module surface**. Do đó "thực thể" ở đây là các build target và module file, cùng mối quan hệ dependency giữa chúng.

## Build Targets (thực thể chính)

- **`cme_proto`** (proto/cme_proto): static lib chứa mã sinh từ mọi `*.proto` qua `file(GLOB_RECURSE)`. Đầu vào của GLOB bao gồm `provider_templates.proto`.
  - Thuộc tính: danh sách proto sources (GLOB tự động).
  - Ràng buộc: xóa một file `.proto` sẽ gỡ khỏi target này → mọi consumer link `cme_proto` bị ảnh hưởng.
- **`cme_editor_core`** (component_map_editor/CMakeLists.txt): static lib chứa services + transport + models của editor production.
  - Sở hữu: `services/TraversalEngine.*`, `services/PerformanceBudgets.*`, `transport/GrpcTemplateDescriptorTransport.*`, `models/TimelineModel.*`.
- **`cme_editor_ui_module`** (qt_add_qml_module): QML module; danh sách QML explicit qua biến `COMPONENT_MAP_EDITOR_QML`.
  - Sở hữu: `ui/qml/items/TimelineItem.qml`, `ui/qml/panels/TimelinePanel.qml`.
- **`cme_editor_compat`** (component_map_editor/CMakeLists.txt, dòng 312): static lib TỰ CHỨA, KHÔNG được production link.
  - Sở hữu: `preview/GraphExecutionSandbox.*` + toàn bộ `compatibility/**` (legacy execution engine, migration flags, actor mailbox).
- **`customize_example_app`** (customize_example/CMakeLists.txt): link `cme_editor_ui_module` → transitively `cme_proto`. KHÔNG link `cme_editor_compat`.
- **Test targets** (tests/CMakeLists.txt): 8 target link `cme_editor_compat`; các test chuyên biệt cho module bị xóa (TraversalEngine, Phase11 transport).

## Module Ownership (sau khi gỡ)

| Module file | Trước | Sau |
|-------------|-------|-----|
| `preview/GraphExecutionSandbox.*` | `cme_editor_compat` | XÓA (cùng target) |
| `compatibility/**` (30 files) | `cme_editor_compat` | XÓA (cùng target) |
| `services/TraversalEngine.*` | `cme_editor_core` | XÓA |
| `services/PerformanceBudgets.*` | `cme_editor_core` | XÓA |
| `transport/GrpcTemplateDescriptorTransport.*` | `cme_editor_core` | XÓA |
| `models/TimelineModel.*` | `cme_editor_core` | XÓA |
| `ui/qml/items/TimelineItem.qml`, `ui/qml/panels/TimelinePanel.qml` | `cme_editor_ui_module` (QML list) | XÓA |
| `proto/provider_templates.proto` | `cme_proto` (GLOB) | GIỮ NGUYÊN |

## Dependency Relationships (quan trọng cho an toàn gỡ)

- Production editor app → `cme_editor_core` → `cme_proto`. Không link `cme_editor_compat`.
- `customize_example_app` → `cme_editor_ui_module` → (`cme_editor_core`) → `cme_proto`.
- `cme_proto` chứa `provider_templates.pb.h` được 14 file production + `customize_example` include. **Do đó `provider_templates.proto` không thể xóa** mà không break những consumer này.
- `cme_editor_compat` chỉ được 8 test target link. Xóa target + test không ảnh hưởng production graph.

## State / Invariants cần bảo toàn

- **Invariant 1**: `cme_editor_core` vẫn compile (không reference tới module đã xóa).
- **Invariant 2**: `cme_proto` vẫn chứa `provider_templates.proto` (GLOB giữ nguyên).
- **Invariant 3**: Production app artifact không link `cme_editor_compat` (đã đúng từ trước, giữ nguyên).
- **Invariant 4**: Grep repo với các symbol/path đã xóa trả về 0 reference trong production sources.

## Validation Rules (từ requirements)

- FR-001: production MUST NOT compile/link simulator/preview/legacy engine → đạt khi `cme_editor_compat` biến mất khỏi build graph production.
- FR-002: production MUST NOT chứa `GrpcTemplateDescriptorTransport` → đạt khi file + 2 dòng CMake bị xóa.
- FR-003: `provider_templates` MUST retained → đạt khi proto còn, GLOB không bị sửa để exclude nó.
- FR-004: zero production reference tới module đã xóa → đạt qua grep.
- FR-005/FR-006/FR-007: behavior + test suite + customize_example không regress.
