@echo off
:begin
@echo %1
if "%1" == "nosuch" goto end
if NOT exist "%1" goto next
cd %1
nmake -fmakefile.vc clean
cd ..
:next
shift
goto begin
:end
echo Bye
