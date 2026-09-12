#ifndef T1_GAMELOOP_H
#define T1_GAMELOOP_H

#include <stdint.h>

#include "T1_types_cpu_to_gpu.h"

#ifdef __cplusplus
extern "C" {
#endif

extern u8 T1_gameloop_active;
extern u8 T1_gameloop_loading_texs;

void T1_gameloop_init(
    void *(* arg_malloc_fptr)(size_t),
    void (* client_update_fptr)(u64),
    void (* client_callback_after_render_fptr)(void),
    void (* client_callback_update_window_resize)(void));

void
T1_gameloop_update_before_render_pass(
    T1GPUFrame * frame_data);

void
T1_gameloop_update_after_render_pass(void);

#ifdef __cplusplus
}
#endif

#endif // T1_GAMELOOP_H
