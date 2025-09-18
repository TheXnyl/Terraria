@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "SRC_DIR=%SCRIPT_DIR%Src"
set "BIN_DIR=%SCRIPT_DIR%Bin"

if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"

for %%F in ("%SRC_DIR%\*.*") do (
    set "FILENAME=%%~nxF"
    set "STAGE="

    :: Detect stage based on filename
    echo !FILENAME! | findstr /i "\.vertex\." >nul
    if !errorlevel! equ 0 set "STAGE=vert"

    echo !FILENAME! | findstr /i "\.fragment\." >nul
    if !errorlevel! equ 0 set "STAGE=frag"

    if defined STAGE (
        glslc -fshader-stage=!STAGE! "%%F" -o "%BIN_DIR%\!FILENAME!.spv"
    ) else (
      echo Skipping %%F (unknown shader stage^)...
    )
)

pause
