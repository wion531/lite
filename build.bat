@echo off

set /A build_debug=0

rem download this:
rem https://nuwen.net/mingw.html

echo compiling (windows)...

windres res.rc -O coff -o res.res

goto build_msvc
gcc src/*.c src/api/*.c src/lib/lua52/*.c src/lib/stb/*.c^
    -O3 -s -std=gnu11 -fno-strict-aliasing -Isrc -DLUA_USE_POPEN^
    -Iwinlib/SDL2-2.0.10/x86_64-w64-mingw32/include^
    -lmingw32 -lm -lSDL2main -lSDL2 -Lwinlib/SDL2-2.0.10/x86_64-w64-mingw32/lib^
    -mwindows res.res^
    -o lite.exe
:build_msvc

call vcvars64

if %build_debug%==0 (
  cl -nologo -O2 -DRENDER_OPENGL -I src src\*.c src\api\*.c src\lib\lua52\*.c^
  src\lib\stb\*.c res.res /link /out:lite.exe /subsystem:windows user32.lib^
  shell32.lib opengl32.lib SDL2main.lib SDL2.lib
  del *.obj
) else (
  cl -nologo -Z7 -DRENDER_OPENGL -I src src\*.c src\api\*.c src\lib\lua52\*.c^
    src\lib\stb\*.c res.res /link /out:lite.exe /subsystem:windows user32.lib^
    shell32.lib opengl32.lib SDL2main.lib SDL2.lib
  del *.obj
)

echo done
