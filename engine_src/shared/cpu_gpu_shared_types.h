#ifndef CPU_GPU_SHARED_TYPES_H
#define CPU_GPU_SHARED_TYPES_H

#define TEXTUREARRAYS_SIZE 31
#define ZLIGHTS_TO_APPLY_ARRAYSIZE 50

//#pragma pack(push, 1)
typedef struct GPU_Vertex {
    float x;
    float y;
    float z;
    float parent_x;
    float parent_y;
    float parent_z;
    float x_angle;
    float y_angle;
    float z_angle;
    float normal_x;
    float normal_y;
    float normal_z;
    float scale_factor;
    float uv  [2];
    float RGBA[4];
    float ignore_lighting;
    float ignore_camera;
    int   texturearray_i; // -1 for no texture
    int   texture_i;      // -1 for no texture
    int   touchable_id;
} GPU_Vertex;

typedef struct GPU_Camera {
    float x;
    float y;
    float z;
    float x_angle;
    float y_angle;
    float z_angle;
} GPU_Camera;

typedef struct GPU_LightCollection {
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
} GPU_LightCollection;

typedef struct GPU_ProjectionConstants {
    float near;
    float far;
    float q;
    float field_of_view_rad;
    float field_of_view_modifier;
    float x_multiplier;
    float y_multiplier;
} GPU_ProjectionConstants;

// trying this for storing which pixels are associated with which touchable_id
// after all rotation/translation/projection is finished
#define MAX_SCREEN_WIDTH 4000
#define MAX_SCREEN_HEIGHT 3000
typedef struct GPU_TouchablePixels {
    int touchable_id[MAX_SCREEN_WIDTH * MAX_SCREEN_HEIGHT];
} GPU_TouchablePixels;
// #pragma pack(pop)

#endif

