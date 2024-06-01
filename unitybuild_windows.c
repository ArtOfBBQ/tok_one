#define WINDOWS_PLATFORM
#define INFLATE_SILENCE
// #define INFLATE_IGNORE_ASSERTS
#define DECODE_PNG_SILENCE
#define DECODE_PNG_IGNORE_CRC_CHECKS
#define DECODE_PNG_IGNORE_ASSERTS
#define DECODE_BMP_SILENCE
#define DECODE_BMP_IGNORE_ASSERTS
#define COMMON_IGNORE_ASSERTS
#define COMMON_SILENCE
#define LOGGER_SILENCE

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
#include "cpu_to_gpu_types.c"
// #include "cpu_to_gpu_types.c"
// 2. Files that are part of the basic platform layer
#include "opengl_extensions.c"
#include "tok_opengl.c"
#include "common_platform_layer.c"
#include "windows_platform_layer.c"
// 3. Files that are dependent on the platform layer
#include "memorystore.c"
#include "objmodel.c"
#include "userinput.c"
#include "tok_random.c"
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
#include "windows_main.c"

