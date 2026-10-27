#ifndef T1_GAMELOOP_H
#define T1_GAMELOOP_H

#include "T1_stdint.h"
#include "T1_types_public.h"

extern u8 T1_gameloop_active;

void T1_gameloop_init(
    void *(* arg_malloc_fptr)(size_t));

// The client's main loop, see T1.h
void T1_window_open(T1Tex tex, char ** const fatal_error);
void T1_window_set_tex(T1Tex tex);
void T1_window_toggle_fullscreen(void);
u32  T1_window_get_width(void);
u32  T1_window_get_height(void);
u32  T1_window_get_pixel_width(void);
u32  T1_window_get_pixel_height(void);

u8   T1_frame_update(void);
u8   T1_frame_resized(void);
u64  T1_frame_elapsed_us(void);

#endif // T1_GAMELOOP_H
