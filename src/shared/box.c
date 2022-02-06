#include "box.h"
#include "stdio.h"
#include "assert.h"

void z_constants_init() {
    near = 0.1f;
    far = 100.0f;
    z_normalisation = far / (far - near);
    field_of_view = 90.0f;
    field_of_view_angle = field_of_view * 0.5f;
    field_of_view_rad =
        (field_of_view_angle / 180.0f) * 3.14159f;
    field_of_view_modifier =
        1.0f / tanf(field_of_view_rad);
    aspect_ratio =
        (float)WINDOW_HEIGHT / (float)WINDOW_WIDTH;
}

zPolygon * get_box() {
    zPolygon * box = malloc(sizeof(zPolygon));
    box->triangles_size = 6 * 2; // 6 faces, 2 per face
    
    box->triangles = malloc(
        sizeof(zTriangle) * box->triangles_size);
    
    box->x = 0.2f;
    box->y = 0.4f;
    box->z = 5.0f;
    box->x_angle = 0.0f;
    box->y_angle = 0.0f;
    box->z_angle = 0.0f;
    
    // north face
    // (exactly the same values as south,
    // except z is always distant instead of close
    box->triangles[0].vertices[0] =
        (zPolygonVertex){ 0.0f, 0.0f, 1.0f };
    box->triangles[0].vertices[1] =
        (zPolygonVertex){ 0.0f, 1.0f, 1.0f };
    box->triangles[0].vertices[2] =
        (zPolygonVertex){ 1.0f, 1.0f, 1.0f };
    
    box->triangles[1].vertices[0] =
        (zPolygonVertex){ 0.0f, 0.0f, 1.0f };
    box->triangles[1].vertices[1] =
        (zPolygonVertex){ 1.0f, 1.0f, 1.0f };
    box->triangles[1].vertices[2] =
        (zPolygonVertex){ 1.0f, 0.0f, 1.0f };
    
    // bottom face
    box->triangles[2].vertices[0] =
        (zPolygonVertex){ 1.0f, 0.0f, 1.0f };
    box->triangles[2].vertices[1] =
        (zPolygonVertex){ 0.0f, 0.0f, 1.0f };
    box->triangles[2].vertices[2] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    
    box->triangles[3].vertices[0] =
        (zPolygonVertex){ 1.0f, 0.0f, 1.0f };
    box->triangles[3].vertices[1] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    box->triangles[3].vertices[2] =
        (zPolygonVertex){ 1.0f, 0.0f, 0.0f };
    
    // west face (like east, but x is always 0 instead of 1)
    box->triangles[4].vertices[0] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    box->triangles[4].vertices[1] =
        (zPolygonVertex){ 0.0f, 1.0f, 0.0f };
    box->triangles[4].vertices[2] =
        (zPolygonVertex){ 0.0f, 1.0f, 1.0f };
    
    box->triangles[5].vertices[0] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    box->triangles[5].vertices[1] =
        (zPolygonVertex){ 0.0f, 1.0f, 1.0f };
    box->triangles[5].vertices[2] =
        (zPolygonVertex){ 0.0f, 0.0f, 1.0f };
    
    // east face
    box->triangles[6].vertices[0] =
        (zPolygonVertex){ 1.0f, 0.0f, 0.0f };
    box->triangles[6].vertices[1] =
        (zPolygonVertex){ 1.0f, 1.0f, 0.0f };
    box->triangles[6].vertices[2] =
        (zPolygonVertex){ 1.0f, 1.0f, 1.0f };
    
    box->triangles[7].vertices[0] =
        (zPolygonVertex){ 1.0f, 0.0f, 0.0f };
    box->triangles[7].vertices[1] =
        (zPolygonVertex){ 1.0f, 1.0f, 1.0f };
    box->triangles[7].vertices[2] =
        (zPolygonVertex){ 1.0f, 0.0f, 1.0f };
    
    // top face
    box->triangles[8].vertices[0] =
        (zPolygonVertex){ 0.0f, 1.0f, 0.0f };
    box->triangles[8].vertices[1] =
        (zPolygonVertex){ 0.0f, 1.0f, 1.0f };
    box->triangles[8].vertices[2] =
        (zPolygonVertex){ 1.0f, 1.0f, 1.0f };
    
    box->triangles[9].vertices[0] =
        (zPolygonVertex){ 0.0f, 1.0f, 0.0f };
    box->triangles[9].vertices[1] =
        (zPolygonVertex){ 1.0f, 1.0f, 1.0f };
    box->triangles[9].vertices[2] =
        (zPolygonVertex){ 1.0f, 1.0f, 0.0f };
    
    // box's south face (2 triangles)
    //                      x     y     z
    box->triangles[10].vertices[0] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    box->triangles[10].vertices[1] =
        (zPolygonVertex){ 0.0f, 1.0f, 0.0f };
    box->triangles[10].vertices[2] =
        (zPolygonVertex){ 1.0f, 1.0f, 0.0f };
    
    box->triangles[11].vertices[0] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    box->triangles[11].vertices[1] =
        (zPolygonVertex){ 1.0f, 1.0f, 0.0f };
    box->triangles[11].vertices[2] =
        (zPolygonVertex){ 1.0f, 0.0f, 0.0f };
    
    return box;
}

void ztriangle_to_2d(
    ColoredVertex recipient[3],
    zTriangle * input,
    float x_offset,
    float y_offset,
    float z_offset,
    simd_float4 color)
{
    for (uint32_t i = 0; i < 3; i++) {
        // final formula to project something {x, y, z} to
        // 2D screen:
        // *z part is not necessary yet but will be coming up
        // x = (aspect_ratio * field_of_view_modifier * x) / z;
        // y = (aspect_ratio * field_of_view_modifier * x) / z;
        // z = (z * z_normalisation) - (z * near);
        float z_modifier =
            (input->vertices[i].z + z_offset)
            * ((far / far - near) - (far * near / far - near));
        
        recipient[i].XY[0] =
            (aspect_ratio
            * field_of_view_modifier
            * (input->vertices[i].x + x_offset)); 
        
        if (recipient[i].XY[0] != 0.0f
            && z_modifier != 0.0f)
        {
            recipient[i].XY[0] /= z_modifier;
        }
        
        // note to self: y transformation doesn't use aspect
        // ratio
        recipient[i].XY[1] =
            field_of_view_modifier
            * (input->vertices[i].y + y_offset);
        if (recipient[i].XY[1] != 0.0f
            && z_modifier != 0.0f)
        {
            recipient[i].XY[1] /= z_modifier;
        }
        
        recipient[i].RGBA = color / ((i * 0.25f) + 1);
    }
}

void free_zpolygon(
    zPolygon * to_free)
{
    free(to_free->triangles);
    free(to_free);
}

zTriangle x_rotate_triangle(
    const zTriangle * input,
    const float angle)
{
    zTriangle return_value;
    
    for (
        uint32_t i = 0;
        i < 3;
        i++)
    {
        return_value.vertices[i].x =
            input->vertices[i].x;

        return_value.vertices[i].y =
            (input->vertices[i].y
                * cos(angle))
            - (input->vertices[i].z
                * sin(angle));
        return_value.vertices[i].z =
            (input->vertices[i].z
                * cos(angle))
            + (input->vertices[i].y
                * sin(angle));
    }
    
    return return_value;
}

zTriangle z_rotate_triangle(
    const zTriangle * input,
    const float angle)
{
    zTriangle return_value;
    
    for (
        uint32_t i = 0;
        i < 3;
        i++)
    {
        return_value.vertices[i].z =
            input->vertices[i].z;
        
        return_value.vertices[i].x =
            (input->vertices[i].x
                * cos(angle))
            - (input->vertices[i].y
                * sin(angle));
        return_value.vertices[i].y =
            (input->vertices[i].y
                * cos(angle))
            + (input->vertices[i].x
                * sin(angle));
    }
    
    return return_value;
}

zTriangle y_rotate_triangle(
    const zTriangle * input,
    const float angle)
{
    zTriangle return_value;
    
    for (
        uint32_t i = 0;
        i < 3;
        i++)
    {
        return_value.vertices[i].y =
            input->vertices[i].y;

        return_value.vertices[i].x =
            (input->vertices[i].x
                * cos(angle))
            - (input->vertices[i].z
                * sin(angle));
        return_value.vertices[i].z =
            (input->vertices[i].z
                * cos(angle))
            + (input->vertices[i].y
                * sin(angle));
    }
    
    return return_value;
}

float get_avg_z(
    const zTriangle * of_triangle)
{
    return (of_triangle->vertices[0].z +
        of_triangle->vertices[1].z +
        of_triangle->vertices[2].z) / 3.0f;
}

void z_sort(
    zTriangle * triangles,
    const uint32_t triangles_size)
{
    zTriangle swap;
   
    for (uint32_t i = 0; i < triangles_size; i++) { 
        
        for (uint32_t j = 0; j < i; j ++)
        {
            if (get_avg_z(triangles + i)
                < get_avg_z(triangles + j))
            {
                swap.vertices[0] =
                    triangles[i].vertices[0];
                swap.vertices[1] =
                    triangles[i].vertices[1];
                swap.vertices[2] =
                    triangles[i].vertices[2];
                
                assert(i > j);
                for (uint32_t k = i; k > j; k--) {
                    triangles[k].vertices[0] =
                        triangles[k-1].vertices[0];
                    triangles[k].vertices[1] =
                        triangles[k-1].vertices[1];
                    triangles[k].vertices[2] =
                        triangles[k-1].vertices[2];
                }
                
                triangles[j].vertices[0] = swap.vertices[0];
                triangles[j].vertices[1] = swap.vertices[1];
                triangles[j].vertices[2] = swap.vertices[2];
                
                break;
            }
        }
    }
}

