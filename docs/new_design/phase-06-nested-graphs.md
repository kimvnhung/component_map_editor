# Phase F: Graph như composite component

## Mục tiêu

Cho phép một graph version được đăng ký và sử dụng như một component trong graph khác,
nhưng vẫn giữ boundary, version, state và lifecycle rõ ràng. Graph phẳng của Phase 1-5
là baseline; Phase F mở rộng runtime mà không copy/paste node của graph con vào graph cha.

## Mô hình khái niệm

Có ba loại thực thể cần phân biệt:

- `GraphDefinition`: graph immutable, định danh bởi `graph_id` và `graph_version`;
- `GraphInstance`: một lần instantiate definition trong một graph cha, có `instance_id`,
  properties và state scope riêng;
- `ComponentInstance`: node native handler hoặc composite reference. Composite reference
  trỏ tới một `GraphDefinition` đã pin version.

Ví dụ:

```text
OrderGraph v7
  checkout: CheckoutGraph v3
    fraud_check: FraudGraph v2
      score -> decision
```

Một `FraudGraph v2` dùng ở hai vị trí phải tạo hai `GraphInstance`; không được dùng chung
actor, mailbox, state hoặc idempotency scope.

## Contract deployment

Mở rộng deployment contract theo hướng tương thích:

```text
GraphDefinition {
  graph_id
  graph_version
  repeated ExposedPort exposed_ports
  repeated ComponentInstance components
  repeated DirectRoute routes
}

ComponentInstance {
  instance_id
  oneof implementation {
    native_handler { component_type }
    composite_ref { graph_id, graph_version }
  }
  properties
  state_schema_version
}

ExposedPort {
  port_id
  direction
  message_type
  internal_port_id
}
```

`graph_id + graph_version` là khóa immutable. Root deployment chứa root graph và toàn bộ
`dependency closure` với đúng version đã pin. Exposed port là public API của composite;
port nội bộ không thể được route trực tiếp từ graph cha.

Không dùng `latest` hoặc resolve version ngầm. Nếu graph con thay đổi, tạo root deployment
version mới và activate atomic. Root graph và dependency versions được lưu cùng deployment
manifest để restart không phụ thuộc registry đang thay đổi.

## Identity và execution plan

- `instance_id` chỉ unique trong graph scope chứa nó.
- Runtime path là identity đầy đủ, ví dụ `root/checkout/fraud_check/score`.
- `GraphInstance` là execution scope: state key, mailbox, metrics, history và idempotency
  namespace phải bao gồm hierarchical path.
- Deployment compiler resolve graph tree thành execution plan immutable. Native component
  tạo actor như hiện tại; composite tạo boundary adapter và các actor con trong scope của nó.
- Route vào composite map từ exposed input tới input port nội bộ. Route ra composite map từ
  output port nội bộ tới exposed output. Boundary adapter phải giữ event id, idempotency key,
  fingerprint và failure code khi chuyển tiếp.
- Scheduler vẫn bảo đảm FIFO và `max_concurrency = 1` trong từng actor; graph boundary
  không tạo thêm retry semantics.

## Validation

Reject trước khi tạo actor nếu gặp:

- `graph_id + graph_version` bị thiếu hoặc dependency không tồn tại;
- exposed port trùng, sai direction, sai message type hoặc map tới port không tồn tại;
- route của graph cha truy cập port không exposed;
- duplicate instance path sau khi expand;
- dependency cycle (`A v1 -> B v1 -> A v1`), kể cả cycle qua nhiều graph;
- graph vượt giới hạn nesting depth hoặc số node sau expansion;
- composite properties/state schema không tương thích contract.

Cycle dữ liệu bên trong một graph vẫn chưa được coi là composite dependency cycle; MVP chỉ
cho phép route trực tiếp như Phase 3 và không thêm loop policy/bounded iteration.

## Lifecycle và activation

- Deploy chỉ stage manifest sau khi validate toàn bộ closure.
- Activate chỉ khi runtime stopped và commit root manifest cùng dependency snapshot atomically.
- Activate root version mới reset execution state của toàn bộ graph-instance tree; không migrate
  state giữa version khác nhau và không chuyển pending event từ deployment cũ.
- Stop/start giữ nguyên snapshot đã pin. Graph registry cập nhật version mới không ảnh hưởng
  active deployment.
- Không tự động retry event tại boundary; lỗi delivery, handler và mailbox-full được ghi như
  event thuộc hierarchical path.

## Persistence

Mở rộng các khóa hiện có:

- `graph_versions`: thêm `graph_id`, parent/dependency manifest và snapshot hash;
- `component_state`: khóa bằng `deployment_id` hoặc root version + `component_path`, không chỉ
  `component_id`;
- `event_inbox`, `event_history`, `event_rejections`, `runtime_logs`: thêm `component_path`,
  `graph_id`, `graph_version` và boundary route khi có;
- thêm bảng `graph_instances` nếu cần query lifecycle/state của từng composite instance.

Recovery đọc root manifest snapshot trước, dựng đúng execution plan rồi phục hồi state theo
path. State của cùng graph definition nhưng khác GraphInstance không được trộn.

## API và query

Giữ `DeployGraph`/`ActivateGraph` làm API root. Deployment request phải mang graph registry
snapshot hoặc danh sách definitions/version được pin; server không resolve `latest`.

Mở rộng query:

- `GetComponentStatus` nhận `component_path`;
- history/log/rejection filter được theo path, graph version và boundary;
- status trả `graph_id`, `graph_version`, parent path và instance path;
- validation trả field path như `components[0].composite_ref.graph_version`.

Có thể bổ sung `GetGraphDefinition`/`ListGraphDependencies` sau MVP, không cần để execute.

## Test matrix và exit criteria

- graph con chạy độc lập như component native qua exposed input/output;
- cùng graph con được instantiate hai lần và state không chia sẻ;
- event qua nhiều boundary giữ đúng idempotency và failure history;
- missing dependency, exposed-port violation và cycle bị reject trước actor creation;
- root deploy pin đúng dependency version, không bị ảnh hưởng bởi registry update;
- activate reset toàn bộ tree nhưng giữ history/log theo policy;
- process restart phục hồi đúng hierarchical state và execution plan;
- gRPC query theo path trả đúng status/history/log.

## Phân kỳ triển khai

1. Thêm protobuf contract và validator cho `GraphDefinition`, `CompositeRef`, `ExposedPort`.
2. Thêm registry/resolver và dependency-cycle validator.
3. Compile execution plan phân cấp với boundary adapter trong runtime core.
4. Mở rộng persistence keys và recovery.
5. Mở rộng gRPC deploy/query và viết integration test process-level.

Chưa làm trong Phase F: dynamic graph update khi runtime đang chạy, hot reload graph con,
shared state giữa instances, distributed execution, retry/dead-letter và arbitrary loop.
