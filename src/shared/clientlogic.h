/*
The functions declared here need to be implemented by your game or
application.
*/

#ifndef CLIENTLOGIC_H
#define CLIENTLOGIC_H

#import <Foundation/Foundation.h>

#include "tok_random.h"
#include "texquad_type.h"
#include "texture_array.h"
#include "zpolygon.h"
#include "lightsource.h"
#include "vertex_types.h"
#include "decodedimage.h"
#include "platform_layer.h"
#include "debigulator/src/decode_png.h"
#include "userinput.h"
#include "text.h"
#include "scheduled_animations.h"
#include "logger.h"

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

extern bool32_t application_running;

/*
The engine will need a name for your application
e.g. To create the application data directory on Apple platforms
e.g. To display in the title bar
*/
char * client_logic_get_application_name();

/*
will be called once at startup, before rendering frame 1
*/
void client_logic_startup(void);

/*
will be called by the platform layer when you start a thread
(see platform_layer.h -> start_thread(int32_t threadmain_id);
*/
void client_logic_threadmain(int32_t threadmain_id);

/*
will be called when a ScheduledAmimation (see scheduled_animations.h) runs out
*/
void client_logic_animation_callback(int32_t callback_id);

/*
will be called once per frame, before rendering that frame
*/
void client_logic_update(uint64_t microseconds_elapsed);

/*
this will be called whenever your app's window resizes - you can react by
updating your layout to match the new window
*/
void client_logic_window_resize(
    const uint32_t new_height,
    const uint32_t new_width);

#endif

