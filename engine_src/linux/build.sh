APP_NAME="hello3dgfx"
PLATFORM="linux"

COMPILER_OPTIONS="-std=c99 -I/usr/include/SDL2 -D_REENTRANT"
LINKER_OPTIONS="-lc -lSDL2 -lGL"

TOK_ONE_SOURCE="src/shared/window_size.c"

echo "Building $APP_NAME for $PLATFORM..."

echo "deleting previous build(s)..."
rm -r -f build/$PLATFORM

echo "Creating build folder..."
mkdir build
mkdir build/$PLATFORM

echo "Compiling & linking $APP_NAME..."
if
sudo gcc $TOK_ONE_SOURCE src/linux/main.c -o build/$PLATFORM/$APP_NAME $COMPILER_OPTIONS $LINKER_OPTIONS
then
echo "succesful compilation, running..."
build/$PLATFORM/$APP_NAME
else
echo "build failed"
fi

