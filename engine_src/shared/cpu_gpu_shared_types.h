#ifndef CPU_GPU_SHARED_TYPES_H
#define CPU_GPU_SHARED_TYPES_H

#define TEXTUREARRAYS_SIZE 31
#define MAX_FILES_IN_SINGLE_TEXARRAY 200

#include "clientlogic_macro_settings.h"

#pragma pack(push, 1)
typedef struct GPUVertex {
    float x;
    float y;
    float z;
    /* Currently same for "entire" triangle */
    float normal_x;
    float normal_y;
    float normal_z;
    float uv  [2];
    float RGBA[4];
    int   texturearray_i; // -1 for no texture
    int   texture_i;      // -1 for no texture
    /* Same for parent */
    float parent_x;       // same for entire parent
    float parent_y;       // same for entire parent
    float parent_z;       // same for entire parent
    float x_angle;        // same for entire parent
    float y_angle;        // same for entire parent
    float z_angle;        // same for entire parent
    float scale_factor;   // same for entire parent
    float ignore_lighting;// same for entire parent
    float ignore_camera;  // same for entire parent
    int   touchable_id;   // same for entire parent
} GPUVertex;

typedef struct GPUCamera {
    float x;
    float y;
    float z;
    float x_angle;
    float y_angle;
    float z_angle;
} GPUCamera;

typedef struct GPULightCollection {
    float light_x[MAX_LIGHTS_PER_BUFFER];
    float light_y[MAX_LIGHTS_PER_BUFFER];
    float light_z[MAX_LIGHTS_PER_BUFFER];
    float ambient[MAX_LIGHTS_PER_BUFFER];
    float diffuse[MAX_LIGHTS_PER_BUFFER];
    float reach  [MAX_LIGHTS_PER_BUFFER];
    float red    [MAX_LIGHTS_PER_BUFFER];
    float green  [MAX_LIGHTS_PER_BUFFER];
    float blue   [MAX_LIGHTS_PER_BUFFER];
    unsigned int lights_size;
} GPULightCollection;

typedef struct GPUProjectionConstants {
    float near;
    float far;
    float q;
    float field_of_view_rad;
    float field_of_view_modifier;
    float x_multiplier;
    float y_multiplier;
} GPUProjectionConstants;
#pragma pack(pop)

#endif

