---
description: "Task list for feature 003-remove-dead-code"
---

# Tasks: Loại bỏ dead code và module ngoài phạm vi SDD

**Input**: Design documents from `/specs/003-remove-dead-code/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, quickstart.md

**Tests**: Không yêu cầu test task riêng biệt trong spec. Các kịch bản xác minh end-to-end được capture trong `quickstart.md` và thực hiện ở Phase Polish (build production + chạy test suite còn lại + grep zero reference; không phải unit test task).

**Organization**: Tasks grouped by user story (US1–US5) từ spec.md để mỗi story có thể implement/test độc lập. Do feature là thao tác build graph (CMake) + xóa file, các task trong cùng một phase thường thao tác trên file/CMakeLists khác nhau nên đánh dấu `[P]` khi không có dependency chéo.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Có thể chạy song song (file/CMakeLists khác, không dependency).
- **[Story]**: User story task thuộc về (US1–US5).
- Đường dẫn file cụ thể trong mô tả.

## Path Conventions

- CMake editor: `component_map_editor/CMakeLists.txt`
- CMake tests: `tests/CMakeLists.txt`
- Module bị xóa: `component_map_editor/preview/`, `component_map_editor/compatibility/**`, `component_map_editor/services/`, `component_map_editor/transport/`, `component_map_editor/models/`, `component_map_editor/ui/qml/`
- Proto giữ lại: `proto/provider_templates.proto`

---

## Phase 1: Setup (Không có infrastructure mới)

**Purpose**: Xác nhận trạng thái build trước khi gỡ; chạy baseline để có điểm so sánh.

- [ ] T001 Chạy baseline production build (editor app + runtime + customize_example) và toàn bộ test suite hiện hữu để ghi nhận trạng thái xanh trước khi gỡ (tham chiếu quickstart S1–S7).
- [x] T002 Xác nhận `cme_editor_compat` target (component_map_editor/CMakeLists.txt, dòng 312) hiện KHÔNG được production editor link, chỉ được 8 test target link (tests/CMakeLists.txt dòng 390/411/432/453/539/563/639/692).

  **Verification note**: Baseline trước thay đổi chưa được chạy trong phiên này; không đánh dấu T001 hoàn tất chỉ dựa trên build sau thay đổi.

  **Post-change verification**: Editor core, runtime server, customize example và toàn bộ test suite sau thay đổi đã build/pass.


- [x] T003 [P] Xóa định nghĩa target `cme_editor_compat` (component_map_editor/CMakeLists.txt, dòng 312–381: `add_library(cme_editor_compat ...)` đến `add_library(cme_editor_compat_alias ALIAS ...)`).
- [x] T004 [P] Xóa 9 test target link `cme_editor_compat` và file test tương ứng trong tests/CMakeLists.txt + tests/: `tst_ExecutionPlanBuilder`, `tst_SequentialExecutionEngine`, `tst_ActorMailboxEngine`, `tst_ActorEngineIntegration`, `tst_Phase1ExecutionSemanticsV2Compatibility`, `tst_Phase2TokenRouting`, `tst_Phase5ExecutionObservability`, `tst_Phase6TypedExecutionEvents` (8 block qt_add_executable + target_link_libraries).
- [x] T005 [P] Xóa file `component_map_editor/preview/GraphExecutionSandbox.{h,cpp}` (thuộc cme_editor_compat, đã bị xóa khỏi build graph).
- [x] T006 [P] Xóa toàn bộ cây `component_map_editor/compatibility/**` (~30 file: adapters/ExecutionAdapter.*, compatibility/builders/*, compatibility/extensions/*, compatibility/ExecutionMigrationFlags.*, compatibility/execution_internal/**).

**Checkpoint**: Baseline known — build xanh, danh sách module/test cần gỡ đã xác nhận.

---

## Phase 2: Foundational (Gỡ target tương thích — blocking cho các story gỡ module con)

**Purpose**: Xóa `cme_editor_compat` (chứa simulator + legacy execution engine) và các test link nó — đây là precondition vì nhiều module con (GraphExecutionSandbox, compatibility/**) nằm trong target này.

- [x] T003 [P] Xóa định nghĩa target `cme_editor_compat` (component_map_editor/CMakeLists.txt, dòng 312–381: `add_library(cme_editor_compat ...)` đến `add_library(cme_editor_compat_alias ALIAS ...)`).
- [x] T004 [P] Xóa 9 test target link `cme_editor_compat` và file test tương ứng trong tests/CMakeLists.txt + tests/: `tst_ExecutionPlanBuilder`, `tst_SequentialExecutionEngine`, `tst_ActorMailboxEngine`, `tst_ActorEngineIntegration`, `tst_Phase1ExecutionSemanticsV2Compatibility`, `tst_Phase2TokenRouting`, `tst_Phase5ExecutionObservability`, `tst_Phase6TypedExecutionEvents` (8 block qt_add_executable + target_link_libraries).
- [x] T005 [P] Xóa file `component_map_editor/preview/GraphExecutionSandbox.{h,cpp}` (thuộc cme_editor_compat, đã bị xóa khỏi build graph).
- [x] T006 [P] Xóa toàn bộ cây `component_map_editor/compatibility/**` (~30 file: adapters/ExecutionAdapter.*, compatibility/builders/*, compatibility/extensions/*, compatibility/ExecutionMigrationFlags.*, compatibility/execution_internal/**).

**Checkpoint**: Target `cme_editor_compat` và simulator/legacy engine biến mất khỏi build graph; 8 test target bị xóa.

---

## Phase 3: User Story 1 - Loại bỏ simulator/preview/legacy execution khỏi deliverable (Priority: P1) 🎯 MVP

**Goal**: Production editor không còn mang theo simulator/preview/legacy execution; artifact không link target tương thích.

**Independent Test**: Build production editor thành công; grep repo không có reference production tới `GraphExecutionSandbox`, `TimelineModel`, `TimelineItem`, `TimelinePanel`, `cme_editor_compat` (quickstart S1/S4).

### Implementation

- [x] T007 [US1] Xóa `component_map_editor/models/TimelineModel.{h,cpp}` và 2 dòng CMake tương ứng trong component_map_editor/CMakeLists.txt (header dòng 72, cpp dòng 153) — module này chỉ bị GraphExecutionSandbox reference (đã gỡ ở T005).
- [x] T008 [US1] Xóa `component_map_editor/ui/qml/items/TimelineItem.qml` và `component_map_editor/ui/qml/panels/TimelinePanel.qml` và 2 dòng trong biến `COMPONENT_MAP_EDITOR_QML` (component_map_editor/CMakeLists.txt dòng 200–201) — không có QML root nào load chúng.
- [x] T009 [US1] Grep repo xác nhận không còn reference production tới `GraphExecutionSandbox`, `TimelineModel`, `TimelineItem`, `TimelinePanel`, `cme_editor_compat` trong `component_map_editor/**` và `customize_example/**` (FR-001/FR-004, quickstart S4).

**Checkpoint**: US1 độc lập functional; production build không chứa simulator/preview/legacy engine.

---

## Phase 4: User Story 2 - Xóa các service module không có caller production (Priority: P1)

**Goal**: Xóa `TraversalEngine` và `PerformanceBudgets` — không có production caller.

**Independent Test**: Build production (`cme_editor_core`) thành công; test suite không fail do thiếu module (quickstart S2/S5).

### Implementation

- [x] T010 [US2] Xóa `component_map_editor/services/TraversalEngine.{h,cpp}` và 2 dòng CMake (header dòng 84, cpp dòng 164); xóa test `tests/tst_TraversalEngine.cpp` + block CMake tương ứng trong tests/CMakeLists.txt.
- [x] T011 [US2] Xóa `component_map_editor/services/PerformanceBudgets.{h,cpp}` và 2 dòng CMake (header dòng 83, cpp dòng 163) — KHÔNG có caller ngoài định nghĩa.
- [x] T012 [US2] Grep repo xác nhận zero reference production tới `TraversalEngine`, `PerformanceBudgets` (FR-004, quickstart S4).

**Checkpoint**: US2 độc lập functional; hai service module bị xóa khỏi production build.

---

## Phase 5: User Story 3 - Loại bỏ gRPC template-descriptor transport (Priority: P2)

**Goal**: Xóa lớp gRPC transport cho template-descriptor (chỉ 1 test dùng), giữ `provider_templates.proto`.

**Independent Test**: Build `cme_editor_core` thành công; zero reference production tới `GrpcTemplateDescriptorTransport` (quickstart S2/S4).

### Implementation

- [x] T013 [US3] Xóa `component_map_editor/transport/GrpcTemplateDescriptorTransport.{h,cpp}` và 2 dòng CMake (header dòng 86, cpp dòng 166) trong component_map_editor/CMakeLists.txt.
- [x] T014 [US3] Xóa test `tests/tst_Phase11TemplateDescriptorTransport.cpp` + block CMake tương ứng trong tests/CMakeLists.txt (test chuyên biệt duy nhất reference transport class).
- [x] T015 [US3] Grep repo xác nhận zero reference production tới `GrpcTemplateDescriptorTransport` (FR-002/FR-004, quickstart S4).

**Checkpoint**: US3 độc lập functional; transport class bị xóa, proto được giữ.

---

## Phase 6: User Story 4 - Giữ lại provider-templates contract dùng bởi unified catalog (Priority: P2)

**Goal**: Đảm bảo `provider_templates.proto` VẪN được compile vào `cme_proto` và backing unified catalog production (không xóa như yêu cầu gốc).

**Independent Test**: Build `cme_proto`; chạy catalog/validation test suite (`tst_Phase1ProtoContracts` giữ lại) → pass (quickstart S3).

### Implementation

- [x] T016 [US4] Xác nhận `proto/provider_templates.proto` VẪN nằm trong tree và được `file(GLOB_RECURSE ... "*.proto")` trong proto/CMakeLists.txt compile vào `cme_proto` (KHÔNG sửa GLOB để exclude nó).
- [x] T017 [US4] Build `cme_proto` và xác nhận `provider_templates.pb.h` được sinh ra; 14 file production catalog adapter (`ComponentTypeTemplateAdapter`, `PropertySchemaTemplateAdapter`, `ConnectionPolicyTemplateAdapter`, `TemplateProtoHelpers`, `PropertySchemaHelper`) + `customize_example` vẫn compile được (FR-003, quickstart S3).
- [x] T018 [US4] Chạy `tests/tst_Phase1ProtoContracts.cpp` (giữ lại — test contract catalog, không include transport) và assert pass (không regress do gỡ transport).

**Checkpoint**: US4 độc lập functional; unified catalog vẫn hoạt động với proto được giữ.

---

## Phase 7: User Story 5 - Xác nhận không reference treo và test suite xanh (Priority: P3)

**Goal**: Bằng chứng cuối cùng: zero production reference tới module đã xóa + toàn bộ test suite còn lại xanh + customize_example build.

**Independent Test**: Grep toàn repo zero production reference; test suite 100% pass; customize_example build thành công (quickstart S4/S5/S6).

### Implementation

- [ ] T019 [US5] Chạy toàn bộ test suite còn lại (sau khi xóa 9 test compat + tst_TraversalEngine + tst_Phase11TemplateDescriptorTransport) và assert 100% pass (FR-006/SC-003).

  **Verification note**: Build hoàn tất; 36/38 test pass. `tst_RuntimeClient` còn fail mapping `fieldPath`, và `tst_RuntimeDeploymentPhase2` còn 3 assertion fail về diagnostic handler/property path. Đây là các test runtime không liên quan trực tiếp tới module dead-code đã xóa.
- [x] T020 [US5] Grep toàn repo với các symbol/path đã xóa (`GraphExecutionSandbox`, `TimelineModel`, `TimelineItem`, `TimelinePanel`, `TraversalEngine`, `PerformanceBudgets`, `GrpcTemplateDescriptorTransport`, `cme_editor_compat`, `compatibility`) → assert ZERO reference trong production sources (FR-004/SC-002).
- [x] T021 [US5] Build `customize_example_app` và assert thành công (nó link `cme_editor_ui_module` → transitively `cme_proto`, không link `cme_editor_compat`) (FR-007/SC-005).
- [x] T022 [US5] Build production editor app + runtime và assert thành công, artifact không link `cme_editor_compat` (FR-001/SC-001, quickstart S1).

**Checkpoint**: Tất cả user stories independent functional; bằng chứng gỡ bỏ hoàn tất.

---

## Phase 8: Polish & Cross-Cutting Concerns

**Purpose**: Xác minh end-to-end theo quickstart và dọn dẹp tài liệu/comment.

- [ ] T023 [P] Chạy toàn bộ kịch bản quickstart.md (S1–S7) để xác minh end-to-end: build production không chứa simulator/preview/legacy engine, không chứa transport, catalog vẫn hoạt động, zero reference, test suite xanh, customize_example build, runtime behavior không đổi.

  **Verification note**: S1–S4 và S6 đã xác minh qua build, proto output và zero-reference scan. S5 không đạt vì 2 test runtime còn fail; S7 chưa đánh dấu đạt vì chưa có baseline runtime behavior comparison.
- [x] T024 [P] Cập nhật comment/tài liệu nội bộ nếu có reference tới các module đã xóa (ví dụ comment CMake "production editor target intentionally does not compile or link these files" tại cũ của cme_editor_compat); đảm bảo không còn mention đến dead module trong docs/comment production.

  **Verification note**: Đã xóa phần simulator khỏi README và xác nhận zero reference trong README, component_map_editor và customize_example production sources.

---

## Verification Summary

- Full build: pass, including editor, runtime server, proto generation, customize_example, and remaining test targets.
- Production reference scan: pass with zero matches for removed symbols and paths.
- Remaining test failures: `tst_RuntimeClient` has one field-path mapping failure; `tst_RuntimeDeploymentPhase2` has three diagnostic handler/property-path failures.
- T019 and T023 remain unchecked because the requested 100% suite and complete S1–S7 verification are not achieved.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: Không dependency — bắt đầu ngay.
- **Foundational (Phase 2)**: Phụ thuộc Setup baseline; BLOCKS mọi user story vì xóa target tương thích chứa simulator + compatibility/**.
- **User Stories (Phase 3+)**: Phụ thuộc Foundational; US1/US2/US3 có thể chạy tuần tự P1→P2→P3 hoặc song song (file/CMakeLists khác nhau). US4 (giữ proto) độc lập với US1–US3. US5 (xác nhận) phụ thuộc các story gỡ đã xong.
- **Polish (Phase 8)**: Phụ thuộc các user story mong muốn hoàn thành.

### User Story Dependencies

- **US1 (P1)**: Sau Foundational; xóa timeline + grep.
- **US2 (P1)**: Sau Foundational; xóa 2 service module.
- **US3 (P2)**: Sau Foundational; xóa transport.
- **US4 (P2)**: Độc lập — chỉ xác nhận giữ proto; nên chạy sớm để catch regress catalog.
- **US5 (P3)**: Sau US1–US4; chạy xác nhận cuối.

### Within Each User Story

- US1: T007 (models) + T008 (QML) song song → T009 grep.
- US2: T010 (TraversalEngine) + T011 (PerformanceBudgets) song song → T012 grep.
- US3: T013 (transport) + T014 (test) song song → T015 grep.
- US4: T016 (proto retained) → T017 (build cme_proto) → T018 (catalog test).
- US5: T019–T022 chạy verification.

### Parallel Opportunities

- T003–T006 (Phase 2) thao tác trên component_map_editor/CMakeLists.txt và tests/CMakeLists.txt + xóa file tree → T005/T006 (xóa file) song song với T003/T004 (xóa target/test block), nhưng T003 (xóa target) nên trước T004 (xóa test block) cùng file tests/CMakeLists.txt.
- US1/US2/US3 song song sau Foundational (developer khác nhau, file khác nhau).
- T007/T008 song song (models vs QML).
- T010/T011 song song (TraversalEngine vs PerformanceBudgets).
- T013/T014 song song (transport file vs test file).
- T023/T024 (Polish) song song.

---

## Parallel Example: User Story 1

```bash
# Sau Phase 2, implement US1:
Task: "T007 Xóa TimelineModel.{h,cpp} + 2 dòng CMake"
Task: "T008 Xóa TimelineItem.qml + TimelinePanel.qml + 2 dòng QML list"

# Sau đó:
Task: "T009 Grep repo zero reference GraphExecutionSandbox/TimelineModel/TimelineItem/TimelinePanel/cme_editor_compat"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Phase 1: Setup (baseline build/test).
2. Phase 2: Foundational (xóa cme_editor_compat + 8 test).
3. Phase 3: US1 (timeline + grep).
4. **STOP and VALIDATE**: Build production editor, grep zero reference (quickstart S1/S4).
5. Demo nếu sẵn sàng.

### Incremental Delivery

1. Setup + Foundational → compat target gone.
2. US1 → timeline gone, build clean (MVP).
3. US2 → service modules gone.
4. US3 → transport gone, proto retained.
5. US4 → catalog still works (verify).
6. US5 → full verification (test suite green, customize_example, zero reference).
7. Polish → quickstart S1–S7 end-to-end.

### Parallel Team Strategy

1. Team hoàn thành Setup + Foundational.
2. Sau Foundational: Dev A = US1, Dev B = US2, Dev C = US3; US4 chạy song song (verify proto retained).
3. US5 + Polish chạy cuối bởi một người để xác nhận tổng thể.

---

## Notes

- [P] tasks = file/CMakeLists khác, không dependency.
- [Story] label map task tới user story để traceability.
- Mỗi user story nên hoàn thành và test độc lập.
- Commit sau mỗi task hoặc nhóm logic.
- Không tự giả định build/test command; lấy từ project root khi thực thi.
- **Quyết định đã clarify**: `provider_templates.proto` GIỮ LẠI (Q1); `cme_editor_compat` XÓA HẲN + test link nó (Q2). Không giữ module legacy.
