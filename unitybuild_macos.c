/*
This file is a 'unity build', a build where all files are unified
into 1 translation unit to make the compiler's job easier and
faster. It has nothing to do with the Unity game engine.

When compiling, use this file as the only input. All other files
are included here, so passing anything else will cause duplicate
symbols.

Example:
MAC_FRAMEWORKS="
    -framework AppKit 
    -framework MetalKit 
    -framework Metal
    -framework AudioToolbox"
gcc -x objective-c -std="c99" -objC -O0 $MAC_FRAMEWORKS unitybuild.c -o build/unitybuild;
*/

#define INFLATE_SILENCE
#define INFLATE_IGNORE_ASSERTS
#define DECODE_PNG_SILENCE
#define DECODE_PNG_IGNORE_CRC_CHECKS
#define DECODE_PNG_IGNORE_ASSERTS
#define DECODE_BMP_SILENCE
#define DECODE_BMP_IGNORE_ASSERTS
#define COMMON_IGNORE_ASSERTS
#define COMMON_SILENCE
#define LOGGER_SILENCE
#define LOGGER_IGNORE_ASSERTS

// 1. Files that don't know about the platform layer
#include "inflate.c"
#include "decode_png.c"
#include "decode_bmp.c"
#include "decodedimage.c"
#include "wav.c"
#include "objparser.c"
#include "common.c"
#include "logger.c"
#include "objectid.c"
#include "audio.c"
#include "window_size.c"
#include "triangle.c"
#include "lightsource.c"
// 2. Files that are part of the basic platform layer
#include "apple_audio.m"
#include "common_platform_layer.c"
#include "macos_platform_layer.m"
#include "apple_platform_layer.m"
#include "linux_apple_platform_layer.c"
// 3. Files that are dependent on the platform layer
#include "memorystore.c"
#include "objmodel.c"
#include "userinput.c"
#include "tok_random.c"
// 4. Platform layer part 2: metal
#include "gpu.m"
// 5. Files that are dependent on the GPU platform layer
#include "texture_array.c"
#include "zpolygon.c"
#include "particle.c"
#include "scheduled_animations.c" // requires zpolygon & parti
#include "text.c"
#include "uielement.c"
#include "clientlogic.c" // requires text and uielement
#include "terminal.c" // requires clientlogic
#include "renderer.c" // requires zpoly & part
#include "gameloop.c" // requires renderer
#include "init_application.c" // requires gameloop
#include "macos_main.m"

