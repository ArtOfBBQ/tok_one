#ifndef USERINPUT_H
#define USERINPUT_H

#include "stdio.h"

#include "common.h"
#include "zpolygon.h"
#include "platform_layer.h"

#define KEYPRESS_MAP_SIZE 1000

typedef struct Touch {
    float start_x;
    float start_y;
    uint64_t started_at;
    float current_x;
    float current_y;
    float finished;
    uint64_t finished_at;
    float handled;
} Touch;

extern Touch current_touch;

extern bool32_t keypress_map[KEYPRESS_MAP_SIZE];

void register_keyup(uint32_t key_id);
void register_keydown(uint32_t key_id);
void register_touchstart(float x, float y);
void register_touchend(float x, float y);

#endif
