@echo off
setlocal

set ROOT=%~dp0
set BUILD_DEBUG=%ROOT%..\Build\Debug
set BUILD_RELEASE=%ROOT%..\Build\Release

cmake --build "%BUILD_DEBUG%" --parallel
cmake --build "%BUILD_RELEASE%" --parallel

endlocal
