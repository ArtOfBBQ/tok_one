APP_NAME="hello3dgfx"
PLATFORM="macos"
COMPILER_ARGS="-I engine_src -I client_src -I sampleproject_src -I engine_src/shared -march=native -Wall -x objective-c -std=c99 -o0 -objC"

MAC_FRAMEWORKS="
    -framework AppKit 
    -framework MetalKit 
    -framework Metal
    -framework AVFoundation"

TOK_ONE_SOURCEFILES="engine_src/$PLATFORM/main.m engine_src/$PLATFORM/macos_platform_layer.c engine_src/shared_apple/apple_platform_layer.c engine_src/shared_apple/gpu.m engine_src/shared_windows_macos/winmac_platform_layer.c engine_src/shared/common_platform_layer.c engine_src/shared/logger.c engine_src/shared/memorystore.c engine_src/shared/tok_random.c engine_src/shared/objmodel.c engine_src/shared/texture_array.c engine_src/shared/userinput.c engine_src/shared/zpolygon.c engine_src/shared/lightsource.c engine_src/shared/renderer.c engine_src/shared/window_size.c engine_src/shared/debigulator/src/decode_png.c engine_src/shared/debigulator/src/decode_bmp.c engine_src/shared/debigulator/src/inflate.c engine_src/shared//decodedimage.c sampleproject_src/clientlogic.c engine_src/shared/common.c engine_src/shared/text.c engine_src/shared/scheduled_animations.c engine_src/shared/init_application.c engine_src/shared/gameloop.c engine_src/shared/terminal.c engine_src/shared/particle.c engine_src/shared/triangle.c engine_src/shared/uielement.c engine_src/shared/objectid.c"

echo "Building $APP_NAME for $PLATFORM..."

echo "create build folder..>"
sudo mkdir -p build/$PLATFORM/$APP_NAME.app

echo "deleting previous build..."
sudo rm -r -f build/$PLATFORM/$APP_NAME.app/*.txt
sudo rm -r -f build/$PLATFORM/$APP_NAME.app/$APP_NAME
sudo rm -r -f build/$PLATFORM/$APP_NAME.app/$APP_NAME.dsym

############
if test -f "build/$PLATFORM/$APP_NAME.app/shaders.metallib"; then
    echo "shaders.metallib already in build folder, skip metal compilation...."
else
    echo "shaders.metallib not in build folder, compiling new metal library..."
    sudo xcrun -sdk macosx metal -gline-tables-only -MO -g -c "engine_src/shared_apple/Shaders.metal" -o resources/Shaders.air
    sudo xcrun -sdk macosx metal -c "engine_src/shared_apple/shaders.metal" -o Shaders.air
    sudo xcrun -sdk macosx metallib resources/Shaders.air -o build/$PLATFORM/$APP_NAME.app/Shaders.metallib
fi
############

############
echo "copy resources..."

pushd resources > /dev/null
for extension in png obj dat
do
    for f in *.$extension;
    do
    if test -f "../build/$PLATFORM/$APP_NAME.app/$f"; then
        echo "$f was already in build folder...."
    else
        echo "copying resource file $f to build folder..."
        sudo cp -r -f $f ../build/$PLATFORM/$APP_NAME.app/$f
    fi
    done
done
popd > /dev/null
############

echo "Compiling & linking $APP_NAME..."
if
sudo gcc $COMPILER_ARGS $MAC_FRAMEWORKS $TOK_ONE_SOURCEFILES -o build/$PLATFORM/$APP_NAME.app/$APP_NAME
then
echo "compilation succesful."
read -p "enter to run app, ctrl-c to quit"
open build/$PLATFORM/$APP_NAME.app
else
echo "compilation failed"
exit 0
fi

