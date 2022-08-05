APP_NAME="hello3dgfx"
PLATFORM="macos"
# COMPILER_ARGS="-fsanitize=address -finstrument-functions -Wall -x objective-c++ -std="c++17" -g -o0 -objC"
# COMPILER_ARGS="-fsanitize=address -Wall -x objective-c++ -std="c++17" -g -o0 -objC"
COMPILER_ARGS="-Wall -x objective-c++ -std="c++17" -g -o0 -objC"

MAC_FRAMEWORKS="
    -framework AppKit 
    -framework MetalKit 
    -framework Metal"

TOK_ONE_SOURCEFILES="src/$PLATFORM/main.mm src/$PLATFORM/macos_platform_layer.c src/shared_apple/apple_platform_layer.c src/shared_apple/gpu.mm src/shared_windows_macos/winmac_platform_layer.c src/shared/common_platform_layer.c src/shared/logger.c src/shared/memorystore.c src/shared/tok_random.c src/shared/draw_triangle.c src/shared/bitmap_renderer.c src/shared/texture_array.c src/shared/userinput.c src/shared/zpolygon.c src/shared/lightsource.c src/shared/software_renderer.c src/shared/window_size.c src/shared/debigulator/src/decode_png.c src/shared/debigulator/src/inflate.c src/shared//decodedimage.c src/shared/clientlogic.c src/shared/common.c src/shared/text.c src/shared/scheduled_animations.c src/shared/texquad_type.c src/shared/init_application.c src/shared/gameloop.c"

echo "Building $APP_NAME for $PLATFORM..."

echo "deleting previous build..."
sudo rm -r -f build/$PLATFORM/$APP_NAME.app/*.txt
sudo rm -r -f build/$PLATFORM/$APP_NAME.app/$APP_NAME
sudo rm -r -f build/$PLATFORM/$APP_NAME.app/$APP_NAME.dsym

echo "Creating build folder..."
sudo mkdir -r build/$PLATFORM/$APP_NAME.app/debugout

############
echo "skipping metal library compilation..."
# echo "Creating metal library..."
# xcrun -sdk macosx metal -gline-tables-only -MO -g -c "src/shared_apple/Shaders.metal" -o resources/Shaders.air
# xcrun -sdk macosx metal -c "src/shared_apple/shaders.metal" -o Shaders.air
# xcrun -sdk macosx metallib resources/Shaders.air -o build/$PLATFORM/$APP_NAME.app/Shaders.metallib
############

echo "skipping resource copy..."
# echo "copy resources..."
# rm build/$PLATFORM/$APP_NAME.app/*.png
# rm build/$PLATFORM/$APP_NAME.app/*.obj
# cp resources/fontmetrics.dat build/$PLATFORM/$APP_NAME.app/fontmetrics.dat
# cp resources/*.png build/$PLATFORM/$APP_NAME.app

echo "Compiling & linking $APP_NAME..."
if
g++ $COMPILER_ARGS $MAC_FRAMEWORKS $TOK_ONE_SOURCEFILES -o build/$PLATFORM/$APP_NAME.app/$APP_NAME
# clang -finstrument-functions -Wall -x objective-c -g -o0 $MAC_FRAMEWORKS -objC $TOK_ONE_SOURCEFILES -o build/$PLATFORM/$APP_NAME.app/$APP_NAME
then
echo "compilation succesful"
else
echo "compilation failed"
exit 0
fi

echo "Booting $APP_NAME"
(cd build/$PLATFORM/$APP_NAME.app && ./$APP_NAME)
# (cd build/$PLATFORM/$APP_NAME.app && gdb ./$APP_NAME)

