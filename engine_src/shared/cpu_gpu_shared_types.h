#ifndef CPU_GPU_SHARED_TYPES_H
#define CPU_GPU_SHARED_TYPES_H

#define TEXTUREARRAYS_SIZE 31
#define ZLIGHTS_TO_APPLY_ARRAYSIZE 50

#pragma pack(push, 1)
typedef struct GPUVertex {
    float x;
    float y;
    float z;
    /* Currently same for "entire" triangle, but who cares about 3 vertices */
    float normal_x;
    float normal_y;
    float normal_z;
    float uv  [2];
    float RGBA[4];
    int   texturearray_i; // -1 for no texture
    int   texture_i;      // -1 for no texture
    /* Same for parent */
    float parent_x; // same for entire parent
    float parent_y; // same for entire parent
    float parent_z; // same for entire parent
    float x_angle; // same for entire parent
    float y_angle; // same for entire parent
    float z_angle; // same for entire parent
    float scale_factor; // same for entire parent
    float ignore_lighting; // same for entire parent
    float ignore_camera; // same for entire parent
    int   touchable_id; // same for entire parent
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
    float light_x[ZLIGHTS_TO_APPLY_ARRAYSIZE];
    float light_y[ZLIGHTS_TO_APPLY_ARRAYSIZE];
    float light_z[ZLIGHTS_TO_APPLY_ARRAYSIZE];
    float ambient[ZLIGHTS_TO_APPLY_ARRAYSIZE];
    float diffuse[ZLIGHTS_TO_APPLY_ARRAYSIZE];
    float reach  [ZLIGHTS_TO_APPLY_ARRAYSIZE];
    float red    [ZLIGHTS_TO_APPLY_ARRAYSIZE];
    float green  [ZLIGHTS_TO_APPLY_ARRAYSIZE];
    float blue   [ZLIGHTS_TO_APPLY_ARRAYSIZE];
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

// trying this for storing which pixels are associated with which touchable_id
// after all rotation/translation/projection is finished
//#define MAX_SCREEN_WIDTH 4000
//#define MAX_SCREEN_HEIGHT 3000
//typedef struct GPU_TouchablePixels {
//    int touchable_id[MAX_SCREEN_WIDTH * MAX_SCREEN_HEIGHT];
//} GPU_TouchablePixels;
#pragma pack(pop)

#endif

