#include "T1_gameloop.h"

#include "T1_assert.h"
#include "T1_global.h"
#include "T1_io.h"
#include "T1_os.h"

u8 T1_gameloop_active = 0;

/*
When nothing was presented (e.g. the window is hidden), nothing paced the
client's loop to the display, so T1_frame_update() handles input until the
next multiple of this many microseconds instead of letting the loop spin
*/
#define T1_GAMELOOP_FRAME_STEP_US (1000000 / 60)
// T1_frame_resized() once the window's size stayed the same this long
#define T1_GAMELOOP_RESIZE_SETTLE_US 200000

typedef struct {
    u64 previous_time; // 0 for none, the next elapsed is then 0
    u64 next_poll_us; // see T1_GAMELOOP_FRAME_STEP_US
    u32 window_tex; // the T1Tex the window shows
    b8 frame_begun; // the client's frame, between two T1_frame_update()s
    b8 frame_resized; // the window settled on a new size
    b8 window_opened; // the first frame reports a resize right away
    b8 presented_once; // before that, the OS may report the window hidden
    b8 run_hidden; // see T1_window_set_pause_when_hidden()
} T1GameloopState;
static T1GameloopState * T1_gl_s = NULL;

void T1_gameloop_init(
    void *(* arg_malloc_fptr)(size_t))
{
    T1_gl_s = arg_malloc_fptr(sizeof(T1GameloopState));
    T1_std_memset(T1_gl_s, 0, sizeof(T1GameloopState));
    T1_gl_s->window_tex = T1_TEX_NONE;
}

void T1_window_open(T1Tex tex, char ** const fatal_error) {
    if (*fatal_error != 0) { return; }
    
    if (!T1_os_gpu_active()) {
        *fatal_error = "T1_window_open() before T1_gpu_init()";
        return;
    }
    if (tex == T1_TEX_NONE) {
        *fatal_error = "T1_window_open() needs a T1Tex to show";
        return;
    }
    
    T1_os_window_open(fatal_error);
    if (*fatal_error != 0) { return; }
    
    T1_gl_s->window_tex = tex;
    
    // The client's first frame reports a resize, so it can build its scene
    // for the window's size there
    T1_gl_s->window_opened = 1;
}

void T1_window_set_pause_when_hidden(u8 pause) {
    T1_gl_s->run_hidden = !pause;
}

void T1_window_set_tex(T1Tex tex) {
    T1_assert(tex != T1_TEX_NONE);
    if (tex == T1_TEX_NONE) { return; }
    T1_gl_s->window_tex = tex;
}

void T1_window_toggle_fullscreen(void) {
    T1_os_toggle_fullscreen();
}

u32 T1_window_get_width(void) {
    return (u32)T1_global->window_wh[0];
}

u32 T1_window_get_height(void) {
    return (u32)T1_global->window_wh[1];
}

u32 T1_window_get_pixel_width(void) {
    return (u32)(
        T1_global->window_wh[0] * T1_os_get_screen_backing_scale_factor());
}

u32 T1_window_get_pixel_height(void) {
    return (u32)(
        T1_global->window_wh[1] * T1_os_get_screen_backing_scale_factor());
}

u8 T1_frame_resized(void) {
    return T1_gl_s->frame_resized;
}

u64 T1_frame_elapsed_us(void) {
    return T1_global->elapsed;
}

/*
After the client's frame: clears its one-shot input events (it has seen
them by now), polls gamepads and handles fullscreen requests
*/
static void T1_gameloop_after_frame(void) {
    T1_io_update_and_clear_for_next_frame();
    
    #if T1_GAMEPAD_ACTIVE == T1_ACTIVE
    T1_os_poll_gamepad_events();
    #elif T1_GAMEPAD_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    if (T1_global->upcoming_fullscreen_request) {
        T1_global->upcoming_fullscreen_request = 0;
        T1_os_enter_fullscreen();
    }
}

/*
Presenting blocks at the display's refresh rate. Without it, handle input
until the rest of the frame step has passed, so the client's loop doesn't
spin.
*/
static void T1_gameloop_pump_events(const b8 presented) {
    if (presented) {
        T1_os_window_pump_events(0);
    } else {
        while (1) {
            u64 now = T1_os_get_current_time_us();
            u64 wait_us = now < T1_gl_s->next_poll_us ?
                T1_gl_s->next_poll_us - now : 0;
            T1_os_window_pump_events(wait_us);
            if (wait_us == 0 || T1_os_window_close_requested()) {
                break;
            }
        }
    }
    
    // Deadlines a fixed step apart, so oversleeping one doesn't slow down
    // the next. If we fell behind (a slow frame), restart from now.
    u64 now = T1_os_get_current_time_us();
    T1_gl_s->next_poll_us += T1_GAMELOOP_FRAME_STEP_US;
    if (T1_gl_s->next_poll_us < now) {
        T1_gl_s->next_poll_us = now + T1_GAMELOOP_FRAME_STEP_US;
    }
}

u8 T1_frame_update(void) {
    T1_assert(T1_gameloop_active);
    T1_assert(T1_gl_s->window_tex != T1_TEX_NONE);
    if (!T1_gameloop_active || T1_gl_s->window_tex == T1_TEX_NONE) {
        return 0;
    }
    
    // Finishes the client's frame: renders its passes and shows the
    // window's T1Tex
    b8 presented = 0;
    if (T1_gl_s->frame_begun) {
        T1_gl_s->frame_begun = 0;
        T1_os_gpu_render_frame();
        if (!T1_gl_s->presented_once || T1_os_window_visible()) {
            presented = T1_os_window_present(T1_gl_s->window_tex);
            T1_gl_s->presented_once |= presented;
        }
        T1_gameloop_after_frame();
    }
    
    T1_gameloop_pump_events(presented);
    
    /*
    A hidden window (minimized or fully covered) pauses the client, and the
    time it was hidden doesn't count as elapsed. Until the window showed a
    frame, macOS may report it hidden although it's on screen.
    */
    while (
        !T1_gl_s->run_hidden &&
        T1_gl_s->presented_once &&
        !T1_os_window_close_requested() &&
        !T1_os_window_visible())
    {
        T1_os_window_pump_events(T1_GAMELOOP_FRAME_STEP_US * 6);
            T1_gl_s->previous_time = 0;
    }
    
    if (T1_os_window_close_requested()) { return 0; }
    
    // Waits until the GPU is done with the frame T1_FRAMES_CAP ago, so the
    // client can write the next one's buffers
    T1_os_gpu_begin_frame();
    
    u64 now = T1_os_get_current_time_us();
    T1_global->elapsed = T1_gl_s->previous_time == 0 ? 0 :
        (u64)((f64)(now - T1_gl_s->previous_time) *
            (f64)T1_global->timedelta_mult);
    T1_gl_s->previous_time = now;
    T1_global->this_frame_timestamp_us = now;
    T1_global->frame_consts.timestamp = (u32)now;
    
    T1_gl_s->frame_resized = 0;
    if (T1_gl_s->window_opened) {
        T1_gl_s->window_opened = 0;
        T1_global->last_resize_request_us = 0;
        T1_gl_s->frame_resized = 1;
    } else if (
        T1_global->last_resize_request_us != 0 &&
        now - T1_global->last_resize_request_us >=
            T1_GAMELOOP_RESIZE_SETTLE_US)
    {
        // Until now the window showed its T1Tex scaled to the new size
        T1_global_init();
        T1_global->last_resize_request_us = 0;
        T1_gl_s->frame_resized = 1;
    }
    
    T1_gl_s->frame_begun = 1;
    return 1;
}
