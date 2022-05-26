#ifndef CLIENTLOGIC_H
#define CLIENTLOGIC_H

// #import <Foundation/Foundation.h>

#include "texquad_type.h"
#include "texture_array.h"
#include "zpolygon.h"
#include "lightsource.h"
#include "vertex_types.h"
#include "debigulator/src/decodedimage.h"
#include "platform_layer.h"
#include "debigulator/src/decode_png.h"
#include "userinput.h"
#include "text.h"
#include "scheduled_animations.h"

#include "dirent.h"
#include "string.h"

#ifdef LORESEEKER
typedef struct NamedTextureArray {
    int32_t texturearray_i;
    int32_t texture_i;
} NamedTextureArray;

#include <string>
#include <unordered_map>
#include "../../../src/animationtracker.hpp"
#include "../../../src/frontendtouch.hpp"
#include "../../../src/frontendcommand.hpp"
#include "../../../src/mainmenu.hpp"
#endif

/*
Prepare your objects for 3D rendering

Write code to update your game or app's state
*/

zPolygon load_from_obj_file(char * filename);

// will be called once at startup, before rendering frame 1
// add your app's code here
void client_logic_startup(void);

// will be called by the platform layer when you start a thread
// (see platform_layer.h -> start_thread(int32_t threadmain_id);
void client_logic_threadmain(int32_t threadmain_id);

// will be called when a ScheduledAmimation
// (see scheduled_animations.h) runs out
void client_logic_animation_callback(int32_t callback_id);

// will be called once per frame, before rendering that frame
// add your app's code here
void client_logic_update(uint64_t microseconds_elapsed);

#endif

