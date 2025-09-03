@echo off
setlocal

set ROOT=%~dp0

%ROOT%cmake_build.bat
%ROOT%make_build.bat

endlocal
pause
