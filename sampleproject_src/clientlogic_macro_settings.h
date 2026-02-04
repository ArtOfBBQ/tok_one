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

#define T1_RENDER_VIEW_CAP 1

#define T1_ACTIVE 1
#define T1_INACTIVE 2

#define T1_Z_PREPASS_ACTIVE 1
#define T1_BLENDING_SHADER_ACTIVE 1

#define T1_STD_ASSERTS_ACTIVE T1_ACTIVE
#define T1_LOGGER_ASSERTS_ACTIVE T1_ACTIVE
#define T1_MEM_ASSERTS_ACTIVE T1_ACTIVE

#define T1_OCCLUSION_ACTIVE 2
#define T1_AMBIENT_LIGHTING_ACTIVE 1
#define T1_DIFFUSE_LIGHTING_ACTIVE 1
#define T1_SPECULAR_LIGHTING_ACTIVE 1
#define T1_ZSPRITE_ANIM_ACTIVE 1
#define T1_TEXQUAD_ANIM_ACTIVE 1
#define T1_FRAME_ANIM_ACTIVE 1
#define T1_OUTLINES_ACTIVE 1
#define T1_TERMINAL_ACTIVE 1
#define T1_TEXTURES_ACTIVE 1
#define T1_BLOOM_ACTIVE 2
#define T1_PARTICLES_ACTIVE 1
#define T1_SHADOWS_ACTIVE 2
#define T1_NORMAL_MAPPING_ACTIVE 2
#define T1_AUDIO_ACTIVE 2
#define T1_ENGINE_SAVEFILE_ACTIVE 1
#define T1_PROFILER_ACTIVE 2
#define T1_TONE_MAPPING_ACTIVE 1
#define T1_COLOR_QUANTIZATION_ACTIVE 1
#define T1_MIPMAPS_ACTIVE 2
#define T1_FOG_ACTIVE 2

#define FRAMES_CAP 3 // 3 for triple-buffering

#define MAX_VERTICES_PER_BUFFER 840000
#define MAX_LIGHTS_PER_BUFFER 3

#define SHADOW_BIAS 0.0001f

/*
The maximum number of sprites in your app.
*/
#define MAX_ZSPRITES_PER_BUFFER  100000

/*
The maximum number of 'flat quads' (particles)
*/
#define MAX_FLATQUADS_PER_BUFFER 300000

/*
Flat textured quads, mostly for UI text
*/
#define MAX_TEXQUADS_PER_BUFFER 50000

/*
The maximum number of 'scheduled animations' simultaneously running
*/
#define T1_ZSPRITE_ANIMS_CAP 180
#define T1_TEXQUAD_ANIMS_CAP 500


#define T1_MESH_CAP 20
// the max # of triangles in all of your meshes/models combined
#define T1_LOCKED_VERTEX_CAP 240000

// The max number of materials in all of your meshes/models combined
#define MATERIAL_NAME_CAP 256
#define ALL_LOCKED_MATERIALS_SIZE 10000

// the max # of permanently stored sounds in your app
#define ALL_PERMASOUNDS_SIZE 100

/*
The max # of simultaneously active particle effects in your app
*/
#define T1_PARTICLE_EFFECTS_SIZE 10

/*
The max # of simultaneously 'shatter' style particle effects in your app
*/
#define T1_SHATTER_EFFECTS_SIZE 120

#endif // CLIENTLOGIC_MACRO_SETTINGS
