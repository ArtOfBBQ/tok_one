APP_NAME="hello3dgfx"
PLATFORM="linux"

COMPILER_OPTIONS_SHARED="
-x c -std=gnu99 -o0 -pthread
-D LINUX_PLATFORM 
-I engine_src
-I client_src
-I sampleproject_src
-I engine_src/shared
-I engine_src/shared_linux_apple
-I engine_src/shared_opengl"

if [[ $1 = "DEBUG" ]]; then
    COMPILER_OPTIONS_EXTRA="-fsanitize=address -w -Wfatal-errors -march=native -g"
else
    if [[ $1 = "RELEASE" ]]; then
        COMPILER_OPTIONS_EXTRA="-D LOGGER_IGNORE_ASSERTS -D NDEBUG -w -Wfatal-errors -march=native -O2"
    else
        echo "pass DEBUG or RELEASE as a command line argument"
        exit 0
    fi
fi

LINKER_OPTIONS="-lc -lm -lGL -lX11 -pthread"

TOK_ONE_LINUX_SOURCE="
engine_src/linux/main.c
engine_src/shared_opengl/opengl.c
engine_src/linux/linux_platform_layer.c
engine_src/linux/linuxkeyboard.c
engine_src/shared_linux_apple/linux_apple_platform_layer.c
"

TOK_ONE_SHARED_SOURCE="
engine_src/shared/common_platform_layer.c
engine_src/shared/logger.c
engine_src/shared/memorystore.c
engine_src/shared/tok_random.c
engine_src/shared/audio.c
engine_src/shared/wav.c
engine_src/shared/objmodel.c
engine_src/shared/objparser.c
engine_src/shared/texture_array.c
engine_src/shared/userinput.c
engine_src/shared/zpolygon.c
engine_src/shared/lightsource.c
engine_src/shared/renderer.c
engine_src/shared/window_size.c
engine_src/shared/debigulator/src/decode_png.c
engine_src/shared/debigulator/src/decode_bmp.c
engine_src/shared/debigulator/src/inflate.c
engine_src/shared//decodedimage.c
sampleproject_src/clientlogic.c
engine_src/shared/common.c
engine_src/shared/text.c
engine_src/shared/scheduled_animations.c
engine_src/shared/init_application.c
engine_src/shared/gameloop.c
engine_src/shared/terminal.c
engine_src/shared/particle.c
engine_src/shared/triangle.c
engine_src/shared/uielement.c
engine_src/shared/objectid.c"


echo "Building $APP_NAME for $PLATFORM..."

echo "deleting previous build(s)..."
rm -r -f build/$PLATFORM/$APP_NAME/$APP_NAME
# rm build/$PLATFORM/$APP_NAME/*.glsl

echo "Creating build folder..."
mkdir build
mkdir build/$PLATFORM
mkdir build/$PLATFORM/$APP_NAME
mkdir build/$PLATFORM/$APP_NAME/resources

############
echo "copy resources..."

pushd resources > /dev/null
for extension in png obj dat glsl
do
    for f in *.$extension;
    do
    if test -f "../build/$PLATFORM/$APP_NAME/$f"; then
        echo "$f was already in build folder...."
    else
        echo "copying resource file $f to build folder..."
        sudo cp -r -f $f ../build/$PLATFORM/$APP_NAME/$f
    fi
    done
done
popd > /dev/null
############

echo "Compiling & linking $APP_NAME..."
if
    sudo gcc $COMPILER_OPTIONS_SHARED $COMPILER_OPTIONS_EXTRA $TOK_ONE_LINUX_SOURCE $TOK_ONE_SHARED_SOURCE -o build/$PLATFORM/$APP_NAME/$APP_NAME $LINKER_OPTIONS 
    then
    echo "succesful compilation, running..."
        if [[ $1 = "DEBUG" ]]; then
            sudo gdb build/$PLATFORM/$APP_NAME/$APP_NAME
        else
            sudo build/$PLATFORM/$APP_NAME/$APP_NAME
        fi
    else
    echo "build failed"
fi

