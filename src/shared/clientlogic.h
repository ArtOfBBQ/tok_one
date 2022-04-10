#ifndef CLIENTLOGIC_H
#define CLIENTLOGIC_H

#include "common.h"
#include "texquad_type.h"
#include "texture_array.h"
#include "zpolygon.h"
#include "vertex_types.h"
#include "decodedimage.h"
#include "platform_layer.h"
#include "decode_png.h"
#include "userinput.h"
#include "text.h"

/*
Prepare your objects for 3D rendering

Write code to update your game or app's state
*/

zPolygon * load_from_obj_file(char * filename);

// A buffer of zPolygon objects that should be rendered
// in your application
// index 0 to zpolygons_to_render_size will be rendered,
// the rest of the array will be ignored
#define ZPOLYGONS_TO_RENDER_ARRAYSIZE 2
extern zPolygon * zpolygons_to_render[ZPOLYGONS_TO_RENDER_ARRAYSIZE];
extern uint32_t zpolygons_to_render_size;

// A buffer of zLightSources to light up your scene(s)
// index 0 to zlights_to_apply_size will be rendered,
// the rest of the array will be ignored
#define ZLIGHTS_TO_APPLY_ARRAYSIZE 50
extern zLightSource zlights_to_apply[ZLIGHTS_TO_APPLY_ARRAYSIZE];
extern uint32_t zlights_to_apply_size;

// A buffer of texture arrays (AKA texture atlases) your
// objects can use
// Each texture atlas must have images of the exact same size
// You can set a zTriangle's texturearray_i to 2 to use
// texture_arrays[2] as its texture during texture mapping
// Set the zTriangle's texture_i to select which texture inside
// the texture atlas to use
// REMINDER: You must define TEXTUREARRAYS_SIZE in vertex_types.h
extern TextureArray texture_arrays[TEXTUREARRAYS_SIZE];

// will be called once at startup, before rendering frame 1
// add your app's code here
void client_logic_startup(void);

// will be called once per frame, before rendering that frame
// add your app's code here
void client_logic_update(uint64_t microseconds_elapsed);

#endif

