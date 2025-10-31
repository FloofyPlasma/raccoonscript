#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
COMPILER="${1:-$SCRIPT_DIR/../../build/raccoonc}"
TEST_DIR="$(cd "$(dirname "$0")" && pwd)"
PASSED=0
FAILED=0
TOTAL=0

echo "======================================"
echo "  Racoon Module System Test Suite"
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
    
    TOTAL=$((TOTAL + 1))
    echo "[$TOTAL] Testing: $test_name (expecting exit code: $expected_code)"
    
    local exe_file="test_${test_name}"
    if ! "$COMPILER" -v "$main_file" -o "$exe_file" 2>&1 | head -20; then
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

run_test "basic_import" 15 "test_math.rac"
run_test "struct_import" 20 "test_geometry.rac"
run_test "multiple_imports" 42 "test_multiple.rac"
run_test "complex_types" 100 "test_complex.rac"
run_test "mixed_exports" 11 "test_mixed.rac"
run_test "calculator" 8 "test_calculator.rac"

rm -f *.racm

echo "======================================"
echo "Module Test Summary"
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
