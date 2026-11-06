# Feature Specification: Core Runtime Component Discovery

**Feature Branch**: `001-component-discovery`

**Created**: 2026-11-04

**Status**: Draft

**Input**: User description: "Thiết kế lại cơ chế khai báo component. Thay vì khai báo ở phần UI, hãy tạo cơ chế discovery trong core_runtime với các mục tiêu: component built-in hoặc discovered từ network; cả hai tuân thủ một chuẩn chung để runtime đọc properties, outputProperties và cách execute; UI nhận toàn bộ thông tin component từ core_runtime qua API tương tự log và history."

## Clarifications

### Session 2026-11-04

- Q: Khi component được discovery từ network, runtime sẽ thực thi component theo mô hình nào? → A: Component chạy như một remote worker qua network; core runtime gửi input và nhận output/event theo execution contract.


## User Scenarios & Testing *(mandatory)*

### User Story 1 - Nhận danh sách component từ runtime (Priority: P1)

Là UI client, tôi muốn lấy danh sách component khả dụng từ core runtime để hiển thị
palette và không phải duy trì khai báo component riêng trong UI.

**Why this priority**: Đây là nền tảng để loại bỏ khai báo trùng lặp ở UI và bảo đảm
UI hiển thị đúng các component mà runtime thực sự có thể thực thi.

**Independent Test**: Khởi động runtime với ít nhất một built-in component, gọi API
component catalog, và xác minh UI client nhận được metadata đầy đủ để hiển thị một
component mà không cần registry riêng.

**Acceptance Scenarios**:

1. **Given** runtime đã khởi động với các built-in component hợp lệ, **When** UI yêu
   cầu component catalog, **Then** runtime trả về danh sách component cùng identity,
   display metadata, properties, output properties và thông tin execute.
2. **Given** runtime không có component khả dụng, **When** UI yêu cầu component catalog,
   **Then** runtime trả về catalog rỗng với trạng thái thành công và không làm UI lỗi.

---

### User Story 2 - Discovery component từ network (Priority: P1)

Là operator, tôi muốn runtime phát hiện component từ nguồn network được cấu hình để
mở rộng khả năng xử lý mà không cần thay đổi phần khai báo component trong UI.

**Why this priority**: Network discovery là nguồn component thứ hai được yêu cầu và
cho phép bổ sung worker độc lập với bản phát hành UI.

**Independent Test**: Cấu hình một nguồn network cung cấp component hợp lệ, chạy
quy trình discovery, sau đó xác minh component xuất hiện trong catalog và có thể được
đọc metadata như built-in component.

**Acceptance Scenarios**:

1. **Given** nguồn network phản hồi một component theo chuẩn chung, **When** runtime
   thực hiện discovery, **Then** component được thêm vào catalog với nguồn được đánh
dấu là network, metadata được giữ nguyên và execution được biểu diễn bằng remote
   worker endpoint/capability theo execution contract.
2. **Given** nguồn network timeout hoặc trả về metadata không hợp lệ, **When** runtime
   thực hiện discovery, **Then** runtime ghi nhận chẩn đoán có cấu trúc, bỏ qua
   component lỗi, và vẫn phục vụ các component hợp lệ khác.

---

### User Story 3 - Tạo và cấu hình graph bằng metadata runtime (Priority: P1)

Là người dùng graph editor, tôi muốn chọn component và cấu hình properties, output
properties, ports từ metadata do runtime cung cấp để graph được kiểm tra trước khi
chạy.

**Why this priority**: Catalog chỉ có giá trị khi UI có thể dùng cùng một nguồn dữ
liệu để tạo graph và hiển thị cấu hình nhất quán với runtime.

**Independent Test**: Lấy metadata một component từ catalog, tạo node graph bằng
identity đó, nhập giá trị property hợp lệ và không hợp lệ, rồi xác minh kết quả
validation tương ứng.

**Acceptance Scenarios**:

1. **Given** catalog chứa component với properties và output properties, **When** UI
   chọn component, **Then** UI có đủ thông tin để dựng form cấu hình, ports và mô tả
   execute mà không cần kiến thức riêng về loại component.
2. **Given** UI gửi graph có property không đúng kiểu hoặc connection không khớp port,
   **When** runtime validate graph, **Then** runtime trả về diagnostic xác định entity,
   property/port và lý do từ chối trước khi execution bắt đầu.

---

### User Story 4 - Theo dõi trạng thái discovery và tương thích (Priority: P2)

Là operator, tôi muốn biết component đến từ đâu, phiên bản nào và trạng thái
availability để xử lý component network không còn truy cập được mà không làm mất
khả năng quan sát runtime.

**Why this priority**: Component network có vòng đời và lỗi kết nối khác built-in;
metadata nguồn và trạng thái giúp vận hành an toàn.

**Independent Test**: Cho nguồn network thay đổi trạng thái sau discovery, truy vấn
catalog hoặc event stream, và xác minh trạng thái component cùng diagnostic được cập
nhật mà không làm biến mất thông tin identity của graph đã lưu.

**Acceptance Scenarios**:

1. **Given** component network đã được phát hiện nhưng nguồn tạm thời unavailable,
   **When** UI truy vấn catalog, **Then** component có trạng thái unavailable và lý do
   rõ ràng; runtime không đánh dấu component đó là executable.
2. **Given** graph đã lưu tham chiếu component theo identity và phiên bản cụ thể,
   **When** catalog có phiên bản mới, **Then** graph cũ vẫn giữ tham chiếu đã lưu và
   runtime báo incompatibility thay vì âm thầm thay thế.

### Edge Cases

- Hai nguồn cung cấp cùng identity và phiên bản component; runtime phải áp dụng quy
tắc conflict xác định và trả diagnostic nếu không thể chọn an toàn.
- Component thiếu property bắt buộc, output property hoặc execute descriptor; runtime
phải từ chối đăng ký component thay vì đưa metadata không đầy đủ cho UI.
- Network trả về dữ liệu lặp lại; discovery phải idempotent và không tạo duplicate
entry trong catalog.
- Discovery bị gián đoạn trong lúc UI đang đọc catalog; UI phải nhận một snapshot
nhất quán hoặc trạng thái lỗi có thể xử lý.
- Component khai báo output hoặc payload không tương thích với connection hiện có;
validation phải thất bại trước execution.
- Component network bị gỡ sau khi graph đã lưu; graph vẫn đọc được identity và
metadata snapshot cần thiết để báo lỗi rõ ràng.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Core runtime MUST là nguồn authoritative duy nhất cho catalog component;
UI MUST NOT cần registry khai báo component riêng để hiển thị hoặc cấu hình component.
- **FR-002**: Core runtime MUST hỗ trợ hai nguồn component: built-in cùng runtime và
component discovered từ network; mỗi entry MUST công khai source type.
- **FR-003**: Mọi component MUST tuân thủ một component contract versioned, bao gồm
stable identity, version, display metadata, properties, output properties, input/output
ports, capability flags và execution descriptor.
- **FR-004**: Component contract MUST mô tả cho mỗi property identity, type, required,
default, constraints và display metadata đủ để UI render và validate form mà không
cần logic riêng theo loại component.
- **FR-005**: Component contract MUST mô tả output properties và output ports gồm
identity, type, cardinality, constraints và display metadata để UI dựng kết quả và
runtime kiểm tra connection.
- **FR-006**: Component contract MUST mô tả execute behavior gồm capability, input
requirements, output guarantees, timeout/cancellation semantics, remote worker endpoint
hoặc capability reference, và error behavior; metadata này MUST được trả về cho UI nhưng
không được xem là quyền thay thế validation runtime.
- **FR-006a**: Network component MUST được thực thi như remote worker qua network; core
runtime MUST gửi input theo execution contract và nhận output/event hoặc structured error.
Runtime MUST NOT tải hoặc chạy mã component network trong cùng process chỉ để thực hiện
feature này.
- **FR-006b**: Remote worker execution MUST có timeout, cancellation, retry/idempotency
và unavailable semantics được mô tả trong descriptor; khi remote worker không thể truy
cập, runtime MUST ngăn execution hoặc trả failure có cấu trúc theo contract.
- **FR-006c**: Remote worker endpoint hoặc capability reference MUST không được xem là
credential; thông tin xác thực và authorization policy MUST được quản lý riêng bởi
runtime và không được trả về UI như component metadata công khai.

- **FR-007**: Runtime MUST validate component contract trước khi đăng ký vào catalog và
MUST từ chối contract thiếu trường bắt buộc, sai version hoặc có identity conflict.
- **FR-008**: Network discovery MUST có cơ chế cấu hình nguồn, timeout, retry policy,
health state và chẩn đoán có cấu trúc; lỗi một nguồn MUST không làm mất component hợp
lệ từ nguồn khác.
- **FR-009**: Discovery MUST idempotent theo component identity, source và version;
việc refresh cùng một descriptor không được tạo duplicate catalog entry.
- **FR-010**: Runtime MUST cung cấp catalog API với snapshot nhất quán, filter theo
identity/source/status/category và metadata đầy đủ theo component contract.
- **FR-011**: Runtime MUST cung cấp event hoặc history record cho các thay đổi discovery
như registered, refreshed, unavailable, rejected và removed theo format có thể tiêu
thụ tương tự cơ chế log/history hiện tại.
- **FR-012**: Runtime MUST trả về diagnostics có cấu trúc cho lỗi contract, discovery,
lookup, property validation, port compatibility và execution availability; mỗi
 diagnostic SHOULD xác định component identity và version khi có thể.
- **FR-013**: Graph node MUST tham chiếu component bằng stable identity và version
resolution; runtime MUST không âm thầm thay thế version mà graph đã lưu.
- **FR-014**: Runtime MUST validate graph sau khi resolve component metadata và trước
khi execution; component unavailable hoặc contract không tương thích MUST ngăn
execution và trả lỗi có thể xử lý.
- **FR-015**: Built-in và network component MUST trải qua cùng catalog, contract
validation, metadata response và graph validation path; khác biệt source MUST không
làm thay đổi shape của dữ liệu UI nhận được.
- **FR-016**: Catalog API MUST hỗ trợ capability negotiation hoặc contract version
để UI biết metadata nào được hỗ trợ và xử lý component contract version không tương
thích một cách rõ ràng.
- **FR-017**: Runtime MUST enforce trust and authorization policy for network
components before making them executable; component chưa được trust hoặc xác thực
MUST chỉ có thể xuất hiện ở trạng thái unavailable/rejected, không được chạy.

### Key Entities

- **Component Contract**: Chuẩn versioned mô tả identity, metadata, properties,
output properties, ports và execution behavior của một component.
- **Component Descriptor**: Một bản mô tả cụ thể được cung cấp bởi built-in registry
hoặc network source, gồm source, version, status và contract.
- **Component Catalog**: Snapshot các descriptor hợp lệ mà runtime cung cấp cho UI.
- **Discovery Source**: Nguồn network hoặc nguồn built-in cùng policy kết nối,
refresh, trust và health.
- **Property Schema**: Định nghĩa property đầu vào hoặc output, kiểu dữ liệu, ràng
buộc, mặc định và metadata hiển thị.
- **Port Contract**: Định nghĩa input/output port, kiểu payload, cardinality và khả
năng kết nối.
- **Execution Descriptor**: Mô tả capability và semantics liên quan đến execute,
remote worker endpoint/capability reference, timeout, cancellation, retry và error.
- **Remote Worker**: Service network thực thi component và trả output/event hoặc
structured error theo execution contract; runtime không chạy mã worker trong cùng process.
- **Discovery Trust Policy**: Quy tắc riêng của runtime xác định network source/worker nào
được phép đăng ký và execute; credential không thuộc metadata UI.

## Remote Execution Boundary

Network component được thực thi bởi remote worker qua network. Core runtime chịu trách
nhiệm resolve descriptor, kiểm tra trust/authorization, gửi input theo execution contract,
áp dụng timeout/cancellation/retry semantics và chuyển output/event hoặc structured error
về graph execution. UI chỉ nhận execution capability và trạng thái availability; UI không
nhận credential, không gọi remote worker trực tiếp và không quyết định quyền execute.

- **Graph Component Reference**: Identity và version resolution mà node trong graph
sử dụng để bảo đảm tính tương thích.
- **Discovery Event**: Record có cấu trúc về thay đổi trạng thái component hoặc nguồn.
- **Graph Component Reference**: Identity và version resolution mà node trong graph
sử dụng để bảo đảm tính tương thích.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Với catalog gồm tối đa 500 component descriptor hợp lệ, UI nhận được
một snapshot nhất quán trong tối đa 1 giây sau khi yêu cầu.
- **SC-002**: 100% component built-in và network hợp lệ được trả về cùng một shape
metadata contract; UI không cần thêm khai báo theo từng loại component.
- **SC-003**: 100% descriptor thiếu trường bắt buộc, sai contract version hoặc vi phạm
trust policy bị loại khỏi execution trước khi được đăng ký executable.
- **SC-004**: Trong một lần discovery có nhiều nguồn, lỗi một nguồn không làm mất
catalog của các nguồn còn lại và được phản ánh bằng discovery event có cấu trúc trong
vòng 5 giây.
- **SC-005**: Refresh lặp lại cùng descriptor ít nhất 10 lần tạo đúng một catalog
entry cho mỗi identity/source/version.
- **SC-006**: Ít nhất 95% lỗi cấu hình component và connection được UI hiển thị với
component identity cùng property/port gây lỗi, không chỉ là thông báo lỗi tổng quát.
- **SC-007**: Người dùng mới có thể tìm component, xem properties và tạo một node hợp
lệ chỉ bằng metadata runtime trong tối đa 3 phút, không cần sửa code hoặc cấu hình UI.
- **SC-008**: Các graph đã lưu trước khi component network unavailable vẫn được mở và
hiển thị reference, version cùng lý do unavailable trong 100% trường hợp kiểm thử.

## Assumptions

- Core runtime và UI giao tiếp qua một API request/response cùng event stream hoặc
history stream hiện có; việc tái sử dụng cơ chế vận chuyển là một ràng buộc tích hợp,
không thay đổi yêu cầu về metadata.
- Built-in component được đóng gói và đăng ký khi runtime khởi động; network component
được phát hiện qua các nguồn được operator cấu hình.
- Versioned contract và stable identity được thiết kế trong feature này; format wire
cụ thể và phương thức transport sẽ được xác định ở giai đoạn planning.
- Phiên bản đầu tập trung vào discovery, catalog, validation và metadata delivery;
không bao gồm marketplace, tự động cài đặt binary, hoặc sandboxing execution ngoài
trust/authorization gate.
- Security policy mặc định là deny-by-default đối với network component chưa được
xác thực; chi tiết trust mechanism sẽ được quyết định trong plan.
- Network component v1 chỉ được thực thi qua remote worker; runtime không tải hoặc chạy
mã network component trong cùng process.
- Remote worker endpoint/capability reference được công khai trong execution descriptor,
nhưng credential và authorization material được runtime quản lý riêng, không trả về UI.
- Remote worker protocol hỗ trợ input, output/event, structured error, timeout,
cancellation và retry/idempotency theo execution descriptor.
- Marketplace, automatic binary installation, local plugin loading, và sandboxing local
component nằm ngoài phạm vi v1.
- Network worker failure làm component unavailable hoặc execution fail có cấu trúc; lỗi
worker không làm mất metadata graph reference đã lưu.
- Runtime vẫn là nơi quyết định component có được execute hay không; UI chỉ tiêu thụ
metadata và gửi graph/configuration.
- Security policy mặc định là deny-by-default đối với network component chưa được
xác thực; chi tiết trust mechanism sẽ được quyết định trong plan.
- Runtime không tải hoặc chạy mã network component trong cùng process; local plugin
loading nằm ngoài phạm vi v1.
- Marketplace và automatic binary installation nằm ngoài phạm vi v1.
