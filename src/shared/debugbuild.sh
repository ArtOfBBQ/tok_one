echo "building toy app for debugging (no GUI)..."

echo "deleting previous build(s)..."
rm -r -f build

echo "Creating build folder..."
mkdir build

echo "Compiling toy app..."
clang -g -pedantic src/shared/debugmain.c src/shared/box.c src/shared/software_renderer.c -o build/debugme

build/debugme
# sudo gdb build/debugme

