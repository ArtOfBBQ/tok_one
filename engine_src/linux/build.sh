APP_NAME="hello3dgfx"
PLATFORM="linux"

COMPILER_OPTIONS="-lm -std=c99 -I /usr/include/SDL2 -D_REENTRANT -I engine_src -I client_src -I sampleproject_src -I engine_src/shared -I engine_src/linux"
LINKER_OPTIONS="-lc -lSDL2 -lGL"

TOK_ONE_SOURCEFILES="engine_src/$PLATFORM/main.c engine_src/shared_windows_macos/winmac_platform_layer.c engine_src/shared/common_platform_layer.c engine_src/shared/logger.c engine_src/shared/memorystore.c engine_src/shared/tok_random.c engine_src/shared/draw_triangle.c engine_src/shared/texture_array.c engine_src/shared/userinput.c engine_src/shared/zpolygon.c engine_src/shared/lightsource.c engine_src/shared/renderer.c engine_src/shared/window_size.c engine_src/shared/debigulator/src/decode_png.c engine_src/shared/debigulator/src/decode_bmp.c engine_src/shared/debigulator/src/inflate.c engine_src/shared//decodedimage.c sampleproject_src/clientlogic.c engine_src/shared/common.c engine_src/shared/text.c engine_src/shared/scheduled_animations.c engine_src/shared/init_application.c engine_src/shared/gameloop.c engine_src/shared/terminal.c"

echo "Building $APP_NAME for $PLATFORM..."

echo "deleting previous build(s)..."
rm -r -f build/$PLATFORM

echo "Creating build folder..."
mkdir build
mkdir build/$PLATFORM

echo "Compiling & linking $APP_NAME..."
if
sudo gcc $TOK_ONE_SOURCEFILES -o build/$PLATFORM/$APP_NAME $COMPILER_OPTIONS $LINKER_OPTIONS
then
echo "succesful compilation, running..."
build/$PLATFORM/$APP_NAME
else
echo "build failed"
fi

