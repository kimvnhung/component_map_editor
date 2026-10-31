# Phase B: Deployment graph contract tối thiểu

## Mục tiêu

Tạo deployment document nhỏ nhất để runtime dựng graph Producer -> Processor -> Sink mà không cần đọc editor QObject.

## Phạm vi

- graph version immutable;
- một active graph version;
- component type và instance id;
- component properties;
- input/output port;
- direct route;
- message type;
- state schema version;
- validation cơ bản.

## Mô hình tối thiểu

### Component instance

- `instance_id` duy nhất trong graph;
- `component_type`;
- configured properties;
- initial state schema version.

### Route

- `connection_id`;
- source instance và output port;
- target instance và input port;
- message type;
- enabled.

Chỉ hỗ trợ direct route trong MVP. Fan-out có thể dùng nhiều direct routes nhưng không cần filter hoặc delivery mode nâng cao.

## Validation bắt buộc

- duplicate instance hoặc connection id;
- port không tồn tại;
- sai direction;
- message type không tương thích;
- route trỏ tới component không tồn tại;
- graph version thiếu hoặc trùng;
- component type không có handler.

Validation result có code ổn định, severity và field path.

## Activation policy

- Runtime phải stopped trước khi activate graph version mới.
- Chỉ một graph version active tại một runtime instance.
- Không migrate state giữa graph versions.
- Activate graph mới reset component state, pending inbox và pending work.
- Event pending của graph cũ không chuyển sang graph mới.

Event history và runtime logs giữ lại theo retention một ngày, trừ khi người dùng yêu cầu reset database toàn bộ.

## Proto

Tạo `proto/runtime/deployment.proto` và dùng package `cme.runtime.v1`. Không thay đổi compatibility proto hiện có.

## Exit criteria

- Graph fixture serialize và deserialize được.
- Runtime nhận deployment document mà không link editor model.
- Clone instance giữ type chung nhưng state/config riêng.
- Route hợp lệ resolve được source port, target port và message type.
- Graph invalid trả validation code ổn định.
- Activate graph mới bị từ chối nếu runtime chưa stopped.

## Trạng thái triển khai

Phase 2 hiện có:

- `proto/deployment.proto` với deployment graph, component, port, route và validation issue;
- `DeploymentValidator` độc lập với editor model;
- `DeploymentManager` với staged graph, active graph và chính sách activate khi runtime stopped;
- test round-trip protobuf cho fixture Producer -> Processor -> Sink;
- test validation cho duplicate ID, port không tồn tại, sai direction, message type và handler thiếu;
- test clone instance giữ cấu hình riêng;
- test không cho redeploy graph version đang active.

Chạy kiểm thử:

```bash
cmake --build build/phase1_runtime --target tst_RuntimeDeploymentPhase2
ctest --test-dir build/phase1_runtime \
  -R '^tst_Runtime(DeploymentPhase2|ServicePhase1)$' --output-on-failure
```

Phase 2 chưa tạo adapter từ `GraphModel`. Runtime chỉ nhận `DeploymentGraph` protobuf, phù hợp với boundary độc lập UI của thiết kế.

Việc reset execution state và chuyển đổi pending work sẽ được hiện thực cùng runtime core ở Phase 3.

Ghi chú: contract được đặt trực tiếp dưới `proto/` để giữ tương thích với cách build protobuf hiện tại; không tạo thêm thư mục lồng nhau.

## Mở rộng sau Phase 2: composite graph

Graph definition có thể được tham chiếu như `component_type` dạng composite trong graph
khác. Contract mở rộng được mô tả tại [phase-06-nested-graphs.md](phase-06-nested-graphs.md)
và phải giữ các invariant sau:

- definition immutable, tham chiếu bằng `graph_id + graph_version`, không dùng `latest`;
- component có `oneof` native handler hoặc composite reference;
- composite chỉ công khai `exposed_ports`; route ngoài không truy cập port nội bộ;
- `instance_id` unique theo scope, runtime dùng qualified path;
- validator kiểm tra dependency closure, port binding, recursive definition và expansion limits;
- graph phẳng hiện tại là root scope không có composite, nên vẫn tương thích.

`DeploymentManager` chỉ stage graph sau khi toàn bộ definition dependency resolve được. Root
activation pin một manifest dependency bất biến; cập nhật graph con không ảnh hưởng deployment
đang active.

## Validation codes bổ sung

`GRAPH_DEFINITION_NOT_FOUND`, `GRAPH_VERSION_NOT_FOUND`, `GRAPH_DEPENDENCY_CYCLE`,
`GRAPH_RECURSION_NOT_ALLOWED`, `EXPOSED_PORT_NOT_FOUND`, `INTERNAL_PORT_NOT_EXPOSED`,
`PORT_BINDING_MESSAGE_TYPE_MISMATCH`, `QUALIFIED_INSTANCE_ID_CONFLICT` và
`GRAPH_EXPANSION_LIMIT_EXCEEDED` phải được giữ ổn định để client có thể xử lý.

## Exit criteria bổ sung cho composite

- serialize/deserialize được root graph kèm dependency closure;
- hai composite instance cùng definition có path và state scope khác nhau;
- graph dependency cycle bị từ chối trước khi tạo actor;
- route qua exposed input/output resolve deterministic.

Mở rộng runtime contract được triển khai trong Phase F, không thay đổi semantics graph phẳng
đã hoàn thành ở Phase 2.
