// TODO: This has 3D elements like zvertex and the zcamera
// the name could be shared2d3d.h instead of lightsource.h

#ifndef LIGHTSOURCE_H
#define LIGHTSOURCE_H

#include <math.h>

#include "cpu_gpu_shared_types.h"
#include "simd.h"
#include "common.h"
#include "window_size.h"
#include "triangle.h"

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

extern GPUCamera camera;

typedef struct zLightSource {
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
    float xyz_offset[3];
    float RGBA[4];
    float reach; // light's reach
    float ambient;     // how much ambient light does this radiate?
    float diffuse;     // how much diffuse light does this radiate?
    float specular;
    float simd_padding[7];
} zLightSource; // 17 floats = 68 bytes

// A buffer of zLightSources to light up your scene(s)
// index 0 to zlights_to_apply_size will be rendered,
// the rest of the array will be ignored
extern zLightSource * zlights_to_apply;
extern uint32_t zlights_to_apply_size;


zLightSource * next_zlight(void);
void commit_zlight(zLightSource * to_request);

void clean_deleted_lights(void);

void project_float4_to_2d_inplace(
    float * position_x,
    float * position_y,
    float * position_z);

// just copy the lights without translation, for hardware renderer
void copy_lights(
    GPULightCollection * lights_for_gpu);

// move each light around the camera (e.g. when the camera moves
// right, we move all lights etc. to the left instead)
// reminder: this is calculated once before 2d and 3d renderer
// and then used in both
void translate_lights(
    GPULightCollection * lights_for_gpu);

void delete_zlight(const int32_t with_object_id);

#ifdef __cplusplus
}
#endif

#endif // LIGHTSOURCE_H
