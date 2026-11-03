# Phase 9 Report: Hardening và Definition of Done

## 1. Thông tin xác minh

| Trường | Giá trị |
|---|---|
| Ngày xác minh | 2026-11-03 |
| Build directory | `build/Desktop_Qt_6_9_1_Debug` |
| Build type | Debug |
| CMake target runtime | `component_map_runtime_process` |
| Test count | 44/44 passed |
| Total test time | 6.40s |

## 2. Phạm vi đã thực hiện

### 2.1 Deployment boundary hardening

Deployment exporter chỉ đưa dynamic properties có trong allowlist vào runtime artifact. Các QObject/editor properties như `color`, geometry và metadata UI không còn bị export nhầm dù tên property xuất hiện trong allowlist. Test `tst_DeploymentGraphExporter::filtersRuntimePropertiesByAllowlist` đã đạt.

### 2.2 Fan-in/fan-out và execution plan

Execution plan giờ giữ lại route giữa các component đã reachable, bao gồm cả upstream branch đi vào một reachable join. Điều này bảo đảm fan-in nhận đủ token từ các nhánh độc lập thay vì chỉ nhận nhánh đi xuống từ configured start component.

Actor engine seed tất cả entry components không có incoming route, không chỉ configured start component. Token payload được ghi vào snapshot sau khi actor xử lý thành công, và telemetry của actor engine ghi nhận token read/write cùng payload bytes.

Các regression test đã đạt:

- `tst_Phase2TokenRouting` — fan-out, fan-in và collision isolation;
- `tst_Phase5ExecutionObservability` — telemetry và redaction;
- `tst_DeploymentGraphExporter` — runtime property boundary.

## 3. Operational checklist

Runbook đã được bổ sung tại [phase9 operations.md](phase9%20operations.md), bao gồm:

- configure/build runtime process;
- start runtime bằng command line;
- chọn endpoint, database và runtime ID;
- deploy/activate/start/publish/query/stop/restart smoke flow;
- database recovery và database lỗi;
- endpoint unavailable, handler failure, malformed request và protocol mismatch;
- lệnh full verification.

## 4. Verification commands

```bash
cmake --build build/Desktop_Qt_6_9_1_Debug -j2
ctest --test-dir build/Desktop_Qt_6_9_1_Debug --output-on-failure
git diff --check
```

Kết quả:

- build thành công;
- `44/44` test đạt;
- runtime process smoke test qua gRPC đạt;
- nested graph test đạt;
- persistence, route contract, actor scheduler và extension tests đạt;
- `git diff --check` không phát hiện lỗi whitespace.

## 5. Definition of Done

| Tiêu chí | Trạng thái | Bằng chứng |
|---|---:|---|
| Build runtime process | Đạt | `component_map_runtime_process` build trong full build |
| Start runtime CLI | Đạt | `tst_RuntimeProcessPhase5` khởi động process qua `QProcess` |
| Deploy graph bằng gRPC | Đạt | Runtime process smoke test |
| Activate graph | Đạt | Runtime process smoke test |
| Start runtime | Đạt | Runtime process smoke test |
| Publish event | Đạt | Runtime process smoke test |
| Kiểm tra status/history/logs | Đạt | Runtime process smoke test |
| Stop và restart process | Đạt | Persistence/runtime process coverage |
| State recovery | Đạt | `tst_RuntimePersistencePhase4`, process persistence flow |
| Nested graph | Đạt | `tst_RuntimeNestedGraphPhase6` |
| Editor/runtime boundary | Đạt | runtime target không link QML/editor compatibility |
| Full regression suite | Đạt | 44/44 tests passed |

## 6. Kết luận

Phase 9 đạt Definition of Done ở mức automated verification hiện có. Runtime process, persistence, nested graph, IPC smoke flow, deployment boundary và compatibility boundary đều có kiểm chứng tự động. Các thay đổi chưa commit; working tree chỉ gồm implementation hardening và hai tài liệu Phase 9.
