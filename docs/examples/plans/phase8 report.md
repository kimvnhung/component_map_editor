# Phase 8 Report: Compatibility cutover và retirement

## 1. Thông tin xác minh

| Trường | Giá trị |
|---|---|
| Ngày xác minh | 2026-11-03 |
| Build directory | `build/Desktop_Qt_6_9_1_Debug` |
| Build type | Debug |
| Production editor target | `libComponentMapEditor` |
| Compatibility target | `cme_editor_compat` |
| Example application | `customize_example_app` |

## 2. Mục tiêu Phase 8

Giữ legacy execution stack cho preview, migration và backward compatibility, nhưng loại bỏ nó khỏi production editor, runtime process, deploy flow và monitoring flow.

## 3. Phần đã hoàn thiện

### 3.1 Cấu trúc source

Legacy source đã được chuyển sang các boundary riêng:

- `component_map_editor/preview/GraphExecutionSandbox.*`;
- `component_map_editor/compatibility/ExecutionMigrationFlags.*`;
- `component_map_editor/compatibility/execution_internal/`;
- `component_map_editor/compatibility/builders/ExecutionPlanBuilder.*`;
- `component_map_editor/compatibility/extensions/CompositeExecutionProvider.*`.

Các include trong test và compatibility implementation đã được cập nhật theo vị trí mới. Documentation của sandbox cũng trỏ tới đường dẫn preview mới.

### 3.2 CMake target isolation

Production editor source list không còn chứa các implementation legacy. `cme_editor_compat` được tạo độc lập và nhận các compatibility source qua `target_sources`.

Production target tiếp tục link editor core, runtime client và QML module. Production target không link `cme_editor_compat`.

Compatibility target giữ alias `cme_editor_compat_alias` để các consumer cũ có thể opt in rõ ràng.

### 3.3 Quy tắc sử dụng

Legacy stack vẫn được giữ cho:

- preview;
- unit test DAG cũ;
- migration;
- backward compatibility.

Runtime process và production monitoring tiếp tục dùng `component_map_runtime` cùng `RuntimeClient`. Không có đường include mới từ runtime process vào compatibility stack.

## 4. Lệnh xác minh

```bash
cmake --build build/Desktop_Qt_6_9_1_Debug -j2
cmake --build build/Desktop_Qt_6_9_1_Debug \
  --target libComponentMapEditor cme_editor_compat customize_example_app -j2
ctest --test-dir build/Desktop_Qt_6_9_1_Debug --output-on-failure \
  -R 'tst_Phase6CutoverReadiness|tst_ExecutionPlanBuilder|tst_SequentialExecutionEngine|tst_ActorMailboxEngine|tst_ActorEngineIntegration|tst_Phase1ExecutionSemanticsV2Compatibility|tst_Phase6TypedExecutionEvents'
git diff --check
```

Kết quả:

- production editor build thành công;
- compatibility target build thành công độc lập;
- `7/7` test cutover và compatibility đạt;
- `git diff --check` không phát hiện lỗi whitespace.

## 5. Acceptance status

- [x] `GraphExecutionSandbox` nằm trong preview boundary.
- [x] `SequentialExecutionEngine` nằm trong compatibility boundary.
- [x] `ActorMailboxExecutionEngine` nằm trong compatibility boundary.
- [x] `ExecutionMigrationFlags` nằm trong compatibility boundary.
- [x] `ExecutionPlanBuilder` nằm trong compatibility boundary.
- [x] Production editor không compile các legacy implementation.
- [x] Production editor không link compatibility target.
- [x] Compatibility target build độc lập.
- [x] Compatibility target có test riêng.
- [x] Runtime process không phụ thuộc compatibility target.
- [x] Production manager không expose `GraphExecutionSandbox`.
- [x] Example application vẫn dùng runtime monitoring panel.
- [x] Compatibility alias được giữ cho consumer cần opt in.

## 6. Test ngoài phạm vi

`tests/tst_Phase2TokenRouting` vẫn có hai failure liên quan token routing fan-in trong legacy sandbox. Failure này không phát sinh từ việc di chuyển source và không thuộc cutover boundary. Các test compatibility trực tiếp và test typed execution events đều đạt.

## 7. Giới hạn còn lại

- Chưa xóa migration flags vì vẫn còn test backward compatibility và legacy callers.
- Chưa đổi tên namespace/class API cũ để tránh breaking consumer; phase này chỉ đổi vị trí logic và target ownership.
- Việc xóa hẳn compatibility stack cần chờ migration documentation, preview API riêng và phân loại toàn bộ test legacy.

## 8. Kết luận

Phase 8 đạt tiêu chí cutover: production editor có thể build mà không link compatibility target, trong khi compatibility target vẫn build và kiểm thử độc lập. Legacy execution stack không còn là đường chạy mặc định của production editor hoặc runtime process.
