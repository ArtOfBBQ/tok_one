/* Schedule "Set and forget" animations */

#ifndef SCHEDULED_ANIMATION_H
#define SCHEDULED_ANIMATION_H

#include "common.h"
#include "bitmap_renderer.h" // for texquads_to_render
#include "zpolygon.h"

void resolve_animation_effects(uint64_t microseconds_elapsed);

typedef struct ScheduledAnimation {
    uint32_t affected_object_id;
    float delta_x_per_second;
    float delta_y_per_second;
    float delta_z_per_second;
    float x_rotation_per_second;
    float y_rotation_per_second;
    float z_rotation_per_second;
    uint64_t remaining_microseconds;
    bool32_t deleted;
} ScheduledAnimation;

void request_scheduled_animation(ScheduledAnimation * to_add);

#endif

