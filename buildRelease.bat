cd buildWin32
call "..\\code\\buildWin32.bat" -noRun -release
cd "..\\buildWin32"
del *.pdb
del *.exp
del *.lib
del *.obj

cd ..
call "C:\\Standalone\\rcedit.exe" "buildWin32\\QuickGui.exe" --set-icon icon.ico

rem PAUSE