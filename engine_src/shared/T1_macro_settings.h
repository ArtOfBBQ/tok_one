#ifndef T1_MACRO_SETTINGS_H
#define T1_MACRO_SETTINGS_H

/*
This header should only contain macro definitions
that modify the behavior of the engine.
*/
#define T1_APP_NAME "Lore Seeker"

#define T1_RENDER_VIEW_CAP 5

#define T1_ACTIVE 1
#define T1_INACTIVE 2

#define DECODED_IMAGE_SILENCE
#define T1_6502_PRINTF_ON 2

#define LS_GUI_ACTIVE 1
#define T1_RAND_ASSERTS_ACTIVE 1
#define T1_TOKEN_ASSERTS_ACTIVE 1
#define T1_LOG_ASSERTS_ACTIVE 1
#define T1_LOG_PRINTF 1
#define T1_OBJPARSER_ASSERTS_ACTIVE 1
#define T1_WAV_PRINTF 1
#define T1_MEM_ASSERTS_ACTIVE 1
#define T1_STD_ASSERTS_ACTIVE 1

#define T1_GAMEPAD_ACTIVE 1
#define T1_Z_PREPASS_ACTIVE 2
#define T1_BLENDING_SHADER_ACTIVE 1
#define T1_SHADOWS_ACTIVE 1
#define T1_OCCLUSION_ACTIVE 2
#define T1_ANIM_ACTIVE 1
#define T1_TERM_ACTIVE 1
#define T1_TEXTURES_ACTIVE 1
#define T1_MIPMAPS_ACTIVE 2
#define T1_AMBIENT_LIGHTING_ACTIVE 1
#define T1_DIFFUSE_LIGHTING_ACTIVE 1
#define T1_SPECULAR_LIGHTING_ACTIVE 1
#define T1_NORMAL_MAPPING_ACTIVE 2
#define T1_REFLECTION_ACTIVE 1
#define T1_BLOOM_ACTIVE 1
#define T1_PARTICLES_ACTIVE 2
#define T1_OUTLINES_ACTIVE 1
#define T1_AUDIO_ACTIVE 2
#define T1_ENGINE_SAVEFILE_ACTIVE 1
#define T1_PROFILER_ACTIVE 2
#define T1_TONE_MAPPING_ACTIVE 1
#define T1_COLOR_QUANTIZATION_ACTIVE 2
#define T1_FOG_ACTIVE 2

#ifdef NS_BLOCK_ASSERTIONS
#ifndef NDEBUG
#define NDEBUG
#endif
#endif

/*
Your application's pre-allocated memory pool.
*/
// 425mb ->                   425...000
#if defined(NDEBUG) || defined(NS_BLOCK_ASSERTIONS)

#if T1_PROFILER_ACTIVE == T1_ACTIVE
#define T1_UNMANAGED_MEM_CAP 333000000
#elif T1_PROFILER_ACTIVE == T1_INACTIVE
#define T1_UNMANAGED_MEM_CAP 333000000 // production
#else
#error
#endif

#else

#if T1_PROFILER_ACTIVE == T1_ACTIVE
#define T1_UNMANAGED_MEM_CAP 333000000
#elif T1_PROFILER_ACTIVE == T1_INACTIVE
#define T1_UNMANAGED_MEM_CAP 333000000
#else
#error
#endif
#endif

// set to 3 for triple-buffering
#define T1_FRAMES_CAP 3

#define T1_MAX_VERTS_PER_BUFFER 1500000
#define T1_MAX_FLATQUADS_PER_BUFFER 75000
#define T1_MAX_TEXQUADS_PER_BUFFER 7500

#define T1_ZLIGHTS_CAP 15

#define T1_ZFAR 12.0f
#define T1_SHADOW_BIAS 0.0008f
#define T1_SHADOW_MULT 0.6f

#define T1_TEXARRAYS_CAP 29
#define T1_TEX_SLICES_CAP 250

/*
The maximum number of sprites in your app.
*/
#define T1_ZSPRITES_CAP 6500

/*
The max number of materials in all of your meshes/models combined
*/
#define T1_MATERIAL_NAME_CAP 256
#define T1_ALL_LOCKED_MATERIALS_SIZE 500

/*
The maximum number of 'scheduled animations' simultaneously running
*/
#define T1_ANIMS_CAP 9000

/*
The maximum number of 'points' in 1 frame of your app
(can be 0)
*/
// #define MAX_POINT_VERTICES 3000

/*
The maximum number of 'line' vertices in 1 frame of your app
Each line has 2 vertices
(can be 0)
*/
// #define MAX_LINE_VERTICES 5000

// the max # of 3d models in your app
#define T1_MESH_CAP 120
// the max # of triangles in all of your 3d models.
#define T1_LOCKED_VERTEX_CAP 330000

// the max # of perma stored sounds in your app
#define T1_ALL_PERMASOUNDS_SIZE 75
// 60million samples = 120MB     60...000
#define T1_ALL_AUDIOSAMPLES_SIZE 45000000

/*
The max # of particle effects in your application. If you set this to 0 the
code for particle effects will be stripped from your app. (Note that this can
be confusing - if you refer to the functions in particle.h a bunch of times in
your code and then set this to 0, you will suddenly have a bunch of broken
references to functions that don't exist, and you might not know to look here)
*/
#define T1_PARTICLE_EFFECTS_SIZE  30
#define T1_SHATTER_EFFECTS_SIZE   20

// these will be ignored on platforms where window size/position
// can't be selected
#define T1_INITIAL_WINDOW_HEIGHT 1000.0f
#define T1_INITIAL_WINDOW_WIDTH  1400.0f // 800.0f// 375.0f
#define T1_INITIAL_WINDOW_LEFT    200.0f
#define T1_INITIAL_WINDOW_BOTTOM  400.0f

#endif // T1_MACRO_SETTINGS_H
