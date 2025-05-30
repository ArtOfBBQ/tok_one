#ifndef CLIENTLOGIC_MACRO_SETTINGS
#define CLIENTLOGIC_MACRO_SETTINGS

/*
This header should only contain macro definitions that modify the behavior
of the engine for your specific app.
*/

#define APPLICATION_NAME "TOK ONE"

// The 2 pools of memory your app allocates on startup
// 55 mb ->                      650...000
#define UNMANAGED_MEMORY_SIZE    650000000


// these will be ignored on platforms where window size/position
// can't be selected
#define INITIAL_WINDOW_HEIGHT  667.0f
#define INITIAL_WINDOW_WIDTH   875.0f // 800.0f// 375.0f
#define INITIAL_WINDOW_LEFT   1650.0f
#define INITIAL_WINDOW_BOTTOM  430.0f


#define SCHEDULED_ANIMS_ACTIVE 1
#define TERMINAL_ACTIVE 1
#define RAW_SHADER_ACTIVE 1 // for points and lines
#define TEXTURES_ACTIVE 1
#define BLOOM_ACTIVE 1
#define PARTICLES_ACTIVE 1
#define SHADOWS_ACTIVE 1
#define AUDIO_ACTIVE 1
#define ENGINE_SAVEFILE_ACTIVE 0
#define PROFILER_ACTIVE 0

#define MAX_RENDERING_FRAME_BUFFERS 3 // 3 for triple-buffering

#define MAX_VERTICES_PER_BUFFER 240000
#define MAX_LIGHTS_PER_BUFFER 3

/*
The maximum number of sprites in your app.
*/
#define MAX_POLYGONS_PER_BUFFER  500
#define MAX_LINE_VERTICES 100
#define MAX_POINT_VERTICES 300

/*
The maximum number of 'scheduled animations' simultaneously running
*/
#define SCHEDULED_ANIMATIONS_ARRAYSIZE 10


#define ALL_MESHES_SIZE 20
// the max # of triangles in all of your meshes/models combined
#define ALL_LOCKED_VERTICES_SIZE 240000

// The max number of materials in all of your meshes/models combined
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
