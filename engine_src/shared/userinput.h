#ifndef USERINPUT_H
#define USERINPUT_H

#include "common.h"
#include "lightsource.h"
#include "platform_layer.h"

#define KEYPRESS_MAP_SIZE 1000

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Interaction {
    int32_t touchable_id;
    bool32_t checked_touchables;
    float screen_x;
    float screen_y;
    uint64_t timestamp;
    bool32_t handled;
} Interaction;
void construct_interaction(Interaction * to_construct);

extern Interaction previous_touch_start;
extern Interaction previous_touch_end;
extern Interaction previous_leftclick_start;
extern Interaction previous_leftclick_end;
extern Interaction previous_touch_or_leftclick_start;
extern Interaction previous_touch_or_leftclick_end;
extern Interaction previous_rightclick_start;
extern Interaction previous_rightclick_end;
extern Interaction previous_mouse_move;
extern Interaction previous_touch_move;
extern Interaction previous_mouse_or_touch_move;

void register_interaction(
    Interaction * touch_record,
    const float x,
    const float y);

extern bool32_t keypress_map[KEYPRESS_MAP_SIZE];

void register_keyup(uint32_t key_id);
void register_keydown(uint32_t key_id);

#ifdef __cplusplus
}
#endif

#endif // USERINPUT_H
