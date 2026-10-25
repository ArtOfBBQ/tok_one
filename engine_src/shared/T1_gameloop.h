#ifndef T1_GAMELOOP_H
#define T1_GAMELOOP_H

#include <stdint.h>

#include "T1_types_cpu_to_gpu.h"

extern u8 T1_gameloop_active;

void T1_gameloop_init(
    void *(* arg_malloc_fptr)(size_t));

// The client's main loop, see T1.h
void T1_window_open(char ** const fatal_error);
void T1_window_toggle_fullscreen(void);
void T1_window_poll_events(void);
u8   T1_window_close_requested(void);
u8   T1_window_visible(void);
void T1_window_present(void);

void T1_frame_begin(void);
u8   T1_frame_should_update(void);
u8   T1_frame_resized(void);
u64  T1_frame_elapsed_us(void);
void T1_frame_render(void);

#endif // T1_GAMELOOP_H
