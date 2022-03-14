echo build script for windows...

echo delete previous build...
del /q build\windows

echo create output folder...
mkdir build\windows

echo copying resource files...
copy resources\teapot.obj build\windows\teapot.obj
copy resources\teddybear.obj build\windows\teddybear.obj
REM copy resources\fs_angrymob.png build\windows\fs_angrymob.png
copy resources\structuredart.png build\windows\structuredart.png
copy resources\structuredart.png build\windows\structuredart2.png
copy resources\phoebus.png build\windows\phoebus.png
copy resources\vertex_shader.glsl build\windows\vertex_shader.glsl
copy resources\fragment_shader.glsl build\windows\fragment_shader.glsl

echo compile program...
set possible_gl_libs=-l opengl32
gcc -g src\shared_windows_android\opengl.c src\shared\inflate.c src\shared\decode_png.c src\windows\main.c src\shared\window_size.c src\shared\zpolygon.c src\shared\software_renderer.c -l opengl32 -l gdi32 -o build\windows\hello3dgfx.exe

echo running program...
pushd build\windows\
hello3dgfx.exe
popd 

