#ifndef CLIENTLOGIC_MACRO_SETTINGS
#define CLIENTLOGIC_MACRO_SETTINGS

/*
This header should only contain macro definitions that modify the behavior
of the engine for your specific app.
*/

// The 2 pools of memory your app allocates on startup
// 350mb ->                    350...000
#define UNMANAGED_MEMORY_SIZE  350000000
// 50 mb ->                    50...000
#define MANAGED_MEMORY_SIZE    50000000

#define APPLICATION_NAME "TOK ONE"

// These startup window settings have no effect on tablets/smartphones
#define INITIAL_WINDOW_HEIGHT    800
#define INITIAL_WINDOW_WIDTH    1000
#define INITIAL_WINDOW_LEFT        0
#define INITIAL_WINDOW_BOTTOM      0

// TODO: implement simple macro replacements in glsl shaders so this can just
// TODO: be changed without having to update shaders
// The maximum number of polygons/lights to send to the GPU each frame
// The maximum number of triangle vertices to be stored on gpu permanently
#define MAX_POLYGONS_PER_BUFFER    800
#define MAX_VERTICES_PER_BUFFER 700000
#define MAX_LIGHTS_PER_BUFFER       75

// The maximum number of 'scheduled animations' simultaneously running
#define SCHEDULED_ANIMATIONS_ARRAYSIZE 3000

// the max # of (probably from .obj) meshes/models in your app
#define ALL_MESHES_SIZE 100
// the max # of triangles in all of your meshes/models combined
#define ALL_MESH_VERTICES_SIZE 90000
// The max number of 'materials' in all of your app/game (must be 1+)
#define MAX_MATERIALS_SIZE 500

/*
The max # of simultaneously active particle effects in your app
*/
#define PARTICLE_EFFECTS_SIZE 10

/*
The max # of simultaneously 'shatter' style particle effects in your app
*/
#define SHATTER_EFFECTS_SIZE 120

#endif // CLIENTLOGIC_MACRO_SETTINGS
