APP_NAME="hello3dgfx"
PLATFORM="macos"

MAC_FRAMEWORKS="
    -framework AppKit 
    -framework MetalKit 
    -framework Metal"

TOK_ONE_SOURCEFILES="src/$PLATFORM/main.mm src/$PLATFORM/platform_layer.c src/shared/draw_triangle.c src/shared/bitmap_renderer.c src/shared_apple/platform_layer.c src/shared_windows_macos/platform_layer.c src/shared_apple/gpu.mm src/shared/texture_array.c src/shared/userinput.c src/shared/zpolygon.c src/shared/software_renderer.c src/shared/window_size.c src/shared/decode_png.c src/shared/inflate.c src/shared/clientlogic.c src/shared/common.c"

echo "Building $APP_NAME for $PLATFORM..."

echo "deleting previous build(s)..."
rm -r -f build

echo "Creating build folder..."
mkdir build
mkdir build/$PLATFORM
mkdir build/$PLATFORM/$APP_NAME.app

echo "Creating metal library..."
# echo "METAL LIBRARY NOT COMPILED - JUST COPYING TO SAVE TIME"
xcrun -sdk macosx metal -gline-tables-only -MO -g -c "src/shared_apple/Shaders.metal" -o resources/Shaders.air
xcrun -sdk macosx metal -c "src/shared_apple/shaders.metal" -o Shaders.air
xcrun -sdk macosx metallib resources/Shaders.air -o build/$PLATFORM/$APP_NAME.app/Shaders.metallib

echo "copy resources..."
cp resources/cardwithuvcoords.obj build/$PLATFORM/$APP_NAME.app/cardwithuvcoords.obj
cp resources/teapot.obj build/$PLATFORM/$APP_NAME.app/teapot.obj
cp resources/phoebus.png build/$PLATFORM/$APP_NAME.app/phoebus.png
cp resources/sampletexture.png build/$PLATFORM/$APP_NAME.app/sampletexture.png
cp resources/town.png build/$PLATFORM/$APP_NAME.app/town.png

echo "Compiling & linking $APP_NAME..."
clang++ -x objective-c -Wall -g -pedantic $MAC_FRAMEWORKS -objC $TOK_ONE_SOURCEFILES -o build/$PLATFORM/$APP_NAME.app/$APP_NAME


echo "Booting $APP_NAME"
(cd build/$PLATFORM/$APP_NAME.app && ./$APP_NAME)
# (cd build/$PLATFORM/$APP_NAME.app && gdb ./$APP_NAME)

