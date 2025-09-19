#include "T1_uiinteraction.h"

bool32_t * T1_keypress_map; //[KEYPRESS_MAP_SIZE];

T1UIInteraction * T1_uiinteractions = NULL;

float mouse_scroll_pos = 0.0f;

void T1_uiinteraction_construct(T1UIInteraction * to_construct) {
    to_construct->touchable_id_top = -1;
    to_construct->touchable_id_pierce = -1;
    to_construct->screen_x = 0;
    to_construct->screen_y = 0;
    to_construct->timestamp = 0;
    to_construct->handled = true;
}

void T1_uiinteraction_register(T1UIInteraction * touch_record)
{
    uint64_t timestamp = T1_platform_get_current_time_us();
    
    touch_record->screen_x =
        T1_uiinteractions[T1_INTR_LAST_GPU_DATA].screen_x;
    touch_record->screen_y =
        T1_uiinteractions[T1_INTR_LAST_GPU_DATA].screen_y;
    touch_record->touchable_id_top =
        T1_uiinteractions[T1_INTR_LAST_GPU_DATA].touchable_id_top;
    touch_record->touchable_id_pierce =
        T1_uiinteractions[T1_INTR_LAST_GPU_DATA].
            touchable_id_pierce;
    touch_record->timestamp = timestamp;
    touch_record->checked_touchables = false;
    touch_record->handled = false;
}

void T1_uiinteraction_register_keyup(uint32_t key_id)
{
    log_assert(key_id < KEYPRESS_MAP_SIZE);
    
    T1_keypress_map[key_id] = false;
}

void T1_uiinteraction_register_keydown(uint32_t key_id)
{
    log_assert(key_id < KEYPRESS_MAP_SIZE);
    
    T1_keypress_map[key_id] = true;
}

void T1_uiinteraction_register_mousescroll(float amount)
{
    mouse_scroll_pos += amount;
    
    if (mouse_scroll_pos < -40.0f) {
        mouse_scroll_pos = -40.0f;
    }
    if (mouse_scroll_pos > 40.0f) {
        mouse_scroll_pos = 40.0f;
    }
}
