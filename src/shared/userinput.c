#include "userinput.h"

bool32_t keypress_map[KEYPRESS_MAP_SIZE];

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

