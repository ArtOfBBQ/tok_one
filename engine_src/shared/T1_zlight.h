#ifndef T1_ZLIGHT_H
#define T1_ZLIGHT_H

#include <math.h>

#include "T1_public_types.h"
#include "T1_cpu_gpu_shared_types.h"
#include "T1_std.h"
#include "T1_global.h"
#include "T1_triangle.h"

#ifdef __cplusplus
extern "C" {
#endif

// A buffer of zLightSources to light up your scene(s)
// index 0 to zlights_to_apply_size will be rendered,
// the rest of the array will be ignored
extern T1zLight * T1_zlights;
extern uint32_t T1_zlights_size;

T1zLight *
T1_zlight_next(void);

void
T1_zlight_commit(T1zLight * to_request);

void
T1_zlight_clean_all_deleted(void);

void
T1_zlight_project_float4_to_2d_inplace(
    float * position_x,
    float * position_y,
    float * position_z);

// just copy the lights without translation, for hardware renderer
void
T1_zlight_copy_all(
    T1GPULight * lights,
    uint32_t * lights_size);

// move each light around the camera (e.g. when the camera moves
// right, we move all lights etc. to the left instead)
// reminder: this is calculated once before 2d and 3d renderer
// and then used in both
void
T1_zlight_translate_all(
    T1GPULight * lights,
    uint32_t * lights_size);

void
T1_zlight_delete(
    const int32_t with_zsprite_id);

void
T1_zlight_point_light_to_location(
    float * recipient_xyz_angle,
    const float * from_pos_xyz,
    const float * point_to_xyz);

void
T1_zlight_update_all_attached_render_views(void);

#ifdef __cplusplus
}
#endif

#endif // T1_ZLIGHT_H
