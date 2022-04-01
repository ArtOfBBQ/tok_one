#include "userinput.h"

bool32_t keypress_map[KEYPRESS_MAP_SIZE];

Touch current_touch;

void register_keyup(uint32_t key_id)
{
    printf("register key up: %u\n", key_id);
    if (key_id > KEYPRESS_MAP_SIZE) {
        printf(
            "error: got key_id: %u whic his bigger than our keymap size %u\n",
            key_id,
            KEYPRESS_MAP_SIZE);
        assert(0);
    }
    
    keypress_map[key_id] = false;
}

void register_keydown(uint32_t key_id)
{ 
    printf("register key down: %u\n", key_id);
    if (key_id > KEYPRESS_MAP_SIZE) {
        printf(
            "error: got key_id: %u whic his bigger than our keymap size %u\n",
            key_id,
            KEYPRESS_MAP_SIZE);
        assert(0);
    }
    
    printf("keydown: %u\n", key_id);
    keypress_map[key_id] = true;
}

void register_touchstart(float x, float y)
{
    if (current_touch.handled) {
        current_touch.start_x = x;
        current_touch.start_y = y;
        current_touch.started_at =
            platform_get_current_time_nanosecs();
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
        platform_get_current_time_nanosecs();
    current_touch.handled = false;
}
