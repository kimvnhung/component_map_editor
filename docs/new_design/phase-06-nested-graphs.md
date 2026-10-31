# Phase F: Nested graph như composite component — flattened runnable MVP

## Mục tiêu và phạm vi

Phase F cho phép một graph được dùng như component trong graph khác, nhưng runtime MVP
vẫn chạy **một root deployment trong một process**. Graph hierarchy là deployment/model
abstraction; compiler flatten toàn bộ graph tree thành execution plan phẳng.

```text
GraphDefinition hierarchy
        ↓ validate + resolve parameters
Deterministic flatten compiler
        ↓
Leaf actors + leaf-to-leaf routes
        ↓
One RuntimeEngine process
```

Graph con không tạo process, scheduler, mailbox hoặc business actor riêng. Multi-process,
message broker, distributed partitioning và horizontal scaling là kiến trúc hậu MVP.

## Quyết định kiến trúc đã chốt

1. Root và nested graph dùng chung `GraphDefinition` model.
2. Deployment gửi kèm toàn bộ dependency closure đã resolve và pin version.
3. Không cho phép structural recursion; dependency cycle bị reject.
4. Có giới hạn cố định cho depth, expanded components/routes và deployment size.
5. Graph cha chỉ truy cập exposed/public ports.
6. Boundary mapping là one-to-one trong MVP.
7. Exposed port mapping khai báo explicit trong definition.
8. Public ports của instance luôn derive từ definition đã pin.
9. Composite không tạo actor riêng.
10. Boundary được rewrite thành route leaf-to-leaf.
11. Execution plan deterministic và có hash.
12. Qualified path là runtime identity.
13. Composite không có business state trong MVP.
14. Graph instance hỗ trợ static parameter binding; không có dynamic expression.
15. `PublishEvent` chỉ xác nhận initial enqueue.
16. Mỗi delivery có event ID riêng; `root_event_id` giữ xuyên flow.
17. Event-flow cycle bị cấm trong MVP.
18. Fan-out nội bộ graph phẳng vẫn giữ như Phase 3; composite boundary không fan-out implicit.
19. Fan-in không do compiler tạo; chỉ thực hiện bởi explicit aggregator component.
20. Activate root version reset toàn bộ execution state.
21. Active pointer chỉ commit sau validate, compile và actor preparation thành công.
22. Không hot-swap graph con khi runtime đang chạy.
23. Persistence key là `deployment_id + qualified_path`.
24. Deployment snapshot được lưu; pending event không chuyển sang plan mới.
25. Client được publish tới public composite port.
26. Query hỗ trợ status của composite và descendants.
27. Graph phẳng và flat client vẫn backward-compatible.

## Graph model

Mọi graph, kể cả root, đều là `GraphDefinition`:

```text
GraphDefinition {
  graph_id
  graph_version
  exposed_ports[]
  components[]
  routes[]
}

Deployment {
  deployment_id
  root_graph_id
  root_graph_version
  definitions[]       // root + dependency closure
  dependency_manifest
}
```

Một `ComponentInstance` có đúng một implementation:

```text
native_handler { component_type }
hoặc
composite_ref { graph_id, graph_version }
```

Composite instance chỉ lưu local `instance_id`, reference và static parameters. Port của
instance không được override; chúng được derive từ `exposed_ports` của definition.

## Exposed port và boundary

Mapping phải explicit:

```text
ExposedPort {
  port_id
  direction
  message_type
  internal_instance_id
  internal_port_id
}
```

MVP yêu cầu:

- public input map tới đúng một internal input;
- public output map từ đúng một internal output;
- endpoint phải tồn tại và message type phải tương thích;
- graph cha không được route tới internal port chưa expose;
- composite boundary không tự tạo fan-out hoặc fan-in.

Fan-out nội bộ vẫn có thể dùng nhiều direct route như Phase 3. Nếu cần fan-out qua
composite, graph phải chứa một component fan-out explicit. Fan-in chỉ được thực hiện bởi
một component aggregator explicit; compiler không tự ghép nhiều output.

## Static parameter binding

Graph definition có thể khai báo parameter schema. Graph instance cung cấp giá trị tĩnh:

```text
FraudGraph definition: threshold: float
fraudCheck instance: threshold = 0.8
```

Compiler validate type/range rồi resolve parameter thành execution plan. Không hỗ trợ
expression động, runtime parameter mutation hoặc tự động truyền toàn bộ properties của
composite xuống mọi child.

## Flatten compiler

Compiler thực hiện:

1. load root definition và dependency closure;
2. validate definition/version/reference/port/parameter;
3. reject structural dependency cycle;
4. expand từng composite theo local scope;
5. tạo qualified path deterministic, ví dụ `root/fraudCheck/score`;
6. resolve exposed input/output qua các cấp composite;
7. rewrite boundary route thành leaf-to-leaf route;
8. giữ source definition path, graph scope và boundary metadata;
9. validate event-flow cycle policy và expansion limits;
10. tính `execution_plan_hash`.

Chỉ leaf native component được tạo actor. Composite chỉ còn metadata phục vụ status,
trace và persistence. Không có composite mailbox hoặc boundary queue riêng.

Execution plan cần giữ tối thiểu:

```text
Leaf:
  qualified_path
  component_type
  graph_scope
  definition_id/version
  state_schema_version
  resolved_properties

Route:
  route_id
  source_leaf_path/port
  target_leaf_path/port
  message_type
  source_definition_path
  target_definition_path
  boundary_path
```

## Event semantics

`PublishEvent` chỉ xác nhận event đã enqueue vào target leaf ban đầu. Runtime không chờ
toàn bộ flow hoàn tất.

Mỗi delivery có metadata riêng:

```text
event_id
root_event_id
causation_id
idempotency_key
hop_count
max_hops
source_path
target_path
```

MVP cấm route cycle, nên `max_hops` chủ yếu là guard defense-in-depth. Event-flow cycle
không được nhầm với structural dependency cycle; cả hai đều bị reject trong MVP nhưng mã
lỗi khác nhau.

Mailbox-full, handler failure và delivery failure giữ qualified target path, graph scope,
route ID và event lineage. Không rollback delivery đã enqueue và không automatic retry.

## State, status và lifecycle

Composite không có business state. State chỉ thuộc leaf actor và dùng key:

```text
(deployment_id, qualified_path)
```

Composite status là aggregate:

```text
stopped nếu runtime stopped
failed  nếu có leaf failed
busy    nếu không failed nhưng có leaf busy
idle    nếu tất cả leaf idle
```

Activation:

```text
validate dependency closure
→ compile execution plan
→ prepare all leaf actors
→ persist deployment snapshot
→ commit active pointer
```

Nếu bất kỳ bước nào thất bại, activation rollback và active deployment cũ không đổi.
Graph con không hot-swap khi runtime running. Muốn đổi dependency version phải deploy
root version mới, stop, activate và start lại.

## Persistence và recovery

Lưu deployment snapshot gồm:

```text
deployment_id
root graph id/version
dependency manifest
definition hashes
execution_plan_hash
serialized definitions
```

Mở rộng record với:

```text
component_path
parent_graph_scope
graph_id
graph_version
root_event_id
causation_id
execution_plan_hash
```

Restart compile lại từ snapshot đã lưu, không resolve `latest`. Pending event gắn với
deployment/plan cũ không được route sang plan mới; activation reset execution state và
pending work của toàn bộ tree nhưng giữ history/log theo retention.

## API và compatibility

`DeployGraph` nhận deployment envelope hoặc adapter tương thích cho flat graph. Client có
thể publish tới:

```text
root/fraudCheck/in
```

nhưng không được publish tới internal path. Query hỗ trợ:

```text
GetComponentStatus(path, include_descendants)
ListEventHistory(deployment_id, path, root_event_id)
ListRuntimeLogs(deployment_id, path)
ListRejections(deployment_id, path)
```

Flat graph là root definition không có composite. Flat client tiếp tục dùng local ID qua
compatibility adapter; runtime nội bộ dùng `root/<local_id>`.

## Giới hạn cố định MVP

Các giới hạn phải được cấu hình thành policy cố định và trả validation code ổn định:

```text
GRAPH_DEPTH_EXCEEDED
GRAPH_EXPANSION_LIMIT_EXCEEDED
GRAPH_DEPLOYMENT_SIZE_EXCEEDED
GRAPH_DEPENDENCY_CYCLE
GRAPH_EVENT_FLOW_CYCLE
```

MVP không bao gồm:

- composite process/service riêng;
- distributed execution hoặc nhiều process cho cùng deployment;
- broker, service discovery, partition/rebalance;
- dynamic graph update/hot reload;
- arbitrary event loop;
- implicit fan-in/fan-out tại boundary;
- shared state giữa graph instances;
- state migration;
- retry/dead-letter/exactly-once.

## Test matrix và exit criteria

- root phẳng vẫn deploy/chạy như trước;
- graph con one-level chạy qua public input/output;
- graph nhiều cấp flatten đúng qualified path;
- hai instance cùng definition có state độc lập;
- missing dependency, duplicate definition, invalid binding và cycle bị reject trước actor;
- parameter binding resolve deterministic;
- boundary route không truy cập internal port;
- fan-out nội bộ hoạt động, boundary fan-out bị reject;
- fan-in implicit bị reject, explicit aggregator hoạt động như leaf;
- execution plan hash ổn định;
- activation rollback khi leaf preparation thất bại;
- restart phục hồi đúng state theo deployment/path;
- active version không bị ảnh hưởng bởi definition version mới;
- query composite status có/không descendants;
- flat client compatibility vẫn hoạt động.

## Hậu MVP

Execution plan phẳng là nền tảng để sau này partition theo component path/partition key
và chạy nhiều worker process. Khi đó có thể thêm transport, state ownership, broker,
replica và rebalancing mà không thay đổi public GraphDefinition hoặc exposed-port contract.
Những phần đó không thuộc runnable MVP Phase F.
