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
    int   polygon_i;      // index into GPUPolygonCollection buffer
} GPUVertex;

typedef struct GPUCamera {
    float x;
    float y;
    float z;
    float x_angle;
    float y_angle;
    float z_angle;
} GPUCamera;

typedef struct GPUPolygonCollection {
    float x              [MAX_POLYGONS_PER_BUFFER];
    float y              [MAX_POLYGONS_PER_BUFFER];
    float z              [MAX_POLYGONS_PER_BUFFER];
    float x_angle        [MAX_POLYGONS_PER_BUFFER];
    float y_angle        [MAX_POLYGONS_PER_BUFFER];
    float z_angle        [MAX_POLYGONS_PER_BUFFER];
    float scale_factor   [MAX_POLYGONS_PER_BUFFER];
    float ignore_lighting[MAX_POLYGONS_PER_BUFFER];
    float ignore_camera  [MAX_POLYGONS_PER_BUFFER];
    unsigned int size;
} GPUPolygonCollection;

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

