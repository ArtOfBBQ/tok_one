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
#include "scheduled_animations.h"

#ifdef LORESEEKER
#include "dirent.h"
#include "../../../src/animationtracker.hpp"
#include "../../../src/mainmenu.hpp"
#endif

/*
Prepare your objects for 3D rendering

Write code to update your game or app's state
*/

zPolygon load_from_obj_file(char * filename);

// A buffer of texture arrays (AKA texture atlases) your
// objects can use
// Each texture atlas must have images of the exact same size
// You can set a zTriangle's texturearray_i to 2 to use
// texture_arrays[2] as its texture during texture mapping
// Set the zTriangle's texture_i to select which texture inside
// the texture atlas to use
// REMINDER: You must define TEXTUREARRAYS_SIZE in vertex_types.h
extern TextureArray texture_arrays[TEXTUREARRAYS_SIZE];
extern uint32_t texture_arrays_size;

// will be called once at startup, before rendering frame 1
// add your app's code here
void client_logic_startup(void);

// will be called once per frame, before rendering that frame
// add your app's code here
void client_logic_update(uint64_t microseconds_elapsed);

// void client_logic_mouseup(
//     float screenspace_x,
//     float screenspace_y,
//     int32_t touchable_id);
#endif

