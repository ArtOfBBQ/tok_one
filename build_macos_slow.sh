APP_NAME="loreseek"

COMPILER_PATHS="
-I engine_src/macos/
-I engine_src/shared/debigulator/src/
-I engine_src/shared/
-I engine_src/shared_apple/
-I engine_src/shared_linux_apple/
-I engine_src/shared_windows_macos/
-I sampleproject_src/
"

COMPILER_ARGS="
-include clientlogic_macro_settings.h
-ferror-limit=2 
-march=native 
-Wall 
-x objective-c -std=c11 
-O0 
-objC"

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
    sudo xcrun -sdk macosx metal -gline-tables-only -MO -g -c "engine_src/shared_apple/Shaders.metal" -I sampleproject_src -I engine_src/shared -o resources/Shaders.air 
    sudo xcrun -sdk macosx metal -c "engine_src/shared_apple/shaders.metal" -I sampleproject_src -I engine_src/shared -o Shaders.air
    sudo xcrun -sdk macosx metallib resources/Shaders.air -o build/macos/$APP_NAME.app/Shaders.metallib
    if test -f "build/macos/$APP_NAME.app/shaders.metallib"; then
	echo "shaders.metallib created! Please compile again."
    else
	echo "Failed to create shaders.metallib. Please check if xcode "
	echo "command line tools and Metal are installed. Try these commands:"
	echo "> xcrun -- version"
	echo "> xcode-select --print-path"
	echo "ls /Application/Xcode.app/Contents/Developer <- xcode is probably here"
	echo "If that path exists but print-path shows a different directory,"
	echo "you may have installed xcode command line tools 1st and xcode "
	echo "itself later. Try pointing xcode-select to the xcode directory: "
	echo "sudo xcode-select -s /Applications/Xcode.app/Contents/Developer"
	echo "Or google for: how to compile metal shader files command line"
	exit 0
    fi
fi
############

############
echo "copy resources..."

pushd resources > /dev/null
for extension in png obj mtl dat
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


SOURCE_FILES="
engine_src/shared/debigulator/src/inflate.c
engine_src/shared/debigulator/src/decode_png.c
engine_src/shared/debigulator/src/decode_bmp.c
engine_src/shared/T1_decodedimage.c
engine_src/shared/T1_wav.c
engine_src/shared/T1_tokenizer.c
engine_src/shared/T1_objparser.c
engine_src/shared/T1_mtlparser.c
engine_src/shared/T1_common.c
engine_src/shared/T1_logger.c
engine_src/shared/T1_collision.c
engine_src/shared/T1_zspriteid.c
engine_src/shared/T1_audio.c
engine_src/shared/T1_engine_globals.c
engine_src/shared/T1_triangle.c
engine_src/shared/T1_material.c
engine_src/shared/T1_lightsource.c
engine_src/shared/T1_cpu_to_gpu_types.c
engine_src/shared_apple/T1_apple_audio.m
engine_src/shared/T1_platform_layer_common.c
engine_src/macos/T1_macos_platform_layer.m
engine_src/shared_apple/T1_apple_platform_layer.m
engine_src/shared_linux_apple/T1_linux_apple_platform_layer.c
engine_src/shared/T1_memorystore.c
engine_src/shared/T1_profiler.c
engine_src/shared/T1_objmodel.c
engine_src/shared/T1_userinput.c
engine_src/shared/T1_random.c
engine_src/shared_apple/T1_gpu.m
engine_src/shared/T1_texture_array.c
engine_src/shared/T1_texture_files.c
engine_src/shared/T1_lines.c
engine_src/shared/T1_zsprite.c
engine_src/shared/T1_particle.c
engine_src/shared/T1_scheduled_animations.c
engine_src/shared/T1_text.c
engine_src/shared/T1_uielement.c
sampleproject_src/clientlogic.c
engine_src/shared/T1_terminal.c
engine_src/shared/T1_renderer.c
engine_src/shared/T1_gameloop.c
engine_src/shared/T1_init_application.c
engine_src/macos/T1_macos_main.m"


echo "Compiling & linking $APP_NAME..."
if
sudo time gcc $COMPILER_PATHS $COMPILER_ARGS $COMPILER_ARGS_EXTRA $MAC_FRAMEWORKS $SOURCE_FILES -o build/macos/$APP_NAME.app/$APP_NAME
then
echo "compilation succesful."
read -p "enter to run app, ctrl-c to quit"
open build/macos/$APP_NAME.app
else
echo "compilation failed"
exit 0
fi

