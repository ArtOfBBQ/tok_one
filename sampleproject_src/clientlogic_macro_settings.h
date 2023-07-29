#ifndef CLIENTLOGIC_MACRO_SETTINGS
#define CLIENTLOGIC_MACRO_SETTINGS

/*
This header should only contain macro definitions that modify the behavior
of the engine for your specific app.
*/

// The 2 pools of memory your app allocates on startup
// 290mb ->                   290...000
#define UNMANAGED_MEMORY_SIZE 290000000
// 90 mb ->                    90...000
#define MANAGED_MEMORY_SIZE    90000000

#define APPLICATION_NAME "TOK ONE"

// These startup window settings have no effect on tablets/smartphones
#define INITIAL_WINDOW_HEIGHT   800
#define INITIAL_WINDOW_WIDTH   1200
#define INITIAL_WINDOW_LEFT     300
#define INITIAL_WINDOW_BOTTOM   100

// The maximum number of 3D or 2D sprites in your app.
#define ZPOLYGONS_TO_RENDER_ARRAYSIZE 500

// The maximum number of 'scheduled animations' simultaneously running
#define SCHEDULED_ANIMATIONS_ARRAYSIZE 3000

// the max # of (probably from .obj) meshes/models in your app
#define ALL_MESHES_SIZE 100
// the max # of triangles in all of your meshes/models combined
#define ALL_MESH_TRIANGLES_SIZE 50000
// The max number of 'materials' in 1 of your meshes_models (must be 1+)
#define MAX_MATERIALS_SIZE 5

/*
The max # of simultaneously active particle effects in your app
*/
#define PARTICLE_EFFECTS_SIZE 10

/*
The max # of simultaneously 'shatter' style particle effects in your app
*/
#define SHATTER_EFFECTS_SIZE 120

#endif // CLIENTLOGIC_MACRO_SETTINGS
