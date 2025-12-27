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

#define T1_SHARED_APPLE_PLATFORM 1

#ifdef NS_BLOCK_ASSERTIONS
#define NDEBUG
#endif

#ifdef NDEBUG
#define INFLATE_SILENCE
#define INFLATE_IGNORE_ASSERTS
#define DECODE_PNG_SILENCE
#define DECODE_PNG_IGNORE_CRC_CHECKS
#define DECODE_PNG_IGNORE_ASSERTS
#define DECODE_BMP_SILENCE
#define DECODE_BMP_IGNORE_ASSERTS
#define COMMON_SILENCE
#define LOGGER_SILENCE
#else
#endif

// 1. Files that don't know about the platform layer
#include "inflate.c"
#include "decode_png.c"
#include "decode_bmp.c"
#include "T1_linalg3d.c"
#include "T1_decodedimage.c"
#include "T1_wav.c"
#include "T1_tokenizer.c"
#include "T1_objparser.c"
#include "T1_mtlparser.c"
#include "T1_std.c"
#include "T1_meta.c"
#include "T1_logger.c"
#include "T1_collision.c"
#include "T1_zspriteid.c"
#include "T1_audio.c"
#include "T1_global.c"
#include "T1_triangle.c"
#include "T1_material.c"
#include "T1_render_view.c"
#include "T1_lightsource.c"
#include "T1_cpu_to_gpu_types.c"
// 2. Files that are part of the basic platform layer
#include "T1_apple_audio.m"
#include "T1_platform_layer_common.c"
#include "T1_linux_apple_platform_layer.c"
#include "T1_macos_platform_layer.m"
#include "T1_apple_platform_layer.m"
// 3. Files that are dependent on the platform layer
#include "T1_mem.c"
#include "T1_mesh.c"
#include "T1_tex.c"
#include "T1_objmodel.c"
#include "T1_io.c"
#include "T1_random.c"
// 4. Platform layer part 2: metal
#include "T1_gpu.m"
// 5. Files that are dependent on the GPU platform layer
#include "T1_texture_array.c"
#include "T1_texture_files.c"
#include "T1_lines.c"
#include "T1_zsprite.c"
#include "T1_frame_anim.c"
#include "T1_particle.c"
#include "T1_easing.c"
#include "T1_zsprite_anim.c"
#include "T1_text.c"
#include "T1_uielement.c"
#include "T1_profiler.c"
#if 0
#include "clientlogic.c" // requires text and uielement
#else
#include "clientlogic_particledesigner.c" // requires text and uielement
#endif
#include "T1_terminal.c" // requires clientlogic
#include "T1_renderer.c" // requires zpoly & part
#include "T1_gameloop.c" // requires renderer
#include "T1_appinit.c" // requires gameloop
#include "T1_macos_main.m"
