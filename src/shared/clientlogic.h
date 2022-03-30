#ifndef CLIENTLOGIC_H
#define CLIENTLOGIC_H

#include "common.h"
#include "zpolygon.h"
#include "vertex_types.h"
#include "decodedimage.h"
#include "platform_layer.h"
#include "decode_png.h"
#include "userinput.h"

/*
Prepare your objects for 3D rendering

Write code to update your game or app's state
*/

zPolygon * load_from_obj_file(char * filename);

// A buffer of zPolygon objects that should be rendered
// in your application
// index 0 to zpolygons_to_render_size will be rendered,
// the rest of the array will be ignored
extern zPolygon * zpolygons_to_render[1000];
extern uint32_t zpolygons_to_render_size;

// A buffer of zLightSources to light up your scene(s)
// index 0 to zlights_to_apply_size will be rendered,
// the rest of the array will be ignored
extern zLightSource zlights_to_apply[50];
extern uint32_t zlights_to_apply_size;

// texquads_to_render is an array of flat 2D textures to
// display on the screen without applying any 3D transformations.
// You can update a texture's contents and push it to the gpu
// with the function platform_update_gpu_texture() in
// combination with this to achieve 2D animation
typedef struct TexQuad {
    uint32_t texturearray_i;
    uint32_t texture_i;
    float left;
    float top;
    float height;
    float width;
    bool32_t visible;
} TexQuad;
#define TEXQUADS_TO_RENDER_ARRAYSIZE 2
extern TexQuad texquads_to_render[TEXQUADS_TO_RENDER_ARRAYSIZE];
extern uint32_t texquads_to_render_size;

// These are 2 images that we're not going to read from disk,
// but just write to ourselves by setting the pixels
#define MINIMAP_PIXELS_WIDTH 100
extern DecodedImage minimap;
extern DecodedImage minimap2;

// A buffer of texture arrays (AKA texture atlases) your
// objects can use
// Each texture atlas must have images of the exact same size
// You can set a zTriangle's texturearray_i to 2 to use
// texture_arrays[2] as its texture during texture mapping
// Set the zTriangle's texture_i to select which texture inside
// the texture atlas to use
// REMINDER: You must define TEXTUREARRAYS_SIZE in vertex_types.h
typedef struct TextureArray {
    DecodedImage * image;
    uint32_t sprite_columns;
    uint32_t sprite_rows;
} TextureArray;
extern TextureArray texture_arrays[TEXTUREARRAYS_SIZE];

// will be called once at startup, before rendering frame 1
// add your app's code here
void client_logic_startup();

// will be called once per frame, before rendering that frame
// add your app's code here
void client_logic_update();

#endif

