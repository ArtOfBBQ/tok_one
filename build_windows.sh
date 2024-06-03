APP_NAME="tok_one"

# C:/"Program Files"/"Microsoft Visual Studio"/2022/Community/VC/Auxiliary/Build/vcvarsall.bat x64

# /clr:nostdlib	Ignore the system .NET framework directory when searching for assemblies.
# 
# /D<name>{=|#}<text>	Defines constants and macros.
# 
# /F	Sets stack size.
# 
# /Fe	Renames the executable file.
# 
# /fsanitize	Enables compilation of sanitizer instrumentation such as AddressSanitizer.
# 
# /I<dir>	Searches a directory for include files.
# 
# /O2	Creates fast code.
# 
# /Od	Disables optimization.
# 
# /Oi[-]	Generates intrinsic functions.
# 
# /options:strict	Unrecognized compiler options are errors.
# 
# 
# /std:c11	C11 standard ISO/IEC 9899:2011.
# note (Jelle): There doesn't seem to be an option for C99 T_T
# 
# /TC	Specifies all source files are C.
# 
# /Wall	Enable all warnings, including warnings that are disabled by default.
# 
# /WX	Treat warnings as errors.

COMPILER_ARGS="
-arch:AVX
-options:strict
-TC
-std:c11
-I"../sampleproject_src/"
-I"../engine_src/windows/"
-I"../engine_src/shared/debigulator/src/"
-I"../engine_src/shared/"
-I"../engine_src/shared_apple/"
-I"../engine_src/shared_linux_apple/"
-I"../engine_src/shared_windows_macos/"
-I"../engine_src/shared_opengl/"
-I"../sampleproject_src/"
"

SOURCE_FILES="../unitybuild_windows.c"

## SOURCE_FILES="
## ../engine_src/shared/debigulator/src/inflate.c
## ../engine_src/shared/debigulator/src/decode_png.c
## ../engine_src/shared/debigulator/src/decode_bmp.c
## ../engine_src/shared/decodedimage.c
## ../engine_src/shared/wav.c
## ../engine_src/shared/objparser.c
## ../engine_src/shared/common.c
## ../engine_src/shared/logger.c
## ../engine_src/shared/objectid.c
## ../engine_src/shared/audio.c
## ../engine_src/shared/window_size.c
## ../engine_src/shared/triangle.c
## ../engine_src/shared/lightsource.c
## ../engine_src/shared/cpu_to_gpu_types.c
## ../engine_src/shared_opengl/opengl_extensions.c
## ../engine_src/shared_opengl/tok_opengl.c
## ../engine_src/shared/common_platform_layer.c
## ../engine_src/windows/windows_platform_layer.c
## ../engine_src/shared/memorystore.c
## ../engine_src/shared/objmodel.c
## ../engine_src/shared/userinput.c
## ../engine_src/shared/tok_random.c
## ../engine_src/shared/texture_array.c
## ../engine_src/shared/zpolygon.c
## ../engine_src/shared/particle.c
## ../engine_src/shared/scheduled_animations.c
## ../engine_src/shared/text.c
## ../engine_src/shared/uielement.c
## ../sampleproject_src/clientlogic.c
## ../engine_src/shared/terminal.c
## ../engine_src/shared/renderer.c
## ../engine_src/shared/gameloop.c
## ../engine_src/shared/init_application.c
## ../engine_src/windows/windows_main.c
## "

if [[ $1 = "DEBUG" ]]; then
COMPILER_ARGS_EXTRA="-Zi -Od"
else
    if [[ $1 = "RELEASE" ]]; then
    COMPILER_ARGS_EXTRA="-O2"
    fi
fi

echo "create build folder..>"
mkdir -p build/windows/$APP_NAME
# mkdir -p build/windows/$APP_NAME

echo "deleting previous build..."
rm -r -f build/*.obj
rm -r -f build/*.pdb
rm -r -f build/windows/$APP_NAME/*.txt
rm -r -f build/windows/$APP_NAME/*.glsl
rm -r -f build/windows/$APP_NAME/$APP_NAME
# rm -r -f build/windows/$APP_NAME/$APP_NAME.dsym
############

# do stuff

############
echo "copy resources..."

pushd resources > /dev/null
cp *.glsl ../build/windows/$APP_NAME/

for extension in png obj dat
do
    for f in *.$extension;
    do
    if test -f "../build/windows/$APP_NAME/$f"; then
        echo "$f was already in build folder...."
    else
        echo "copying resource file $f to build folder..."
        cp -r -f $f ../build/windows/$APP_NAME/$f
    fi
    done
done
popd > /dev/null
############

# kernel32.lib: for GetProcAddress()
pushd build
cl $COMPILER_ARGS $COMPILER_ARGS_EXTRA $SOURCE_FILES -Fe"windows/$APP_NAME/$APP_NAME.exe" -link kernel32.lib user32.lib gdi32.lib opengl32.lib
# cl $COMPILER_ARGS $COMPILER_ARGS_EXTRA ../unitybuild_windows.c -Fe"windows/$APP_NAME/$APP_NAME.exe" -link kernel32.lib user32.lib gdi32.lib opengl32.lib
popd

echo "Press any key to run program, ctrl-C to abort..."
read -n 1 -s
build/windows/$APP_NAME/$APP_NAME.exe

