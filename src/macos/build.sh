APP_NAME="hello3dgfx"
PLATFORM="macos"
COMPILER_ARGS="-march=native -Wall -x objective-c -std=c99 -g -o0 -objC"
# COMPILER_ARGS="-march=native -Wall -x objective-c -std=c99 -o0 -objC"

MAC_FRAMEWORKS="
    -framework AppKit 
    -framework MetalKit 
    -framework Metal
    -framework AVFoundation"

TOK_ONE_SOURCEFILES="src/$PLATFORM/main.m src/$PLATFORM/macos_platform_layer.c src/shared_apple/apple_platform_layer.c src/shared_apple/gpu.m src/shared_windows_macos/winmac_platform_layer.c src/shared/common_platform_layer.c src/shared/logger.c src/shared/memorystore.c src/shared/tok_random.c src/shared/draw_triangle.c src/shared/bitmap_renderer.c src/shared/texture_array.c src/shared/userinput.c src/shared/zpolygon.c src/shared/lightsource.c src/shared/software_renderer.c src/shared/window_size.c src/shared/debigulator/src/decode_png.c src/shared/debigulator/src/decode_bmp.c src/shared/debigulator/src/inflate.c src/shared//decodedimage.c src/shared/clientlogic.c src/shared/common.c src/shared/text.c src/shared/scheduled_animations.c src/shared/texquad_type.c src/shared/init_application.c src/shared/gameloop.c"

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
    sudo xcrun -sdk macosx metal -gline-tables-only -MO -g -c "src/shared_apple/Shaders.metal" -o resources/Shaders.air
    sudo xcrun -sdk macosx metal -c "src/shared_apple/shaders.metal" -o Shaders.air
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
echo "compilation succesful"
else
echo "compilation failed"
exit 0
fi

echo "Booting $APP_NAME"
(cd build/$PLATFORM/$APP_NAME.app && ./$APP_NAME)

