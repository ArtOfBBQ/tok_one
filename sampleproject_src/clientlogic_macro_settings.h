#ifndef CLIENTLOGIC_MACRO_SETTINGS
#define CLIENTLOGIC_MACRO_SETTINGS

/*
This header should only contain macro definitions that modify the behavior
of the engine for your specific app.
*/
#ifndef APPLICATION_NAME
#define APPLICATION_NAME "PARTICLE EDITOR"
#endif

// The 2 pools of memory your app allocates on startup
// 55 mb ->                      650...000
#define UNMANAGED_MEMORY_SIZE    650000000


// these will be ignored on platforms where window size/position
// can't be selected
#define INITIAL_WINDOW_HEIGHT  800.0f
#define INITIAL_WINDOW_WIDTH  1200.0f // 800.0f// 375.0f
#define INITIAL_WINDOW_LEFT   1650.0f
#define INITIAL_WINDOW_BOTTOM  430.0f

#define T1_ACTIVE 1
#define T1_INACTIVE 2

#define T1_STD_ASSERTS_ACTIVE T1_ACTIVE
#define T1_LOGGER_ASSERTS_ACTIVE T1_ACTIVE
#define T1_MEM_ASSERTS_ACTIVE T1_ACTIVE

#define T1_AMBIENT_LIGHTING_ACTIVE 1
#define T1_DIFFUSE_LIGHTING_ACTIVE 1
#define T1_SPECULAR_LIGHTING_ACTIVE 1
#define T1_SCHEDULED_ANIMS_ACTIVE 1
#define T1_TERMINAL_ACTIVE 1
#define T1_TEXTURES_ACTIVE 1
#define T1_BLOOM_ACTIVE 2
#define T1_PARTICLES_ACTIVE 1
#define T1_SHADOWS_ACTIVE 2
#define T1_NORMAL_MAPPING_ACTIVE 2
#define T1_AUDIO_ACTIVE 1
#define T1_ENGINE_SAVEFILE_ACTIVE 1
#define T1_PROFILER_ACTIVE 2
#define T1_TONE_MAPPING_ACTIVE 1
#define T1_COLOR_QUANTIZATION_ACTIVE 1
#define T1_MIPMAPS_ACTIVE 1
#define T1_FOG_ACTIVE 1

#define MAX_RENDERING_FRAME_BUFFERS 3 // 3 for triple-buffering

#define MAX_VERTICES_PER_BUFFER 240000
#define MAX_LIGHTS_PER_BUFFER 3

#define SHADOW_BIAS 0.0001f

/*
The maximum number of sprites in your app.
*/
#define MAX_ZSPRITES_PER_BUFFER  100000

/*
The maximum number of circles (particles)
*/
#define MAX_CIRCLES_PER_BUFFER 300000

/*
The maximum number of 'scheduled animations' simultaneously running
*/
#define SCHEDULED_ANIMATIONS_ARRAYSIZE 180


#define ALL_MESHES_SIZE 20
// the max # of triangles in all of your meshes/models combined
#define ALL_LOCKED_VERTICES_SIZE 240000

// The max number of materials in all of your meshes/models combined
#define MATERIAL_NAME_CAP 256
#define ALL_LOCKED_MATERIALS_SIZE 10000

// the max # of permanently stored sounds in your app
#define ALL_PERMASOUNDS_SIZE 100

/*
The max # of simultaneously active particle effects in your app
*/
#define PARTICLE_EFFECTS_SIZE 10

/*
The max # of simultaneously 'shatter' style particle effects in your app
*/
#define SHATTER_EFFECTS_SIZE 120

#endif // CLIENTLOGIC_MACRO_SETTINGS
