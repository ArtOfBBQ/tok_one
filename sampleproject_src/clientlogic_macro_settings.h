#ifndef CLIENTLOGIC_MACRO_SETTINGS
#define CLIENTLOGIC_MACRO_SETTINGS

/*
This header should only contain macro definitions that modify the behavior
of the engine. For now it's just to set the memory requirements of your app,
I'll add some flags to ignore rotations etc
*/

/*
The maximum number of sprites/meshes in your app.
*/
#define ZPOLYGONS_TO_RENDER_ARRAYSIZE 500

/*
the max # of triangles in 1 zpolygon. 2 is enough for simple 2D sprites,
but that will block you from being able to make text labels which have
2 triangles per letter.
*/
#define POLYGON_TRIANGLES_SIZE 1000


// 290mb ->                   290...000
#define UNMANAGED_MEMORY_SIZE 290000000
// 90 mb ->                    90...000
#define MANAGED_MEMORY_SIZE    90000000

#define INITIAL_WINDOW_HEIGHT   800
#define INITIAL_WINDOW_WIDTH   1200
#define INITIAL_WINDOW_LEFT     300
#define INITIAL_WINDOW_BOTTOM   100


#endif // CLIENTLOGIC_MACRO_SETTINGS
