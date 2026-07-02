#include "T1_io.h"

#include "T1_public_types.h"
#include "T1_platform_layer.h"
#include "T1_global.h"
#include "T1_profiler.h"
#include "T1_std.h"
#include "T1_log.h"

#define T1_IO_SHORT_DURATION 300000

typedef struct {
    u64 timestamp;
    s32 scene_id;
    b8  is_up_short;
    b8  is_up_long;
    b8  is_active;
} T1IOFrameEvent;

#define T1_IO_MAX_EVENTS_PER_FRAME 10
typedef struct {
    // T1IOKey key; // not needed, it's the index
    T1IOFrameEvent list[T1_IO_MAX_EVENTS_PER_FRAME];
    u64 last_down_timestamp;
    s32 is_down_for_scene_id;
} T1IOFrameEventQueue;

#define SCENE_IDS_STACK_CAP 20
typedef struct {
    T1IOFrameEventQueue frame_map[T1_IO_KEY_ABOVEBOUNDS];
    s32 scene_ids_stack[SCENE_IDS_STACK_CAP];
    f32 mouse_x;
    f32 mouse_y;
    f32 last_drag_start_x;
    f32 last_drag_start_y;
    s32 ondown_lclick_touch_id;
    s32 cur_mouse_touch_id;
    s32 next_scene_id;
    s32 dragging_at_scene_id;
    u8 scene_ids_stack_i;
    u8 mouse_pos_changed;
} T1IOState;

T1IOState * T1_io = NULL;

void
T1_io_init(
    void *(* arg_malloc_func)(u64))
{
    T1_io = (T1IOState *)(arg_malloc_func(
        sizeof(T1IOState)));
    T1_std_memset(T1_io, 0, sizeof(T1IOState));
    
    T1_io->dragging_at_scene_id = -1;
    
    for (uint32_t i = 0; i < SCENE_IDS_STACK_CAP; i++) {
        T1_io->scene_ids_stack[i] = -1;    
    }
    
    for (uint32_t i = 0; i < T1_IO_KEY_ABOVEBOUNDS; i++) {
        T1_io->frame_map[i].is_down_for_scene_id = -1;
    }
}

static T1IOFrameEvent *
T1_io_get_open_event(
    const u32 for_key_id)
{
    T1_log_assert(for_key_id < T1_IO_KEY_ABOVEBOUNDS);
    
    s32 insert_i = 0;
    while (
        insert_i < T1_IO_MAX_EVENTS_PER_FRAME &&
        T1_io->frame_map[for_key_id].list[insert_i].is_active)
    {
        insert_i++;
    }
    
    if (T1_io->frame_map[for_key_id].list[insert_i].is_active) {
        return NULL;
    }
    
    T1_io->frame_map[for_key_id].list[insert_i].scene_id =
        T1_io->scene_ids_stack[T1_io->scene_ids_stack_i];
    
    return T1_io->frame_map[for_key_id].list + insert_i;
}

void
T1_io_register_keyup(const u32 key_id)
{
    if (
        key_id == T1_IO_MOUSE_LCLICK &&
        T1_io->dragging_at_scene_id >= 0)
    {
        T1_io->dragging_at_scene_id = -1;
    }
    
    T1_io->frame_map[key_id].is_down_for_scene_id = -1;
    
    if (
        key_id == T1_IO_MOUSE_LCLICK &&
        (T1_io->ondown_lclick_touch_id !=
            T1_io->cur_mouse_touch_id))
    {
        return;
    }
    
    u64 last_down = T1_io->frame_map[key_id].
        last_down_timestamp;
    
    T1IOFrameEvent * next = T1_io_get_open_event(key_id);
    
    if (next) {
        next->is_active = true;
        next->timestamp = T1_os_get_current_time_us();
        
        u64 since_down = next->timestamp - last_down;
        if (last_down >= next->timestamp) {
            next->is_up_short = 0;
            next->is_up_long = 0;
        } else if (since_down > T1_IO_SHORT_DURATION) {
            next->is_up_short = 0;
            next->is_up_long = 1;
        } else {
            next->is_up_short = 1;
            next->is_up_long = 0;
        }
    }
}

void T1_io_register_keyup_force_up_short(
    u32 key_id)
{
    T1_io->frame_map[key_id].is_down_for_scene_id = -1;
    
    T1IOFrameEvent * next = T1_io_get_open_event(key_id);
    
    if (next) {
        next->is_active = true;
        next->timestamp = T1_os_get_current_time_us();
        
        next->is_up_long = 0;
        next->is_up_short = 1;
    }
}

void
T1_io_register_keydown(const u32 key_id)
{
    if (key_id == T1_IO_MOUSE_LCLICK &&
        T1_io->dragging_at_scene_id < 0)
    {
        T1_io->dragging_at_scene_id =
            T1_io->scene_ids_stack[T1_io->scene_ids_stack_i];
        T1_io->last_drag_start_x = T1_io->mouse_x;
        T1_io->last_drag_start_y = T1_io->mouse_y;
        
        T1_io->ondown_lclick_touch_id =
            T1_os_gpu_get_touch_id_at_screen_pos(
                T1_io->mouse_x,
                T1_io->mouse_y);
    }
    
    T1_io->frame_map[key_id].is_down_for_scene_id =
        T1_io->scene_ids_stack[T1_io->scene_ids_stack_i];
    T1_io->frame_map[key_id].last_down_timestamp =
        T1_os_get_current_time_us();
    
    T1IOFrameEvent * next = T1_io_get_open_event(key_id);
    
    if (next) {
        next->timestamp = T1_io->frame_map[key_id].
            last_down_timestamp;
        next->is_active = true;
        next->is_up_long = false;
        next->is_up_short = false;
    }
}

void
T1_io_register_mouse_move(
    const f32 screen_x,
    const f32 screen_y)
{
    if (T1_io->mouse_x != screen_x ||
        T1_io->mouse_y != screen_y)
    {
        T1_io->mouse_pos_changed = true;
        T1_io->mouse_x = screen_x;
        T1_io->mouse_y = screen_y;
    }
}

void
T1_io_update_and_clear_for_next_frame(void)
{   
    // at the end, reset the frame data
    for (T1IOKey i = 0; i < T1_IO_KEY_ABOVEBOUNDS; i++) {
        for (uint32_t j = 0; j < T1_IO_MAX_EVENTS_PER_FRAME; j++) {
            T1_io->frame_map[i].list[j].is_active = false;
        }
    }
    
    T1_io->cur_mouse_touch_id = T1_os_gpu_get_touch_id_at_screen_pos(
        T1_io->mouse_x,
        T1_io->mouse_y);
}

/*
Public functions (exposed via T1.h)
*/
s32 T1_io_create_scene_and_return_id(void) {
    T1_io->next_scene_id += 1;
    return T1_io->next_scene_id;
}

void T1_io_scene_stack_push(const s32 scene_id) {
    T1_log_assert(T1_io->scene_ids_stack[
        T1_io->scene_ids_stack_i] != scene_id);
    T1_log_assert((T1_io->scene_ids_stack_i + 1) <
        SCENE_IDS_STACK_CAP);
    T1_io->scene_ids_stack_i++;
    T1_io->scene_ids_stack[T1_io->scene_ids_stack_i] = scene_id;
}

void T1_io_scene_stack_pop(void) {
    T1_log_assert(T1_io->scene_ids_stack_i > 0);
    T1_io->scene_ids_stack_i -= 1;
}

b8 T1_io_key_is_down(T1IOKey key, const s32 scene_id)
{
    T1_log_assert(key < T1_IO_KEYBOARD_ABOVE_KEYBOARD_BOUNDS);
    
    if (scene_id < 0) {
        T1_log_assert(scene_id == -1);
        return T1_io->frame_map[key].is_down_for_scene_id >= 0;
    } else {    
        return T1_io->frame_map[key].is_down_for_scene_id == scene_id;
    }
}

b8 T1_io_key_consume_tap_began_frame(
    T1IOKey key,
    const s32 scene_id)
{
    T1_log_assert(scene_id >= -1);
    
    for (
        s32 i = 0;
        i < T1_IO_MAX_EVENTS_PER_FRAME;
        i++)
    {
        b8 scene_hit =
            (scene_id == T1_io->frame_map[key].list[i].scene_id) ||
            scene_id == -1;
        
        if (
            scene_hit &&
            T1_io->frame_map[key].list[i].is_active &&
            !T1_io->frame_map[key].list[i].is_up_short &&
            !T1_io->frame_map[key].list[i].is_up_long)
        {
            T1_io->frame_map[key].list[i].is_active = false;
            return true;
        }
    }
    
    return false;
}

b8 T1_io_key_consume_short_tap_this_frame(T1IOKey key, const s32 scene_id) {        
    // T1_log_assert(0);
    for (
        s32 i = 0;
        i < T1_IO_MAX_EVENTS_PER_FRAME;
        i++)
    {
        b8 scene_hit =
            (scene_id == T1_io->frame_map[key].list[i].scene_id) ||
            scene_id == -1;
        
        if (
            scene_hit &&
            T1_io->frame_map[key].list[i].is_active &&
            T1_io->frame_map[key].list[i].is_up_short &&
            !T1_io->frame_map[key].list[i].is_up_long)
        {
            T1_io->frame_map[key].list[i].is_active = false;
            return true;
        }
    }
    
    return false;
}

b8 T1_io_key_consume_long_tap_this_frame(T1IOKey key, const s32 scene_id) {        
    // T1_log_assert(0);
    for (
        s32 i = 0;
        i < T1_IO_MAX_EVENTS_PER_FRAME;
        i++)
    {
        b8 scene_hit =
            (scene_id == T1_io->frame_map[key].list[i].scene_id) ||
            scene_id == -1;
        
        if (
            scene_hit &&
            T1_io->frame_map[key].list[i].is_active &&
            T1_io->frame_map[key].list[i].is_up_long)
        {
            T1_io->frame_map[key].list[i].is_active = false;
            return true;
        }
    }
    
    return false;
}

f32 T1_io_get_mouse_x_this_frame(void) {
    return T1_io->mouse_x;
}

f32 T1_io_get_mouse_y_this_frame(void) {
    return T1_io->mouse_y;
}

s32 T1_io_get_mouse_touch_id_this_frame(void) {
    return T1_io->cur_mouse_touch_id;
}

b8 T1_io_consume_mouse_changed(const s32 scene_id) {
    (void)scene_id; // TODO: implement me
    
    u8 out = T1_io->mouse_pos_changed;
    T1_io->mouse_pos_changed = false;
    return out;
}

b8 T1_io_consume_mouse_drag(
    f32 * delta_x,
    f32 * delta_y,
    const s32 scene_id)
{
    if (T1_io->dragging_at_scene_id == scene_id) {
        *delta_x = T1_io->last_drag_start_x - T1_io->mouse_x;
        *delta_y = T1_io->last_drag_start_y - T1_io->mouse_y;
        T1_io->last_drag_start_x = T1_io->mouse_x;
        T1_io->last_drag_start_y = T1_io->mouse_y;
        return true;
    }
    
    return false;
}
