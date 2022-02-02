APP_NAME="hello3dgfx"
PLATFORM="macos"

MAC_FRAMEWORKS="
    -framework AppKit 
    -framework MetalKit 
    -framework Metal"

echo "Building $APP_NAME for $PLATFORM..."

echo "deleting previous build(s)..."
rm -r -f build

echo "Creating build folder..."
mkdir build
mkdir build/$PLATFORM
mkdir build/$PLATFORM/$APP_NAME.app

echo "Creating metal library..."
# xcrun -sdk macosx metal -gline-tables-only -MO -g -c "src/$PLATFORM/Shaders.metal" -o Shaders.air
xcrun -sdk macosx metal -c "src/$PLATFORM/shaders.metal" -o Shaders.air
xcrun -sdk macosx metallib Shaders.air -o build/$PLATFORM/$APP_NAME.app/Shaders.metallib
rm -r Shaders.air

echo "Compiling $APP_NAME..."
clang -x objective-c -g -pedantic -c -objC src/$PLATFORM/main.mm -o build/main.o
clang -g -pedantic -c src/shared/box.c -o build/box.o
clang -g -pedantic -c src/shared/software_renderer.c -o build/software_renderer.o

echo "Linking $APP_NAME..."
clang -g -pedantic $MAC_FRAMEWORKS build/main.o build/box.o build/software_renderer.o -o build/$PLATFORM/$APP_NAME.app/$APP_NAME

echo "Booting $APP_NAME"
build/$PLATFORM/$APP_NAME.app/$APP_NAME

