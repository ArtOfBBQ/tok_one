echo build script for windows...

echo delete previous build...
del /q build\windows

echo create output folder...
mkdir build\windows

echo copying resource files...
copy resources\teapot.obj build\windows\teapot.obj
copy resources\teddybear.obj build\windows\teddybear.obj

echo compile program...
set possible_gl_libs=-l opengl32
gcc -g -Wall src\windows\main.c src\shared\window_size.c src\shared\box.c src\shared\software_renderer.c -l opengl32 -l gdi32 -o build\windows\hello3dgfx.exe

echo running program...
pushd build\windows\
hello3dgfx.exe
popd 

