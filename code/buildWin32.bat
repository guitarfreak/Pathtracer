@echo off

set scriptpath=%~d0%~p0
cd %scriptpath%

if not exist "..\buildWin32" mkdir ..\buildWin32
pushd ..\buildWin32

set APP_NAME=Raycaster

set INC=
set LIBS=
set LINC=

set LINKER_LIBS= -DEFAULTLIB:Opengl32.lib -DEFAULTLIB:ws2_32.lib -DEFAULTLIB:Shell32.lib -DEFAULTLIB:user32.lib -DEFAULTLIB:Gdi32.lib -DEFAULTLIB:Shlwapi.lib -DEFAULTLIB:Dwmapi.lib

set                  PATH=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin\amd64;%PATH%
set          INC=%INC% -I"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\include"
set LINC=%LINC% -LIBPATH:"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\lib\amd64"
set          INC=%INC% -I"C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Include"
set LINC=%LINC% -LIBPATH:"C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64"

set INC=%INC% -I"C:\Standalone\iaca"


set BUILD_MODE=-O2
set MODE_DEFINE=
if "%~2"=="-release" (
	rem -Oy
	rem -Zo
	set BUILD_MODE=-O2
	set MODE_DEFINE=-DRELEASE_BUILD
)


rem -EHsc -GR -MD -MTd -Zi -MP 
set COMPILER_OPTIONS= -MD %BUILD_MODE% -nologo -Oi -FC -wd4838 -wd4005 -fp:fast -fp:except- -Gm- -GR- -EHa- -Z7
rem set LINKER_OPTIONS= -link -SUBSYSTEM:WINDOWS -OUT:%APP_NAME%.exe -incremental:no -opt:ref
set LINKER_OPTIONS= -link -SUBSYSTEM:WINDOWS -OUT:%APP_NAME%.exe -incremental:no

del main_*.pdb > NUL 2> NUL
echo. 2>lock.tmp
cl %COMPILER_OPTIONS% ..\code\app.cpp %MODE_DEFINE% -LD %INC% -link -incremental:no -opt:ref -PDB:main_%random%.pdb -EXPORT:appMain %LINC% %LINKER_LIBS%
del lock.tmp

cl %COMPILER_OPTIONS% ..\code\main.cpp %MODE_DEFINE% %INC% %LINKER_OPTIONS% %LINC% %LINKER_LIBS%


:parseParameters
IF "%~1"=="" GOTO parseParametersEnd
IF "%~1"=="-run" call %APP_NAME%.exe
SHIFT
GOTO parseParameters
:parseParametersEnd

popd
set LOCATION=

rem exit -b