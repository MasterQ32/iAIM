REM Set those to the correct paths of your system.
set SDL2_image_ROOT=D:\libs\SDL2_image-2.0.1\
set SDL2_mixer_ROOT=D:\libs\SDL2_mixer-2.0.1\
set SDL2_ROOT=D:\libs\SDL2-2.0.4\

REM Either x86 or x64
set ARCH=x64

REM Compiler Options
set LINK=/SUBSYSTEM:WINDOWS 
SET CL=%CL% /I "%SDL2_ROOT%\include" 
SET CL=%CL% /I "%SDL2_image_ROOT%\include"
SET CL=%CL% /I "%SDL2_mixer_ROOT%\include"
cl "%SDL2_ROOT%\lib\%ARCH%\SDL2.lib" "%SDL2_ROOT%\lib\%ARCH%\SDL2main.lib" "%SDL2_image_ROOT%\lib\%ARCH%\SDL2_image.lib" "%SDL2_mixer_ROOT%\lib\%ARCH%\SDL2_mixer.lib"   main.c
move /Y main.exe iAIM.exe

xcopy /Y "%SDL2_ROOT%\lib\%ARCH%\*.dll" .
xcopy /Y "%SDL2_image_ROOT%\lib\%ARCH%\*.dll" .
xcopy /Y "%SDL2_mixer_ROOT%\lib\%ARCH%\*.dll" .