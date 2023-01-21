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

// indexes in the interactions array
#define INTR_PREVIOUS_TOUCH_START               0
#define INTR_PREVIOUS_TOUCH_END                 1
#define INTR_PREVIOUS_LEFTCLICK_START           2
#define INTR_PREVIOUS_LEFTCLICK_END             3
#define INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START  4
#define INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END    5
#define INTR_PREVIOUS_RIGHTCLICK_START          6
#define INTR_PREVIOUS_RIGHTCLICK_END            7
#define INTR_PREVIOUS_MOUSE_MOVE                8
#define INTR_PREVIOUS_TOUCH_MOVE                9
#define INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE      10
// permanent size of the interactions array
#define USER_INTERACTIONS_SIZE                 11
extern Interaction * user_interactions;

void register_interaction(
    Interaction * touch_record,
    const float x,
    const float y);

extern bool32_t * keypress_map; //[KEYPRESS_MAP_SIZE];

void register_keyup(uint32_t key_id);
void register_keydown(uint32_t key_id);

#ifdef __cplusplus
}
#endif

#endif // USERINPUT_H
