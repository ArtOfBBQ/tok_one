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
#include <stdint.h>

extern uint8_t T1_log_app_running;


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
    uint8_t * success,
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
