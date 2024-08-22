#include "userinput.h"

bool32_t * keypress_map; //[KEYPRESS_MAP_SIZE];

Interaction * user_interactions = NULL;

float mouse_scroll_pos = 0.0f;

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
    uint64_t timestamp = platform_get_current_time_microsecs();
    //if (timestamp - touch_record->timestamp < 100000) { return; }
    
    touch_record->screen_x           = x;
    touch_record->screen_y           = y;
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
