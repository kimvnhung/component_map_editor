---
description: "Task list for feature 002-normalize-validation-issue"
---

# Tasks: Chuẩn hóa structured validation issue

**Input**: Design documents from `/specs/002-normalize-validation-issue/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/validation-issue.md, quickstart.md

**Tests**: Không yêu cầu test task riêng biệt trong spec. Các kịch bản xác minh end-to-end được capture trong `quickstart.md` và thực hiện ở Phase Polish (run validation scenarios, không phải unit test task).

**Organization**: Tasks grouped by user story (US1, US2, US3) để mỗi story có thể implement/test độc lập.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Có thể chạy song song (file khác, không dependency)
- **[Story]**: User story task thuộc về (US1, US2, US3)
- Đường dẫn file cụ thể trong mô tả

## Path Conventions

- Proto chung: `proto/validation.proto`, `proto/deployment.proto`
- Editor adapters: `component_map_editor/adapters/`
- Editor services: `component_map_editor/services/`
- Runtime validation: `component_map_runtime/src/`

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Chuẩn bị contract chung và catalog; không thay đổi hành vi runtime/editor.

- [x] T001 Tạo `DiagnosticCodeCatalog` (header/enum tĩnh) trong `component_map_editor/services/DiagnosticCodeCatalog.h` định nghĩa các mã tối thiểu theo SDD F2.4: `CORE_NULL_GRAPH`, `CORE_VALIDATION_PROVIDER_ERROR`, `DUPLICATE_COMPONENT_ID`, `MISSING_DESCRIPTOR`, `MISSING_SCHEMA`, `INVALID_PORT_METADATA`, `MISSING_HANDLER_CAPABILITY`, `INCOMPATIBLE_COMPONENT_METADATA`, `PROVIDER_DISCOVERY_REGISTRATION_FAILURE`, `COMPONENT_PROPERTY_REQUIRED`.
- [x] T002 Đưa hàm `stableDiagnosticCode` (hiện ở `RuntimeClient.cpp`) thành shared util dùng chung cho editor và runtime; đảm bảo `code` normalize thành UPPER_SNAKE_CASE (trim, upper, `[^A-Z0-9_]+` → `_`).
- [x] T003 [P] Mở rộng `proto/validation.proto::ValidationIssue` thêm trường `path` (string, field 3), `details` (map<string,string>, field 5), `remediation` (string, field 6); giữ `code`, `severity`, `message`. Cập nhật `proto/CMakeLists.txt` nếu cần regenerate.

**Checkpoint**: Contract chung và catalog sẵn sàng; chưa thay đổi hành vi truyền tải issue.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Adapter layer map 2 chiều giữa unified `ValidationIssue` và các shape cũ; bắt buộc trước mọi user story.

- [x] T004 [P] Trong `component_map_editor/adapters/ValidationAdapter.cpp`, cập nhật `validationIssueToVariantMap` để xuất khóa `path` (từ `component_id`/`connection_id` theo quy ước `components/<id>...` / `connections/<id>...` / `graph` khi rỗng) và giữ `code`, `severity`, `message`, `details`, `remediation`.
- [x] T005 [P] Cập nhật `variantMapToValidationIssue` (ValidationAdapter) để đọc `path` và map ngược thành `component_id`/`connection_id` tương ứng khi serialize sang proto, giữ tương thích wire.
- [x] T006 [P] Tạo `component_map_editor/adapters/DeploymentValidationAdapter.{h,cpp}` map 2 chiều `DeploymentValidationIssue` ↔ unified `ValidationIssue`: `field_path` → `path` (giữ nguyên định danh), `code` → `code` qua `stableDiagnosticCode`, `component_identity`/`component_version` → `details`.
- [x] T007 Cập nhật `RuntimeClient::deploymentValidationIssueMap` (`component_map_editor/services/RuntimeClient.cpp`) dùng `DeploymentValidationAdapter` để trả shape unified (`code`, `severity`, `path`, `message`, `details`) thay vì shape `fieldPath`/`componentIdentity` cũ.

**Checkpoint**: Mọi adapter map về unified shape; user story implementation có thể bắt đầu.

---

## Phase 3: User Story 1 - Contract chung và error codes dùng chung (Priority: P1) 🎯 MVP

**Goal**: Cùng một lỗi logic sinh ra `code`, `severity`, `path` tương thích giữa editor validation và runtime `ValidateDeployment`.

**Independent Test**: Với một graph lỗi cố định, chạy editor `validationIssues(graph)` và runtime `ValidateDeployment(snapshot)` → assert `code`/`severity`/`path` khớp (quickstart S1 / SC-001).

### Implementation for User Story 1

- [x] T008 [US1] Cập nhật `ValidationService::validationIssues` (`component_map_editor/services/ValidationService.cpp`) sinh issue dùng `code` từ `DiagnosticCodeCatalog` (qua `stableDiagnosticCode`) và `path` theo quy ước định danh (`components/<componentId>...`, `connections/<connectionId>...`, `graph` cho `CORE_NULL_GRAPH`).
- [x] T009 [US1] Đảm bảo editor validation provider phát sinh `severity` hợp lệ (`ERROR`/`WARNING`/`INFO`); thiếu/không hợp lệ → `ERROR` (fail-closed) theo edge case trong spec.
- [x] T010 [US1] Tại runtime, đảm bảo `ValidateDeployment` sinh `DeploymentValidationIssue` với `code` qua `stableDiagnosticCode` và `field_path` dùng định danh thực thể (không chỉ mục), đồng bộ semantics với editor.
- [x] T011 [US1] Thêm mapping catalog → runtime issue trong `component_map_runtime/src/` (nơi sinh validation issue) để cùng lỗi logic ↔ cùng `code` (ví dụ `COMPONENT_PROPERTY_REQUIRED`).

**Checkpoint**: US1 độc lập functional; cùng lỗi có code/severity/path tương thích editor/runtime.

---

## Phase 4: User Story 2 - Giữ structured issue xuyên exporter, client, runtime và manager (Priority: P1)

**Goal**: Issue structured được giữ nguyên xuyên mọi boundary; chỉ UI format thành text.

**Independent Test**: Đẩy graph lỗi qua exporter → client → runtime `ValidateDeployment` → manager; assert structured shape bảo toàn ở mỗi boundary (quickstart S2 / SC-002).

### Implementation for User Story 2

- [x] T012 [US2] Đảm bảo `ValidationService::validationIssues` trả `QVariantList` structured (không `QStringList` chuỗi) làm contract liên tầng; giữ `validationErrors()`/`validate()` chỉ làm tiện ích UI/text.
- [x] T013 [US2] Đảm bảo exporter (`component_map_editor/deployment/` hoặc tương đương) truyền issue structured vào deployment snapshot, không flatten thành chuỗi.
- [x] T014 [US2] Đảm bảo `RuntimeClient` truyền structured issue từ runtime về editor (qua `DeploymentValidationAdapter`) và manager xử lý structured list, chỉ UI format text.
- [x] T015 [US2] Đảm bảo trường tùy chọn `details`/`remediation` (nếu có) được giữ nguyên qua mọi boundary (không bị cắt bỏ).

**Checkpoint**: US1 + US2 independent functional; structured issue bảo toàn xuyên boundary.

---

## Phase 5: User Story 3 - Ổn định path để test và automation tham chiếu (Priority: P2)

**Goal**: `path` ổn định, dùng định danh thực thể, tham chiếu được bằng `code` + `path`.

**Independent Test**: Graph cố định chạy validation ≥10 lần và sau reorder entity → assert `path` hằng số; automation test tham chiếu issue bằng `code`+`path` ổn định (quickstart S4/S5 / SC-004/SC-005).

### Implementation for User Story 3

- [x] T016 [US3] Chuẩn hóa hàm sinh `path` tập trung (trong `ValidationAdapter` hoặc util) theo quy ước: `components/<componentId>.properties.<prop>`, `components/<componentId>.ports.<portId>`, `connections/<connectionId>.<field>`, `graph` cho lỗi root; KHÔNG dùng chỉ mục mảng.
- [x] T017 [US3] Đảm bảo editor và runtime dùng cùng hàm sinh `path` (hoặc map tương đương) để `path` đồng dạng (cùng cú pháp) giữa hai bên.
- [x] T018 [US3] Đảm bảo `path` không đổi khi thứ tự entity trong graph thay đổi (chỉ định danh) — coverage test qua reorder.

**Checkpoint**: Tất cả user stories independent functional; `path` ổn định và tham chiếu được.

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Edge cases, severity behavior, và xác minh end-to-end theo quickstart.

- [x] T019 [P] Xử lý edge case fail-closed: issue thiếu/không hợp lệ `severity` → `ERROR` (chặn) trong cả editor (`ValidationService::issueIsError`) và runtime.
- [x] T020 [P] Xử lý edge case runtime không tin client string: `ValidateDeployment` luôn re-validate snapshot và sinh structured issue riêng (FR-004).
- [x] T021 [P] Xử lý edge case warning đi cùng error: result giữ cả hai, deployment bị chặn bởi error nhưng warning vẫn hiển thị/ghi nhận (FR-005/FR-009, SDD D9).
- [x] T022 [P] Đảm bảo `GraphValidationResult.is_valid` = false khi có bất kỳ `ERROR` nào; warning không làm `is_valid=false`.
- [x] T023 Chạy các kịch bản validation trong `quickstart.md` (S1–S7) để xác minh end-to-end: code/severity/path tương thích, structured bảo toàn, warning không mất data, path ổn định, runtime re-validate, fail-closed.
- [x] T024 Cập nhật tài liệu/comment tại `ValidationService`, `RuntimeClient`, `ValidationAdapter` để ghi rõ hợp đồng issue chuẩn hóa (chỉ UI format text).

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: Không dependency — bắt đầu ngay.
- **Foundational (Phase 2)**: Phụ thuộc Setup; BLOCKS mọi user story.
- **User Stories (Phase 3+)**: Phụ thuộc Foundational; có thể chạy tuần tự P1→P2→P3 hoặc song song nếu đủ người.
- **Polish (Phase 6)**: Phụ thuộc các user story mong muốn hoàn thành.

### User Story Dependencies

- **US1 (P1)**: Sau Foundational; không dependency story khác.
- **US2 (P1)**: Sau Foundational; có thể song song với US1 (file khác: service/exporter/client).
- **US3 (P2)**: Sau Foundational; có thể song song với US1/US2.

### Within Each User Story

- Setup catalog/adapter trước → service/runtime → xác minh.
- Story hoàn thành trước khi chuyển priority tiếp theo.

### Parallel Opportunities

- T001/T002/T003 (Phase 1) chạy song song.
- T004–T007 (Phase 2) adapter song song trong phase.
- US1/US2/US3 có thể song song sau Foundational (developer khác nhau).
- T019–T022 (Polish) đánh dấu [P] chạy song song.

---

## Parallel Example: User Story 1

```bash
# Sau Phase 1+2, implement US1:
Task: "T008 Update ValidationService::validationIssues to emit catalog code + stable path"
Task: "T010 Runtime ValidateDeployment emits stableDiagnosticCode + identity-based field_path"

# Song song US2 (file khác):
Task: "T012 Ensure ValidationService::validationIssues returns structured QVariantList"
Task: "T014 RuntimeClient passes structured issue via DeploymentValidationAdapter"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Phase 1: Setup (catalog + stableDiagnosticCode + proto path/details).
2. Phase 2: Foundational (adapter map 2 chiều).
3. Phase 3: US1 (code/severity/path tương thích editor/runtime).
4. **STOP and VALIDATE**: Chạy quickstart S1 (SC-001).
5. Demo nếu sẵn sàng.

### Incremental Delivery

1. Setup + Foundational → foundation ready.
2. US1 → test độc lập (S1) → MVP.
3. US2 → test độc lập (S2) → structured xuyên boundary.
4. US3 → test độc lập (S4/S5) → path ổn định.
5. Polish → edge cases + quickstart S1–S7.

### Parallel Team Strategy

1. Team hoàn thành Setup + Foundational cùng nhau.
2. Sau Foundational: Dev A = US1, Dev B = US2, Dev C = US3.
3. Mỗi story hoàn thành và tích hợp độc lập.

---

## Notes

- [P] tasks = file khác, không dependency.
- [Story] label map task tới user story để traceability.
- Mỗi user story nên hoàn thành và test độc lập.
- Commit sau mỗi task hoặc nhóm logic.
- Tránh: task mơ hồ, xung đột cùng file, dependency chéo phá vỡ independence.
- Không tự giả định build/test command; lấy từ project root khi thực thi.

---

## Phase 7: Convergence

**Purpose**: Đóng khoảng cách giữa spec/plan/tasks và code hiện tại. Phát hiện dựa trên đánh giá code thực tế (không chỉ tin trạng thái checkbox).

- [x] T025 Align runtime validation codes with `DiagnosticCodeCatalog` per FR-002/SC-001 (T011, partial). Runtime `component_map_runtime/src/DeploymentValidator.cpp`, `DeploymentManager.cpp` và `FlattenCompiler.cpp` currently emit raw hardcoded code literals (e.g. `"REQUIRED_PROPERTY_MISSING"`, `"INSTANCE_ID_REQUIRED"`, `"GRAPH_VERSION_REQUIRED"`, `"COMPONENT_PROPERTY_REQUIRED"`-equivalent) instead of referencing `component_map_editor/services/DiagnosticCodeCatalog.h` constants or the shared normalize-mapped catalog. Map each runtime issue code to the corresponding `DiagnosticCodeCatalog` entry (or a runtime-local mirror of the same stable code) so the same logical error yields the same `code` as the editor (quickstart S1 / SC-001).
- [x] T026 Emit editor validation path with the same depth as runtime per FR-008/SC-001 (T016, partial). Editor validation providers and `RuleRuntimeEngine` emit only `components/<id>` / `connections/<id>` (derived from `entityType`+`entityId`), while runtime emits `components/<id>.properties.<prop>` / `components/<id>.ports.<portId>`. Extend the editor path builder (e.g. `cme::adapter::validationPathFor` in `component_map_editor/adapters/ValidationAdapter.cpp` and the provider/rule `makeIssue` call sites) to carry the sub-field (property/port) so the path is identical in shape to the runtime `field_path` for the same logical error (quickstart S1 / SC-001).

---

## Phase 8: Convergence

**Purpose**: Đóng khoảng cách mã hóa hợp đồng issue chuẩn hóa giữa editor và runtime (FR-003/SC-001). Đánh giá thực tế code cho thấy `path` shape và code-normalization đã khớp, nhưng editor thiếu validation dựa trên catalog/descriptor và runtime vẫn dùng literal code phân kỳ so với `DiagnosticCodeCatalog`, nên cùng lỗi logic chưa sinh cùng `code` 100% (S1/SC-001).

- [ ] T027 Add an editor-side validation provider that consults the runtime component catalog/descriptor (`m_componentTypeRegistry` / `m_propertySchemaRegistry`, fed from `m_runtimeCatalog`) to detect catalog-covered logical errors — missing required property, missing/invalid handler, missing descriptor — and emit the matching `DiagnosticCodeCatalog` code with `path` `components/<id>.properties.<prop>` / `components/<id>`, so the editor and runtime yield the same `code`/`severity`/`path` for the same logical error per FR-003/SC-001 (missing).
- [ ] T028 Reconcile runtime `DeploymentValidator` issue codes with `DiagnosticCodeCatalog`: use the catalog code (or the runtime-local mirror kept in sync) for catalog-equivalent logical errors instead of divergent literals — e.g. `MISSING_DESCRIPTOR` instead of `COMPONENT_DESCRIPTOR_NOT_FOUND`, `DUPLICATE_COMPONENT_ID` instead of `DUPLICATE_INSTANCE_ID` — so runtime codes match the editor/catalog for the same error per FR-002/T025 (partial).
- [ ] T029 Add an automated contract test asserting editor `ValidationService::validationIssues` and runtime `ValidateDeployment` emit identical `code`/`severity`/`path` for the shared logical-error test cases (quickstart S1–S5), satisfying SC-001 and the Constitution V contract-test quality gate; or record executed quickstart S1–S7 evidence if a test is intentionally deferred per the tasks.md note (missing).
- [ ] T030 Remove dead helpers `RuntimeClient::enrichValidationIssue`, `componentPathFromFieldPath`, `propertyPathFromFieldPath`, `portPathFromFieldPath` (never called) and unused `ValidationService::buildGraphSnapshot`; the `*FromFieldPath` helpers parse the forbidden `components[...]` array-index style that contradicts FR-008, so either delete them or rewire to the normalized path convention (unrequested).
