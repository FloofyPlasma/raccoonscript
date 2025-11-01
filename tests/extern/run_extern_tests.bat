@echo off
setlocal enabledelayedexpansion

set "COMPILER=%~1"
if "%COMPILER%"=="" set "COMPILER=..\..\build\Release\raccoonc.exe"

set PASSED=0
set FAILED=0
set TOTAL=0

echo ======================================
echo   Raccoon C Interop Test Suite
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
shift
shift
shift

set /a TOTAL+=1
echo [%TOTAL%] Testing: %TEST_NAME% (expecting exit code: %EXPECTED_CODE%)

set "EXE_FILE=test_%TEST_NAME%.exe"
set "COMPILE_CMD=%COMPILER% -v %MAIN_FILE%"

:collect_c_files
if "%~1"=="" goto :compile
set "COMPILE_CMD=!COMPILE_CMD! -c %~1"
shift
goto :collect_c_files

:compile
set "COMPILE_CMD=!COMPILE_CMD! -o %EXE_FILE%"
call :exec_compile "!COMPILE_CMD!"
if errorlevel 1 (
    echo   X Compilation failed
    set /a FAILED+=1
    echo.
    goto :EOF
)

call :run_exe "%EXE_FILE%" %EXPECTED_CODE%
del /q "%EXE_FILE%" 2>nul
echo.
exit /b

:exec_compile
set "CMD=%~1"
cmd /c %CMD% >nul 2>&1
exit /b %errorlevel%

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
call :run_test extern_simple 42 test_extern_simple.rac shim_simple.c
call :run_test extern_arithmetic 15 test_extern_arithmetic.rac shim_arithmetic.c
call :run_test extern_multiple 100 test_extern_multiple.rac shim_multiple.c
call :run_test extern_pointers 99 test_extern_pointers.rac shim_pointers.c
call :run_test extern_structs 42 test_extern_structs.rac shim_structs.c
call :run_test extern_mixed 10 test_extern_mixed.rac shim_mixed.c

del /q *.o *.racm 2>nul

echo ======================================
echo C Interop Test Summary
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
