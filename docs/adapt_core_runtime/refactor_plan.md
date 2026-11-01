Sau phase 1, editor đã có exporter sang `DeploymentGraph`, nhưng vẫn đang chứa execution simulator và nhiều đường xử lý cũ. Ranh giới hiện tại mới tách ở mức contract, chưa tách ở mức module và runtime flow.

## Hiện trạng

### Runtime độc lập đã tồn tại

Runtime thật nằm trong [component_map_runtime/](component_map_runtime/), gồm:

- `DeploymentManager`
- `DeploymentValidator`
- `FlattenCompiler`
- `RuntimeCore`
- `RuntimePersistence`
- `RuntimeGrpcServer`

Runtime nhận [DeploymentGraph](proto/deployment.proto) và không phụ thuộc `GraphModel`, QML hoặc editor QObject.

### Editor vẫn sở hữu execution runtime cũ

[ComponentMapEditorManager](component_map_editor/componentmapeditormanager.h) vẫn expose:

```text
editorManager.executionSandbox
```

Manager trực tiếp tạo và quản lý [GraphExecutionSandbox](component_map_editor/services/GraphExecutionSandbox.h).

Sandbox tiếp tục sở hữu:

- execution state;
- ready queue;
- pending indegree;
- token transport;
- timeline;
- execution telemetry;
- engine selection;
- provider execution.

Nó vẫn chọn giữa:

- `SequentialExecutionEngine`;
- `ActorMailboxExecutionEngine`.

Đây là execution stack thứ hai, trùng trách nhiệm với runtime độc lập.

### QML vẫn chạy sandbox

[ExecutionPanel.qml](component_map_editor/ui/qml/ExecutionPanel.qml) vẫn gọi:

- `start()`;
- `step()`;
- `run()`;
- `pause()`;
- `reset()`.

[customize_example/qml/Main.qml](customize_example/qml/Main.qml) vẫn gắn panel này vào editor.

Vì vậy UI hiện chưa deploy hoặc query runtime thật. Nó chỉ chạy mô phỏng trong process của editor.

### Build target chưa phản ánh boundary

[component_map_editor/CMakeLists.txt](component_map_editor/CMakeLists.txt) vẫn build chung:

- editor models;
- QML;
- execution sandbox;
- sequential engine;
- actor mailbox engine;
- execution adapters;
- execution plan builder;
- runtime extension implementation.

Điều này khiến việc kiểm soát dependency bằng compiler gần như không thể.

### Test suite vẫn khóa vào execution cũ

Các test sau vẫn phụ thuộc sandbox hoặc execution engine cũ:

- [tst_ActorEngineIntegration.cpp](tests/tst_ActorEngineIntegration.cpp)
- [tst_ActorMailboxEngine.cpp](tests/tst_ActorMailboxEngine.cpp)
- [tst_SequentialExecutionEngine.cpp](tests/tst_SequentialExecutionEngine.cpp)
- [tst_Phase1ExecutionSemanticsV2Compatibility.cpp](tests/tst_Phase1ExecutionSemanticsV2Compatibility.cpp)
- [tst_Phase2TokenRouting.cpp](tests/tst_Phase2TokenRouting.cpp)
- [tst_Phase5ExecutionObservability.cpp](tests/tst_Phase5ExecutionObservability.cpp)
- [tst_Phase6TypedExecutionEvents.cpp](tests/tst_Phase6TypedExecutionEvents.cpp)

Các test này cần được phân loại trước khi xóa code. Một số test xác nhận behavior legacy, không phải behavior bắt buộc của editor.

## Những phần chưa nên xóa

Các phần sau vẫn thuộc authoring và nên giữ:

- [models/](component_map_editor/models/)
- [commands/](component_map_editor/commands/)
- [GraphEditorController](component_map_editor/services/GraphEditorController.h)
- [routing/](component_map_editor/routing/)
- [ui/](component_map_editor/ui/)
- [GraphSchema](component_map_editor/services/GraphSchema.h)
- [ExportService](component_map_editor/services/ExportService.h)
- `TypeRegistry`
- property schema
- connection policy
- validation phục vụ authoring
- `TraversalEngine`

`TraversalEngine` là graph analysis của editor, không phải execution scheduler. Không nên xóa chỉ vì nó có từ “traversal”.

## Các phần cần dọn

### Nhóm execution simulator

- [GraphExecutionSandbox.h](component_map_editor/services/GraphExecutionSandbox.h)
- [GraphExecutionSandbox.cpp](component_map_editor/services/GraphExecutionSandbox.cpp)
- [services/execution_internal/](component_map_editor/services/execution_internal/)
- [ExecutionMigrationFlags.h](component_map_editor/services/ExecutionMigrationFlags.h)
- [ExecutionMigrationFlags.cpp](component_map_editor/services/ExecutionMigrationFlags.cpp)

### Nhóm coupling trực tiếp

- [ExecutionAdapter.h](component_map_editor/adapters/ExecutionAdapter.h)
- [ExecutionAdapter.cpp](component_map_editor/adapters/ExecutionAdapter.cpp)
- [ExecutionPlanBuilder.h](component_map_editor/extensions/contracts/builders/ExecutionPlanBuilder.h)
- [ExecutionPlanBuilder.cpp](component_map_editor/extensions/contracts/builders/ExecutionPlanBuilder.cpp)
- `utils/common.h`, vì file này vẫn expose `ExecutionSandboxFactory`
- `CompositeExecutionProvider`, vì nó còn tạo `GraphExecutionSandbox` lồng nhau

### Nhóm QML và tài liệu legacy

- [ExecutionPanel.qml](component_map_editor/ui/qml/ExecutionPanel.qml)
- [EXECUTION_SANDBOX_UI.md](customize_example/EXECUTION_SANDBOX_UI.md)
- hướng dẫn workflow còn gọi `GraphExecutionSandbox`
- các test chỉ phục vụ preview simulator

Không nên xóa các file này ngay. Cần thay thế caller trước.

## Plan dọn dẹp đề xuất

### Phase 2: Tạo runtime client boundary

Mục tiêu là editor không còn gọi sandbox để quan sát execution.

Tạo lớp client riêng, chẳng hạn:

```text
RuntimeClient
  ├── deployGraph(DeploymentGraph)
  ├── activateGraph(graphVersion)
  ├── runtimeStatus()
  ├── publishEvent()
  ├── componentStatus()
  ├── eventHistory()
  └── runtimeLogs()
```

Client nên dùng các contract đã có:

- [proto/runtime/control.proto](proto/runtime/control.proto)
- [proto/runtime/event.proto](proto/runtime/event.proto)
- [proto/runtime/query.proto](proto/runtime/query.proto)

Không mở rộng `GrpcTemplateDescriptorTransport` cho lifecycle runtime. Template descriptor và runtime deployment là hai boundary khác nhau.

**Điều kiện hoàn tất:**

- editor có thể deploy `DeploymentGraph`;
- editor có thể đọc runtime status;
- UI không cần include `GraphExecutionSandbox`.

### Phase 3: Tách editor document khỏi runtime artifact

Exporter vừa thêm ở [DeploymentGraphExporter.h](component_map_editor/services/DeploymentGraphExporter.h) là bước đúng, nhưng cần hoàn thiện contract:

- xác định nguồn chính thức của `graph_version`;
- xác định port từ component type descriptor;
- không dùng mặc định `out`, `in`, `event` khi graph cần port thật;
- ánh xạ `state_schema_version`;
- xử lý composite component;
- kiểm tra `DeploymentValidator` trước khi deploy.

Hiện exporter đọc `sourcePort`, `targetPort` và `messageType` từ dynamic property. Đây là giải pháp chuyển tiếp, không nên là contract lâu dài.

Nên phân biệt:

```text
ExportService
  → editor document JSON

DeploymentGraphExporter
  → runtime deployment contract
```

**Điều kiện hoàn tất:**

- runtime validate được artifact do editor tạo;
- artifact không chứa dữ liệu UI như màu, kích thước hoặc routing side;
- mọi route có port và message type xác định;
- test round-trip từ graph editor sang runtime deployment pass.

### Phase 4: Thay thế ExecutionPanel

Thay [ExecutionPanel.qml](component_map_editor/ui/qml/ExecutionPanel.qml) bằng panel runtime client:

```text
Deploy → Activate → Runtime status → Query history/status
```

Panel mới không được gọi:

- `start`;
- `step`;
- `run`;
- `pause`;
- `reset` của sandbox.

Nếu vẫn cần local preview, nên đặt nó trong một module simulator tùy chọn, không nằm trong core editor. Simulator phải nhận `DeploymentGraph`, thay vì tự đọc `GraphModel` và tự xây execution snapshot.

**Điều kiện hoàn tất:**

- editor mặc định không có sandbox;
- example dùng runtime client hoặc chỉ hiển thị deployment result;
- local preview được tách khỏi production editor path.

### Phase 5: Di chuyển và phân loại tests

Chia test hiện tại thành ba nhóm:

| Nhóm | Hành động |
|---|---|
| Authoring | Giữ trong editor |
| Runtime | Chuyển sang runtime target |
| Legacy simulator | Xóa sau khi thay thế |

Các test runtime nên kiểm tra:

- deploy graph do exporter tạo;
- activation;
- publish event;
- component status;
- event history;
- persistence sau restart.

Các test sandbox không còn là regression bắt buộc của editor. Có thể giữ tạm trong target simulator riêng để so sánh behavior, sau đó xóa.

Đồng thời thay test source-inspection có absolute path trong [tst_Phase6CutoverReadiness.cpp](tests/tst_Phase6CutoverReadiness.cpp).

### Phase 6: Gỡ sandbox khỏi manager và QML module

Sau khi runtime client đã hoạt động:

1. Xóa include sandbox khỏi [componentmapeditormanager.h](component_map_editor/componentmapeditormanager.h).
2. Xóa property `executionSandbox`.
3. Xóa member và lifecycle management trong `.cpp`.
4. Xóa `ExecutionPanel.qml` khỏi danh sách QML production.
5. Xóa sandbox và execution files khỏi [component_map_editor/CMakeLists.txt](component_map_editor/CMakeLists.txt).
6. Xóa `ExecutionSandboxFactory` khỏi [utils/common.h](component_map_editor/utils/common.h).
7. Build editor mà không link execution implementation.

**Điều kiện hoàn tất:**

```text
libComponentMapEditor
  không compile actor
  không compile mailbox
  không compile execution plan
  không expose execution state
```

### Phase 7: Cô lập hoặc xóa extension runtime cũ

[extensions/runtime/](component_map_editor/extensions/runtime/) cần được phân loại kỹ hơn:

- schema và type registry phục vụ authoring thì giữ hoặc chuyển thành shared contract client;
- rule runtime và execution provider thì chuyển sang runtime hoặc simulator;
- composite execution provider cần loại khỏi editor sau khi sandbox bị gỡ;
- sample execution providers chỉ giữ trong runtime fixture hoặc simulator test.

Không nên xóa toàn bộ thư mục cùng lúc vì property schema và type descriptor có thể vẫn được UI dùng.

### Phase 8: Tách target CMake

Kiến trúc đích nên là:

```text
cme_graph_contracts
  ├── protobuf contract
  └── deployment types

cme_editor
  ├── GraphModel
  ├── commands
  ├── controller
  ├── routing
  ├── rendering
  └── QML

cme_runtime_client
  ├── DeploymentGraphExporter
  ├── gRPC runtime client
  └── runtime query models

component_map_runtime
  ├── execution
  ├── persistence
  ├── deployment
  └── gRPC server

cme_editor_compat
  └── legacy migration adapters
```

Dependency direction:

```text
cme_editor → cme_graph_contracts
cme_runtime_client → cme_graph_contracts
component_map_runtime → cme_graph_contracts

cme_editor không → execution_internal
component_map_runtime không → QML hoặc routing
```

## Thứ tự thực hiện an toàn

1. Hoàn thiện metadata và validation của `DeploymentGraphExporter`.
2. Tạo runtime client cho control, event và query.
3. Viết integration test exporter → runtime validator → runtime manager.
4. Chuyển UI khỏi `GraphExecutionSandbox`.
5. Chuyển hoặc xóa các test simulator.
6. Gỡ sandbox khỏi manager và CMake.
7. Tách extension runtime và compatibility target.
8. Xóa migration flags, execution adapter và execution plan builder khi không còn caller.
9. Cập nhật README và tài liệu sandbox để phản ánh kiến trúc mới.

## Kết luận

Sau phase 1, editor đã có đường tạo `DeploymentGraph`, nhưng chưa phải authoring-only vì vẫn:

- khởi tạo sandbox trong manager;
- chạy execution trực tiếp trong QML;
- compile actor và mailbox engine;
- giữ execution provider trong editor;
- dùng test suite dựa trên simulator cũ.

Bước tiếp theo có rủi ro thấp nhất là xây runtime client và chuyển `ExecutionPanel` sang client đó. Chỉ sau khi luồng này hoạt động mới nên gỡ `GraphExecutionSandbox` và các execution internals khỏi editor.