APP_NAME="hello3dgfx"

COMPILER_PATHS="
-I engine_src/macos/
-I engine_src/shared/debigulator/src/
-I engine_src/shared/
-I engine_src/shared_apple/
-I engine_src/shared_linux_apple/
-I engine_src/shared_windows_macos/
-I sampleproject_src/
"

COMPILER_ARGS="-march=native -Wall -x objective-c -std=c11 -O0 -objC"

if [[ $1 = "DEBUG" ]]; then
COMPILER_ARGS_EXTRA="-g"
else
    if [[ $1 = "RELEASE" ]]; then
    COMPILER_ARGS_EXTRA=""
    fi
fi

MAC_FRAMEWORKS="
    -framework AppKit 
    -framework MetalKit 
    -framework Metal
    -framework AudioToolbox"

echo "create build folder..>"
sudo mkdir -p build/macos/$APP_NAME.app

echo "deleting previous build..."
sudo rm -r -f build/macos/$APP_NAME.app/*.txt
sudo rm -r -f build/macos/$APP_NAME.app/$APP_NAME sudo rm -r -f build/macos/$APP_NAME.app/$APP_NAME.dsym

############
if test -f "build/macos/$APP_NAME.app/shaders.metallib"; then
    echo "shaders.metallib already in build folder, skip metal compilation...."
else
    echo "shaders.metallib not in build folder, compiling new metal library..."
    sudo xcrun -sdk macosx metal -gline-tables-only -MO -g -c "engine_src/shared_apple/Shaders.metal" -o resources/Shaders.air
    sudo xcrun -sdk macosx metal -c "engine_src/shared_apple/shaders.metal" -o Shaders.air
    sudo xcrun -sdk macosx metallib resources/Shaders.air -o build/macos/$APP_NAME.app/Shaders.metallib
fi
############

############
echo "copy resources..."

pushd resources > /dev/null
for extension in png obj dat
do
    for f in *.$extension;
    do
    if test -f "../build/macos/$APP_NAME.app/$f"; then
        echo "$f was already in build folder...."
    else
        echo "copying resource file $f to build folder..."
        sudo cp -r -f $f ../build/macos/$APP_NAME.app/$f
    fi
    done
done
popd > /dev/null
############

echo "Compiling & linking $APP_NAME..."
if
sudo time gcc $COMPILER_PATHS $COMPILER_ARGS $COMPILER_ARGS_EXTRA $MAC_FRAMEWORKS unitybuild_macos.c -o build/macos/$APP_NAME.app/$APP_NAME
then
echo "compilation succesful."
read -p "enter to run app, ctrl-c to quit"
open build/macos/$APP_NAME.app
else
echo "compilation failed"
exit 0
fi

