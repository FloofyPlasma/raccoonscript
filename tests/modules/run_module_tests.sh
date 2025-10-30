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

LINKER=""
if command -v clang >/dev/null 2>&1; then
    LINKER="clang"
elif command -v gcc >/dev/null 2>&1; then
    LINKER="gcc"
else
    echo "Error: No suitable linker found (clang or gcc)"
    exit 1
fi
echo "Using linker: $LINKER"
echo ""

run_test() {
    local test_name="$1"
    local expected_code="$2"
    shift 2
    local files=("$@")
    
    TOTAL=$((TOTAL + 1))
    echo "[$TOTAL] Testing: $test_name (expecting exit code: $expected_code)"
    
    local object_files=()
    local compile_failed=0
    
    for file in "${files[@]}"; do
        local obj_file="${file%.rac}.o"
        if ! "$COMPILER" "$file" -o "$obj_file" --no-link 2>&1 | head -20; then
            echo "  ✗ Compilation of $file failed"
            compile_failed=1
            break
        fi
        object_files+=("$obj_file")
    done
    
    if [ $compile_failed -eq 1 ]; then
        FAILED=$((FAILED + 1))
        echo ""
        return
    fi
    
    local exe_file="test_${test_name}"
    if ! $LINKER "${object_files[@]}" -o "$exe_file" 2>&1 | head -20; then
        echo "  ✗ Linking failed"
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
    
    # Cleanup
    rm -f "$exe_file" "${object_files[@]}"
    echo ""
}

run_test "basic_import" 15 "math.rac" "test_math.rac"
run_test "struct_import" 20 "geometry.rac" "test_geometry.rac"
run_test "multiple_imports" 42 "math.rac" "util.rac" "test_multiple.rac"
run_test "complex_types" 100 "data.rac" "test_complex.rac"
run_test "mixed_exports" 11 "mixed.rac" "test_mixed.rac"
run_test "calculator" 8 "calculator.rac" "test_calculator.rac"

rm -f *.racm

# Summary
echo "======================================"
echo "Module Test Summary"
echo "======================================"
echo "Total:  $TOTAL"
echo "Passed: $PASSED"
echo "Failed: $FAILED"
echo "======================================"

if [ $FAILED -gt 0 ]; then
    echo "❌ Some module tests failed"
    exit 1
elif [ $TOTAL -eq 0 ]; then
    echo "⚠️  No module tests were run"
    exit 0
else
    echo "✅ All module tests passed!"
    exit 0
fi
