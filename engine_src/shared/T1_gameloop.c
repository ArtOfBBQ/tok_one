#include "T1_gameloop.h"

#include "T1_assert.h"
#include "T1_global.h"
#include "T1_io.h"
#include "T1_os.h"

u8 T1_gameloop_active = 0;

/*
When the last frame wasn't presented, nothing paced the client's loop to the
display, so T1_window_poll_events() handles input until the next multiple of
frame_step_us microseconds, instead of letting the loop spin. 0 lets it
spin, see T1_window_set_fps_cap().
*/
#define T1_GAMELOOP_DEFAULT_FPS_CAP 60

typedef struct {
    b8 frame_begun; // between T1_frame_begin() and T1_frame_render()
    u64 previous_time;
    u64 frame_no;
    u32 loading_text_T1_id;
    b8 frame_should_update; // the client should run its update
    b8 frame_resized; // the window settled on a new size
    b8 frame_rendered; // since the last T1_window_poll_events()
    b8 frame_presented; // since the last T1_window_poll_events()
    u64 next_poll_us; // see T1_GAMELOOP_DEFAULT_FPS_CAP
    u64 frame_step_us; // 0 for no cap
} T1GameloopState;
static T1GameloopState * T1_gl_s = NULL;

void T1_gameloop_init(
    void *(* arg_malloc_fptr)(size_t))
{
    T1_gl_s = arg_malloc_fptr(sizeof(T1GameloopState));
    T1_std_memset(T1_gl_s, 0, sizeof(T1GameloopState));
    T1_gl_s->loading_text_T1_id = T1_ID_NONE;
    T1_window_set_fps_cap(T1_GAMELOOP_DEFAULT_FPS_CAP);
}

void T1_window_set_fps_cap(u32 fps) {
    T1_gl_s->frame_step_us = fps > 0 ? 1000000 / fps : 0;
}

void T1_frame_begin(void)
{
    T1_gl_s->frame_should_update = 0;
    T1_gl_s->frame_resized = 0;
    T1_gl_s->frame_begun = 0;
    
    // Waits until the GPU is done with the frame T1_FRAMES_CAP ago
    if (!T1_os_gpu_begin_frame()) {
        return;
    }
    T1_gl_s->frame_begun = 1;
    
    T1_global->elapsed =
        T1_global->this_frame_timestamp_us -
            T1_gl_s->previous_time;
    
    T1_global->elapsed = (u64)(
        (f64)T1_global->elapsed *
        (f64)T1_global->timedelta_mult);
    
    T1_gl_s->previous_time =
        T1_global->this_frame_timestamp_us;
    
    if (!T1_gameloop_active) {
        return;
    }
    
    T1_global->this_frame_timestamp_us =
        T1_os_get_current_time_us();
    T1_global->frame_consts.timestamp =
        (u32)T1_global->this_frame_timestamp_us;
    
    if (T1_gl_s->previous_time < 1) {
        T1_gl_s->previous_time = T1_global->this_frame_timestamp_us;
        return;
    }
    
    T1_gl_s->frame_no++;
    
    if (
        T1_global->this_frame_timestamp_us -
            T1_global->last_resize_request_us <
                T1_GLOBAL_WINDOW_RESIZE_TIMEOUT)
    {
        if (
            T1_global->this_frame_timestamp_us -
            T1_global->last_resize_request_us < 200000)
        {
            // possibly a request we already handled, or not the final
            // request, wait...
            return;
        } else {
            T1_global_init();
            
            T1_global->last_resize_request_us = 0;
            
            // The client rebuilds its size dependent state now
            T1_gl_s->frame_resized = 1;
       }
    } else {
        // The client runs its update between T1_frame_begin() and
        // T1_frame_render()
        T1_gl_s->frame_should_update = 1;
    }
}

u8 T1_frame_should_update(void) {
    return T1_gl_s->frame_should_update;
}

u8 T1_frame_resized(void) {
    return T1_gl_s->frame_resized;
}

u64 T1_frame_elapsed_us(void) {
    return T1_global->elapsed;
}

void T1_frame_render(void) {
    T1_gl_s->frame_rendered = 0;
    
    if (!T1_gl_s->frame_begun) {
        return;
    }
    T1_gl_s->frame_begun = 0;
    
    T1_gl_s->frame_rendered = T1_os_gpu_render_frame();
}

void T1_window_present(void) {
    if (!T1_gl_s->frame_rendered) {
        return;
    }
    
    T1_gl_s->frame_presented = T1_os_window_present();
}

void T1_window_open(char ** const fatal_error) {
    if (*fatal_error != 0) { return; }
    
    // Nothing can be drawn without shaders, so fail loudly here
    if (T1_os_gpu_active() && !T1_os_gpu_shaders_ready()) {
        *fatal_error = "T1_window_open() before T1_gpu_try_shader_source() succeeded";
        return;
    }
    
    T1_os_window_open(fatal_error);
    if (*fatal_error != 0) { return; }
    
    // The client's first frame reports a resize, so it can build its scene
    // for the window's size there
    T1_os_layer_start_window_resize(T1_os_get_current_time_us());
}

void T1_window_blit_rgba(const u8 * rgba, u32 width, u32 height) {
    T1_os_window_blit_rgba(rgba, width, height);

    // A finished frame like a rendered one, so this frame's input events
    // are cleared, but it doesn't pace the loop like presenting does
    T1_gl_s->frame_rendered = 1;
}

void T1_window_toggle_fullscreen(void) {
    T1_os_toggle_fullscreen();
}

u8 T1_window_close_requested(void) {
    return T1_os_window_close_requested();
}

u8 T1_window_visible(void) {
    return T1_os_window_visible();
}

u32 T1_window_get_width(void) {
    return (u32)T1_global->window_wh[0];
}

u32 T1_window_get_height(void) {
    return (u32)T1_global->window_wh[1];
}

/*
What used to run after each rendered frame: clears the frame's one-shot
input events (the client has seen them by now), polls gamepads and handles
fullscreen requests.
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

void T1_window_poll_events(void) {
    // Like before, input events stay queued across frames that weren't
    // rendered (e.g. while a resize settles)
    if (T1_gl_s->frame_rendered) {
        T1_gameloop_after_frame();
        T1_gl_s->frame_rendered = 0;
    }
    
    // Presenting blocks at the display's refresh rate. Without it (nothing
    // rendered, or a software blit), handle input until the rest of the
    // frame step has passed, so the client's loop doesn't spin.
    if (T1_gl_s->frame_presented) {
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
    T1_gl_s->frame_presented = 0;
    
    // Deadlines a fixed step apart, so oversleeping one doesn't slow down
    // the next. If we fell behind (a slow frame), restart from now.
    u64 now = T1_os_get_current_time_us();
    T1_gl_s->next_poll_us += T1_gl_s->frame_step_us;
    if (T1_gl_s->next_poll_us < now) {
        T1_gl_s->next_poll_us = now + T1_gl_s->frame_step_us;
    }
}
