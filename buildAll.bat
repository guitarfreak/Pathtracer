@echo off

if "%BUILD_ALL_MODE%"=="release" (
	call ".\code\buildWin.bat" -x64 -noRun -releaseMode -folder
	call ".\code\buildWin.bat" -x86 -noRun -releaseMode -folder
)

if "%BUILD_ALL_MODE%"=="optimized" (
	call ".\code\buildWin.bat" -x64 -noRun -releaseMode -folder -fullOptimize
	call ".\code\buildWin.bat" -x86 -noRun -releaseMode -folder -fullOptimize
)

if "%BUILD_ALL_MODE%"=="ship" (
	call ".\code\buildWin.bat" -x64 -noRun -releaseMode -folder -fullOptimize -ship
	call ".\code\buildWin.bat" -x86 -noRun -releaseMode -folder -fullOptimize -ship
)
