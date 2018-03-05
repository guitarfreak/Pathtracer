@echo off

call ".\code\buildWin64.bat" -noRun -release -ship
call ".\code\buildWin32.bat" -noRun -release -ship
