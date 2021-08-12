@echo off
rmdir /Q /S bin
mkdir bin
pushd bin

rem Name
set name=App

rem Include directories 
set inc=/I ..\include /I ..\third_party\include\

rem Source files
set src_main=..\source\*.c ..\source\render_passes\*.c

rem All source together
set src_all=%src_main%

rem OS Libraries
set os_libs= opengl32.lib kernel32.lib user32.lib ^
shell32.lib vcruntime.lib msvcrt.lib gdi32.lib Advapi32.lib

rem Link options
set l_options=/EHsc /link /SUBSYSTEM:CONSOLE /NODEFAULTLIB:msvcrt.lib

rem Compile Release
cl /MP /FS /Ox /W1 /Fe%name%.exe %src_all% %inc% ^
/EHsc /link /SUBSYSTEM:CONSOLE /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:LIBCMT ^
%os_libs%

rem Compile Debug
rem cl /w /MP -Zi /DEBUG:FULL /Fe%name%.exe %src_all% %inc% ^
rem /EHsc /link /SUBSYSTEM:CONSOLE /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:LIBCMT ^
rem %os_libs%

popd
