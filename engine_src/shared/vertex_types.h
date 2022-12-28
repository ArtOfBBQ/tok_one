#ifndef VERTEX_TYPES_H
#define VERTEX_TYPES_H

#define TEXTUREARRAYS_SIZE 31
#define ZLIGHTS_TO_APPLY_ARRAYSIZE 50

// #pragma pack(push, 1)
typedef struct Vertex {
    float x;
    float y;
    float z;
    float w;
    float orig_x;
    float orig_y;
    float orig_z;
    float normal_x;
    float normal_y;
    float normal_z;
    float uv  [2];
    float RGBA[4];
    int   texturearray_i; // -1 for no texture
    int   texture_i;      // -1 for no texture
    int   touchable_id;
} Vertex;

typedef struct LightCollection {
    float light_x[ZLIGHTS_TO_APPLY_ARRAYSIZE];
    float light_y[ZLIGHTS_TO_APPLY_ARRAYSIZE];
    float light_z[ZLIGHTS_TO_APPLY_ARRAYSIZE];
    float orig_x [ZLIGHTS_TO_APPLY_ARRAYSIZE];
    float orig_y [ZLIGHTS_TO_APPLY_ARRAYSIZE];
    float orig_z [ZLIGHTS_TO_APPLY_ARRAYSIZE];
    float ambient[ZLIGHTS_TO_APPLY_ARRAYSIZE];
    float diffuse[ZLIGHTS_TO_APPLY_ARRAYSIZE];
    float reach  [ZLIGHTS_TO_APPLY_ARRAYSIZE];
    float red    [ZLIGHTS_TO_APPLY_ARRAYSIZE];
    float green  [ZLIGHTS_TO_APPLY_ARRAYSIZE];
    float blue   [ZLIGHTS_TO_APPLY_ARRAYSIZE];
    unsigned int lights_size;
} LightCollection;
// #pragma pack(pop)

#endif
