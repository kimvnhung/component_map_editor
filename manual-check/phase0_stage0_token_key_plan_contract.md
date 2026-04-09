# Stage 0 Contract: Token Key Rollout Baseline

Date: 2026-04-06
Branch: create_example

## Confirmed UX Outcomes

1. New connection always receives a token key automatically.
2. PropertyPanel provides a selectable token-key list (no manual typing) when selector mode is enabled.

## Feature Flags (Stage 0)

- `connectionTokenKeyEnabled`
  - Default: `false`
  - Purpose: gates token-key assignment on new connection creation.
- `inspectorTokenKeySelectorEnabled`
  - Default: `false`
  - Purpose: gates PropertyPanel token-key selection UI.

## Compatibility Contract

1. Old graph files that do not contain a token-key field must continue to load and execute.
2. Existing schema `textfield` key editors remain supported when selector mode is unavailable or disabled.
3. Stage 0 only introduces flags and guardrails; it does not force behavior changes by default.

## Baseline Freeze Expectations

1. No existing test behavior regresses due to Stage 0 scaffolding.
2. New Stage 0 assertions verify token-key flags are default-off.

## Verification Target (Stage 0)

- `tst_Phase0DesignBBaselineFreeze`
  - `tokenTransportFlag_defaultOn`
  - `tokenKeyFlags_defaultOff`
  - existing baseline determinism/traversal/no-live-mutation checks
