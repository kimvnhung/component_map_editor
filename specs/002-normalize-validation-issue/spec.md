# Feature Specification: Chuẩn hóa structured validation issue

**Feature Branch**: `002-normalize-validation-issue`

**Created**: 2026-11-06

**Status**: Draft

**Input**: User description: "Chuẩn hóa structured validation issue. Hiện trạng: runtime đã tạo validation issue, nhưng một số đường UI/API còn chuyển kết quả thành chuỗi; semantics giữa editor và runtime chưa hoàn toàn thống nhất. Mục tiêu: dùng issue contract chung gồm code, severity, path, message và details/remediation tùy chọn; phân biệt lỗi chặn deployment với warning. Thay đổi cần làm: Chuẩn hóa model/protobuf và error codes dùng chung. Giữ structured issue xuyên exporter, client, runtime và manager; chỉ format thành text ở UI. Ổn định path để test và automation có thể tham chiếu. Acceptance: cùng một lỗi có code, severity và path tương thích ở editor/runtime; warning được hiển thị nhưng không làm mất dữ liệu cấu trúc."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Contract chung và error codes dùng chung (Priority: P1)

Người thiết kế workflow và nhà phát triển component định nghĩa và sử dụng một contract validation issue duy nhất gồm `code`, `severity`, `path`, `message` và trường tùy chọn `details`/`remediation`. Một danh mục error code dùng chung được chuẩn hóa sao cho cùng một lỗi logic sinh ra cùng `code`, cùng `severity` (error/warning) và cùng `path` ở mọi đường đi.

**Why this priority**: Đây là nền tảng của feature. Nếu contract và mã lỗi không thống nhất, UI, exporter, runtime và manager sẽ tiếp tục diễn giải khác nhau và không thể đạt mục tiêu "cùng một lỗi có code, severity và path tương thích".

**Independent Test**: Có thể kiểm thử độc lập bằng một test contract xác nhận một graph lỗi cố định sinh ra danh sách issue có `code`, `severity`, `path` giống hệt nhau khi đi qua editor validation và runtime `ValidateDeployment`, ở cả model nội bộ lẫn protobuf/public API.

**Acceptance Scenarios**:

1. **Given** một danh mục error code dùng chung đã được định nghĩa, **When** cùng một lỗi logic được phát hiện ở editor và ở runtime, **Then** cả hai MUST trả về issue có cùng `code`, cùng `severity` và cùng `path`.
2. **Given** một validation issue, **When** hệ thống sinh issue đó, **Then** issue MUST chứa ít nhất `code`, `severity`, `path` và `message`, và MAY chứa `details`/`remediation`.
3. **Given** một issue có `severity = error`, **When** issue nằm trong kết quả validate deployment, **Then** deployment MUST bị chặn (stage/activate bị từ chối).
4. **Given** một issue có `severity = warning`, **When** issue nằm trong kết quả validate, **Then** stage/activate MUST được phép nhưng warning MUST được hiển thị và ghi nhận cùng kết quả validation/deployment.

---

### User Story 2 - Giữ structured issue xuyên exporter, client, runtime và manager (Priority: P1)

Dữ liệu validation issue MUST được giữ nguyên dạng cấu trúc xuyên suốt các đường: exporter (editor → snapshot), client (gRPC/API), runtime validation và manager xử lý kết quả. Chỉ tầng UI mới được chuyển issue thành văn bản để hiển thị; các tầng khác KHÔNG được chuyển issue thành chuỗi rồi truyền tiếp.

**Why this priority**: Đây là sửa trực tiếp "hiện trạng" người dùng nêu: một số đường UI/API còn chuyển kết quả thành chuỗi. Giữ structured issue là điều kiện để automation, test và downstream có thể tham chiếu.

**Independent Test**: Có thể kiểm thử bằng một integration test: đưa graph lỗi qua exporter → client → runtime `ValidateDeployment` → manager, và khẳng định payload issue vẫn giữ nguyên `code`/`severity`/`path`/`details` ở mỗi boundary (so sánh serialized vs deserialized).

**Acceptance Scenarios**:

1. **Given** một issue structured được exporter sinh ra, **When** issue được truyền qua client và runtime, **Then** runtime nhận được issue/result có cùng trường cấu trúc, không bị làm mất thành chuỗi không khôi phục được.
2. **Given** runtime trả về danh sách issue structured, **When** manager và client nhận kết quả, **Then** chúng MUST truyền tiếp structured issue, chỉ UI được format thành text.
3. **Given** một issue có `details`/`remediation`, **When** issue đi qua các boundary, **Then** trường tùy chọn MUST được giữ nguyên nếu có (không bị cắt bỏ chỉ vì một tầng không dùng tới).

---

### User Story 3 - Ổn định path để test và automation tham chiếu (Priority: P2)

`path` của issue MUST theo quy tắc ổn định, xác định (ví dụ `components[2].properties.inputARef`), độc lập với ngôn ngữ giao diện hoặc thứ tự hiển thị. Test và automation MUST có thể tham chiếu một issue cụ thể bằng `code` + `path`.

**Why this priority**: Ổn định path là điều kiện để acceptance "test và automation có thể tham chiếu" và để UI có thể highlight đúng node/property. Thấp hơn P1 vì phụ thuộc vào contract đã chuẩn hóa.

**Independent Test**: Có thể kiểm thử độc lập bằng test xác nhận với một graph cố định, `path` của mỗi issue là hằng số qua nhiều lần chạy và qua editor/runtime.

**Acceptance Scenarios**:

1. **Given** một graph lỗi cố định, **When** validation chạy nhiều lần, **Then** issue cùng logic MUST luôn có cùng `path`.
2. **Given** một issue referencing một component property, **When** issue được phát sinh từ editor và từ runtime, **Then** `path` MUST đồng dạng (cùng cú pháp định danh entity).
3. **Given** một automation test tham chiếu issue bằng `code` + `path`, **When** implementation thay đổi nội bộ, **Then** `path` của issue đó MUST không đổi nếu entity/field đích không đổi.

---

### Edge Cases

- Khi một issue có `severity` không xác định hoặc thiếu, hệ thống MUST xử lý như lỗi chặn (fail-closed) thay vì bỏ qua.
- Khi runtime nhận snapshot từ client không đáng tin cậy, runtime MUST validate lại và sinh issue structured riêng thay vì tin kết quả string của client.
- Khi `path` không thể tính (ví dụ lỗi toàn graph, không thuộc node cụ thể), issue MUST dùng `path` biểu diễn root graph ổn định (ví dụ `graph`) thay vì để trống.
- Khi cùng một lỗi logic sinh nhiều issue (ví dụ thiếu property và thiếu port), mỗi issue MUST có `path` riêng biệt để UI/automation phân biệt.
- Khi warning tồn tại cùng error, kết quả validate MUST chứa cả hai, deployment bị chặn bởi error nhưng warning vẫn MUST hiển thị.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST define một shared validation issue contract gồm các trường `code` (machine-readable), `severity` (`error` | `warning`), `path` (stable entity reference), `message` (human-readable), và các trường tùy chọn `details` và `remediation`.
- **FR-002**: System MUST maintain một shared, versioned error-code catalog covering ít nhất: duplicate component ID, missing descriptor, missing schema, invalid port metadata, missing handler/capability, incompatible component metadata, provider/discovery registration failure, và property/required-field violations.
- **FR-003**: Editor validation và runtime `ValidateDeployment` MUST use cùng một rule/contract semantics và MUST produce issue có cùng `code`, `severity` và `path` cho cùng một lỗi logic.
- **FR-004**: Runtime MUST re-validate dữ liệu nhận được từ client và MUST NOT trust client-supplied validation strings; runtime MUST produce its own structured issues.
- **FR-005**: System MUST distinguish error (chặn stage/activate) và warning (không chặn nhưng MUST được hiển thị và ghi nhận).
- **FR-006**: Exporter, client, runtime và manager MUST preserve structured validation issues across boundaries; chỉ UI layer MAY format issue thành text để hiển thị.
- **FR-007**: Shared contract MUST be usable both as internal model và as protobuf/public API representation without losing semantics (eventId/correlationId, identity, message type, payload, status, structured diagnostics).
- **FR-008**: `path` MUST follow a stable, language-agnostic convention and MUST remain constant across runs and across editor/runtime for the same logical error; test/automation MUST be able to reference an issue by `code` + `path`.
- **FR-009**: System MUST NOT lose structured data when an issue is a warning; warning MUST remain queryable/inspectable alongside errors.
- **FR-010**: System MUST return validation results as a structured list của issues (mỗi issue có `code`, `severity`, `path`, `message`, và tùy chọn `details`/`remediation`) thay vì a single concatenated string.

### Key Entities

- **ValidationIssue**: đại diện một phát hiện của validation. Trường: `code`, `severity`, `path`, `message`, `details?`, `remediation?`. Là contract chung xuyên editor, runtime, client, exporter, manager.
- **Severity**: giá trị `error` hoặc `warning`; quyết định có chặn deployment hay không.
- **ErrorCodeCatalog**: danh mục mã lỗi dùng chung, versioned, ánh xạ mỗi lỗi logic sang `code` ổn định.
- **ValidationResult**: danh sách `ValidationIssue` có phân biệt error/warning, có thể trả về từ editor validation và runtime `ValidateDeployment`.NdExployment`.
- **GraphDocument / DeploymentSnapshot**: thực thể được validate; `path` tham chiếu vào entity bên trong (component, connection, property, port).

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Cùng một lỗi logic sinh ra issue có `code`, `severity` và `path` tương thích 100% giữa editor validation và runtime `ValidateDeployment` trên bộ test cases đã định nghĩa.
- **SC-002**: 100% các đường exporter → client → runtime → manager giữ nguyên structured issue; không có boundary nào chuyển issue thành chuỗi không khôi phục được.
- **SC-003**: Warning được hiển thị và ghi nhận trong 100% trường hợp có warning, và không làm mất dữ liệu cấu trúc của bất kỳ issue nào.
- **SC-004**: `path` của mỗi issue logic cố định là hằng số qua ít nhất 10 lần chạy lặp lại và qua editor/runtime trên bộ test cases đã định nghĩa.
- **SC-005**: Mọi error-code trong danh mục dùng chung có thể được tham chiếu bởi automation test thông qua `code` + `path` một cách ổn định.

## Assumptions

- Shared contract đã tồn tại một phần (runtime đã sinh issue), nên feature này chuẩn hóa và mở rộng thay vì thiết kế từ đầu.
- Internal model và protobuf/public API có thể ánh xạ 1-1 về cùng trường; không đổi semantics khi chuyển đổi.
- `path` convention mặc định dùng cú pháp `components[<index>].properties.<prop>` / `connections[<index>].<field>` / `graph` cho lỗi root; convention cụ thể sẽ chốt trong implementation plan.
- UI layer là nơi duy nhất format issue thành text; các layer khác (exporter, client, runtime, manager) không cần sinh text.
- Error-code catalog dùng chung được version qua protocol/schema version cơ chế đã có của hệ thống, không yêu cầu graph version.
- Các đường API/protobuf hiện tại chưa chuẩn hóa sẽ được đưa về shared contract; không thêm graph-version requirement vào graph document.
