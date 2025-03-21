#ifndef CPU_GPU_SHARED_TYPES_H
#define CPU_GPU_SHARED_TYPES_H

#define SHADOWMAP_TEXTUREARRAY_I 120

#ifndef TEXTUREARRAYS_SIZE
#define TEXTUREARRAYS_SIZE 31
#endif

#define DOWNSAMPLES_SIZE 5
#define DOWNSAMPLES_CUTOFF 4

#ifndef MAX_FILES_IN_SINGLE_TEXARRAY
#define MAX_FILES_IN_SINGLE_TEXARRAY 200
#endif

#ifndef MAX_POLYGONS_PER_BUFFER
#define MAX_POLYGONS_PER_BUFFER  75000
#endif

#ifndef MAX_LIGHTS_PER_BUFFER
#define MAX_LIGHTS_PER_BUFFER       5
#endif

#ifndef MAX_MATERIALS_PER_POLYGON
#define MAX_MATERIALS_PER_POLYGON   15
#endif

#pragma pack(push, 1)
typedef struct GPUVertex {
    int locked_vertex_i; // index into GPULockedVertex buffer
    int polygon_i;       // index into GPUPolygonCollection buffer
} GPUVertex;

/*
A 'set and forget' description of a triangle vertex, intended to stay static
for a long time or even the entire duration of the app.

When our app loads, we parse .obj files to get the triangle vertices in our 3D
models up-front, and keep these inplace. Ideally, we send them to the GPU once
and never update them, which will easily work for the small apps/games I'm
making.
 
To manipulate the location, direction, size etc. of our objects, we manipulate
the parents that contain these (see zpolygons_to_render in zpolygon.h),
not this.
*/
typedef struct GPULockedVertex {
    float        xyz       [3];     // 12 bytes
    float        normal_xyz[3];     // 12 bytes
    float        uv        [2];     //  8 bytes
    unsigned int parent_material_i; // 4 bytes
    float        padding[3];        // 12 bytes
} GPULockedVertex;

typedef struct GPUCamera {
    float xyz[3];           // 12 bytes
    float xyz_angle[3];     // 12 bytes
    float xyz_cosangle[3];  // 12 bytes
    float xyz_sinangle[3];  // 12 bytes
    float padding[2];       //  8 bytes
} GPUCamera;

typedef struct GPUPolygon {
    float        xyz[3];
    float        xyz_angle[3];
    float        bonus_rgb[3];
    float        xyz_multiplier[3]; // determines width/height/depth
    float        xyz_offset[3];
    float        scale_factor;
    float        ignore_lighting;
    float        ignore_camera;
    float        simd_padding[4]; // make sure touchable_id is behind this
    uint32_t     remove_shadow;
    int          touchable_id;
} GPUPolygon; // 24 floats (3 SIMD runs)

typedef struct GPUPolygonCollection {
    GPUPolygon   polygons[MAX_POLYGONS_PER_BUFFER];
    unsigned int size;
} GPUPolygonCollection;

#define SPECULAR_GLASS 0.5f
#define SPECULAR_PLASTIC 0.5f
#define SPECULAR_QUARTZ 0.57f
#define SPECULAR_ICE 0.224f
#define SPECULAR_WATER 0.255f
#define SPECULAR_MILK 0.277f
#define SPECULAR_SKIN 0.35f
#define SPECULAR_SILVER 0.508f
#define SPECULAR_GOLD 0.62f
typedef struct GPUPolygonMaterial {
    float rgba[4];          // 16 bytes
    float rgb_cap[3];       // 12 bytes // capped by default to avoid bloom
    int   texturearray_i;   //  4 bytes
    int   texture_i;        //  4 bytes
    float diffuse;          //  4 bytes
    float specular;         //  4 bytes
    float padding[5];       //  4 bytes
} GPUPolygonMaterial;

typedef struct GPULightCollection {
    float        light_x              [MAX_LIGHTS_PER_BUFFER];
    float        light_y              [MAX_LIGHTS_PER_BUFFER];
    float        light_z              [MAX_LIGHTS_PER_BUFFER];
    float        angle_x              [MAX_LIGHTS_PER_BUFFER];
    float        angle_y              [MAX_LIGHTS_PER_BUFFER];
    float        angle_z              [MAX_LIGHTS_PER_BUFFER];
    float        ambient              [MAX_LIGHTS_PER_BUFFER];
    float        diffuse              [MAX_LIGHTS_PER_BUFFER];
    float        specular             [MAX_LIGHTS_PER_BUFFER];
    float        reach                [MAX_LIGHTS_PER_BUFFER];
    float        red                  [MAX_LIGHTS_PER_BUFFER];
    float        green                [MAX_LIGHTS_PER_BUFFER];
    float        blue                 [MAX_LIGHTS_PER_BUFFER];
    unsigned int shadowcaster_i;
    unsigned int lights_size;
} GPULightCollection;

typedef struct GPUProjectionConstants {
    float znear;
    float zfar;
    float field_of_view_rad;
    float field_of_view_modifier;
    float x_multiplier;
    float y_multiplier;
    float padding[2];
} GPUProjectionConstants;

typedef struct GPUDownsamplingConstants
{
    float texture_width;
    float texture_height;
    float brightness_threshold;
    float brightness_reduction;
} GPUDownsamplingConstants;

typedef struct GPUPostProcessingConstants
{
    unsigned int timestamp;
    float screen_width;
    float screen_height;
    float nonblur_pct;
    float blur_pct;
} GPUPostProcessingConstants;

typedef struct GPURawVertex {
    float xyz[3];
    float color;
} GPURawVertex;

typedef struct PostProcessingVertex
{
    float position[2];
    float texcoord[2];
} PostProcessingVertex;

#pragma pack(pop)

#endif
