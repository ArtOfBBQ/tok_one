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
#define ZPOLYGONS_TO_RENDER_ARRAYSIZE 50

/*
the max # of triangles in 1 zpolygon. Use 2 for a simple 2D app or game, since
a 2D sprite is represented with only 2 triangles.
*/
#define POLYGON_TRIANGLES_SIZE 8400

#endif // CLIENTLOGIC_MACRO_SETTINGS

