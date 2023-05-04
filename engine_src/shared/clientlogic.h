/*
The functions declared here need to be implemented by your game or
application.
*/

#ifndef CLIENTLOGIC_H
#define CLIENTLOGIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tok_random.h"
#include "texture_array.h"
#include "zpolygon.h"
#include "lightsource.h"
#include "particle.h"
#include "cpu_gpu_shared_types.h"
#include "decodedimage.h"
#include "platform_layer.h"
#include "debigulator/src/decode_png.h"
#include "debigulator/src/decode_bmp.h"
#include "userinput.h"
#include "text.h"
#include "uielement.h"
#include "scheduled_animations.h"
#include "logger.h"
#include "terminal.h"

#include "clientlogic_additional_defs.h"

extern bool32_t application_running;

/*
will be called once when the app shuts down
*/
void client_logic_shutdown(void);

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
Will be called when the debugging terminal receives a command, so you can make
your app respond to the commands of your choice

You're expected to send a response by filling in 'response', and you must not
exceed the character limit ('response_cap')
*/
void client_logic_evaluate_terminal_command(
    char * command,
    char * response,
    const uint32_t response_cap);

/*
this will be called whenever your app's window resizes - you can react by
updating your layout to match the new window
*/
void client_logic_window_resize(
    const uint32_t new_height,
    const uint32_t new_width);

#ifdef __cplusplus
}
#endif

#endif
