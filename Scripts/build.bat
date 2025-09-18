@echo off
setlocal

set ROOT=%~dp0

call "%ROOT%cmake_build.bat"
call "%ROOT%make_build.bat"

endlocal
