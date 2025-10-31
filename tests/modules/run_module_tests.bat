@echo off
setlocal enabledelayedexpansion

set "COMPILER=%~1"
if "%COMPILER%"=="" set "COMPILER=..\..\build\Release\raccoonc.exe"

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

REM --- Test runner ---
:run_test
set "TEST_NAME=%~1"
set "EXPECTED_CODE=%~2"
set "MAIN_FILE=%~3"

set /a TOTAL+=1
echo [%TOTAL%] Testing: %TEST_NAME% (expecting exit code: %EXPECTED_CODE%)

%COMPILER% -v "%MAIN_FILE%" -o "test_%TEST_NAME%.exe" >nul 2>&1
if errorlevel 1 (
    echo   X Compilation failed
    set /a FAILED+=1
    echo.
    shift
    shift
    shift
    goto :EOF
)

call :run_exe "test_%TEST_NAME%.exe" %EXPECTED_CODE%
del /q "test_%TEST_NAME%.exe" 2>nul
echo.
exit /b

:run_exe
set "EXE=%~1"
set "EXPECTED=%~2"
"%EXE%"
set "CODE=!errorlevel!"
if "!CODE!"=="%EXPECTED%" (
    echo   √ Test passed (exit code: !CODE!)
    set /a PASSED+=1
) else (
    echo   X Exit code mismatch: expected %EXPECTED%, got !CODE!
    set /a FAILED+=1
)
exit /b

REM --- Tests ---
call :run_test basic_import 15 test_math.rac
call :run_test struct_import 20 test_geometry.rac
call :run_test multiple_imports 42 test_multiple.rac
call :run_test complex_types 100 test_complex.rac
call :run_test mixed_exports 11 test_mixed.rac
call :run_test calculator 8 test_calculator.rac

del /q *.racm 2>nul

echo ======================================
echo Module Test Summary
echo ======================================
echo Total:  %TOTAL%
echo Passed: %PASSED%
echo Failed: %FAILED%
echo ======================================

if %FAILED% gtr 0 (
    exit /b 1
) else (
    exit /b 0
)
