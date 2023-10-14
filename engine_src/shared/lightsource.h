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
#include "memorystore.h"

#ifdef __cplusplus
extern "C" {
#endif

zVertex x_rotate_zvertex(
    const zVertex * input,
    const float angle);
void x_rotate_zvertices_inplace(
    SIMD_FLOAT * vec_to_rotate_y,
    SIMD_FLOAT * vec_to_rotate_z,
    const SIMD_FLOAT cos_angles,
    const SIMD_FLOAT sin_angles);

zVertex y_rotate_zvertex(
    const zVertex * input,
    const float angle);
void y_rotate_zvertices_inplace(
    SIMD_FLOAT * vec_to_rotate_x,
    SIMD_FLOAT * vec_to_rotate_z,
    const SIMD_FLOAT cos_angles,
    const SIMD_FLOAT sin_angles);

zVertex z_rotate_zvertex(
    const zVertex * input,
    const float angle);
void z_rotate_zvertices_inplace(
    SIMD_FLOAT * vec_to_rotate_x,
    SIMD_FLOAT * vec_to_rotate_y,
    const SIMD_FLOAT cos_angles,
    const SIMD_FLOAT sin_angles);

extern GPUCamera camera;

typedef struct zLightSource {
    int32_t object_id; // you can make a group of lights and/or texquads by
                       // giving them the same positive object_id, then make
                       // ScheduledAnimations that affect the entire group
                       // set to -1 to not be a party of any group
    bool32_t deleted;
    bool32_t committed;
    float x;
    float y;
    float z;
    float x_offset;    // these 3 _offset values are unaffected by animations,
    float y_offset;    // so you can give the light the same object_id as a
    float z_offset;    // sprite and move them both, keeping the light offset
    float RGBA[4];
    float reach;       // max distance before light intensity 0
    float ambient;     // how much ambient light does this radiate?
    float diffuse;     // how much diffuse light does this radiate?
} zLightSource;

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

#ifdef __cplusplus
}
#endif

#endif // LIGHTSOURCE_H
