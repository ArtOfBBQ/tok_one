#ifndef T1_ZLIGHT_H
#define T1_ZLIGHT_H

#include <math.h>

#include "T1_cpu_gpu_shared_types.h"
#include "T1_simd.h"
#include "T1_std.h"
#include "T1_global.h"
#include "T1_triangle.h"
#include "T1_render_view.h"

#ifdef __cplusplus
extern "C" {
#endif


void x_rotate_zvertex_f3_known_cossin(
    float inout_xyz[3],
    const float cosf_angle,
    const float sinf_angle);
void x_rotate_zvertex_f3(
    float inout_xyz[3],
    const float angle);
void x_rotate_zvertices_inplace(
    SIMD_FLOAT * vec_to_rotate_y,
    SIMD_FLOAT * vec_to_rotate_z,
    const SIMD_FLOAT cos_angles,
    const SIMD_FLOAT sin_angles);

void y_rotate_zvertex_known_cossin(
    float inout_xyz[3],
    const float cos_angle,
    const float sin_angle);
void y_rotate_zvertex_f3(
    float inout_xyz[3],
    const float angle);
void y_rotate_zvertices_inplace(
    SIMD_FLOAT * vec_to_rotate_x,
    SIMD_FLOAT * vec_to_rotate_z,
    const SIMD_FLOAT cos_angles,
    const SIMD_FLOAT sin_angles);

void z_rotate_zvertex_f3_known_cossin(
    float inout_xyz[3],
    const float angle_cos,
    const float angle_sin);
void z_rotate_zvertex_f3(
    float inout_xyz[3],
    const float angle);
void z_rotate_zvertices_inplace(
    SIMD_FLOAT * vec_to_rotate_x,
    SIMD_FLOAT * vec_to_rotate_y,
    const SIMD_FLOAT cos_angles,
    const SIMD_FLOAT sin_angles);

typedef struct {
    // you can make a group of lights and/or texquads by
    // giving them the same positive object_id, then make
    // ScheduledAnimations that affect the entire group
    // set to -1 to not be a party of any group
    union {
        int32_t object_id;
        float flt_object_id;
    };
    union {
        bool32_t deleted;
        float flt_deleted;
    };
    union {
        bool32_t committed;
        float flt_committed;
    };
    float xyz[3];
    float xyz_angle[3];
    float xyz_offset[3];
    float RGBA[4];
    float reach; // light's reach
    float diffuse;     // how much diffuse light does this radiate?
    float specular;
    float simd_padding[5];
} T1zLightSource; // 17 floats = 68 bytes

// A buffer of zLightSources to light up your scene(s)
// index 0 to zlights_to_apply_size will be rendered,
// the rest of the array will be ignored
extern T1zLightSource * zlights_to_apply;
extern uint32_t zlights_to_apply_size;

T1zLightSource * T1_zlight_next(void);
void T1_zlight_commit(
    T1zLightSource * to_request);

void T1_zlight_clean_all_deleted(void);

void T1_zlight_project_float4_to_2d_inplace(
    float * position_x,
    float * position_y,
    float * position_z);

// just copy the lights without translation, for hardware renderer
void T1_zlight_copy_all(
    T1GPULight * lights,
    uint32_t * lights_size);

// move each light around the camera (e.g. when the camera moves
// right, we move all lights etc. to the left instead)
// reminder: this is calculated once before 2d and 3d renderer
// and then used in both
void T1_zlight_translate_all(
    T1GPULight * lights,
    uint32_t * lights_size);

void T1_zlight_delete(
    const int32_t with_zsprite_id);

void T1_zlight_point_light_to_location(
    float * recipient_xyz_angle,
    const float * from_pos_xyz,
    const float * point_to_xyz);

#ifdef __cplusplus
}
#endif

#endif // T1_ZLIGHT_H
