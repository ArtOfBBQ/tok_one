#include "userinput.h"

bool32_t keypress_map[KEYPRESS_MAP_SIZE];

Touch current_touch;

MouseEvent last_mouse_down;
MouseEvent last_mouse_up;
MouseEvent last_right_mouse_down;
MouseEvent last_right_mouse_up;
MouseEvent last_mouse_move;

void buffer_mousedown(
    float screenspace_x,
    float screenspace_y)
{
    if (last_mouse_down.handled == false) {
        return;
    }
    
    // note: this returns -1 when no touchable is there
    // which is what we want
    int32_t colliding_touchable =
        find_touchable_at(
            /* normalized_x : */
                ((screenspace_x * 2)
                    / window_width) - 1.0f,
            /* normalized_y : */
                ((screenspace_y * 2)
                    / window_height) - 1.0f);
    
    last_mouse_down.happened_at =
        platform_get_current_time_microsecs();
    last_mouse_down.screenspace_x = screenspace_x;
    last_mouse_down.screenspace_y = screenspace_y;
    last_mouse_down.touchable_id = colliding_touchable;
    last_mouse_down.handled = false;
}

void buffer_mouseup(
    float screenspace_x,
    float screenspace_y)
{
    if (last_mouse_up.handled == false) {
        return;
    }
    
    // note: this returns -1 when no touchable is there
    // which is what we want
    int32_t colliding_touchable =
        find_touchable_at(
            /* normalized_x : */
                ((screenspace_x * 2)
                    / window_width) - 1.0f,
            /* normalized_y : */
                ((screenspace_y * 2)
                    / window_height) - 1.0f);
    
    last_mouse_up.happened_at =
        platform_get_current_time_microsecs();
    last_mouse_up.screenspace_x = screenspace_x;
    last_mouse_up.screenspace_y = screenspace_y;
    last_mouse_up.touchable_id = colliding_touchable;
    last_mouse_up.handled = false;
}

void buffer_mousemove(
    float screenspace_x,
    float screenspace_y)
{
    if (last_mouse_move.handled == false) {
        return;
    }
    
    // note: this returns -1 when no touchable is there
    // which is what we want
    int32_t colliding_touchable =
        find_touchable_at(
            /* normalized_x : */
                ((screenspace_x * 2)
                    / window_width) - 1.0f,
            /* normalized_y : */
                ((screenspace_y * 2)
                    / window_height) - 1.0f);
    
    last_mouse_move.happened_at =
        platform_get_current_time_microsecs();
    last_mouse_move.screenspace_x = screenspace_x;
    last_mouse_move.screenspace_y = screenspace_y;
    last_mouse_move.touchable_id = colliding_touchable;
    last_mouse_move.handled = false;
}

void register_keyup(uint32_t key_id)
{
    log_append("register key up: ");
    log_append_uint(key_id);
    log_append("\n");
    
    log_assert(key_id < KEYPRESS_MAP_SIZE);
    
    keypress_map[key_id] = false;
}

void register_keydown(uint32_t key_id)
{ 
    log_append("register key down: ");
    log_append_uint(key_id);
    log_append("\n");
    log_assert(key_id < KEYPRESS_MAP_SIZE);
    
    keypress_map[key_id] = true;
}

void register_touchstart(float x, float y)
{
    if (current_touch.handled) {
        current_touch.start_x = x;
        current_touch.start_y = y;
        current_touch.started_at =
            platform_get_current_time_microsecs();
        current_touch.current_x = x;
        current_touch.current_y = y;
        current_touch.finished = false;
    }
}

void register_touchend(float x, float y)
{
    current_touch.current_x = x;
    current_touch.current_y = y;
    current_touch.finished = true;
    current_touch.finished_at =
        platform_get_current_time_microsecs();
    current_touch.handled = false;
}
