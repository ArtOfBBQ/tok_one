APP_NAME="hello3dgfx"

COMPILER_PATHS="
-I engine_src/macos/
-I engine_src/shared/debigulator/src/
-I engine_src/shared/
-I engine_src/shared_apple/
-I engine_src/shared_linux_apple/
-I engine_src/shared_windows_macos/
-I sampleproject_src/
"

COMPILER_ARGS="
-D SHARED_APPLE_PLATFORM
-ferror-limit=2 
-march=native 
-Wall 
-x objective-c -std=c11 
-O0 
-objC"

if [[ $1 = "DEBUG" ]]; then
COMPILER_ARGS_EXTRA="-g"
else
    if [[ $1 = "RELEASE" ]]; then
    COMPILER_ARGS_EXTRA=""
    fi
fi

MAC_FRAMEWORKS="
    -framework AppKit 
    -framework MetalKit 
    -framework Metal
    -framework AudioToolbox"

echo "create build folder..>"
sudo mkdir -p build/macos/$APP_NAME.app

echo "deleting previous build..."
sudo rm -r -f build/macos/$APP_NAME.app/*.txt
sudo rm -r -f build/macos/$APP_NAME.app/$APP_NAME sudo rm -r -f build/macos/$APP_NAME.app/$APP_NAME.dsym

############
if test -f "build/macos/$APP_NAME.app/shaders.metallib"; then
    echo "shaders.metallib already in build folder, skip metal compilation...."
else
    echo "shaders.metallib not in build folder, compiling new metal library..."
    sudo xcrun -sdk macosx metal -gline-tables-only -MO -g -c "engine_src/shared_apple/Shaders.metal" -o resources/Shaders.air
    sudo xcrun -sdk macosx metal -c "engine_src/shared_apple/shaders.metal" -o Shaders.air
    sudo xcrun -sdk macosx metallib resources/Shaders.air -o build/macos/$APP_NAME.app/Shaders.metallib
fi
############

############
echo "copy resources..."

pushd resources > /dev/null
for extension in png obj dat
do
    for f in *.$extension;
    do
    if test -f "../build/macos/$APP_NAME.app/$f"; then
        echo "$f was already in build folder...."
    else
        echo "copying resource file $f to build folder..."
        sudo cp -r -f $f ../build/macos/$APP_NAME.app/$f
    fi
    done
done
popd > /dev/null
############


SOURCE_FILES="
engine_src/shared/debigulator/src/inflate.c
engine_src/shared/debigulator/src/decode_png.c
engine_src/shared/debigulator/src/decode_bmp.c
engine_src/shared/decodedimage.c
engine_src/shared/wav.c
engine_src/shared/objparser.c
engine_src/shared/common.c
engine_src/shared/logger.c
engine_src/shared/collision.c
engine_src/shared/objectid.c
engine_src/shared/audio.c
engine_src/shared/window_size.c
engine_src/shared/triangle.c
engine_src/shared/lightsource.c
engine_src/shared/cpu_to_gpu_types.c
engine_src/shared_apple/apple_audio.m
engine_src/shared/common_platform_layer.c
engine_src/macos/macos_platform_layer.m
engine_src/shared_apple/apple_platform_layer.m
engine_src/shared_linux_apple/linux_apple_platform_layer.c
engine_src/shared/memorystore.c
engine_src/shared/objmodel.c
engine_src/shared/userinput.c
engine_src/shared/tok_random.c
engine_src/shared_apple/gpu.m
engine_src/shared/texture_array.c
engine_src/shared/lines.c
engine_src/shared/zpolygon.c
engine_src/shared/particle.c
engine_src/shared/scheduled_animations.c
engine_src/shared/text.c
engine_src/shared/uielement.c
sampleproject_src/clientlogic.c
engine_src/shared/terminal.c
engine_src/shared/renderer.c
engine_src/shared/gameloop.c
engine_src/shared/init_application.c
engine_src/macos/macos_main.m"


echo "Compiling & linking $APP_NAME..."
if
sudo time gcc $COMPILER_PATHS $COMPILER_ARGS $COMPILER_ARGS_EXTRA $MAC_FRAMEWORKS $SOURCE_FILES -o build/macos/$APP_NAME.app/$APP_NAME
then
echo "compilation succesful."
read -p "enter to run app, ctrl-c to quit"
open build/macos/$APP_NAME.app
else
echo "compilation failed"
exit 0
fi

