@echo off
setlocal enabledelayedexpansion

set "COMPILER=%~1"
set "COMPILER=..\..\build\Release\raccoonc.exe"

set PASSED=0
set FAILED=0
set TOTAL=0

echo ======================================
echo   Racoon Module System Test Suite
echo ======================================
echo.
echo Using compiler: %COMPILER%
echo Test directory: %~dp0
echo.

REM Detect linker
set LINKER=
where clang.exe >nul 2>&1
if %ERRORLEVEL% equ 0 (
    set "LINKER=clang.exe"
) else (
    where gcc.exe >nul 2>&1
    if %ERRORLEVEL% equ 0 (
        set "LINKER=gcc.exe"
    ) else (
        where cl.exe >nul 2>&1
        if %ERRORLEVEL% equ 0 (
            set "LINKER=cl.exe"
            set "LINKER_STYLE=msvc"
        ) else (
            echo Error: No suitable linker found
            exit /b 1
        )
    )
)

echo Using linker: %LINKER%
echo.

REM Test 1: Basic import
set /a TOTAL+=1
echo [%TOTAL%] Testing: basic_import (expecting exit code: 15)
%COMPILER% math.rac -o math.o --no-link >nul 2>&1
if errorlevel 1 (
    echo   X Compilation of math.rac failed
    set /a FAILED+=1
    goto test2
)
%COMPILER% test_math.rac -o test_math.o --no-link >nul 2>&1
if errorlevel 1 (
    echo   X Compilation of test_math.rac failed
    set /a FAILED+=1
    goto test2
)
if "%LINKER_STYLE%"=="msvc" (
    %LINKER% /Fe:test_basic_import.exe math.o test_math.o >nul 2>&1
) else (
    %LINKER% math.o test_math.o -o test_basic_import.exe >nul 2>&1
)
if errorlevel 1 (
    echo   X Linking failed
    set /a FAILED+=1
    goto test2
)
test_basic_import.exe
if !errorlevel! equ 15 (
    echo   √ Test passed (exit code: 15^)
    set /a PASSED+=1
) else (
    echo   X Exit code mismatch: expected 15, got !errorlevel!
    set /a FAILED+=1
)
del /q test_basic_import.exe math.o test_math.o 2>nul
echo.

:test2
REM Test 2: Struct import
set /a TOTAL+=1
echo [%TOTAL%] Testing: struct_import (expecting exit code: 20)
%COMPILER% geometry.rac -o geometry.o --no-link >nul 2>&1
if errorlevel 1 (
    echo   X Compilation of geometry.rac failed
    set /a FAILED+=1
    goto test3
)
%COMPILER% test_geometry.rac -o test_geometry.o --no-link >nul 2>&1
if errorlevel 1 (
    echo   X Compilation of test_geometry.rac failed
    set /a FAILED+=1
    goto test3
)
if "%LINKER_STYLE%"=="msvc" (
    %LINKER% /Fe:test_struct_import.exe geometry.o test_geometry.o >nul 2>&1
) else (
    %LINKER% geometry.o test_geometry.o -o test_struct_import.exe >nul 2>&1
)
if errorlevel 1 (
    echo   X Linking failed
    set /a FAILED+=1
    goto test3
)
test_struct_import.exe
if !errorlevel! equ 20 (
    echo   √ Test passed (exit code: 20^)
    set /a PASSED+=1
) else (
    echo   X Exit code mismatch: expected 20, got !errorlevel!
    set /a FAILED+=1
)
del /q test_struct_import.exe geometry.o test_geometry.o 2>nul
echo.

:test3
REM Test 3: Multiple imports
set /a TOTAL+=1
echo [%TOTAL%] Testing: multiple_imports (expecting exit code: 42)
%COMPILER% util.rac -o util.o --no-link >nul 2>&1
if errorlevel 1 (
    echo   X Compilation of util.rac failed
    set /a FAILED+=1
    goto test4
)
%COMPILER% math.rac -o math.o --no-link >nul 2>&1
if errorlevel 1 (
    echo   X Compilation of math.rac failed
    set /a FAILED+=1
    goto test4
)
%COMPILER% test_multiple.rac -o test_multiple.o --no-link >nul 2>&1
if errorlevel 1 (
    echo   X Compilation of test_multiple.rac failed
    set /a FAILED+=1
    goto test4
)
if "%LINKER_STYLE%"=="msvc" (
    %LINKER% /Fe:test_multiple_imports.exe math.o util.o test_multiple.o >nul 2>&1
) else (
    %LINKER% math.o util.o test_multiple.o -o test_multiple_imports.exe >nul 2>&1
)
if errorlevel 1 (
    echo   X Linking failed
    set /a FAILED+=1
    goto test4
)
test_multiple_imports.exe
if !errorlevel! equ 42 (
    echo   √ Test passed (exit code: 42^)
    set /a PASSED+=1
) else (
    echo   X Exit code mismatch: expected 42, got !errorlevel!
    set /a FAILED+=1
)
del /q test_multiple_imports.exe util.o test_multiple.o 2>nul
echo.

:test4
REM Test 4: Complex types
set /a TOTAL+=1
echo [%TOTAL%] Testing: complex_types (expecting exit code: 100)
%COMPILER% data.rac -o data.o --no-link >nul 2>&1
if errorlevel 1 (
    echo   X Compilation of data.rac failed
    set /a FAILED+=1
    goto test5
)
%COMPILER% test_complex.rac -o test_complex.o --no-link >nul 2>&1
if errorlevel 1 (
    echo   X Compilation of test_complex.rac failed
    set /a FAILED+=1
    goto test5
)
if "%LINKER_STYLE%"=="msvc" (
    %LINKER% /Fe:test_complex_types.exe data.o test_complex.o >nul 2>&1
) else (
    %LINKER% data.o test_complex.o -o test_complex_types.exe >nul 2>&1
)
if errorlevel 1 (
    echo   X Linking failed
    set /a FAILED+=1
    goto test5
)
test_complex_types.exe
if !errorlevel! equ 100 (
    echo   √ Test passed (exit code: 100^)
    set /a PASSED+=1
) else (
    echo   X Exit code mismatch: expected 100, got !errorlevel!
    set /a FAILED+=1
)
del /q test_complex_types.exe data.o test_complex.o 2>nul
echo.

:test5
REM Test 5: Mixed exports
set /a TOTAL+=1
echo [%TOTAL%] Testing: mixed_exports (expecting exit code: 11)
%COMPILER% mixed.rac -o mixed.o --no-link >nul 2>&1
if errorlevel 1 (
    echo   X Compilation of mixed.rac failed
    set /a FAILED+=1
    goto test6
)
%COMPILER% test_mixed.rac -o test_mixed.o --no-link >nul 2>&1
if errorlevel 1 (
    echo   X Compilation of test_mixed.rac failed
    set /a FAILED+=1
    goto test6
)
if "%LINKER_STYLE%"=="msvc" (
    %LINKER% /Fe:test_mixed_exports.exe mixed.o test_mixed.o >nul 2>&1
) else (
    %LINKER% mixed.o test_mixed.o -o test_mixed_exports.exe >nul 2>&1
)
if errorlevel 1 (
    echo   X Linking failed
    set /a FAILED+=1
    goto test6
)
test_mixed_exports.exe
if !errorlevel! equ 11 (
    echo   √ Test passed (exit code: 11^)
    set /a PASSED+=1
) else (
    echo   X Exit code mismatch: expected 11, got !errorlevel!
    set /a FAILED+=1
)
del /q test_mixed_exports.exe mixed.o test_mixed.o 2>nul
echo.

:test6
REM Test 6: Calculator
set /a TOTAL+=1
echo [%TOTAL%] Testing: calculator (expecting exit code: 8)
%COMPILER% calculator.rac -o calculator.o --no-link >nul 2>&1
if errorlevel 1 (
    echo   X Compilation of calculator.rac failed
    set /a FAILED+=1
    goto cleanup
)
%COMPILER% test_calculator.rac -o test_calculator.o --no-link >nul 2>&1
if errorlevel 1 (
    echo   X Compilation of test_calculator.rac failed
    set /a FAILED+=1
    goto cleanup
)
if "%LINKER_STYLE%"=="msvc" (
    %LINKER% /Fe:test_calculator_module.exe calculator.o test_calculator.o >nul 2>&1
) else (
    %LINKER% calculator.o test_calculator.o -o test_calculator_module.exe >nul 2>&1
)
if errorlevel 1 (
    echo   X Linking failed
    set /a FAILED+=1
    goto cleanup
)
test_calculator_module.exe
if !errorlevel! equ 8 (
    echo   √ Test passed (exit code: 8^)
    set /a PASSED+=1
) else (
    echo   X Exit code mismatch: expected 8, got !errorlevel!
    set /a FAILED+=1
)
del /q test_calculator_module.exe calculator.o test_calculator.o 2>nul
echo.

:cleanup

del /q *.racm *.o *.exe 2>nul

echo ======================================
echo Module Test Summary
echo ======================================
echo Total:  %TOTAL%
echo Passed: %PASSED%
echo Failed: %FAILED%
echo ======================================

if %FAILED% gtr 0 (
    echo X Some module tests failed
    exit /b 1
) else if %TOTAL% equ 0 (
    echo ! No module tests were run
    exit /b 0
) else (
    echo √ All module tests passed!
    exit /b 0
)
