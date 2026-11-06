# Quickstart: Loại bỏ dead code và module ngoài phạm vi SDD

**Feature**: `003-remove-dead-code`
**Date**: 2026-11-06

Hướng dẫn xác minh end-to-end rằng việc gỡ bỏ thành công và không ảnh hưởng production. Không chứa implementation; tham chiếu `data-model.md` và `research.md`.

## Prerequisites

- Repo build được (Qt 6.9, CMake, Conan) — command build/test cụ thể lấy từ project root, không tự giả định.
- Đã apply các bước gỡ trong `tasks.md` (xóa file + dòng CMake).
- Có sẵn test harness Qt Test (`tests/`) và build production (editor app, `customize_example`, runtime).

## Validation scenarios

### S1 — Build production không chứa simulator/preview/legacy engine [FR-001, SC-001]

1. Build production editor app (và runtime).
2. **Assert**: build thành công; artifact KHÔNG link `cme_editor_compat` (target đã bị xóa). Có thể verify bằng cách grep build graph hoặc `nm`/dependency dump không chứa symbol `GraphExecutionSandbox`, `ActorMailboxExecutionEngine`, `SequentialExecutionEngine`.

### S2 — Build không chứa gRPC template-descriptor transport [FR-002]

1. Build `cme_editor_core`.
2. **Assert**: không còn file `transport/GrpcTemplateDescriptorTransport.*` trong tree; build không reference class này.

### S3 — Catalog production vẫn hoạt động (proto retained) [FR-003, SC-004]

1. Build `cme_proto` → confirm `provider_templates.proto` vẫn được compile vào `cme_proto`.
2. Chạy catalog/validation test suite (bao gồm `tst_Phase1ProtoContracts` giữ lại).
3. **Assert**: tất cả pass — unified catalog template adapters (`ComponentTypeTemplateAdapter`, `PropertySchemaTemplateAdapter`, …) vẫn link và hoạt động.

### S4 — Zero production reference tới module đã xóa [FR-004, SC-002]

1. Từ repo root, grep các symbol/path:
   - `GraphExecutionSandbox`, `TimelineModel`, `TimelineItem`, `TimelinePanel`
   - `TraversalEngine`, `PerformanceBudgets`
   - `GrpcTemplateDescriptorTransport`
   - `cme_editor_compat`
2. **Assert**: 0 kết quả trong `component_map_editor/**` và `customize_example/**` production sources (chỉ có thể xuất hiện trong comment/lịch sử, không phải code path).

### S5 — Test suite còn lại xanh [FR-006, SC-003]

1. Build & chạy toàn bộ test suite còn lại (sau khi xóa 9 test target link compat + `tst_TraversalEngine` + `tst_Phase11TemplateDescriptorTransport`).
2. **Assert**: 100% pass (không dangling test, không compile error do thiếu module).

### S6 — `customize_example` vẫn build [FR-007, SC-005]

1. Build `customize_example_app`.
2. **Assert**: thành công — vì nó chỉ phụ thuộc `provider_templates` proto (được giữ), không phụ thuộc `cme_editor_compat`.

### S7 — Runtime production behavior không đổi [FR-005]

1. Chạy kịch bản runtime validation/staging/activation cơ bản (từ feature 002 hoặc harness có sẵn).
2. **Assert**: hành vi validation, staging, activation giống trước khi gỡ (các module bị xóa không ảnh hưởng runtime path).

## Expected outcomes

- Tất cả S1–S7 pass.
- Không còn target `cme_editor_compat` trong build graph production.
- `provider_templates.proto` vẫn hiện diện và được catalog production dùng.
- Grep toàn repo trả về zero production reference tới module đã xóa.

## Notes

- Command build/test chính xác lấy từ project (README / instructions nội bộ); không tự giả định.
- Nếu build fail do một caller production chưa quét thấy → khôi phục module đó và điều tra trước khi gỡ tiếp (edge case trong spec).
