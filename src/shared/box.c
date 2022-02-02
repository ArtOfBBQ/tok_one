#include "box.h"
#include "stdio.h"
#include "assert.h"

void z_constants_init() {
    near = 0.1f;
    far = 1000.0f;
    z_normalisation = far / (far - near);
    printf("z_normalisation: %f\n", z_normalisation);
    field_of_view = 90.0f;
    field_of_view_angle = field_of_view * 0.5f;
    field_of_view_rad =
        (field_of_view_angle / 180.0f) * 3.14159f;
    printf(
        "field_of_view_rad: %f\n",
        field_of_view_rad);
    field_of_view_modifier =
        1.0f / tanf(field_of_view_rad);
    assert(field_of_view_modifier < 1.05);
    assert(field_of_view_modifier > 0.95f);
    printf(
        "field_of_view_modifier: %f\n",
        field_of_view_modifier);
    aspect_ratio = 1.0f;
    /*
    aspect_ratio =
        (float)WINDOW_HEIGHT / (float)WINDOW_WIDTH;
    */
    printf(
        "aspect_ratio: %f\n",
        aspect_ratio);
}

void ztriangle_to_2d(
    ColoredVertex recipient[3],
    zPolygonVertex * input,
    float x_offset,
    float y_offset,
    float z_offset,
    simd_float4 color)
{
    // final formula to project something {x, y, z} to 2D screen:
    // *z part is not necessary yet but will be coming up
    // x = (aspect_ratio * field_of_view_modifier * x) / z;
    // y = (aspect_ratio * field_of_view_modifier * x) / z;
    // z = (z * z_normalisation) - (z * near);
    assert(field_of_view_modifier < 1.05f);
    assert(field_of_view_modifier > 0.95f);
    assert(aspect_ratio < 1.05f);
    assert(aspect_ratio > 0.95f);
    for (int i = 0; i < 3; i++) {
        
        float z_modifier = input[i].z + z_offset;
        if (z_modifier == 0.0f) { z_modifier = 0.001f; }
        assert(z_modifier != 0.0f);
        
        recipient[i].XY[0] =
            (aspect_ratio
            * field_of_view_modifier
            * (input[i].x + x_offset)); 
        
        if (recipient[i].XY[0] != 0.0f) {
            recipient[i].XY[0] /= z_modifier;
        }
        
        recipient[i].XY[1] =
            (aspect_ratio
            * field_of_view_modifier
            * (input[i].y + y_offset));
        if (recipient[i].XY[1] != 0.0f) {
            recipient[i].XY[1] /= z_modifier;
        }
        
        recipient[i].RGBA = color;
    }
}

void free_zpolygon(
    zPolygon * to_free)
{
    free(to_free->triangle_vertices);
    free(to_free->triangle_colors);
    free(to_free);
}

