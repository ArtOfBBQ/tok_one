#ifndef CLIENTLOGIC_MACRO_SETTINGS
#define CLIENTLOGIC_MACRO_SETTINGS

/*
This header should only contain macro definitions that modify the behavior
of the engine for your specific app.
*/

#define APPLICATION_NAME "TOK ONE"

// The 2 pools of memory your app allocates on startup
// 150 mb ->                    125...000
#define UNMANAGED_MEMORY_SIZE   125000000
// 50 mb ->                      20...000
#define MANAGED_MEMORY_SIZE      20000000


// these will be ignored on platforms where window size/position
// can't be selected
#define INITIAL_WINDOW_HEIGHT  667.0f
#define INITIAL_WINDOW_WIDTH   875.0f // 800.0f// 375.0f
#define INITIAL_WINDOW_LEFT   1650.0f
#define INITIAL_WINDOW_BOTTOM  430.0f


#define TEXTURES_ACTIVE 1
#define BLOOM_ACTIVE 0
#define SHADOWS_ACTIVE 0
#define AUDIO_ACTIVE 0
#define ENGINE_SAVEFILE_ACTIVE 0

#define MAX_RENDERING_FRAME_BUFFERS 2 // 3 for triple-buffering

#define MAX_VERTICES_PER_BUFFER  1500000
#define MAX_LIGHTS_PER_BUFFER 100

/*
The maximum number of sprites in your app.
*/
#define MAX_POLYGONS_PER_BUFFER  6500
#define MAX_MATERIALS_PER_POLYGON 15
#define MAX_LINE_VERTICES 10
#define MAX_POINT_VERTICES 30

/*
The maximum number of 'scheduled animations' simultaneously running
*/
#define SCHEDULED_ANIMATIONS_ARRAYSIZE 100


#define ALL_MESHES_SIZE 100
// the max # of triangles in all of your meshes/models combined
#define ALL_LOCKED_VERTICES_SIZE 150000
// The max number of 'materials ids' in 1 of your zpolygons
// #define MAX_MATERIALS_SIZE 10

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

