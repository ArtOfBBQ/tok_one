#include "zpolygon.h"

ProjectionConstants projection_constants = {};

void init_projection_constants() {

    if (window_height < 50.0f || window_width < 50.0f) {
        printf("ERROR: unexpected window size [%f,%f]\n",
            window_height,
            window_width);
        assert(0);
    }
    
    ProjectionConstants * pjc = &projection_constants;
    
    pjc->near = 0.5f;
    pjc->far = 150.0f;
    pjc->field_of_view = 90.0f;
    pjc->z_normalisation =
        pjc->far /
            (pjc->far -
                pjc->near);
    
    pjc->field_of_view_angle =
        pjc->field_of_view * 0.5f;
    pjc->field_of_view_rad =
        (pjc->field_of_view_angle / 180.0f)
            * 3.14159f;
    
    pjc->field_of_view_modifier =
        1.0f / tanf(pjc->field_of_view_rad);
    pjc->aspect_ratio =
        window_height / window_width; 
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
    return_value->z = 50.0f;
    return_value->triangles_size = 0;
    
    FileBuffer * buffer = platform_read_file(filename);
   
    // TODO: think about buffer size 
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
            assert(
                new_vertices[new_vertex_i].x == new_vertex.x);
            assert(
                new_vertices[new_vertex_i].y
                    == new_vertex.y);
            assert(
                new_vertices[new_vertex_i].z
                    == new_vertex.z);
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
            i += chars_till_next_space(buffer->contents + i);
            assert(buffer->contents[i] == ' ');
            i += chars_till_next_nonspace(buffer->contents + i);
            assert(buffer->contents[i] != ' ');
            
            // read 2nd vertex index
            uint32_t vertex_i_1 = atoi(buffer->contents + i);
            i += chars_till_next_space(buffer->contents + i);
            assert(buffer->contents[i] == ' ');
            i += chars_till_next_nonspace(buffer->contents + i);
            assert(buffer->contents[i] != ' ');
            
            // read 3rd vertex index
            uint32_t vertex_i_2 = atoi(buffer->contents + i);
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

            new_triangle.color[0] = 0.75f;
            new_triangle.color[1] = 0.30f;
            new_triangle.color[2] = 0.20f;
            new_triangle.color[3] = 1.0f;
            
            new_triangle.texture_i = -1;
            
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
    
    return return_value;
}

zPolygon * get_box() {
    zPolygon * box = malloc(sizeof(zPolygon));
    box->triangles_size = 6 * 2; // 6 faces, 2 per face
    
    box->triangles = malloc(
        sizeof(zTriangle) * box->triangles_size);
    
    box->x = -1.5f;
    box->y = -3.5f;
    box->z = 10.0f;
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
    box->triangles[0].texture_i = 0;
    
    box->triangles[1].vertices[0] =
        (zVertex){ 0.0f, 0.0f, 0.0f };
    box->triangles[1].vertices[1] =
        (zVertex){ 1.0f, 1.0f, 0.0f };
    box->triangles[1].vertices[2] =
        (zVertex){ 1.0f, 0.0f, 0.0f };
    box->triangles[1].texture_i = 0;
    
    // EAST face
    box->triangles[2].vertices[0] =
        (zVertex){ 1.0f, 0.0f, 0.0f };
    box->triangles[2].vertices[1] =
        (zVertex){ 1.0f, 1.0f, 0.0f };
    box->triangles[2].vertices[2] =
        (zVertex){ 1.0f, 1.0f, 1.0f };
    box->triangles[2].texture_i = 1;
    
    box->triangles[3].vertices[0] =
        (zVertex){ 1.0f, 0.0f, 0.0f };
    box->triangles[3].vertices[1] =
        (zVertex){ 1.0f, 1.0f, 1.0f };
    box->triangles[3].vertices[2] =
        (zVertex){ 1.0f, 0.0f, 1.0f };
    box->triangles[3].texture_i = 1;
    
    // NORTH face
    box->triangles[4].vertices[0] =
        (zVertex){ 1.0f, 0.0f, 1.0f };
    box->triangles[4].vertices[1] =
        (zVertex){ 1.0f, 1.0f, 1.0f };
    box->triangles[4].vertices[2] =
        (zVertex){ 0.0f, 1.0f, 1.0f };
    box->triangles[4].texture_i = 0;
    
    box->triangles[5].vertices[0] =
        (zVertex){ 1.0f, 0.0f, 1.0f };
    box->triangles[5].vertices[1] =
        (zVertex){ 0.0f, 1.0f, 1.0f };
    box->triangles[5].vertices[2] =
        (zVertex){ 0.0f, 0.0f, 1.0f };
    box->triangles[5].texture_i = 0;
    
    // WEST face
    box->triangles[6].vertices[0] =
        (zVertex){ 0.0f, 0.0f, 1.0f };
    box->triangles[6].vertices[1] =
        (zVertex){ 0.0f, 1.0f, 1.0f };
    box->triangles[6].vertices[2] =
        (zVertex){ 0.0f, 1.0f, 0.0f };
    box->triangles[6].texture_i = 1;
    
    box->triangles[7].vertices[0] =
        (zVertex){ 0.0f, 0.0f, 1.0f };
    box->triangles[7].vertices[1] =
        (zVertex){ 0.0f, 1.0f, 0.0f };
    box->triangles[7].vertices[2] =
        (zVertex){ 0.0f, 0.0f, 0.0f };
    box->triangles[7].texture_i = 1;
    
    // TOP face
    box->triangles[8].vertices[0] =
        (zVertex){ 0.0f, 1.0f, 0.0f };
    box->triangles[8].vertices[1] =
        (zVertex){ 0.0f, 1.0f, 1.0f };
    box->triangles[8].vertices[2] =
        (zVertex){ 1.0f, 1.0f, 1.0f };
    box->triangles[8].texture_i = 0;
    
    box->triangles[9].vertices[0] =
        (zVertex){ 0.0f, 1.0f, 0.0f };
    box->triangles[9].vertices[1] =
        (zVertex){ 1.0f, 1.0f, 1.0f };
    box->triangles[9].vertices[2] =
        (zVertex){ 1.0f, 1.0f, 0.0f };
    box->triangles[9].texture_i = 0;
    
    // BOTTOM face
    box->triangles[10].vertices[0] =
        (zVertex){ 1.0f, 0.0f, 1.0f };
    box->triangles[10].vertices[1] =
        (zVertex){ 0.0f, 0.0f, 1.0f };
    box->triangles[10].vertices[2] =
        (zVertex){ 0.0f, 0.0f, 0.0f };
    box->triangles[10].texture_i = 1;
    
    box->triangles[11].vertices[0] =
        (zVertex){ 1.0f, 0.0f, 1.0f };
    box->triangles[11].vertices[1] =
        (zVertex){ 0.0f, 0.0f, 0.0f };
    box->triangles[11].vertices[2] =
        (zVertex){ 1.0f, 0.0f, 0.0f };
    box->triangles[11].texture_i = 1;
    
    return box;
}

void ztriangle_to_2d(
    Vertex recipient[3],
    zTriangle * input)
{
    ProjectionConstants * pjc = &projection_constants;
    
    for (uint32_t i = 0; i < 3; i++) {
        // final formula to project something {x, y, z} to
        // 2D screen:
        // x = (aspect_ratio * field_of_view_mod * x) / z;
        // y = (aspect_ratio * field_of_view_mod * x) / z;
        // z = (z * z_normalisation) - (z * near);
        float z_modifier =
            input->vertices[i].z
                * ((pjc->far / pjc->far - pjc->near)
                - (pjc->far * pjc->near / pjc->far - pjc->near));
        
        recipient[i].x =
            (pjc->aspect_ratio
            * pjc->field_of_view_modifier
            * input->vertices[i].x); 
        
        if (recipient[i].x != 0.0f
            && z_modifier != 0.0f)
        {
            recipient[i].x /= z_modifier;
        }
        
        // note to self: y transformation
        // doesn't use aspect ratio
        recipient[i].y =
            pjc->field_of_view_modifier
            * input->vertices[i].y;
        
        if (recipient[i].y != 0.0f
            && z_modifier != 0.0f)
        {
            recipient[i].y /= z_modifier;
        }
        
        for (uint32_t j = 0; j < 4; j++) {
            recipient[i].RGBA[j] = input->color[j];
        }
        
        recipient[i].texture_i = input->texture_i;
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

    return_value.texture_i = input->texture_i;
    for (uint32_t i = 0; i < 4; i++) {
        return_value.color[i] = input->color[i];
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
    
    return_value.texture_i = input->texture_i;
    for (uint32_t i = 0; i < 4; i++) {
        return_value.color[i] = input->color[i];
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
    
    return_value.texture_i = input->texture_i;
    for (uint32_t i = 0; i < 4; i++) {
        return_value.color[i] = input->color[i];
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
    
    return_value.texture_i = input->texture_i;
    for (uint32_t i = 0; i < 4; i++) {
        return_value.color[i] = input->color[i];
    }
    
    return return_value;
}

float get_avg_z(const zTriangle * of_triangle)
{
    return (of_triangle->vertices[0].z +
        of_triangle->vertices[1].z +
        of_triangle->vertices[2].z) / 3.0f;
}

int sorter_cmpr_lowest_z(const void * a, const void * b) {
    return get_avg_z(a) < get_avg_z(b) ? -1 : 1;
}

void normalize_zvertex(
    zVertex * to_normalize)
{
    float sum_squares =
        (to_normalize->x * to_normalize->x) +
        (to_normalize->y * to_normalize->y) +
        (to_normalize->z * to_normalize->z);
    float length = sqrtf(sum_squares);
    to_normalize->x /= length;
    to_normalize->y /= length;
    to_normalize->z /= length;
}

float dot_of_vertices(
    const zVertex vertex_1,
    const zVertex vertex_2)
{
    float dot_x = (vertex_1.x * vertex_2.x);
    float dot_y = (vertex_1.y * vertex_2.y);
    float dot_z = (vertex_1.z * vertex_2.z);
    
    return dot_x + dot_y + dot_z;
}


float get_distance(
    const zVertex p1,
    const zVertex p2)
{
    return sqrtf(
        ((p1.x - p2.x) * (p1.x - p2.x))
        + ((p1.y - p2.y) * (p1.y - p2.y))
        + ((p1.z - p2.z) * (p1.z - p2.z)));
}

float get_distance_to_ztriangle(
    const zVertex p1,
    const zTriangle p2)
{
    return (
        get_distance(p1, p2.vertices[0]) +
        get_distance(p1, p2.vertices[1]) +
        get_distance(p1, p2.vertices[2])) / 3.0f;
}

float get_visibility_rating(
    const zVertex observer,
    const zTriangle * observed)
{
        zVertex normal;
        zVertex line1;
        zVertex line2;
        
        line1.x =
            observed->vertices[1].x
                - observed->vertices[0].x;
        line1.y =
            observed->vertices[1].y
                - observed->vertices[0].y;
        line1.z =
            observed->vertices[1].z
                - observed->vertices[0].z;
        normalize_zvertex(&line1);
        
        line2.x =
            observed->vertices[2].x
                - observed->vertices[0].x;
        line2.y =
            observed->vertices[2].y
                - observed->vertices[0].y;
        line2.z =
            observed->vertices[2].z
                - observed->vertices[0].z;
        normalize_zvertex(&line2);
        
        normal.x =
            (line1.y * line2.z) - (line1.z * line2.y);
        normal.y =
            (line1.z * line2.x) - (line1.x * line2.z);
        normal.z =
            (line1.x * line2.y) - (line1.y * line2.x);
        
        normalize_zvertex(&normal); 
        
        // compare normal's similarity to a point between
        // observer & triangle location 
        zVertex triangle_minus_observer;
        triangle_minus_observer.x =
            observed->vertices[0].x - observer.x;
        triangle_minus_observer.y =
            observed->vertices[0].y - observer.y;
        triangle_minus_observer.z =
            observed->vertices[0].z - observer.z;
        normalize_zvertex(&triangle_minus_observer);
        
        return dot_of_vertices(
            normal,
            triangle_minus_observer);
}

