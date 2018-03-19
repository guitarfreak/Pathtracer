@echo off

call ".\code\buildWin64.bat" -x64 -noRun -release -ship
call ".\code\buildWin64.bat" -x86 -noRun -release -ship
