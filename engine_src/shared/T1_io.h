#ifndef T1_IO_H
#define T1_IO_H

#include <stdint.h>
#include <stddef.h>

#include "T1_public_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    u64 timestamp;
    s32 touch_id_pierce;
    s32 touch_id_top;
    f32 screen_x;
    f32 screen_y;
    u8 checked_touch_ids;
} T1IOEvent;

void
T1_io_init(
    void *(* arg_malloc_func)(u64));

void
T1_io_event_construct(
    T1IOEvent * to_construct);

void
T1_io_event_register(
    T1IOEvent * touch_record);

void
T1_io_register_keyup(const u32 key_id);
void
T1_io_register_keyup_force_up_short(const u32 key_id);
void
T1_io_register_keydown(const u32 key_id);

void
T1_io_register_mouse_move(const f32 screen_x, const f32 screen_y);

void
T1_io_update_and_clear_for_next_frame(void);

/*
Public functions (exposed via T1.h)
*/
b8  T1_io_key_is_down(T1IOKey key);
b8  T1_io_key_consume_short_tap_this_frame(T1IOKey key);
b8  T1_io_key_consume_long_tap_this_frame(T1IOKey key);
f32 T1_io_get_mouse_x_this_frame(void);
f32 T1_io_get_mouse_y_this_frame(void);
s32 T1_io_get_mouse_touch_id_this_frame(void);

#ifdef __cplusplus
}
#endif

#endif // T1_IO_H
