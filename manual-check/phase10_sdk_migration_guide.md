# Phase 10 SDK Migration Guide (Contract Compatibility)

## Scope
This guide covers migration from legacy extension execution semantics contract v0 to current contract v1 without editing core library sources.

## Audience
Extension pack maintainers upgrading existing business packs.

## Summary
- Current core contract version: execution semantics v1.
- Core provides a built-in adapter for execution semantics v0 providers.
- Legacy packs can continue registering a v0 provider through ExtensionContractRegistry and run on current core.

## Migration Path
1. Keep legacy provider implementation unchanged (v0 signature without trace parameter).
2. Register provider through ExtensionContractRegistry using registerExecutionSemanticsProvider(...).
3. Declare metadata.contractVersions.executionSemantics = 0 in pack manifest.
4. Run compatibility_checker_tool against the manifest.
5. Address reported breaking/deprecated entries using migration hints.

## Adapter Behavior
- Adapter class: ExecutionSemanticsV0Adapter.
- Injects trace metadata:
  - adapter = executionSemantics.v0
  - providerId = legacy provider id
- Preserves legacy execution output state behavior.

## Compatibility Checker Tool
Command:

compatibility_checker_tool <manifest.json> [core-api-version]

Example:

compatibility_checker_tool component_map_editor/extensions/sample_pack/manifest.sample.workflow.legacy_v0.json 1.0.0

Output:
- JSON report with fields:
  - extensionId
  - declaredRange
  - rangeCompatible
  - breaking[]
  - deprecated[]
  - breakingCount
  - deprecatedCount
  - compatible

## Intentional Breaking Changes Catalog
- EXT-EXEC-001: execution semantics v1 trace output requirements.

## Current Deprecations Catalog
- EXT-ACTION-DEP-001: action descriptor key iconName deprecated in favor of icon.
- EXT-SCHEMA-DEP-001: property schema key widget deprecated in favor of editor.

## Verification Checklist
- Legacy pack loads and executes with adapter enabled.
- Compatibility checker reports all intentional breaking changes for legacy manifest.
- Modern manifest with current contract versions reports zero breaking changes.

## Notes
This migration path is designed so at least one legacy pack can be migrated without any core source edits.
