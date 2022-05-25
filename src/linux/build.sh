APP_NAME="hello3dgfx"
PLATFORM="linux"

TOK_ONE_SOURCEFILES="src/$PLATFORM/main.c src/$PLATFORM/platform_layer.c src/shared/draw_triangle.c src/shared/bitmap_renderer.c src/shared_windows_macos/platform_layer.c src/shared/texture_array.c src/shared/userinput.c src/shared/zpolygon.c src/shared/lightsource.c src/shared/software_renderer.c src/shared/window_size.c src/shared/debigulator/src/decode_png.c src/shared/debigulator/src/inflate.c src/shared/debigulator/src/decodedimage.c src/shared/clientlogic.c src/shared/common.c src/shared/text.c src/shared/scheduled_animations.c src/shared/texquad_type.c"

echo "Building $APP_NAME for $PLATFORM..."

echo "deleting previous build(s)..."
rm -r -f build/$PLATFORM/$APP_NAME.app/$APP_NAME
rm -r -f build/$PLATFORM/$APP_NAME.app/$APP_NAME.dsym

echo "Creating build folder..."
mkdir build
mkdir build/$PLATFORM
mkdir build/$PLATFORM/$APP_NAME.app

# echo "skipping resource copy..."
echo "copy resources..."
cp resources/cardwithuvcoords.obj build/$PLATFORM/$APP_NAME.app/cardwithuvcoords.obj
cp resources/teapot.obj build/$PLATFORM/$APP_NAME.app/teapot.obj
cp resources/phoebus.png build/$PLATFORM/$APP_NAME.app/phoebus.png
cp resources/sampletexture.png build/$PLATFORM/$APP_NAME.app/sampletexture.png
cp resources/font.png build/$PLATFORM/$APP_NAME.app/font.png
cp resources/structuredart1.png build/$PLATFORM/$APP_NAME.app/structuredart1.png
cp resources/structuredart2.png build/$PLATFORM/$APP_NAME.app/structuredart2.png
cp resources/structuredart3.png build/$PLATFORM/$APP_NAME.app/structuredart3.png
cp resources/replacement.png build/$PLATFORM/$APP_NAME.app/replacement.png

echo "Compiling & linking $APP_NAME..."
sudo gcc -D LONGINT64 -Wall -std="c99" -g -o0 $TOK_ONE_SOURCEFILES -o build/$PLATFORM/$APP_NAME.app/$APP_NAME -lm

echo "Booting $APP_NAME"
# (cd build/$PLATFORM/$APP_NAME.app && ./$APP_NAME)
# (cd build/$PLATFORM/$APP_NAME.app && valgrind --leak-check=full ./$APP_NAME)
# (cd build/$PLATFORM/$APP_NAME.app && gdb ./$APP_NAME)

