@echo off
setlocal enabledelayedexpansion

:: Set build configuration
set BUILD_TYPE=Release
set OUTPUT_DIR=windows_release
set ZIP_NAME=OBD2_Diagnostic_Tool_Win64.zip

:: Create build directory
if not exist build mkdir build
cd build

:: Generate build files with CMake
cmake -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DBUILD_SHARED_LIBS=OFF ^
    ..

:: Build the project
cmake --build . --config %BUILD_TYPE%

:: Create release directory
cd ..
if not exist %OUTPUT_DIR% mkdir %OUTPUT_DIR%

:: Copy necessary files
copy build\%BUILD_TYPE%\obd2_program.exe %OUTPUT_DIR%\
copy win\*.dll %OUTPUT_DIR%\
copy LICENSE %OUTPUT_DIR%\
copy README.md %OUTPUT_DIR%\
copy docs\*.md %OUTPUT_DIR%\docs\
copy CONFIG.CFG %OUTPUT_DIR%\

:: Create ZIP file
powershell Compress-Archive -Path %OUTPUT_DIR%\* -DestinationPath %ZIP_NAME% -Force

echo Build complete! Release package created as %ZIP_NAME%
pause
