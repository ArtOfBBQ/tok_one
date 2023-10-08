APP_NAME="hello3dgfx"
PLATFORM="linux"

COMPILER_OPTIONS="-w -Wfatal-errors -I engine_src -I client_src -I sampleproject_src -I engine_src/shared -march=native -x c -std=c99 -o0"

LINKER_OPTIONS="-lc -lGL -lX11"

TOK_ONE_LINUX_SOURCE="
engine_src/linux/main.c"

# TOK_ONE_LINUX_SOURCE="
# engine_src/linux/main.c
# engine_src/linux/platform_layer.c
# "

# TOK_ONE_SHARED_SOURCE="
# engine_src/shared/common_platform_layer.c
# engine_src/shared/logger.c
# engine_src/shared/memorystore.c
# engine_src/shared/tok_random.c
# engine_src/shared/objmodel.c
# engine_src/shared/texture_array.c
# engine_src/shared/userinput.c
# engine_src/shared/zpolygon.c
# engine_src/shared/lightsource.c
# engine_src/shared/renderer.c
# engine_src/shared/window_size.c
# engine_src/shared/debigulator/src/decode_png.c
# engine_src/shared/debigulator/src/decode_bmp.c
# engine_src/shared/debigulator/src/inflate.c
# engine_src/shared//decodedimage.c
# sampleproject_src/clientlogic.c
# engine_src/shared/common.c
# engine_src/shared/text.c
# engine_src/shared/scheduled_animations.c
# engine_src/shared/init_application.c
# engine_src/shared/gameloop.c
# engine_src/shared/terminal.c
# engine_src/shared/particle.c
# engine_src/shared/triangle.c
# engine_src/shared/uielement.c
# engine_src/shared/objectid.c"

echo "Building $APP_NAME for $PLATFORM..."

echo "deleting previous build(s)..."
rm -r -f build/$PLATFORM

echo "Creating build folder..."
mkdir build
mkdir build/$PLATFORM

echo "Compiling & linking $APP_NAME..."
if
sudo gcc $COMPILER_OPTIONS $TOK_ONE_LINUX_SOURCE $TOK_ONE_SHARED_SOURCE -o build/$PLATFORM/$APP_NAME $LINKER_OPTIONS 
then
echo "succesful compilation, running..."
build/$PLATFORM/$APP_NAME
else
echo "build failed"
fi

