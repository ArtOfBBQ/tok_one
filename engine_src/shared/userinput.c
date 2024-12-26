#include "userinput.h"

bool32_t * keypress_map; //[KEYPRESS_MAP_SIZE];

Interaction * user_interactions = NULL;

float mouse_scroll_pos = 0.0f;

void construct_interaction(Interaction * to_construct) {
    to_construct->touchable_id_top = -1;
    to_construct->touchable_id_pierce = -1;
    to_construct->screen_x = 0;
    to_construct->screen_y = 0;
    to_construct->timestamp = 0;
    to_construct->handled = true;
}

void register_interaction(Interaction * touch_record)
{
    uint64_t timestamp = platform_get_current_time_microsecs();
    
    touch_record->screen_x            =
        user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE].screen_x;
    touch_record->screen_y            =
        user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE].screen_y;
    touch_record->touchable_id_top    =
        user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE].touchable_id_top;
    touch_record->touchable_id_pierce =
        user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE].
            touchable_id_pierce;
    touch_record->timestamp          = timestamp;
    touch_record->checked_touchables = false;
    touch_record->handled            = false;
}

void register_keyup(uint32_t key_id)
{
    log_assert(key_id < KEYPRESS_MAP_SIZE);
    
    keypress_map[key_id] = false;
}

void register_keydown(uint32_t key_id)
{
    log_assert(key_id < KEYPRESS_MAP_SIZE);
    
    keypress_map[key_id] = true;
}

void register_mousescroll(float amount)
{
    mouse_scroll_pos += amount;
    
    if (mouse_scroll_pos < -40.0f) {
        mouse_scroll_pos = -40.0f;
    }
    if (mouse_scroll_pos > 40.0f) {
        mouse_scroll_pos = 40.0f;
    }
}
