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

void T1_io_init(void *(* arg_malloc_func)(u64));
void T1_io_event_construct(T1IOEvent * to_construct);
void T1_io_event_register(T1IOEvent * touch_record);
void T1_io_register_keyup(u32 key_id, u8 debounces);
void T1_io_register_keyup_force_up_short(u32 key_id);
void T1_io_register_keydown(u32 key_id, u8 debounces);
void T1_io_register_key_move_to_pos(T1IOKey key_id, f32 x, f32 y);
void T1_io_update_and_clear_for_next_frame(void);

/*
Public functions (exposed via T1.h)
*/
s32  T1_io_create_scene_and_return_id(void);
void T1_io_scene_stack_push(const s32 scene_id);
void T1_io_scene_stack_pop(void);
s32  T1_io_scene_stack_get_active_scene_id(void);
b8   T1_io_key_is_down(T1IOKey key, s32 scene_id);
b8   T1_io_key_consume_tap_began_frame(T1IOKey key, s32 scene_id);
b8   T1_io_key_consume_short_tap_this_frame(T1IOKey key, s32 scene_id);
b8   T1_io_key_consume_long_tap_this_frame(T1IOKey key, s32 scene_id);
f32  T1_io_get_pos_x_this_frame(T1IOKey key); 
f32  T1_io_get_pos_y_this_frame(T1IOKey key); 
s32  T1_io_get_mouse_touch_id_this_frame(void);
b8   T1_io_consume_mouse_changed(s32 scene_id);
b8   T1_io_consume_mouse_drag(f32 * delta_x, f32 * delta_y, s32 scene_id);

#ifdef __cplusplus
}
#endif

#endif // T1_IO_H
