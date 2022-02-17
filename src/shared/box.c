#include "box.h"
#include "stdio.h"
#include "assert.h"
#include "platform_layer.h"

float near = 0.1f;
float far = 60.0f;
float z_normalisation;
float field_of_view = 90.0f;
float field_of_view_angle;
float field_of_view_rad;
float field_of_view_modifier;
float aspect_ratio;

void z_constants_init() {
    z_normalisation = far / (far - near);
    field_of_view_angle = field_of_view * 0.5f;
    field_of_view_rad =
        (field_of_view_angle / 180.0f) * 3.14159f;
    field_of_view_modifier =
        1.0f / tanf(field_of_view_rad);
    aspect_ratio =
        (float)WINDOW_HEIGHT / (float)WINDOW_WIDTH;
}

void print_line(char * multi_line_input) {

    uint32_t i = 0;
    while (multi_line_input[i] != '\n') {
        printf("%c", multi_line_input[i]);
        i++;
    }
    printf("\n");
}

uint32_t chars_till_next_space(
    char * buffer)
{
    uint32_t i = 0;

    while (buffer[i] != '\n' && buffer[i] != ' ') {
        i++;
    }

    return i;
}

uint32_t chars_till_next_nonspace(
    char * buffer)
{
    uint32_t i = 0;

    while (buffer[i] == ' ') {
        i++;
    }
    
    return i;
}

zPolygon * load_from_obj_file(char * filename) {
    
    zPolygon * return_value = malloc(sizeof(zPolygon));
    return_value->x = 0.0f;
    return_value->y = 0.0f;
    return_value->z = 40.0f;
    return_value->triangles_size = 0;
    
    FileBuffer * buffer = platform_read_file(filename);
    
    // pass through buffer once to read all vertices 
    zVertex new_vertices[5000];
    
    uint32_t i = 0;
    uint32_t new_vertex_i = 0;
    while (i < buffer->size) {
        
        // read the 1st character, which denominates the type
        // of information
        if (buffer->contents[i] == 'v') {
            // discard the 'v'
            i++;
            
            // read vertex data
            zVertex new_vertex;
            
            // skip the space(s) after the 'v'
            assert(buffer->contents[i] == ' ');
            i += chars_till_next_nonspace(buffer->contents + i);
            assert(buffer->contents[i] != ' ');
            
            // read vertex x
            new_vertex.x = atof(buffer->contents + i);

            // discard vertex x
            i += chars_till_next_space(buffer->contents + i);
            assert(buffer->contents[i] == ' ');

            // discard the spaces after vertex x
            i += chars_till_next_nonspace(buffer->contents + i);
            assert(buffer->contents[i] != ' ');
            
            // read vertex y
            new_vertex.y = atof(buffer->contents + i);
            i += chars_till_next_space(buffer->contents + i);
            assert(buffer->contents[i] == ' ');
            i += chars_till_next_nonspace(buffer->contents + i);
            assert(buffer->contents[i] != ' ');
            
            // read vertex z
            new_vertex.z = atof(buffer->contents + i);
            i += chars_till_next_space(buffer->contents + i);
            assert(buffer->contents[i] == '\n');
            i++;
            
            new_vertices[new_vertex_i] = new_vertex;
            assert(new_vertices[new_vertex_i].x = new_vertex.x);
            assert(new_vertices[new_vertex_i].y = new_vertex.y);
            assert(new_vertices[new_vertex_i].z = new_vertex.z);
            new_vertex_i++;
        } else {
            if (buffer->contents[i] == 'f') {
                return_value->triangles_size += 1;
            }
            // skip until the next line break character 
            while (buffer->contents[i] != '\n') {
                i++;
            }
            
            // skip the line break character
            i++;
        }
    }
    
    // pass through buffer->contents again to read all triangles 
    return_value->triangles =
        malloc(sizeof(zTriangle) * return_value->triangles_size);
    
    i = 0;
    uint32_t new_triangle_i = 0;
    while (i < buffer->size) {
        if (buffer->contents[i] == 'f') {
            print_line(buffer->contents + i);
            
            // discard the 'f'
            i++;
            assert(buffer->contents[i] == ' ');
            
            // skip the space(s) after the 'f'
            i += chars_till_next_nonspace(buffer->contents + i);
            assert(buffer->contents[i] != ' ');
            
            // read triangle data
            zTriangle new_triangle;
            
            // read 1st vertex index
            uint32_t vertex_i_0 = atoi(buffer->contents + i);
            printf("vertex_i_0: %u\n", vertex_i_0);
            i += chars_till_next_space(buffer->contents + i);
            assert(buffer->contents[i] == ' ');
            i += chars_till_next_nonspace(buffer->contents + i);
            assert(buffer->contents[i] != ' ');
            
            // read 2nd vertex index
            uint32_t vertex_i_1 = atoi(buffer->contents + i);
            printf("vertex_i_1: %u\n", vertex_i_1);
            i += chars_till_next_space(buffer->contents + i);
            assert(buffer->contents[i] == ' ');
            i += chars_till_next_nonspace(buffer->contents + i);
            assert(buffer->contents[i] != ' ');
            
            // read 3rd vertex index
            uint32_t vertex_i_2 = atoi(buffer->contents + i);
            printf("vertex_i_2: %u\n", vertex_i_2);
            i += chars_till_next_space(buffer->contents + i);
            assert(buffer->contents[i] == '\n');
            i++;
            
            assert(vertex_i_0 != vertex_i_1);
            assert(vertex_i_0 != vertex_i_2);
            assert(vertex_i_0 > 0);
            assert(vertex_i_1 > 0);
            assert(vertex_i_2 > 0);
            
            new_triangle.vertices[0] =
                new_vertices[vertex_i_0 - 1];
            new_triangle.vertices[1] =
                new_vertices[vertex_i_1 - 1];
            new_triangle.vertices[2] =
                new_vertices[vertex_i_2 - 1];
            
            return_value->triangles[new_triangle_i] =
                new_triangle;
            new_triangle_i++;
        } else {
            // skip until the next line break character 
            while (buffer->contents[i] != '\n') {
                i++;
            }
            
            // skip the line break character
            i++;
        }
    }
    
    free(buffer->contents);
    free(buffer);
    
    printf("triangles size: %u\n", return_value->triangles_size); 
    for (uint32_t i = 0; i < return_value->triangles_size; i++)
    {
        if (
            return_value->triangles[i].vertices[0].x ==
                return_value->triangles[i].vertices[1].x
            && return_value->triangles[i].vertices[0].y ==
                return_value->triangles[i].vertices[1].y
            && return_value->triangles[i].vertices[0].z ==
                return_value->triangles[i].vertices[1].z)
        {
            printf(
                "tri i: %u, {%f,%f,%f},{%f,%f,%f},{%f,%f,%f}\n",
                i,
                return_value->triangles[i].vertices[0].x,
                return_value->triangles[i].vertices[0].y,
                return_value->triangles[i].vertices[0].z,
                return_value->triangles[i].vertices[1].x,
                return_value->triangles[i].vertices[1].y,
                return_value->triangles[i].vertices[1].z,
                return_value->triangles[i].vertices[2].x,
                return_value->triangles[i].vertices[2].y,
                return_value->triangles[i].vertices[2].z);
            assert(0);
        }
        
        if (
            return_value->triangles[i].vertices[0].x ==
                return_value->triangles[i].vertices[2].x
            && return_value->triangles[i].vertices[0].y ==
                return_value->triangles[i].vertices[2].y
            && return_value->triangles[i].vertices[0].z ==
                return_value->triangles[i].vertices[2].z)
        {
            printf(
                "triangle i: %u, {%f,%f,%f},{%f,%f,%f},{%f,%f,%f}\n",
                i,
                return_value->triangles[i].vertices[0].x,
                return_value->triangles[i].vertices[0].y,
                return_value->triangles[i].vertices[0].z,
                return_value->triangles[i].vertices[1].x,
                return_value->triangles[i].vertices[1].y,
                return_value->triangles[i].vertices[1].z,
                return_value->triangles[i].vertices[2].x,
                return_value->triangles[i].vertices[2].y,
                return_value->triangles[i].vertices[2].z);
            assert(0);
        }
    }
    
    return return_value;
}

zPolygon * get_box() {
    zPolygon * box = malloc(sizeof(zPolygon));
    box->triangles_size = 6 * 2; // 6 faces, 2 per face
    
    box->triangles = malloc(
        sizeof(zTriangle) * box->triangles_size);
    
    box->x = 0.0;
    box->y = 0.0;
    box->z = 8.0f;
    box->x_angle = 0.5f;
    box->y_angle = 0.5f;
    box->z_angle = 0.5f;
    
    // SOUTH face
    box->triangles[0].vertices[0] =
        (zVertex){ 0.0f, 0.0f, 0.0f };
    box->triangles[0].vertices[1] =
        (zVertex){ 0.0f, 1.0f, 0.0f };
    box->triangles[0].vertices[2] =
        (zVertex){ 1.0f, 1.0f, 0.0f };
    
    box->triangles[1].vertices[0] =
        (zVertex){ 0.0f, 0.0f, 0.0f };
    box->triangles[1].vertices[1] =
        (zVertex){ 1.0f, 1.0f, 0.0f };
    box->triangles[1].vertices[2] =
        (zVertex){ 1.0f, 0.0f, 0.0f };
    
    // EAST face
    box->triangles[2].vertices[0] =
        (zVertex){ 1.0f, 0.0f, 0.0f };
    box->triangles[2].vertices[1] =
        (zVertex){ 1.0f, 1.0f, 0.0f };
    box->triangles[2].vertices[2] =
        (zVertex){ 1.0f, 1.0f, 1.0f };
    
    box->triangles[3].vertices[0] =
        (zVertex){ 1.0f, 0.0f, 0.0f };
    box->triangles[3].vertices[1] =
        (zVertex){ 1.0f, 1.0f, 1.0f };
    box->triangles[3].vertices[2] =
        (zVertex){ 1.0f, 0.0f, 1.0f };
    
    // NORTH face
    box->triangles[4].vertices[0] =
        (zVertex){ 1.0f, 0.0f, 1.0f };
    box->triangles[4].vertices[1] =
        (zVertex){ 1.0f, 1.0f, 1.0f };
    box->triangles[4].vertices[2] =
        (zVertex){ 0.0f, 1.0f, 1.0f };
    
    box->triangles[5].vertices[0] =
        (zVertex){ 1.0f, 0.0f, 1.0f };
    box->triangles[5].vertices[1] =
        (zVertex){ 0.0f, 1.0f, 1.0f };
    box->triangles[5].vertices[2] =
        (zVertex){ 0.0f, 0.0f, 1.0f };
    
    // WEST face
    box->triangles[6].vertices[0] =
        (zVertex){ 0.0f, 0.0f, 1.0f };
    box->triangles[6].vertices[1] =
        (zVertex){ 0.0f, 1.0f, 1.0f };
    box->triangles[6].vertices[2] =
        (zVertex){ 0.0f, 1.0f, 0.0f };
    
    box->triangles[7].vertices[0] =
        (zVertex){ 0.0f, 0.0f, 1.0f };
    box->triangles[7].vertices[1] =
        (zVertex){ 0.0f, 1.0f, 0.0f };
    box->triangles[7].vertices[2] =
        (zVertex){ 0.0f, 0.0f, 0.0f };
    
    // TOP face
    box->triangles[8].vertices[0] =
        (zVertex){ 0.0f, 1.0f, 0.0f };
    box->triangles[8].vertices[1] =
        (zVertex){ 0.0f, 1.0f, 1.0f };
    box->triangles[8].vertices[2] =
        (zVertex){ 1.0f, 1.0f, 1.0f };
    
    box->triangles[9].vertices[0] =
        (zVertex){ 0.0f, 1.0f, 0.0f };
    box->triangles[9].vertices[1] =
        (zVertex){ 1.0f, 1.0f, 1.0f };
    box->triangles[9].vertices[2] =
        (zVertex){ 1.0f, 1.0f, 0.0f };
    
    // BOTTOM face
    box->triangles[10].vertices[0] =
        (zVertex){ 1.0f, 0.0f, 1.0f };
    box->triangles[10].vertices[1] =
        (zVertex){ 0.0f, 0.0f, 1.0f };
    box->triangles[10].vertices[2] =
        (zVertex){ 0.0f, 0.0f, 0.0f };
    
    box->triangles[11].vertices[0] =
        (zVertex){ 1.0f, 0.0f, 1.0f };
    box->triangles[11].vertices[1] =
        (zVertex){ 0.0f, 0.0f, 0.0f };
    box->triangles[11].vertices[2] =
        (zVertex){ 1.0f, 0.0f, 0.0f };
    
    return box;
}

void ztriangle_to_2d(
    ColoredVertex recipient[3],
    zTriangle * input,
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
            input->vertices[i].z
            * ((far / far - near) - (far * near / far - near));
        
        recipient[i].XY[0] =
            (aspect_ratio
            * field_of_view_modifier
            * input->vertices[i].x); 
        
        if (recipient[i].XY[0] != 0.0f
            && z_modifier != 0.0f)
        {
            recipient[i].XY[0] /= z_modifier;
        }
        
        // note to self: y transformation doesn't use aspect
        // ratio
        recipient[i].XY[1] =
            field_of_view_modifier
            * input->vertices[i].y;
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
        // X = x
        return_value.vertices[i].x =
            input->vertices[i].x;
        
        // Y = y*cos(theta) - z*sin(theta); 
        return_value.vertices[i].y =
            (input->vertices[i].y
                * cosf(angle))
            - (input->vertices[i].z
                * sinf(angle));
        
        // Z = y*sin(theta) + z*cos(theta);
        return_value.vertices[i].z =
            (input->vertices[i].y
                * sinf(angle)) +
            (input->vertices[i].z
                * cosf(angle));
    }
    
    return return_value;
}

zTriangle z_rotate_triangle(
    const zTriangle * input,
    const float angle)
{
    zTriangle return_value;
    
    for(
        uint32_t i = 0;
        i < 3;
        i++)
    {
        // Z = z; 
        return_value.vertices[i].z =
            input->vertices[i].z;
       
        // X = x*cos(theta) - y*sin(theta);
        return_value.vertices[i].x =
            (input->vertices[i].x
                * cosf(angle))
            - (input->vertices[i].y
                * sinf(angle));
        
        // Y = x*sin(theta) + y*cos(theta);
        return_value.vertices[i].y =
            (input->vertices[i].y
                * cosf(angle))
            + (input->vertices[i].x
                * sinf(angle));
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
        // Y = y;
        return_value.vertices[i].y =
            input->vertices[i].y;
        
        // X = x*cos(theta) + z*sin(theta);
        return_value.vertices[i].x =
            (input->vertices[i].x
                * cosf(angle))
            + (input->vertices[i].z
                * sinf(angle));

        // Z = z*cos(theta) - x*sin(theta);
        return_value.vertices[i].z =
            (input->vertices[i].z
                * cosf(angle))
            - (input->vertices[i].x
                * sinf(angle));
    }
    
    return return_value;
}

zTriangle translate_ztriangle(
    const zTriangle * input,
    const float by_x,
    const float by_y,
    const float by_z)
{
    zTriangle return_value;
    
    for (uint32_t i = 0; i < 3; i++) {
        return_value.vertices[i].x =
            input->vertices[i].x + by_x;
        return_value.vertices[i].y =
            input->vertices[i].y + by_y;
        return_value.vertices[i].z =
            input->vertices[i].z + by_z;
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
                for (
                    uint32_t k = i;
                    k > j;
                    k--)
                {
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

