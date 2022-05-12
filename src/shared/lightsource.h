// TODO: This has 3D elements like zvertex and the zcamera
// the name could be shared2d3d.h instead of lightsource.h

#ifndef LIGHTSOURCE_H
#define LIGHTSOURCE_H

#include <math.h>

#include "common.h"

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
    float x;
    float y;
    float z;
    float RGBA[4];
    float reach;   // max distance before light intensity 0
    float ambient; // how much ambient light does this radiate?
    float diffuse; // how much diffuse light does this radiate?
} zLightSource;

// A buffer of zLightSources to light up your scene(s)
// index 0 to zlights_to_apply_size will be rendered,
// the rest of the array will be ignored
#define ZLIGHTS_TO_APPLY_ARRAYSIZE 50
extern zLightSource zlights_to_apply[ZLIGHTS_TO_APPLY_ARRAYSIZE];
extern uint32_t zlights_to_apply_size;

// move each light around the camera (e.g. when the camera moves
// right, we move all lights etc. to the left instead)
// reminder: this is calculated once before 2d and 3d renderer
// and then used in both
void translate_lights(
    const zLightSource * originals,
    zLightSource * out_translated,
    const uint32_t lights_count);

#endif

