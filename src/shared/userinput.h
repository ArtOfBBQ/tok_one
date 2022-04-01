#ifndef USERINPUT_H
#define USERINPUT_H

#include "stdio.h"

#include "common.h"
#include "zpolygon.h"

#define KEYPRESS_MAP_SIZE 1000

extern bool32_t keypress_map[KEYPRESS_MAP_SIZE];

void register_keyup(uint32_t key_id);
void register_keydown(uint32_t key_id);
void register_touchstart(float x, float y);
void register_touchend(float x, float y);

#endif
