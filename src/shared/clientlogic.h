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

// will be called once per frame, before rendering that frame
// add your app's code here
void client_logic_update(uint64_t microseconds_elapsed);

#endif

