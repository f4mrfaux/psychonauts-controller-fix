@echo off
setlocal
set VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat
if not exist "%VCVARS%" set VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat
call "%VCVARS%" >nul || exit /b 1
cd /d "%~dp0"
if not exist build mkdir build

set FLAGS=/nologo /W4 /O2 /MT /D_CRT_SECURE_NO_WARNINGS /Fobuild\
cl %FLAGS% /LD src\main.c src\padlogic.c src\gamepatch.c src\profiles.c ^
   /link /DEF:src\xinput1_3.def /OUT:build\xinput1_3.dll /IMPLIB:build\xinput1_3.lib user32.lib kernel32.lib || exit /b 1
if not exist build\testpad mkdir build\testpad
cl %FLAGS% /DPSX_TEST_PAD /LD src\main.c src\padlogic.c src\gamepatch.c src\profiles.c ^
   /link /DEF:src\xinput1_3.def /OUT:build\testpad\xinput1_3.dll /IMPLIB:build\testpad\xinput1_3.lib user32.lib kernel32.lib || exit /b 1
cl %FLAGS% /Febuild\test_main.exe tests\test_main.c src\padlogic.c src\gamepatch.c src\profiles.c ^
   /link user32.lib kernel32.lib || exit /b 1
