#include "box.h"
#include "stdio.h"
#include "assert.h"

void z_constants_init() {
    near = 0.1f;
    far = 1000.0f;
    z_normalisation = far / (far - near);
    field_of_view = 90.0f;
    field_of_view_angle = field_of_view * 0.5f;
    field_of_view_rad =
        (field_of_view_angle / 180.0f) * 3.14159f;
    field_of_view_modifier =
        1.0f / tanf(field_of_view_rad);
    assert(field_of_view_modifier < 1.05);
    assert(field_of_view_modifier > 0.95f);
    aspect_ratio = 1.0f;
    /*
    aspect_ratio =
        (float)WINDOW_HEIGHT / (float)WINDOW_WIDTH;
    */
}

zPolygon * get_box() {
    zPolygon * box = malloc(sizeof(zPolygon));
    box->triangles_size = 6 * 2; // 6 faces, 2 per face
    box->vertices_size = box->triangles_size * 3;
    box->triangle_vertices = malloc(
        sizeof(zPolygonVertex) * box->vertices_size);
    
    box->x = 50.0f;
    box->y = 500.0f;
    box->z = 1.5f;

    // north face
    // (exactly the same values as south,
    // except z is always distant instead of close
    box->triangle_vertices[0] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.5f };
    box->triangle_vertices[1] =
        (zPolygonVertex){ 0.0f, 250.0f, 0.5f };
    box->triangle_vertices[2] =
        (zPolygonVertex){ 250.0f, 250.0f, 0.5f };
    
    box->triangle_vertices[3] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.5f };
    box->triangle_vertices[4] =
        (zPolygonVertex){ 250.0f, 250.0f, 0.5f };
    box->triangle_vertices[5] =
        (zPolygonVertex){ 250.0f, 0.0f, 0.5f };
    
    // bottom face
    box->triangle_vertices[6] =
        (zPolygonVertex){ 250.0f, 0.0f, 0.5f };
    box->triangle_vertices[7] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.5f };
    box->triangle_vertices[8] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    
    box->triangle_vertices[9] =
        (zPolygonVertex){ 250.0f, 0.0f, 0.5f };
    box->triangle_vertices[10] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    box->triangle_vertices[11] =
        (zPolygonVertex){ 250.0f, 0.0f, 0.0f };
    
    // west face (like east, but x is always 0 instead of 1)
    box->triangle_vertices[12] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    box->triangle_vertices[13] =
        (zPolygonVertex){ 0.0f, 250.0f, 0.0f };
    box->triangle_vertices[14] =
        (zPolygonVertex){ 0.0f, 250.0f, 0.5f };
    
    box->triangle_vertices[15] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    box->triangle_vertices[16] =
        (zPolygonVertex){ 0.0f, 250.0f, 0.5f };
    box->triangle_vertices[17] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.5f };
    
    // east face
    box->triangle_vertices[18] =
        (zPolygonVertex){ 250.0f, 0.0f, 0.0f };
    box->triangle_vertices[19] =
        (zPolygonVertex){ 250.0f, 250.0f, 0.0f };
    box->triangle_vertices[20] =
        (zPolygonVertex){ 250.0f, 250.0f, 0.5f };
    
    box->triangle_vertices[21] =
        (zPolygonVertex){ 250.0f, 0.0f, 0.0f };
    box->triangle_vertices[22] =
        (zPolygonVertex){ 250.0f, 250.0f, 0.5f };
    box->triangle_vertices[23] =
        (zPolygonVertex){ 250.0f, 0.0f, 0.5f };
    
    // top face
    box->triangle_vertices[24] =
        (zPolygonVertex){ 0.0f, 250.0f, 0.0f };
    box->triangle_vertices[25] =
        (zPolygonVertex){ 0.0f, 250.0f, 0.5f };
    box->triangle_vertices[26] =
        (zPolygonVertex){ 250.0f, 250.0f, 0.5f };
    
    box->triangle_vertices[27] =
        (zPolygonVertex){ 0.0f, 250.0f, 0.0f };
    box->triangle_vertices[28] =
        (zPolygonVertex){ 250.0f, 250.0f, 0.5f };
    box->triangle_vertices[29] =
        (zPolygonVertex){ 250.0f, 250.0f, 0.0f };
    
    // box's south face (2 triangles)
    //                      x     y     z
    box->triangle_vertices[30] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    box->triangle_vertices[31] =
        (zPolygonVertex){ 0.0f, 250.0f, 0.0f };
    box->triangle_vertices[32] =
        (zPolygonVertex){ 250.0f, 250.0f, 0.0f };
    
    box->triangle_vertices[33] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    box->triangle_vertices[34] =
        (zPolygonVertex){ 250.0f, 250.0f, 0.0f };
    box->triangle_vertices[35] =
        (zPolygonVertex){ 250.0f, 0.0f, 0.0f };

    return box;
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

void x_rotate_zpolygon(
    zPolygon * to_rotate,
    const float angle)
{
    for (
        uint32_t i = 0;
        i < to_rotate->vertices_size;
        i += 1)
    {
        to_rotate->triangle_vertices[i].y =
            (to_rotate->triangle_vertices[i].y * cos(angle))
            - (to_rotate->triangle_vertices[i].z * sin(angle));
        to_rotate->triangle_vertices[i].z =
            (to_rotate->triangle_vertices[i].z * cos(angle))
            + (to_rotate->triangle_vertices[i].y * sin(angle));
    }
}

