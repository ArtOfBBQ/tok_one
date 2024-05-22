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
#include "windows_main.c"
#include "common_platform_layer.c"
#include "opengl.c"
#include "opengl_extensions.c"
