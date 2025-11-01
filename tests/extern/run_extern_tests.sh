#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
COMPILER="${1:-$SCRIPT_DIR/../../build/raccoonc}"
TEST_DIR="$(cd "$(dirname "$0")" && pwd)"

PASSED=0
FAILED=0
TOTAL=0

echo "======================================"
echo "  Raccoon C Interop Test Suite"
echo "======================================"
echo ""
echo "Using compiler: $COMPILER"
echo "Test directory: $TEST_DIR"
echo ""

cd "$TEST_DIR" || exit 1

run_test() {
    local test_name="$1"
    local expected_code="$2"
    local main_file="$3"
    shift 3
    local c_files=("$@")
    
    TOTAL=$((TOTAL + 1))
    
    echo "[$TOTAL] Testing: $test_name (expecting exit code: $expected_code)"
    
    local exe_file="test_${test_name}"
    
    # Build compiler command
    local compile_cmd="$COMPILER -v $main_file"
    for c_file in "${c_files[@]}"; do
        compile_cmd="$compile_cmd -c $c_file"
    done
    compile_cmd="$compile_cmd -o $exe_file"
    
    if ! eval "$compile_cmd" 2>&1 | head -20; then
        echo "  ✗ Compilation failed"
        FAILED=$((FAILED + 1))
        echo ""
        return
    fi
    
    set +e
    chmod +x "$exe_file"
    ./"$exe_file"
    actual_code=$?
    set -e
    
    if [ "$actual_code" -eq "$expected_code" ]; then
        echo "  ✓ Test passed (exit code: $actual_code)"
        PASSED=$((PASSED + 1))
    else
        echo "  ✗ Exit code mismatch: expected $expected_code, got $actual_code"
        FAILED=$((FAILED + 1))
    fi
    
    rm -f "$exe_file"
    echo ""
}

run_test "extern_simple" 42 "test_extern_simple.rac" "shim_simple.c"
run_test "extern_arithmetic" 15 "test_extern_arithmetic.rac" "shim_arithmetic.c"
run_test "extern_multiple" 100 "test_extern_multiple.rac" "shim_multiple.c"
run_test "extern_pointers" 99 "test_extern_pointers.rac" "shim_pointers.c"
run_test "extern_structs" 42 "test_extern_structs.rac" "shim_structs.c"
run_test "extern_mixed" 10 "test_extern_mixed.rac" "shim_mixed.c"
run_test "extern_void" 5 "test_extern_void.rac" "shim_void.c"

rm -f *.o *.racm

echo "======================================"
echo "C Interop Test Summary"
echo "======================================"
echo "Total:  $TOTAL"
echo "Passed: $PASSED"
echo "Failed: $FAILED"
echo "======================================"

if [ $FAILED -gt 0 ]; then
    exit 1
else
    exit 0
fi
