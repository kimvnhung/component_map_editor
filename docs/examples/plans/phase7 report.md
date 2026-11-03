# Phase 7 Report: Hoàn thiện RuntimeClient và UI monitoring

## 1. Thông tin xác minh

| Trường | Giá trị |
|---|---|
| Ngày xác minh | 2026-11-03 |
| Build directory | `build/Desktop_Qt_6_9_1_Debug` |
| Build type | Debug |
| Qt | 6.9.1 |
| C++ | C++17 |
| Editor client target | `cme_runtime_client` |
| Example application | `customize_example_app` |
| Runtime process | `component_map_runtime_process` |

## 2. Mục tiêu Phase 7

Biến `customize_example_app` thành editor và monitoring client. Runtime state, mailbox, handler và persistence tiếp tục thuộc runtime process.

## 3. Phần đã hoàn thiện

### 3.1 RuntimeClient

`RuntimeClient` hiện hỗ trợ:

- timeout mặc định cho mọi gRPC call;
- cấu hình timeout bằng `setTimeoutMs`;
- endpoint address configuration;
- channel state gồm idle, connecting, ready, transient failure và shutdown;
- reconnect bằng cách tạo lại channel và stubs;
- hủy client channel hiện tại qua `cancelPendingCalls`;
- protocol version check từ runtime status response;
- application error mapping giữ nguyên error code và error message;
- các RPC control, event và query đều dùng deadline.

Runtime status response được bổ sung `protocol_version`. Runtime server trả protocol version từ `RuntimeDescriptor`.

### 3.2 Manager facade

`ComponentMapEditorManager` tiếp tục là facade của editor và runtime client. Facade hiện expose cho QML:

- runtime status;
- active graph version;
- last error;
- channel state;
- connected state;
- reconnect command;
- configurable polling interval.

Manager tạo polling timer để refresh runtime status. Manager không giữ actor state, mailbox, execution queue, runtime handler hoặc runtime persistence.

### 3.3 UI monitoring panels

Tách `ExecutionPanel` thành các vùng QML:

- `RuntimeOverviewPanel`;
- `DeploymentPanel`;
- `ComponentStatusPanel`;
- `EventHistoryPanel`;
- `RuntimeLogPanel`;
- `RejectionPanel`.

Các command đã được biểu diễn trong UI:

- Deploy;
- Activate qua deploy flow hiện hữu;
- Start;
- Stop;
- Refresh;
- Publish Event qua manager API hiện hữu;
- Reconnect.

Execution panel dùng polling snapshot. Khi UI đóng, panel không gọi stop runtime.

### 3.4 Production UI boundary

UI monitoring chỉ hiển thị runtime status, deployment controls, component status, event history, logs và rejections. Không thêm các control debugger như step, run max steps, tick, ready queue, breakpoint hoặc engine mode.

## 4. Lệnh xác minh

```bash
cmake --build build/Desktop_Qt_6_9_1_Debug -j2
ctest --test-dir build/Desktop_Qt_6_9_1_Debug --output-on-failure \
  -R 'tst_RuntimeClient|tst_EditorQmlModuleContract|tst_EditorFacadeContract|tst_CustomizeExampleAuthoring|tst_RuntimeProcessPhase5'
git diff --check
```

Kết quả: build thành công, `5/5` test phase 7 và regression liên quan đạt, và `git diff --check` không phát hiện lỗi whitespace.

## 5. Acceptance status

- [x] RuntimeClient có timeout mặc định.
- [x] RuntimeClient có endpoint configuration.
- [x] RuntimeClient có channel state.
- [x] RuntimeClient có reconnect.
- [x] RuntimeClient có protocol version check.
- [x] RuntimeClient map application error code và message.
- [x] RuntimeClient có cơ chế hủy channel và calls hiện tại.
- [x] Manager facade gọi RuntimeClient thay vì giữ runtime execution state.
- [x] Manager map protobuf response sang `QVariantMap` và `QVariantList`.
- [x] Manager phát status changes.
- [x] UI có runtime overview.
- [x] UI có deployment controls.
- [x] UI có component status.
- [x] UI có event history.
- [x] UI có runtime logs.
- [x] UI có rejection list.
- [x] UI dùng polling snapshot.
- [x] UI không stop runtime khi panel bị đóng.
- [x] Production UI không thêm execution debugger controls.

## 6. Giới hạn còn lại

- RuntimeClient đang dùng synchronous gRPC calls. Cancellation hiện hủy channel client, còn cancellation token và asynchronous call chain có thể hoàn thiện ở phase hardening.
- Runtime status hiện là snapshot polling, chưa có streaming live updates.
- Local process discovery chưa được thêm vì endpoint configuration hiện đủ cho MVP.
- Deployment panel đang dùng flow deploy-and-activate hiện hữu; các nút activate độc lập có thể tách ở phase sau nếu protocol cần.
- Tiêu chí end-to-end đóng app, runtime tiếp tục xử lý, mở app lại và query state mới cần test process-level riêng với fixture persistent.

## 7. Kết luận

Phase 7 đã hoàn thiện RuntimeClient boundary, manager monitoring facade và UI monitoring MVP. Application hiện truy vấn và điều khiển runtime qua gRPC, không sở hữu execution state. Có thể chuyển sang phase 8 với các giới hạn nêu trên được giữ nguyên như technical debt đã biết.
