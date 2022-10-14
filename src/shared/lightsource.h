// TODO: This has 3D elements like zvertex and the zcamera
// the name could be shared2d3d.h instead of lightsource.h

#ifndef LIGHTSOURCE_H
#define LIGHTSOURCE_H

#include <math.h>

#include "common.h"
#include "window_size.h"
#include "memorystore.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct zVertex {
    float x;
    float y;
    float z;
    float uv[2];         // texture coords, ignored if untextured
} zVertex;

zVertex x_rotate_zvertex(
    const zVertex * input,
    const float angle);
zVertex y_rotate_zvertex(
    const zVertex * input,
    const float angle);
zVertex z_rotate_zvertex(
    const zVertex * input,
    const float angle);

typedef struct zCamera {
    float x;
    float y;
    float z;
    float x_angle;
    float y_angle;
    float z_angle;
} zCamera;
extern zCamera camera;

typedef struct zLightSource {
    int32_t object_id; // you can make a group of lights and/or texquads by
                       // giving them the same positive object_id, then make
                       // ScheduledAnimations that affect the entire group
                       // set to -1 to not be a party of any group
    bool32_t deleted;
    float x;
    float y;
    float z;
    float RGBA[4];
    float reach;       // max distance before light intensity 0
    float ambient;     // how much ambient light does this radiate?
    float diffuse;     // how much diffuse light does this radiate?
} zLightSource;

// A buffer of zLightSources to light up your scene(s)
// index 0 to zlights_to_apply_size will be rendered,
// the rest of the array will be ignored
#define ZLIGHTS_TO_APPLY_ARRAYSIZE 500
extern zLightSource * zlights_to_apply;
extern zLightSource * zlights_transformed;
extern uint32_t zlights_to_apply_size;
extern uint32_t zlights_transformed_size;

// void delete_lights_with_object_id(uint32_t object_id);

void clean_deleted_lights(void);

// move each light around the camera (e.g. when the camera moves
// right, we move all lights etc. to the left instead)
// reminder: this is calculated once before 2d and 3d renderer
// and then used in both
void translate_lights(void);

#ifdef __cplusplus
}
#endif

#endif // LIGHTSOURCE_H
