@echo off

set 7ZIP_PATH=C:\Program Files\7-Zip\7z.exe
set APP_NAME=Raycaster

set scriptpath=%~d0%~p0
cd %scriptpath%

set PLATFORM=win64
set BUILD_FOLDER=buildWin64

if "%~3"=="-ship" goto buildSetup
goto buildSetupEnd
:buildSetup
	set BUILD_FOLDER=releaseBuild
	if exist "..\%BUILD_FOLDER%" rmdir "..\%BUILD_FOLDER%" /S /Q
:buildSetupEnd

if not exist "..\%BUILD_FOLDER%" mkdir "..\%BUILD_FOLDER%"
pushd "..\%BUILD_FOLDER%"

set INC=
set LINC=

set LINKER_LIBS= -DEFAULTLIB:Opengl32.lib -DEFAULTLIB:Shell32.lib -DEFAULTLIB:user32.lib -DEFAULTLIB:Gdi32.lib -DEFAULTLIB:Shlwapi.lib -DEFAULTLIB:Winmm.lib

set                  PATH=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin\amd64;%PATH%
set          INC=%INC% -I"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\include"
set LINC=%LINC% -LIBPATH:"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\lib\amd64"
set          INC=%INC% -I"C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Include"
set LINC=%LINC% -LIBPATH:"C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64"

set INC=%INC% -I"..\libs\freetype 2.9\include"
set LINC=%LINC% -LIBPATH:"..\libs\freetype 2.9\lib\%PLATFORM%"
set LINKER_LIBS=%LINKER_LIBS% -DEFAULTLIB:freetype.lib



set BUILD_MODE=-Od
set MODE_DEFINE=
if "%~2"=="-release" (
	rem -Oy -Zo
	set BUILD_MODE=-O2
	rem set BUILD_MODE=-Od
	set MODE_DEFINE=-DRELEASE_BUILD
)

if "%~3"=="-ship" (
	set MODE_DEFINE=%MODE_DEFINE% -DSHIPPING_MODE
)

rem -d2cgsummary -Bt
set COMPILER_OPTIONS= -MD %BUILD_MODE% -nologo -Oi -FC -wd4838 -wd4005 -fp:fast -fp:except- -Gm- -GR- -EHa- -Z7
set LINKER_OPTIONS= -link -SUBSYSTEM:WINDOWS -OUT:%APP_NAME%.exe -incremental:no -opt:ref


del main_*.pdb > NUL 2> NUL
echo. 2>lock.tmp
cl %COMPILER_OPTIONS% ..\code\app.cpp %MODE_DEFINE% -LD %INC% -link -incremental:no -opt:ref -PDB:main_%random%.pdb -EXPORT:appMain %LINC% %LINKER_LIBS%
del lock.tmp

cl %COMPILER_OPTIONS% ..\code\main.cpp %MODE_DEFINE% %INC% %LINKER_OPTIONS% %LINC% %LINKER_LIBS%



if "%~3"=="-ship" goto packShippingFolder
goto packShippingFolderEnd

:packShippingFolder

	rem This is suboptimal.

	cd ..

	mkdir ".\%BUILD_FOLDER%\data"
	xcopy ".\data" ".\%BUILD_FOLDER%\data" /E /Q

	del ".\%BUILD_FOLDER%\*.pdb"
	del ".\%BUILD_FOLDER%\*.exp"
	del ".\%BUILD_FOLDER%\*.lib"
	del ".\%BUILD_FOLDER%\*.obj"

	xcopy ".\libs\freetype 2.9\lib\%PLATFORM%\*.dll" ".\%BUILD_FOLDER%" /Q

	call "C:\\Standalone\\rcedit.exe" "%BUILD_FOLDER%\\%APP_NAME%.exe" --set-icon icon.ico

	set RELEASE_FOLDER=".\releases\%PLATFORM%\%APP_NAME%"
	if exist %RELEASE_FOLDER% rmdir %RELEASE_FOLDER% /S /Q
	mkdir %RELEASE_FOLDER%

	xcopy %BUILD_FOLDER% %RELEASE_FOLDER% /E /Q

	rmdir ".\%BUILD_FOLDER%" /S /Q

	call "C:\Program Files\7-Zip\7z.exe" a %RELEASE_FOLDER%.zip %RELEASE_FOLDER%

:packShippingFolderEnd


:parseParameters
IF "%~1"=="" GOTO parseParametersEnd
IF "%~1"=="-run" call %APP_NAME%.exe
SHIFT
GOTO parseParameters
:parseParametersEnd

rem popd
set LOCATION=


rem exit -b