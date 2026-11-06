# Research: Loại bỏ dead code và module ngoài phạm vi SDD

**Feature**: `003-remove-dead-code`
**Date**: 2026-11-06

Mục tiêu của research phase: xác nhận chính xác build wiring (CMake) và caller graph của mỗi module được yêu cầu gỡ, để plan phase có thể liệt kê bước xóa cụ thể, có thể thực thi. Toàn bộ phát hiện bên dưới đã được xác minh bằng static scan trên working tree hiện tại.

## R1 — `cme_editor_compat` target và các test link nó

- **Discovery**: `component_map_editor/CMakeLists.txt` định nghĩa `cme_editor_compat` (STATIC, dòng 312). Nó chứa `preview/GraphExecutionSandbox.*` và toàn bộ cây `compatibility/**` (execution engine legacy, migration flags, actor mailbox).
- **Consumer duy nhất**: 8 test target trong `tests/CMakeLists.txt` link `cme_editor_compat` (dòng 390, 411, 432, 453, 539, 563, 639, 692). Production editor (`cme_editor_core`, `cme_editor_ui_module`, app) KHÔNG link target này.
- **Test bị ảnh hưởng (9 file, vì 1 test link qua 2 đường)**:
  - `tst_TraversalEngine.cpp` (link thêm `cme_editor_core`?)
  - `tst_ExecutionPlanBuilder.cpp`, `tst_SequentialExecutionEngine.cpp`, `tst_ActorMailboxEngine.cpp`, `tst_ActorEngineIntegration.cpp`,
    `tst_Phase1ExecutionSemanticsV2Compatibility.cpp`, `tst_Phase2TokenRouting.cpp`, `tst_Phase5ExecutionObservability.cpp`, `tst_Phase6TypedExecutionEvents.cpp`.
- **Decision**: XÓA hẳn target `cme_editor_compat` và cả 9 file test + khối `qt_add_executable`/`target_link_libraries` tương ứng trong `tests/CMakeLists.txt`. Không giữ module legacy (SDD cấm simulate, không owner — theo clarification Q2).
- **Rationale**: target đã nằm ngoài production build; giữ nó chỉ tăng bề mặt bảo trì mà không phục vụ luồng production.

## R2 — `provider_templates.proto` (GIỮ LẠI)

- **Discovery**: `proto/CMakeLists.txt` compile proto qua `file(GLOB_RECURSE PROTO_FILES ... "*.proto")` vào target `cme_proto`. Do đó `provider_templates.proto` được glob tự động — xóa file sẽ gỡ khỏi `cme_proto`.
- **Caller production**: 14 file `#include "provider_templates.pb.h"` trong unified catalog template adapters (`ComponentTypeTemplateAdapter`, `PropertySchemaTemplateAdapter`, `ConnectionPolicyTemplateAdapter`, `TemplateProtoHelpers`, `PropertySchemaHelper`) và `customize_example` (`filecomponenttypeprovider`, `customizecomponenttypeprovider`, `customizeconnectionpolicyprovider`).
- **Decision**: GIỮ `provider_templates.proto` nguyên vẹn. Chỉ xóa lớp transport `GrpcTemplateDescriptorTransport` (xem R3). Đây là clarification Q1 đã xác nhận.
- **Rationale**: xóa proto sẽ break build catalog production và `customize_example` (vốn link `cme_editor_ui_module` → transitively `cme_proto`). Điều kiện "không có caller production" của yêu cầu gốc KHÔNG thỏa mãn.

## R3 — `GrpcTemplateDescriptorTransport` (XÓA)

- **Discovery**: `component_map_editor/CMakeLists.txt` liệt kê rõ ràng (dòng 86 header, 166 cpp) trong `cme_editor_core` target.
- **Caller**: chỉ `tests/tst_Phase11TemplateDescriptorTransport.cpp` (`#include "provider_templates.pb.h"` + transport). Không có production caller.
- **Decision**: XÓA `transport/GrpcTemplateDescriptorTransport.{h,cpp}` và 2 dòng CMake. XÓA test `tst_Phase11TemplateDescriptorTransport.cpp` + khối CMake tương ứng.
- **Note**: `tst_Phase1ProtoContracts.cpp` cũng include `provider_templates.pb.h` nhưng test contract catalog (KHÔNG include transport) → GIỮ test này, vì proto giữ lại.

## R4 — `TraversalEngine` & `PerformanceBudgets` (XÓA)

- **Discovery**: cả hai nằm trong `cme_editor_core` (`services/TraversalEngine.{h,cpp}` dòng 84/164; `services/PerformanceBudgets.{h,cpp}` dòng 83/163).
- **Caller**:
  - `TraversalEngine`: chỉ chính định nghĩa + `tests/tst_TraversalEngine.cpp`.
  - `PerformanceBudgets`: KHÔNG có caller nào ngoài định nghĩa (grep toàn repo trả về 0 ngoài file nó).
- **Decision**: XÓA 4 file + 4 dòng CMake (2 header, 2 cpp). XÓA `tst_TraversalEngine.cpp` + khối CMake.

## R5 — Preview timeline (XÓA)

- **Discovery**:
  - `component_map_editor/preview/GraphExecutionSandbox.{h,cpp}` — thuộc `cme_editor_compat` (R1).
  - `component_map_editor/models/TimelineModel.{h,cpp}` — liệt kê explicit trong `cme_editor_core` (dòng 72/153). Chỉ bị reference bởi `GraphExecutionSandbox` (prod caller duy nhất).
  - `ui/qml/items/TimelineItem.qml`, `ui/qml/panels/TimelinePanel.qml` — liệt kê explicit trong biến `COMPONENT_MAP_EDITOR_QML` (dòng 184 QML glob list, dòng 200-201). KHÔNG có QML root nào `import`/load chúng.
- **Decision**: Đã gỡ `GraphExecutionSandbox` qua R1; thêm gỡ `TimelineModel.{h,cpp}` (2 dòng CMake) + 2 file QML (2 dòng CMake QML list).

## R6 — `customize_example` dependency

- **Discovery**: `customize_example/CMakeLists.txt` link `cme_editor_ui_module` (transitively `cme_proto`). Nó `#include "provider_templates.pb.h"` ở 3 file provider. Nó KHÔNG link `cme_editor_compat`.
- **Decision**: Không cần thay đổi dependency. Vì proto (R2) giữ lại, `customize_example` vẫn build xanh (SC-005).

## R7 — Build verification strategy

- **Discovery**: build command lấy từ project root (không tự giả định). Test suite là Qt Test (`tst_*.cpp`) + gRPC runtime tests.
- **Decision**: Sau khi gỡ, chạy production build (editor app + `customize_example` + runtime) và toàn bộ test suite còn lại. Grep repo để chứng minh zero reference production tới module đã xóa (SC-002).

## Tổng hợp quyết định (decision table)

| Module | Hành động | Cơ sở |
|--------|-----------|-------|
| `cme_editor_compat` (simulator + legacy engine) | XÓA target + 9 test | R1, Q2 |
| `provider_templates.proto` | GIỮ | R2, Q1 |
| `GrpcTemplateDescriptorTransport.{h,cpp}` | XÓA + xóa test | R3 |
| `TraversalEngine.{h,cpp}` | XÓA + xóa test | R4 |
| `PerformanceBudgets.{h,cpp}` | XÓA | R4 |
| `TimelineModel.{h,cpp}` | XÓA | R5 |
| `TimelineItem.qml`, `TimelinePanel.qml` | XÓA | R5 |
| `customize_example` dependency | KHÔNG đổi | R6 |
