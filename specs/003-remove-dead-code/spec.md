# Feature Specification: Loại bỏ dead code và module ngoài phạm vi SDD

**Feature Branch**: `003-remove-dead-code`

**Created**: 2026-11-06

**Status**: Draft

**Input**: User description: "Đơn giản hóa: xóa dead code và module ngoài phạm vi SDD. SDD cấm editor-side simulate/preview/execute thử graph (F1.4, F5.1, mục 14.15 out-of-scope). Gỡ local execution simulator + preview timeline, gỡ compatibility/** khỏi default build, xóa TraversalEngine/PerformanceBudgets/GrpcTemplateDescriptorTransport. Mục tiêu: giảm bề mặt bảo trì, không đổi behavior runtime production."

## Clarifications

### Session 2026-11-06

- Q: `provider_templates.proto` có được xóa hay giữ lại? → A: Giữ lại proto; chỉ xóa lớp `GrpcTemplateDescriptorTransport`. Proto được 14 file adapter catalog production và `customize_example` `#include`, nên xóa sẽ phá vỡ build catalog. (FR-003)
- Q: Xử lý target `cme_editor_compat` (chứa simulator + legacy engine, hiện chỉ test link)? → A: Xóa hẳn target và các test link nó; không giữ module legacy (không có owner, SDD cấm simulate). (FR-001, FR-006, FR-008)

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Loại bỏ simulator/preview/legacy execution khỏi deliverable (Priority: P1)

Người bảo trì muốn editor không còn mang theo khả năng simulate/preview/execute thử graph — thứ SDD cấm rõ (F1.4, F5.1, mục 14.15 out-of-scope). Hiện tại các module này chỉ nằm trong một target tương thích (`cme_editor_compat`) không được bất kỳ luồng UI/app production nào instantiate, nhưng vẫn được biên dịch và kéo theo timeline model/UI không được load.

**Why this priority**: Đây là cốt lõi của feature — loại bỏ code trái với quyết định SDD và giảm rủi ro bảo trì lớn nhất (≈1034 dòng simulator + engine legacy).

**Independent Test**: Sau khi gỡ, production editor build thành công và không link target tương thích chứa simulator; validation suite hiện hữu vẫn xanh.

**Acceptance Scenarios**:

1. **Given** một delivered production editor, **When** build hoàn tất, **Then** artifact KHÔNG chứa local graph execution simulator, preview timeline model, preview timeline UI, hay legacy execution engine.
2. **Given** source production, **When** quét reference (grep) các module đã gỡ, **Then** KHÔNG có file production nào còn tham chiếu đến chúng.
3. **Given** người dùng mở editor, **When** họ tác tác với graph, **Then** không có đường UI nào kích hoạt simulate/preview (đã bị gỡ).

---

### User Story 2 - Xóa các service module không có caller production (Priority: P1)

Người bảo trì muốn xóa các module dịch vụ không được dùng bởi bất kỳ luồng production nào để giảm bề mặt bảo trì.

**Why this priority**: Loại bỏ code chết rõ ràng, rủi ro thấp, tác động trực tiếp đến mục tiêu "giảm bề mặt bảo trì".

**Independent Test**: Build production và chạy test suite; assert không có lỗi biên dịch hay test fail do thiếu các module này.

**Acceptance Scenarios**:

1. **Given** source production, **When** quét caller của traversal engine và performance budgets, **Then** chỉ có chính định nghĩa và test riêng của chúng tham chiếu (không có caller production) → được xóa.
2. **Given** production build, **When** hoàn tất, **Then** hai module này không còn xuất hiện trong bất kỳ target production nào.

---

### User Story 3 - Loại bỏ gRPC template-descriptor transport không thuộc luồng catalog đã duyệt (Priority: P2)

Người bảo trì muốn gỡ bỏ lớp transport gRPC cho template-descriptor vì unified catalog đã được SDD xác định dùng cơ chế `ComponentContract`, không dùng template-descriptor gRPC transport.

**Why this priority**: Module này chỉ được test dùng, không nằm trong luồng catalog runtime đã chuẩn hóa; gỡ bỏ giảm rủi ro bảo trì.

**Independent Test**: Build production và grep reference; assert transport class đã bị xóa và test chuyên biệt của nó bị xóa cùng (không dangling).

**Acceptance Scenarios**:

1. **Given** source production, **When** quét caller của gRPC template-descriptor transport, **Then** chỉ có test chuyên biệt tham chiếu → transport class và test đó bị xóa.
2. **Given** production build, **When** hoàn tất, **Then** không còn file production nào tham chiếu transport class này.

---

### User Story 4 - Giữ lại provider-templates contract dùng bởi unified catalog (Priority: P2)

Người bảo trì cần đảm bảo unified component catalog tiếp tục hoạt động. Khác với premise ban đầu, `provider_templates.proto` KHÔNG phải dead code: nó được 14 file production trong các template adapter của unified catalog (`ComponentTypeTemplateAdapter`, `PropertySchemaTemplateAdapter`, `TemplateProtoHelpers`, …) và `customize_example` sử dụng.

**Why this priority**: Sai sót nếu xóa proto này sẽ phá vỡ catalog production; do đó phải giữ lại dù transport layer đi kèm bị gỡ.

**Independent Test**: Build production và chạy catalog/validation test suite; assert catalog unified vẫn hoạt động bình thường với proto được giữ.

**Acceptance Scenarios**:

1. **Given** production build, **When** hoàn tất, **Then** `provider_templates` contract VẪN được biên dịch và linked bởi các catalog adapter production.
2. **Given** catalog test suite, **When** chạy, **Then** tất cả test pass (không regress do việc gỡ transport).

---

### User Story 5 - Xác nhận không reference treo và test suite xanh (Priority: P3)

Người bảo trì muốn bằng chứng cuối cùng rằng việc gỡ bỏ không ảnh hưởng production path.

**Why this priority**: Là bước xác nhận chất lượng, đảm bảo acceptance tổng thể.

**Independent Test**: Chạy toàn bộ test suite hiện hữu sau khi gỡ; assert green. Grep toàn repo đảm bảo zero reference production tới module đã xóa.

**Acceptance Scenarios**:

1. **Given** toàn bộ source sau gỡ, **When** grep reference tới mọi module đã xóa, **Then** không còn reference production nào (không có module legacy được giữ).
2. **Given** test suite, **When** chạy, **Then** tất cả test hiện hữu pass (hoặc test cho module bị xóa bị xóa cùng, không dangling).
3. **Given** `customize_example`, **When** build, **Then** vẫn build thành công (vì phụ thuộc vào proto được giữ, không phụ thuộc vào `cme_editor_compat`).

---

### Edge Cases

- Khi một module bị xóa hóa ra vẫn có caller production chưa quét thấy → build/test fail; hệ thống MUST phục hồi module đó và điều tra trước khi gỡ tiếp (không để production broken).
- Khi test suite có test chuyên biệt cho module bị xóa (kể cả 8 test target link `cme_editor_compat`) → test đó MUST bị xóa cùng module, không để lại test dangling.
- Khi `customize_example` phụ thuộc vào artifact bị ảnh hưởng → MUST build tiếp tục thành công (hiện tại nó chỉ phụ thuộc `provider_templates` proto được giữ, không phụ thuộc `cme_editor_compat`).
- Khi không có owner hay kế hoạch cutover được duyệt cho simulator → module MUST bị xóa hẳn (không giữ lại module legacy không người sở hữu).

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Production editor MUST NOT compile hoặc link local graph execution simulator, preview timeline model, preview timeline UI, hay legacy execution engine.
- **FR-002**: Production build MUST NOT chứa gRPC template-descriptor transport class.
- **FR-003**: System MUST retain `provider_templates` contract (proto) vì nó backing unified component catalog production.
- **FR-004**: Không có file source production nào MUST tham chiếu bất kỳ module đã bị gỡ.
- **FR-005**: Production runtime validation và catalog behavior MUST không đổi so với trước khi gỡ (không regression).
- **FR-006**: Existing test suite MUST vẫn xanh sau khi gỡ (các test cho module bị xóa MUST bị xóa cùng, không để test dangling).
- **FR-007**: `customize_example` MUST tiếp tục build thành công (phụ thuộc vào proto được giữ).
- **FR-008**: Bất kỳ test chuyên biệt nào cho module bị xóa MUST bị xóa cùng module, không để test dangling; không giữ lại module legacy riêng.

### Key Entities

- **Removed Modules**: local graph execution simulator, preview timeline (model + UI), legacy execution engine (target tương thích), traversal engine service, performance budgets service, gRPC template-descriptor transport.
- **Retained Contract**: `provider_templates` proto — backing unified component catalog template adapters trong production.
- **Compatibility Target**: `cme_editor_compat` — chỉ được test link; production editor app không link target này.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 100% production build artifacts KHÔNG chứa simulator/preview/legacy execution engine (xác minh qua build config + grep).
- **SC-002**: Grep toàn bộ production sources trả về ZERO reference tới các module đã gỡ.
- **SC-003**: Existing test suite pass 100% (green) sau khi gỡ, hoặc các test cho module bị xóa bị xóa cùng (không còn test dangling).
- **SC-004**: Unified catalog và runtime validation behavior không đổi (không regression trong catalog/validation test).
- **SC-005**: `customize_example` build thành công (100%).

## Assumptions

- Các module được liệt kê (simulator, preview timeline, legacy execution engine, traversal engine, performance budgets, gRPC template-descriptor transport) đã được xác nhận không có caller production qua static reference scan; chỉ có target tương thích (chỉ test link) và test riêng tham chiếu chúng.
- Không có kế hoạch cutover được duyệt cho simulator/preview → XÓA hẳn target `cme_editor_compat` và các test link nó (không giữ module legacy không owner), phù hợp SDD cấm simulate.
- `provider_templates.proto` ĐƯỢC GIỮ LẠI vì nó backing unified catalog template adapters production (14 file) và `customize_example`; chỉ lớp gRPC transport của nó bị gỡ. Đây là hiệu chỉnh so với yêu cầu gốc "xóa provider_templates.proto" — điều kiện "nếu xác nhận không có caller production" của chính người dùng KHÔNG thỏa mãn.
- `customize_example` KHÔNG phụ thuộc `cme_editor_compat`; nó chỉ phụ thuộc `provider_templates` proto được giữ. Do đó không cần cập nhật dependency của `customize_example` với `cme_editor_compat`.
- Runtime production behavior (validation, staging, activation) không được implementation của các module bị gỡ ảnh hưởng; việc gỡ chỉ giảm bề mặt biên dịch, không đổi semantics.
