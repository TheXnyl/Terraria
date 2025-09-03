@echo off

setlocal

set ROOT=%~dp0
set SRC=%ROOT%../Vulkan

set BUILD_DEBUG=%ROOT%..\Build\Debug
set BUILD_RELEASE=%ROOT%..\Build\Release

cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -B "%BUILD_DEBUG%" "%SRC%"
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -B "%BUILD_RELEASE%" "%SRC%"

cmake --build "%BUILD_DEBUG%"
cmake --build "%BUILD_RELEASE%"

endlocal
pause
