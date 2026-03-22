# Phase 11: Manual User Journey Validation Guide

## Overview

This guide provides step-by-step instructions for executing the 6 critical user journeys for Phase 11 QA validation. The tests are designed to be run manually with operator guidance, capturing both automated observations and human judgment.

## Prerequisites

1. **Build the Application**
   ```bash
   cd /home/hungkv/projects/component_map_editor
   cmake --build build/Desktop_Qt_6_9_1-Debug --target all -j$(nproc)
   ```

2. **Verify All Extension Packs Available**
   - Modern v1 Pack: `component_map_editor/extensions/sample_pack/manifest.sample.workflow.json`
   - Legacy v0 Pack: `component_map_editor/extensions/sample_pack/manifest.sample.workflow.legacy_v0.json`
   - Extended Pack: Should have extensive rules and validation schema (create if needed)

3. **Terminal Access**: Open a terminal in the project root directory

## Quick Start

### Run All User Journeys

```bash
./manual-check/phase11_user_journeys.sh
```

### Run Specific Pack & Size

```bash
# Modern pack only, small graphs
./manual-check/phase11_user_journeys.sh modern small

# Legacy compatibility pack, all sizes
./manual-check/phase11_user_journeys.sh legacy all

# Extended features pack, large graphs
./manual-check/phase11_user_journeys.sh extended large
```

## User Journey Descriptions

### Journey 1: Create & Connect
**Purpose**: Validate basic graph editing functionality and rule acceptance  
**Estimated Duration**: 2–3 minutes per scenario  
**Expected Outcome**: 5 nodes + 3 connections created and validated without errors

**Manual Steps**:
1. Launch the application with the selected pack
2. Create 5 components using the toolbar or right-click menu
3. Verify all 5 components appear in the canvas
4. Create 3 connections between the components
5. Verify all connections are valid (no error indicators)
6. Trigger validation (menu or Ctrl+Shift+V)
7. Confirm validation completes successfully

**Pass Criteria**:
- All components and connections created successfully
- No UI crashes or hangs
- Validation returns success (green status)

---

### Journey 2: Component Validation
**Purpose**: Validate extension validator correctness and error reporting  
**Estimated Duration**: 2–3 minutes per scenario  
**Expected Outcome**: Validators correctly identify schema violations

**Manual Steps**:
1. Load the pack variant
2. Create a component with invalid properties (intentionally)
   - Example: Missing required field, incorrect value type
3. Trigger validation
4. Observe error message displayed in error panel
5. Fix the invalid property
6. Re-trigger validation
7. Confirm error is no longer displayed

**Pass Criteria**:
- Validator correctly identified the schema violation
- Error message is clear and actionable
- After fix, error disappears and component is valid

---

### Journey 3: Simulate (Execute)
**Purpose**: Validate graph execution engine and trace output correctness  
**Estimated Duration**: 3–5 minutes per scenario  
**Estimated Duration (Large Graphs)**: 30–50 seconds execution + 5 min validation

**Manual Steps**:
1. Create or load a graph with ≥ 5 nodes in the selected size
2. Trigger execution (menu: Execute or toolbar button)
3. Monitor for trace output (status bar or log panel)
4. Wait for execution to complete (no timeout/hang)
5. Inspect one component's state properties after execution
6. Verify state properties changed as expected
7. **For Legacy Packs Only**: Check trace includes adapter annotation

**Pass Criteria**:
- Execution completed without crash
- Component states updated correctly
- Trace output present and accurate
- **Legacy Packs**: Trace includes `"adapter": "executionSemantics.v0"`

---

### Journey 4: Undo/Redo Stress
**Purpose**: Validate command history and state machine correctness  
**Estimated Duration**: 5–8 minutes per scenario  

**Manual Steps**:
1. Create a graph with 5 nodes and 3 connections
2. Take a mental snapshot or screenshot of initial state
3. Perform 20 rapid modifications:
   - Add/delete nodes
   - Modify component properties
   - Create/break connections
   - This should take 30–60 seconds
4. Undo 10 times (Ctrl+Z × 10)
5. Observe graph state after 10 undos (should be stable)
6. Redo 5 times (Ctrl+Y × 5)
7. Verify graph state matches expectations

**Pass Criteria**:
- All undo/redo operations completed without crash
- Graph state remained consistent throughout
- After redo, state correctly reconstructed
- No duplicate or missing nodes/connections after undo/redo

---

### Journey 5: Export/Import Cycle
**Purpose**: Validate persistence service and schema robustness  
**Estimated Duration**: 3–5 minutes per scenario  

**Manual Steps**:
1. Create a graph with 5 nodes and 3 connections
2. Set custom properties on at least one component
3. Export to JSON (File > Export or Ctrl+Shift+E)
4. Verify JSON file was created successfully
5. Clear the graph (File > New)
6. Import the exported JSON file (File > Import or Ctrl+I)
7. Verify all 5 nodes and 3 connections reappeared
8. Click on one component and verify custom properties preserved

**Pass Criteria**:
- Export created valid JSON file
- All nodes and connections re-imported
- Custom properties and metadata preserved
- No data loss or corruption detected

---

### Journey 6: Extension Lifecycle (Hot-Reload)
**Purpose**: Validate extension registry and hot-reload mechanism  
**Estimated Duration**: 4–6 minutes per scenario  
**Note**: Requires external file modification during test

**Manual Steps**:
1. Load the application with the selected pack
2. Verify extension features available (custom types, rules)
3. Create a graph using the loaded rules/types
4. **Developer Action**: Modify one rule JSON file in `component_map_editor/extensions/sample_pack/`
5. Save the modified file (should trigger automatic hot-reload)
6. Verify application didn't crash; no restart required
7. Test the modified rule by creating new components or validating
8. Confirm modified behavior works as expected

**Developer Modifications for Testing**:
- Change a rule condition or predicate
- Add a new component type property
- Modify a validation constraint
- Update rule priority or ordering

**Pass Criteria**:
- Hot-reload triggered automatically (no UI prompt needed)
- Application remained responsive; no hang or crash
- Modified rules applied immediately
- New behavior works correctly

---

## Test Result Recording

### Automated Logging
Each test run automatically generates:
- **Timestamp**: When test started
- **Log file**: `manual-check/phase11_journey_results_YYYYMMDD_HHMMSS.log`
- **Status**: PASS/FAIL/SKIP for each scenario
- **Duration**: Elapsed time in seconds
- **Notes**: Any issues or observations

### Manual Observations
During each journey step, you'll be prompted with observation checks:
- Answer: `1` = Pass/Yes
- Answer: `2` = Fail/No
- Or enter free-form observation text

### Result Aggregation
After all journeys complete, summary statistics are printed:
- Total Passed: X
- Total Failed: Y
- Pass Rate: Z%

---

## Troubleshooting

### Application Won't Start
```bash
# Rebuild with clean configuration
cmake --build build/Desktop_Qt_6_9_1-Debug --target clean
cmake --build build/Desktop_Qt_6_9_1-Debug --target all -j$(nproc)
```

### Extension Pack Not Loading
- Verify manifest JSON files exist and are valid
- Check `EXAMPLE_EXTENSION_MANIFEST_DIR` environment variable is set
- Look for pack loading errors in application log

### Script Not Executable
```bash
chmod +x manual-check/phase11_user_journeys.sh
```

### Hot-Reload Not Triggering (Journey 6)
- Ensure rule file is in the watched directory
- Try modifying `component_map_editor/extensions/sample_pack/rules/*.json`
- Check RuleHotReloadService logs for watch errors

---

## Pass/Fail Criteria Summary

### Overall Phase 11 Pass
**All of the following must be true:**
1. ✓ All 6 user journeys pass on all pack variants (18/18 scenarios = PASS)
2. ✓ No unhandled exceptions or segmentation faults
3. ✓ No data corruption in export/import
4. ✓ Performance gates met (latency, memory)
5. ✓ Undo/redo history consistency verified

### Per-Journey Pass
- **Create & Connect**: 5 nodes + 3 connections created; validated successfully
- **Component Validation**: Schema violations correctly detected; errors cleared after fix
- **Simulate (Execute)**: Execution completed; trace output present; states updated
- **Undo/Redo**: 20 operations performed; undo/redo completed without crash
- **Export/Import**: JSON roundtrip; all properties preserved
- **Extension Lifecycle**: Hot-reload succeeded; rules applied; no restart needed

---

## Sign-Off

After running all journeys, provide results to:
- **QA Lead**: Responsible for journey execution and defect triage
- **Architecture Lead**: Reviews stability and extension contract adherence
- **Product Owner**: Reviews feature completeness vs. roadmap

Required sign-off format:
```
Date: [YYYY-MM-DD]
Executed By: [Tester Name]
Packs Tested: [modern|legacy|extended] 
Sizes Tested: [small|medium|large]
Journeys Passed: X/6
Overall Status: [PASS|FAIL]
Critical Defects Found: [Count]
High Defects Found: [Count]
Signature: _________________
```

---

## Advanced: Parallel Execution (For Multiple Testers)

Run journeys in parallel on different workstations:

**Workstation A**: Modern pack, all sizes
```bash
./manual-check/phase11_user_journeys.sh modern all
```

**Workstation B**: Legacy pack, all sizes
```bash
./manual-check/phase11_user_journeys.sh legacy all
```

**Workstation C**: Extended pack, all sizes
```bash
./manual-check/phase11_user_journeys.sh extended all
```

Consolidate results in a master log:
```bash
cat manual-check/phase11_journey_results_*.log > manual-check/phase11_consolidated_results.log
```

---

## See Also

- [QA Matrix Strategy](phase11_qa_matrix_strategy.md)
- [Test Scenarios Matrix](phase11_test_scenarios_matrix.md)
- [Performance Gates](phase11_performance_gates.json) (generated after benchmark runs)
- [Defect Log](phase11_defect_log.md)
