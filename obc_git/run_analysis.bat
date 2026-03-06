@echo off
chcp 65001 > nul
echo Compiling Fault Detector...
gcc multi_file_fault_detector.c -o multi_file_fault_detector.exe
if %errorlevel% neq 0 (
    echo Compilation Failed! Please check if GCC is installed.
    pause
    exit /b %errorlevel%
)

echo Compilation Successful. Running Analysis...
echo ------------------------------------------------
multi_file_fault_detector.exe
echo ------------------------------------------------
pause
