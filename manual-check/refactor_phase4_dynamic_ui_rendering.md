# Phase 4 — Dynamic UI Rendering

## Goal
Allow each business pack to define inspector/forms/panels declaratively.

## Implementation Summary
- Added runtime schema cache: `extensions/runtime/PropertySchemaRegistry`.
- Added sectioned, normalized schema model with fallback rules:
  - schema row normalization (`key`, `widget/editor`, `section`, `order`, `hint`, `options`, `validation`, `visibleWhen`)
  - schema-error fallback row when required keys are missing
  - built-in fallback component/connection forms when no provider schema exists
- Added dynamic renderer: `ui/qml/SchemaFormRenderer.qml`.
- Replaced hardcoded inspector with schema-driven `ui/qml/PropertyPanel.qml`.
- Enforced command-pipeline edits from inspector:
  - component edits -> `UndoStack::pushSetComponentProperty`
  - connection edits -> `UndoStack::pushSetConnectionProperty` / `pushSetConnectionSides`
- Opened dynamic component property updates in command pipeline by allowing non-empty dynamic property names in `UndoStack::pushSetComponentProperty`.

## Pass Conditions Verification
1. Business pack changes editor UI layout and fields via schema only
- Verified with updated sample schema (`SamplePropertySchemaProvider`) using sections, hints, visibility, options, and validation metadata.

2. Form edits update graph state through command pipeline only
- `PropertyPanel` no longer writes directly to model properties; all edits require `UndoStack`.

3. No UI freeze under rapid property edits at target graph size
- Uses command merge semantics for repeated property writes (`SetComponentPropertyCommand::mergeWith`).
- Dynamic renderer relies on lightweight `Repeater` + `Loader` with narrow per-field updates.

## Automated Validation
- Added `tests/tst_PropertySchemaRegistry.cpp`.
- Full suite passed:
  - `tst_InteractionStateManager`
  - `tst_PersistenceValidation`
  - `tst_GraphModelIndexing`
  - `tst_ExtensionContractRegistry`
  - `tst_ExtensionStartupLoader`
  - `tst_TypeRegistry`
  - `tst_GraphEditorController`
  - `tst_PropertySchemaRegistry`
