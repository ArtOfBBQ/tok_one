/*
The functions declared here need to be implemented by your game or
application.
*/

#ifndef CLIENTLOGIC_H
#define CLIENTLOGIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

#include "T1_meta.h"
#include "T1_rand.h"
#include "T1_mem.h"
#include "T1_collision.h"
#include "T1_tex_array.h"
#include "T1_tex_files.h"
#include "T1_lines.h"
#include "T1_texquad.h"
#include "T1_zsprite.h"
#include "T1_zlight.h"
#include "T1_particle.h"
#include "T1_cpu_gpu_shared_types.h"
#include "T1_img.h"
#include "T1_platform_layer.h"
#include "debigulator/src/decode_png.h"
#include "debigulator/src/decode_bmp.h"
#include "T1_io.h"
#include "T1_text.h"
#include "T1_ui_widget.h"
#include "T1_zsprite_anim.h"
#include "T1_texquad_anim.h"
#include "T1_logger.h"
#include "T1_term.h"

#include "clientlogic_additional_defs.h"

extern bool32_t T1_app_running;


void T1_clientlogic_init(void);

/*
will be called once when the app shuts down
*/
void T1_clientlogic_shutdown(void);


/*
will be called once at startup, before rendering frame 1

If you draw objects here they will be deleted by an automatic screen resize
*/
void T1_clientlogic_early_startup(
    bool32_t * success,
    char * error_message);

/*
will be called once at startup, before rendering frame 1

You can draw initial objects here and they will remain
*/
void T1_clientlogic_late_startup(void);

/*
will be called by the platform layer when you start a thread
(see platform_layer.h -> start_thread(int32_t threadmain_id);
*/
void T1_clientlogic_threadmain(int32_t threadmain_id);

/*
will be called once per frame, before rendering that frame
*/
void T1_clientlogic_update(
    uint64_t elapsed_us);

/*
will be called once per frame, after rendering that frame
*/
void T1_clientlogic_update_after_render_pass(void);

/*
Will be called when the debugging terminal receives a command, so you can make
your app respond to the commands of your choice

You're expected to send a response by filling in 'response', and you must not
exceed the character limit ('response_cap')
*/
void T1_clientlogic_evaluate_terminal_command(
    char * command,
    char * response,
    const uint32_t response_cap);

/*
this will be called whenever your app's window resizes - you can react by
updating your layout to match the new window
*/
void T1_clientlogic_window_resize(
    const uint32_t new_width,
    const uint32_t new_height);

#ifdef __cplusplus
}
#endif

#endif
