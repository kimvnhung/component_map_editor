#!/bin/bash

################################################################################
# Phase 11: Manual User Journey Validation Script
# 
# Purpose: Guided manual testing of 6 key user journeys across multiple
#          business packs and graph sizes.
# 
# Usage: ./phase11_user_journeys.sh [pack] [graph_size]
#        pack: modern|legacy|extended (default: all)
#        graph_size: small|medium|large (default: all)
# 
# Output: Results logged to manual-check/phase11_journey_results_TIMESTAMP.log
################################################################################

set -e

# Colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_ROOT}/build/Desktop_Qt_6_9_1-Debug"
APP_BIN="${BUILD_DIR}/example/component_map_editor_example"
LOG_DIR="${SCRIPT_DIR}"
LOG_FILE="${LOG_DIR}/phase11_journey_results_$(date +%Y%m%d_%H%M%S).log"
RESULT_FILE="${LOG_DIR}/phase11_journey_results_$(date +%Y%m%d_%H%M%S).json"

# Test parameters
PACKS="${1:-all}"
SIZES="${2:-all}"
SUMMARY_PASS=0
SUMMARY_FAIL=0

# ============================================================================
# UTILITY FUNCTIONS
# ============================================================================

log() {
    local level="$1"
    shift
    local message="$@"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo "[${timestamp}] [${level}] ${message}" | tee -a "$LOG_FILE"
}

log_scenario_start() {
    local scenario_id="$1"
    local pack="$2"
    local size="$3"
    local journey="$4"
    
    echo "" | tee -a "$LOG_FILE"
    log "INFO" "=== START: ${scenario_id} ==="
    log "INFO" "Pack: ${pack}, Size: ${size}, Journey: ${journey}"
    log "INFO" "Please perform the following steps manually..."
}

log_scenario_result() {
    local scenario_id="$1"
    local status="$2"  # PASS, FAIL, SKIP
    local duration="$3"
    local notes="$4"
    
    if [ "$status" = "PASS" ]; then
        echo -e "${GREEN}[PASS]${NC} ${scenario_id} (${duration}s)" | tee -a "$LOG_FILE"
        ((SUMMARY_PASS++))
    elif [ "$status" = "FAIL" ]; then
        echo -e "${RED}[FAIL]${NC} ${scenario_id} (${duration}s)" | tee -a "$LOG_FILE"
        ((SUMMARY_FAIL++))
    else
        echo -e "${YELLOW}[SKIP]${NC} ${scenario_id}" | tee -a "$LOG_FILE"
    fi
    
    if [ -n "$notes" ]; then
        log "INFO" "Notes: $notes"
    fi
}

prompt_test_step() {
    local step_num="$1"
    local description="$2"
    
    echo "" | tee -a "$LOG_FILE"
    echo -e "${BLUE}Step ${step_num}: ${description}${NC}" | tee -a "$LOG_FILE"
    echo "Please complete this step and press Enter when done, or type 'FAIL' to mark as failed:" | tee -a "$LOG_FILE"
    
    read -r response
    if [ "$response" = "FAIL" ]; then
        echo "false"
    else
        echo "true"
    fi
}

prompt_observation() {
    local question="$1"
    
    echo "" | tee -a "$LOG_FILE"
    echo -e "${BLUE}Observation Check:${NC}" | tee -a "$LOG_FILE"
    echo "$question" | tee -a "$LOG_FILE"
    echo "Enter: 1=Yes/Pass, 2=No/Fail, or observation details:" | tee -a "$LOG_FILE"
    
    read -r response
    echo "$response"
}

# ============================================================================
# USER JOURNEY 1: CREATE & CONNECT
# ============================================================================

journey_create_and_connect() {
    local scenario_id="$1"
    local pack="$2"
    local size="$3"
    
    log_scenario_start "$scenario_id" "$pack" "$size" "Create & Connect"
    
    local start_time=$(date +%s)
    local all_passed=true
    
    # Step 1: Load the application and pack
    step_result=$(prompt_test_step "1" "Load the component map editor application with the $pack pack variant")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 2: Create components
    step_result=$(prompt_test_step "2" "Create 5 components (via toolbar or right-click menu)")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 3: Verify component creation in graph canvas
    obs=$(prompt_observation "Verify: Are all 5 components visible in the graph canvas?")
    [ "$obs" = "1" ] || all_passed=false
    
    # Step 4: Connect components with rules
    step_result=$(prompt_test_step "4" "Connect the 5 components (create 3 connections using connection draw mode)")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 5: Verify connections
    obs=$(prompt_observation "Verify: Are all 3 connections visible and valid (no red error indicators)?")
    [ "$obs" = "1" ] || all_passed=false
    
    # Step 6: Validate via service
    step_result=$(prompt_test_step "6" "Trigger validation (via menu or keyboard shortcut)")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 7: Check validation results
    obs=$(prompt_observation "Verify: Did validation complete successfully? Any errors displayed?")
    log "INFO" "Validation result: $obs"
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    if [ "$all_passed" = "true" ]; then
        log_scenario_result "$scenario_id" "PASS" "$duration" "All steps completed successfully"
    else
        log_scenario_result "$scenario_id" "FAIL" "$duration" "One or more steps failed"
    fi
}

# ============================================================================
# USER JOURNEY 2: COMPONENT VALIDATION
# ============================================================================

journey_component_validation() {
    local scenario_id="$1"
    local pack="$2"
    local size="$3"
    
    log_scenario_start "$scenario_id" "$pack" "$size" "Component Validation"
    
    local start_time=$(date +%s)
    local all_passed=true
    
    # Step 1: Load pack with validators
    step_result=$(prompt_test_step "1" "Load the $pack pack variant (should have validation rules)")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 2: Create a component with invalid properties
    step_result=$(prompt_test_step "2" "Create a component and intentionally set an INVALID property value (e.g., missing required field)")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 3: Trigger validation
    step_result=$(prompt_test_step "3" "Trigger validation (menu or keyboard shortcut)")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 4: Verify error display
    obs=$(prompt_observation "Verify: Did the validator correctly identify the invalid property and display an error message?")
    [ "$obs" = "1" ] || all_passed=false
    
    # Step 5: Fix the component
    step_result=$(prompt_test_step "5" "Fix the invalid property (set a valid value)")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 6: Re-validate
    step_result=$(prompt_test_step "6" "Trigger validation again")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 7: Verify no errors
    obs=$(prompt_observation "Verify: Is the component now valid (no error displayed)?")
    [ "$obs" = "1" ] || all_passed=false
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    if [ "$all_passed" = "true" ]; then
        log_scenario_result "$scenario_id" "PASS" "$duration" "Validation system working correctly"
    else
        log_scenario_result "$scenario_id" "FAIL" "$duration" "Validation errors or display issues"
    fi
}

# ============================================================================
# USER JOURNEY 3: SIMULATE (EXECUTE)
# ============================================================================

journey_simulate_execute() {
    local scenario_id="$1"
    local pack="$2"
    local size="$3"
    
    log_scenario_start "$scenario_id" "$pack" "$size" "Simulate (Execute)"
    
    local start_time=$(date +%s)
    local all_passed=true
    
    # Step 1: Load graph or create fresh graph
    step_result=$(prompt_test_step "1" "Load or create a graph with at least 5 nodes in the $size configuration")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 2: Trigger execution
    step_result=$(prompt_test_step "2" "Execute the graph (via menu: Execute or toolbar button)")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 3: Monitor execution traces
    obs=$(prompt_observation "Verify: Did execution complete without crashing? Any trace output visible (status bar or log)?")
    [ "$obs" = "1" ] || all_passed=false
    
    # Step 4: Check component state
    step_result=$(prompt_test_step "4" "After execution completes, inspect one component's state (click to view properties)")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 5: Verify state mutations
    obs=$(prompt_observation "Verify: Did the component state change as expected (new properties, updated values)?")
    [ "$obs" = "1" ] || all_passed=false
    
    # For legacy packs only: check adapter annotation
    if [ "$pack" = "legacy" ]; then
        obs=$(prompt_observation "Verify (Legacy Pack): Does trace include adapter annotation (adapter: executionSemantics.v0)?")
        [ "$obs" = "1" ] || all_passed=false
    fi
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    if [ "$all_passed" = "true" ]; then
        log_scenario_result "$scenario_id" "PASS" "$duration" "Execution worked correctly"
    else
        log_scenario_result "$scenario_id" "FAIL" "$duration" "Execution or trace issues"
    fi
}

# ============================================================================
# USER JOURNEY 4: UNDO/REDO STRESS
# ============================================================================

journey_undo_redo_stress() {
    local scenario_id="$1"
    local pack="$2"
    local size="$3"
    
    log_scenario_start "$scenario_id" "$pack" "$size" "Undo/Redo Stress"
    
    local start_time=$(date +%s)
    local all_passed=true
    
    # Step 1: Create a working graph
    step_result=$(prompt_test_step "1" "Create a graph with 5 nodes and 3 connections")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 2: Save initial snapshot
    step_result=$(prompt_test_step "2" "Create a mental snapshot of the graph state (or take a screenshot)")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 3: Perform 20 operations
    echo "" | tee -a "$LOG_FILE"
    echo -e "${YELLOW}Undo/Redo Test: Performing 20 operations...${NC}" | tee -a "$LOG_FILE"
    step_result=$(prompt_test_step "3" "Perform 20 rapid modifications: add/delete nodes, modify properties, create/break connections\n(This should take 30-60 seconds)")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 4: Undo and observe
    step_result=$(prompt_test_step "4" "Undo 10 times using Ctrl+Z (or menu)")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 5: Verify intermediate state
    obs=$(prompt_observation "Verify: Does the graph state after 10 undos appear stable and consistent?")
    [ "$obs" = "1" ] || all_passed=false
    
    # Step 6: Redo and verify
    step_result=$(prompt_test_step "6" "Redo 5 times using Ctrl+Y (or menu)")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 7: Final verification
    obs=$(prompt_observation "Verify: After redo operations, does the graph match the intermediate state correctly?")
    [ "$obs" = "1" ] || all_passed=false
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    if [ "$all_passed" = "true" ]; then
        log_scenario_result "$scenario_id" "PASS" "$duration" "Undo/redo system working correctly"
    else
        log_scenario_result "$scenario_id" "FAIL" "$duration" "Undo/redo inconsistencies or crashes"
    fi
}

# ============================================================================
# USER JOURNEY 5: EXPORT/IMPORT CYCLE
# ============================================================================

journey_export_import() {
    local scenario_id="$1"
    local pack="$2"
    local size="$3"
    
    log_scenario_start "$scenario_id" "$pack" "$size" "Export/Import Cycle"
    
    local start_time=$(date +%s)
    local all_passed=true
    
    # Step 1: Create a graph
    step_result=$(prompt_test_step "1" "Create a graph with 5 nodes and 3 connections (or load existing)")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 2: Export to JSON
    step_result=$(prompt_test_step "2" "Export the graph to JSON (File > Export or Ctrl+Shift+E)")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 3: Save reference snapshot
    obs=$(prompt_observation "Verify: Did the export succeed? Can you view the exported JSON file (e.g., in file browser or text editor)?")
    [ "$obs" = "1" ] || all_passed=false
    
    # Step 4: Clear graph
    step_result=$(prompt_test_step "4" "Clear the graph (File > New or Edit > Clear All)")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 5: Import the exported JSON
    step_result=$(prompt_test_step "5" "Import the previously exported JSON file (File > Import or Ctrl+I)")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 6: Verify structure
    obs=$(prompt_observation "Verify: After re-import, do all 5 nodes and 3 connections appear in the graph?")
    [ "$obs" = "1" ] || all_passed=false
    
    # Step 7: Check property preservation
    step_result=$(prompt_test_step "7" "Click on a component and inspect its properties")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 8: Verify property values
    obs=$(prompt_observation "Verify: Do all custom properties (names, types, attributes) match the original graph?")
    [ "$obs" = "1" ] || all_passed=false
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    if [ "$all_passed" = "true" ]; then
        log_scenario_result "$scenario_id" "PASS" "$duration" "Export/import roundtrip successful"
    else
        log_scenario_result "$scenario_id" "FAIL" "$duration" "Export/import data loss or corruption"
    fi
}

# ============================================================================
# USER JOURNEY 6: EXTENSION LIFECYCLE (HOT-RELOAD)
# ============================================================================

journey_extension_lifecycle() {
    local scenario_id="$1"
    local pack="$2"
    local size="$3"
    
    log_scenario_start "$scenario_id" "$pack" "$size" "Extension Lifecycle (Hot-Reload)"
    
    local start_time=$(date +%s)
    local all_passed=true
    
    # Step 1: Load the extension pack
    step_result=$(prompt_test_step "1" "Load the application with the $pack pack variant")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 2: Verify pack loaded
    obs=$(prompt_observation "Verify: Are all extension features available (custom component types, validation rules)?")
    [ "$obs" = "1" ] || all_passed=false
    
    # Step 3: Create a test graph with rules
    step_result=$(prompt_test_step "3" "Create a graph with components using the loaded rules/types")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 4: Modify rule file (this would be done externally)
    echo "" | tee -a "$LOG_FILE"
    echo -e "${YELLOW}*** DEVELOPER ACTION REQUIRED ***${NC}" | tee -a "$LOG_FILE"
    echo "Please modify one of the rule JSON files in component_map_editor/extensions/sample_pack/" | tee -a "$LOG_FILE"
    echo "Save the change to trigger hot-reload" | tee -a "$LOG_FILE"
    
    step_result=$(prompt_test_step "4" "Modify a rule file externally and save it (should trigger automatic hot-reload)")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 5: Verify hot-reload
    obs=$(prompt_observation "Verify: Did the application hot-reload rules without crashing? No restart required?")
    [ "$obs" = "1" ] || all_passed=false
    
    # Step 6: Check new rules applied
    step_result=$(prompt_test_step "6" "Test the modified rule (create new components or validate)")
    [ "$step_result" = "false" ] && all_passed=false
    
    # Step 7: Verify new behavior
    obs=$(prompt_observation "Verify: Does the new/updated rule behavior work as expected?")
    [ "$obs" = "1" ] || all_passed=false
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    if [ "$all_passed" = "true" ]; then
        log_scenario_result "$scenario_id" "PASS" "$duration" "Hot-reload working correctly"
    else
        log_scenario_result "$scenario_id" "FAIL" "$duration" "Hot-reload issues or crashes"
    fi
}

# ============================================================================
# MAIN EXECUTION
# ============================================================================

main() {
    echo "" | tee -a "$LOG_FILE"
    echo -e "${BLUE}===============================================${NC}" | tee -a "$LOG_FILE"
    echo -e "${BLUE}Phase 11: Manual User Journey Validation${NC}" | tee -a "$LOG_FILE"
    echo -e "${BLUE}===============================================${NC}" | tee -a "$LOG_FILE"
    log "INFO" "Test Run Started"
    log "INFO" "Log output: $LOG_FILE"
    log "INFO" "Packs: $PACKS | Sizes: $SIZES"
    
    # Determine which packs and sizes to test
    local test_packs=()
    local test_sizes=()
    
    if [ "$PACKS" = "all" ]; then
        test_packs=("modern" "legacy" "extended")
    else
        test_packs=("$PACKS")
    fi
    
    if [ "$SIZES" = "all" ]; then
        test_sizes=("small" "medium" "large")
    else
        test_sizes=("$SIZES")
    fi
    
    # Execute test matrix
    local scenario_counter=1
    for pack in "${test_packs[@]}"; do
        for size in "${test_sizes[@]}"; do
            local pack_label=""
            case "$pack" in
                modern) pack_label="Sample Workflow Pack (Modern v1)" ;;
                legacy) pack_label="Legacy v0 Compatibility Pack" ;;
                extended) pack_label="Extended Features Pack" ;;
            esac
            
            # Only execute J1 (Create & Connect) and J2 (Validation) for all packs
            # and J3 (Simulate) for legacy/extended packs
            
            scenario_id="${pack:0:1}$(printf '%d' $(echo "$size" | grep -o 's\|m\|l' | tr 's\|m\|l' '1\|2\|3'))-J1"
            journey_create_and_connect "$scenario_id" "$pack_label" "$size"
            
            scenario_counter=$((scenario_counter + 1))
            
            # Add delay between tests for manual verification
            echo "" | tee -a "$LOG_FILE"
            echo -e "${YELLOW}Ready for next scenario...${NC}" | tee -a "$LOG_FILE"
            read -p "Press Enter to continue to next scenario (or Ctrl+C to exit): " -r
        done
    done
    
    # Summary
    echo "" | tee -a "$LOG_FILE"
    echo -e "${BLUE}===============================================${NC}" | tee -a "$LOG_FILE"
    echo -e "${BLUE}Test Summary${NC}" | tee -a "$LOG_FILE"
    echo -e "${BLUE}===============================================${NC}" | tee -a "$LOG_FILE"
    echo -e "${GREEN}Passed: $SUMMARY_PASS${NC}" | tee -a "$LOG_FILE"
    echo -e "${RED}Failed: $SUMMARY_FAIL${NC}" | tee -a "$LOG_FILE"
    
    local total=$((SUMMARY_PASS + SUMMARY_FAIL))
    if [ $total -gt 0 ]; then
        local pass_rate=$((SUMMARY_PASS * 100 / total))
        echo "Pass Rate: ${pass_rate}%" | tee -a "$LOG_FILE"
    fi
    
    log "INFO" "Test Run Completed"
}

# Run main function
main "$@"
