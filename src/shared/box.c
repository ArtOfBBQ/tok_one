#include "box.h"
#include "stdio.h"
#include "assert.h"

void z_constants_init() {
    near = 0.1f;
    far = 100000.0f;
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
    box->vertices_size = box->triangles_size * 3;
    box->triangle_vertices = malloc(
        sizeof(zPolygonVertex) * box->vertices_size);
    
    box->x = 0.2f;
    box->y = 0.4f;
    box->z = 5.0f;
    
    // north face
    // (exactly the same values as south,
    // except z is always distant instead of close
    box->triangle_vertices[0] =
        (zPolygonVertex){ 0.0f, 0.0f, 1.0f };
    box->triangle_vertices[1] =
        (zPolygonVertex){ 0.0f, 1.0f, 1.0f };
    box->triangle_vertices[2] =
        (zPolygonVertex){ 1.0f, 1.0f, 1.0f };
    
    box->triangle_vertices[3] =
        (zPolygonVertex){ 0.0f, 0.0f, 1.0f };
    box->triangle_vertices[4] =
        (zPolygonVertex){ 1.0f, 1.0f, 1.0f };
    box->triangle_vertices[5] =
        (zPolygonVertex){ 1.0f, 0.0f, 1.0f };
    
    // bottom face
    box->triangle_vertices[6] =
        (zPolygonVertex){ 1.0f, 0.0f, 1.0f };
    box->triangle_vertices[7] =
        (zPolygonVertex){ 0.0f, 0.0f, 1.0f };
    box->triangle_vertices[8] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    
    box->triangle_vertices[9] =
        (zPolygonVertex){ 1.0f, 0.0f, 1.0f };
    box->triangle_vertices[10] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    box->triangle_vertices[11] =
        (zPolygonVertex){ 1.0f, 0.0f, 0.0f };
    
    // west face (like east, but x is always 0 instead of 1)
    box->triangle_vertices[12] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    box->triangle_vertices[13] =
        (zPolygonVertex){ 0.0f, 1.0f, 0.0f };
    box->triangle_vertices[14] =
        (zPolygonVertex){ 0.0f, 1.0f, 1.0f };
    
    box->triangle_vertices[15] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    box->triangle_vertices[16] =
        (zPolygonVertex){ 0.0f, 1.0f, 1.0f };
    box->triangle_vertices[17] =
        (zPolygonVertex){ 0.0f, 0.0f, 1.0f };
    
    // east face
    box->triangle_vertices[18] =
        (zPolygonVertex){ 1.0f, 0.0f, 0.0f };
    box->triangle_vertices[19] =
        (zPolygonVertex){ 1.0f, 1.0f, 0.0f };
    box->triangle_vertices[20] =
        (zPolygonVertex){ 1.0f, 1.0f, 1.0f };
    
    box->triangle_vertices[21] =
        (zPolygonVertex){ 1.0f, 0.0f, 0.0f };
    box->triangle_vertices[22] =
        (zPolygonVertex){ 1.0f, 1.0f, 1.0f };
    box->triangle_vertices[23] =
        (zPolygonVertex){ 1.0f, 0.0f, 1.0f };
    
    // top face
    box->triangle_vertices[24] =
        (zPolygonVertex){ 0.0f, 1.0f, 0.0f };
    box->triangle_vertices[25] =
        (zPolygonVertex){ 0.0f, 1.0f, 1.0f };
    box->triangle_vertices[26] =
        (zPolygonVertex){ 1.0f, 1.0f, 1.0f };
    
    box->triangle_vertices[27] =
        (zPolygonVertex){ 0.0f, 1.0f, 0.0f };
    box->triangle_vertices[28] =
        (zPolygonVertex){ 1.0f, 1.0f, 1.0f };
    box->triangle_vertices[29] =
        (zPolygonVertex){ 1.0f, 1.0f, 0.0f };
    
    // box's south face (2 triangles)
    //                      x     y     z
    box->triangle_vertices[30] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    box->triangle_vertices[31] =
        (zPolygonVertex){ 0.0f, 1.0f, 0.0f };
    box->triangle_vertices[32] =
        (zPolygonVertex){ 1.0f, 1.0f, 0.0f };
    
    box->triangle_vertices[33] =
        (zPolygonVertex){ 0.0f, 0.0f, 0.0f };
    box->triangle_vertices[34] =
        (zPolygonVertex){ 1.0f, 1.0f, 0.0f };
    box->triangle_vertices[35] =
        (zPolygonVertex){ 1.0f, 0.0f, 0.0f };
    
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
    for (int i = 0; i < 3; i++) {
        
        float z_modifier =
            (input[i].z + z_offset)
            * ((far / far - near) - (far * near / far - near));
        
        recipient[i].XY[0] =
            (aspect_ratio
            * field_of_view_modifier
            * (input[i].x + x_offset)); 
        
        if (recipient[i].XY[0] != 0.0f
            && z_modifier != 0.0f)
        {
            recipient[i].XY[0] /= z_modifier;
        }
        
        // note to self: y transformation doesn't use aspect
        // ratio
        recipient[i].XY[1] =
            field_of_view_modifier
            * (input[i].y + y_offset);
        if (recipient[i].XY[1] != 0.0f
            && z_modifier != 0.0f) {
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

void y_rotate_zpolygon(
    zPolygon * to_rotate,
    const float angle)
{
    for (
        uint32_t i = 0;
        i < to_rotate->vertices_size;
        i += 1)
    {
        to_rotate->triangle_vertices[i].x =
            (to_rotate->triangle_vertices[i].x * cos(angle))
            - (to_rotate->triangle_vertices[i].z * sin(angle));
        to_rotate->triangle_vertices[i].z =
            (to_rotate->triangle_vertices[i].z * cos(angle))
            + (to_rotate->triangle_vertices[i].x * sin(angle));
    }
}


float get_avg_z(
    zPolygon * of_zpolygon,
    uint32_t at_i)
{
    return (of_zpolygon->triangle_vertices[at_i].z +
        of_zpolygon->triangle_vertices[at_i+1].z +
        of_zpolygon->triangle_vertices[at_i+2].z) / 3.0f;
}


void z_sort(
    zPolygon * to_sort)
{
    // we want the triangles with the highest z
    // to be drawn first, so they need to have the
    // lowest index
    //
    // as a hack, let's just use the average z of 3
    // vertices


    // TODO: this function should leave all x's and y's and
    // z's as is,
    // but just in a different order
    // delete this checksum (and again at the end) if it doesnt
    // bug for a while
    uint32_t x_checksum = 0;
    uint32_t y_checksum = 0;
    uint32_t z_checksum = 0;
    for (int i = 0; i < to_sort->vertices_size; i++) {
        x_checksum += to_sort->triangle_vertices[i].x;
        y_checksum += to_sort->triangle_vertices[i].y;
        z_checksum += to_sort->triangle_vertices[i].z;
    }
    
    /*
    */
    zPolygonVertex swap[3];
    uint32_t i = 0;
    while (i < (to_sort->vertices_size - 3))
    {
        i += 3;
        printf("i : %u\n", i);
        
        for (uint32_t j = 0; j < i; j += 3)
        {
            printf("j : %u\n", j);
            if (get_avg_z(to_sort, i)
                >= get_avg_z(to_sort, j))
            {
                printf(
                    "get_avg_z(to_sort, %u) is %f, bigger than get_avg_z(to_sort, %u) of %f, next j...\n",
                    i,
                    get_avg_z(to_sort, i),
                    j,
                    get_avg_z(to_sort, j));
            } else {
                printf(
                    "i is smaller z than j, insert to %u...\n",
                    j);
                swap[0] = to_sort->triangle_vertices[i];
                swap[1] = to_sort->triangle_vertices[i+1];
                swap[2] = to_sort->triangle_vertices[i+2];
                
                assert(i > j);
                for (uint32_t k = i; k > j; k -= 3) {
                    to_sort->triangle_vertices[k] =
                        to_sort->triangle_vertices[k-3];
                    to_sort->triangle_vertices[k+1] =
                        to_sort->triangle_vertices[k-2];
                    to_sort->triangle_vertices[k+2] =
                        to_sort->triangle_vertices[k-1];
                }
                to_sort->triangle_vertices[j] = swap[0];
                to_sort->triangle_vertices[j+1] = swap[1];
                to_sort->triangle_vertices[j+2] = swap[2];
                
                for (
                    uint32_t check_i = 0;
                    check_i < i - 3;
                    check_i += 3)
                {
                    printf(
                        "triangle_vertices[%u]'s avg z:%f\n",
                        check_i,
                        get_avg_z(to_sort, check_i));
                    printf(
                        "triangle_vertices[%u]'s avg z:%f\n",
                        check_i + 3,
                        get_avg_z(to_sort, check_i + 3));
                    assert(
                        get_avg_z(to_sort, check_i)
                        <= get_avg_z(to_sort, check_i + 3));
                }
                
                assert(
                    get_avg_z(to_sort, i)
                    > get_avg_z(to_sort, j));
                break;
            }
        }    
    }
    
    for (
        uint32_t i = 0;
        i < to_sort->vertices_size - 3;
        i += 3)
    {
        assert(
            get_avg_z(to_sort, i) <=
            get_avg_z(to_sort, i + 3));
    }
    
    // TODO: delete this checksum (and also at the beginning of
    // the function)
    uint32_t after_x_checksum = 0;
    uint32_t after_y_checksum = 0;
    uint32_t after_z_checksum = 0;
    for (int i = 0; i < to_sort->vertices_size; i++) {
        after_x_checksum += to_sort->triangle_vertices[i].x;
        after_y_checksum += to_sort->triangle_vertices[i].y;
        after_z_checksum += to_sort->triangle_vertices[i].z;
    }
    assert(x_checksum == after_x_checksum);
    assert(y_checksum == after_y_checksum);
    assert(z_checksum == after_z_checksum);
}

