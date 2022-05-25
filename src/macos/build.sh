APP_NAME="hello3dgfx"
PLATFORM="macos"

# echo "Booting without building..."
# cd build/$PLATFORM/$APP_NAME.app && lldb ./$APP_NAME
# exit 0

MAC_FRAMEWORKS="
    -framework AppKit 
    -framework MetalKit 
    -framework Metal"

TOK_ONE_SOURCEFILES="src/$PLATFORM/main.mm src/$PLATFORM/platform_layer.c src/shared_apple/platform_layer.c src/shared_apple/gpu.mm src/shared_windows_macos/platform_layer.c src/shared/draw_triangle.c src/shared/bitmap_renderer.c src/shared/texture_array.c src/shared/userinput.c src/shared/zpolygon.c src/shared/lightsource.c src/shared/software_renderer.c src/shared/window_size.c src/shared/debigulator/src/decode_png.c src/shared/debigulator/src/inflate.c src/shared/debigulator/src/decodedimage.c src/shared/clientlogic.c src/shared/common.c src/shared/text.c src/shared/scheduled_animations.c src/shared/texquad_type.c"

echo "Building $APP_NAME for $PLATFORM..."

echo "deleting previous build(s)..."
sudo rm -r -f build/$PLATFORM/$APP_NAME.app/$APP_NAME
sudo rm -r -f build/$PLATFORM/$APP_NAME.app/$APP_NAME.dsym

echo "Creating build folder..."
sudo mkdir -r build/$PLATFORM/$APP_NAME.app/debugout

############
echo "Creating metal library..."
# echo "METAL LIBRARY NOT COMPILED - JUST COPYING TO SAVE TIME"
xcrun -sdk macosx metal -gline-tables-only -MO -g -c "src/shared_apple/Shaders.metal" -o resources/Shaders.air
xcrun -sdk macosx metal -c "src/shared_apple/shaders.metal" -o Shaders.air
xcrun -sdk macosx metallib resources/Shaders.air -o build/$PLATFORM/$APP_NAME.app/Shaders.metallib
############

echo "skipping resource copy..."
echo "copy resources..."
cp resources/cardwithuvcoords.obj build/$PLATFORM/$APP_NAME.app/cardwithuvcoords.obj
cp resources/teapot.obj build/$PLATFORM/$APP_NAME.app/teapot.obj
cp resources/*.png build/$PLATFORM/$APP_NAME.app

echo "Compiling & linking $APP_NAME..."
clang++ -D LONGLONGINT64 -Wall -x objective-c++ -std="c++17" -g -o0 $MAC_FRAMEWORKS -objC $TOK_ONE_SOURCEFILES -o build/$PLATFORM/$APP_NAME.app/$APP_NAME
# clang++ -x objective-c -Wall -g -pedantic $MAC_FRAMEWORKS -objC $TOK_ONE_SOURCEFILES -o build/$PLATFORM/$APP_NAME.app/$APP_NAME
# clang -x objective-c -Wall -g -pedantic $MAC_FRAMEWORKS -objC $TOK_ONE_SOURCEFILES -o build/$PLATFORM/$APP_NAME.app/$APP_NAME

echo "Booting $APP_NAME"
(cd build/$PLATFORM/$APP_NAME.app && ./$APP_NAME)
# (cd build/$PLATFORM/$APP_NAME.app && gdb ./$APP_NAME)

