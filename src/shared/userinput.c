#include "userinput.h"

bool32_t keypress_map[KEYPRESS_MAP_SIZE];

Interaction previous_touch_start;
Interaction previous_touch_end;
Interaction previous_leftclick_start;
Interaction previous_leftclick_end;
Interaction previous_touch_or_leftclick_start;
Interaction previous_touch_or_leftclick_end;
Interaction previous_rightclick_start;
Interaction previous_rightclick_end;
Interaction previous_mouse_move;
Interaction previous_touch_move;
Interaction previous_mouse_or_touch_move;

void construct_interaction(Interaction * to_construct) {
    to_construct->touchable_id = -1;
    to_construct->screen_x = 0;
    to_construct->screen_y = 0;
    to_construct->timestamp = 0;
    to_construct->handled = true;
}

void register_interaction(
    Interaction * touch_record,
    const float x,
    const float y)
{
    // note: this returns -1 when no touchable is there
    // which is what we want
    touch_record->touchable_id =
        find_touchable_at(
            /* normalized_x : */
                ((x * 2) / window_width) - 1.0f,
            /* normalized_y : */
                ((y * 2) / window_height) - 1.0f);
    touch_record->screen_x = x;
    touch_record->screen_y = y;
    touch_record->timestamp = platform_get_current_time_microsecs();
    touch_record->handled = false;
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

