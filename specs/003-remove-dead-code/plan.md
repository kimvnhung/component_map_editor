# Implementation Plan: Loại bỏ dead code và module ngoài phạm vi SDD

**Branch**: `003-remove-dead-code` | **Date**: 2026-11-06 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/003-remove-dead-code/spec.md`

## Summary

SDD cấm editor-side simulate/preview/execute thử graph (F1.4, F5.1, mục 14.15 out-of-scope). Hiện tại source vẫn mang theo các module ngoài phạm vi này hoặc không có caller production:
- `cme_editor_compat` target (local execution simulator `GraphExecutionSandbox` + legacy execution engine + migration flags) — chỉ được 8 test link, không nằm trong production editor.
- `services/TraversalEngine.*`, `services/PerformanceBudgets.*` — không có production caller.
- `transport/GrpcTemplateDescriptorTransport.*` — chỉ 1 test chuyên biệt dùng.
- Preview timeline (`TimelineModel.*`, `TimelineItem.qml`, `TimelinePanel.qml`) — chỉ simulator reference, không được QML root load.

Mục tiêu: giảm bề mặt bảo trì, loại bỏ code trái quyết định SDD, **không đổi behavior runtime production**.

Cách tiếp cận (từ research): xóa file + dòng CMake tương ứng, xóa các test dangling, xóa target `cme_editor_compat`. **Giữ lại `provider_templates.proto`** vì nó backing unified catalog production (14 file + `customize_example`), chỉ gỡ lớp gRPC transport. Đây là 2 clarification đã chốt (Q1, Q2).

## Technical Context

**Language/Version**: C++ (Qt 6.9 / QML), CMake, Python chỉ cho tooling/test nếu cần.

**Primary Dependencies**: Qt 6, protobuf (proto/), CMake, Conan. Build graph qua static lib targets (`cme_proto`, `cme_editor_core`, `cme_editor_ui_module`, `cme_editor_compat`, `customize_example_app`) + Qt Test.

**Storage**: Không liên quan (không đổi persistence).

**Testing**: Qt Test (`tests/tst_*.cpp`). Chiến lược verify: build production + chạy test suite còn lại + grep zero reference (xem `quickstart.md` S1–S7).

**Target Platform**: Desktop (Qt/QML client editor) + standalone runtime. `customize_example` là app ví dụ.

**Project Type**: Desktop application (editor) + runtime service + example app.

**Performance Goals**: Không đổi (gỡ code chỉ giảm bề mặt biên dịch, không đổi runtime hot path).

**Constraints**: Không được break production build hay runtime behavior; build command lấy từ project root (không tự giả định).

**Scale/Scope**: ~30 file `compatibility/**` + ~9 file simulator/timeline/service/transport + 9 test file gỡ bỏ; giữ `provider_templates.proto`.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

Dựa trên `.specify/memory/constitution.md` v2.0.0:

1. **I. Contract-First Module Boundaries** — Target `cme_editor_compat` đã nằm ngoài production graph (chỉ test link). Gỡ nó KHÔNG tạo reverse dependency mới; unified catalog (`cme_proto`, template adapters) giữ nguyên. ✅ (Không vi phạm; giảm code trái SDD.)
2. **II. Unified Component Catalog** — `provider_templates.proto` được GIỮ để backing unified catalog production. Không đổi contract. ✅ (FR-003 align.)
3. **III. Immutable Deployment and Independent Runtime** — SDD cấm editor simulate/preview/execute; gỡ simulator/preview trực tiếp tuân thủ nguyên tắc này. Runtime không đổi. ✅ (Feature phục vụ nguyên tắc này.)
4. **IV. Reliable Runtime Semantics and Recovery** — Runtime execution contract không bị động đến (các module gỡ là editor-side simulator, không phải runtime path). ✅
5. **V. Observable, Portable, and Compatible Infrastructure** — Không đổi public contract hay catalog; chỉ gỡ code editor-side không dùng. ✅

**Gate result**: PASS. Không có vi phạm constitution. Không cần Complexity Tracking.

## Project Structure

### Documentation (this feature)

```text
specs/003-remove-dead-code/
├── plan.md              # This file (/speckit-plan)
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
├── contracts/           # N/A (no new external interface introduced)
└── tasks.md            # Phase 2 output (/speckit-tasks)
```

### Source Code (repository root) — affected build graph

```text
proto/
└── provider_templates.proto        # GIỮ (globbed into cme_proto; backs unified catalog)

component_map_editor/
├── CMakeLists.txt                  # Remove: compat target (312+), GrpcTemplateDescriptorTransport
│                                  #   (86/166), TraversalEngine (84/164), PerformanceBudgets (83/163),
│                                  #   TimelineModel (72/153), QML list entries (200-201)
├── preview/
│   └── GraphExecutionSandbox.{h,cpp}   # XÓA (belongs to cme_editor_compat)
├── compatibility/**                    # XÓA (30 files, cme_editor_compat)
├── services/
│   ├── TraversalEngine.{h,cpp}         # XÓA
│   └── PerformanceBudgets.{h,cpp}      # XÓA
├── transport/
│   └── GrpcTemplateDescriptorTransport.{h,cpp}  # XÓA
├── models/
│   └── TimelineModel.{h,cpp}           # XÓA
└── ui/qml/
    ├── items/TimelineItem.qml          # XÓA
    └── panels/TimelinePanel.qml        # XÓA

tests/CMakeLists.txt                   # Remove: 9 test targets link cme_editor_compat + tst_TraversalEngine
│                                     #   + tst_Phase11TemplateDescriptorTransport (delete files + blocks)
tests/tst_*.cpp                        # Delete: 9 compat tests, tst_TraversalEngine, tst_Phase11TemplateDescriptorTransport
                                        # KEEP: tst_Phase1ProtoContracts (uses retained proto, not transport)

customize_example/                     # KHÔNG đổi (links cme_editor_ui_module → cme_proto; proto retained)
```

**Structure Decision**: Chỉ sửa build graph (CMake) và xóa file; không tạo module mới, không đổi runtime hay catalog production. `cme_editor_compat` bị xóa hoàn toàn (không module legacy), phù hợp SDD cấm simulate và không có owner.

## Complexity Tracking

Không có vi phạm constitution cần biện minh. (Bảng để trống theo template.)
