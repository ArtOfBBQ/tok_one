#include "zpolygon.h"

ProjectionConstants projection_constants = {};

// If you want to draw 3D objects to the screen, you need
// to set them up here
zPolygon zpolygons_to_render[ZPOLYGONS_TO_RENDER_ARRAYSIZE];
uint32_t zpolygons_to_render_size = 0;

void init_projection_constants() {
    
    if (window_height < 50.0f || window_width < 50.0f) {
        char unexpected_window_sz_msg[256];
        strcpy_capped(unexpected_window_sz_msg, 256, "ERROR: unexpected window size [");
        strcat_int_capped(unexpected_window_sz_msg, 256, (int)window_height);
        strcat_capped(unexpected_window_sz_msg, 256, ",");
        strcat_int_capped(unexpected_window_sz_msg, 256, (int)window_width);
        strcat_capped(unexpected_window_sz_msg, 256, "]\n");
        log_append(unexpected_window_sz_msg);
        log_dump_and_crash(unexpected_window_sz_msg);
    }
    
    ProjectionConstants * pjc = &projection_constants;
    
    pjc->near = 0.1f;
    pjc->far = 1000.0f;
    
    float field_of_view = 90.0f;
    pjc->field_of_view_rad =
        ((field_of_view * 0.5f) / 180.0f) * 3.14159f;
    pjc->field_of_view_modifier =
        1.0f / tanf(pjc->field_of_view_rad);
    
    pjc->aspect_ratio =
        window_height / window_width; 
}

static uint32_t chars_till_next_space_or_slash(
    char * buffer)
{
    uint32_t i = 0;
    
    while (
        buffer[i] != '\n'
        && buffer[i] != ' '
        && buffer[i] != '/')
    {
        i++;
    }
    
    return i;
}

static uint32_t chars_till_next_nonspace(
    char * buffer)
{
    uint32_t i = 0;

    while (buffer[i] == ' ') {
        i++;
    }
    
    return i;
}

zPolygon parse_obj(
    char * rawdata,
    uint64_t rawdata_size)
{
    zPolygon return_value;
    return_value.x = 0.0f;
    return_value.y = 0.0f;
    return_value.z = 1.0f;
    return_value.z_angle = 0.0f;
    return_value.y_angle = 0.0f; // 3.14 to face camera
    return_value.x_angle = 0.0f;
    return_value.triangles_size = 0;
    
    // TODO: think about buffer size 
    // pass through buffer once to read all vertices 
    #define LOADING_OBJ_BUF_SIZE 8000
    zVertex * new_vertices = (zVertex *)malloc_from_managed(sizeof(zVertex) * LOADING_OBJ_BUF_SIZE);
    float uv_u[LOADING_OBJ_BUF_SIZE];
    float uv_v[LOADING_OBJ_BUF_SIZE];
    uint32_t new_uv_i = 0;
    
    uint32_t i = 0;
    uint32_t new_vertex_i = 0;
    while (i < rawdata_size) {
        
        // read the 1st character, which denominates the type
        // of information
        if (rawdata[i] == 'v'
            && rawdata[i+1] == ' ') {
            // discard the 'v'
            i++;
            
            // read vertex data
            zVertex new_vertex;
            
            // skip the space(s) after the 'v'
            log_assert(rawdata[i] == ' ');
            i += chars_till_next_nonspace(rawdata + i);
            log_assert(rawdata[i] != ' ');
            
            // read vertex x
            new_vertex.x = string_to_float(rawdata + i);
            
            // discard vertex x
            i += chars_till_next_space_or_slash(
                rawdata + i);
            log_assert(rawdata[i] == ' ');
            
            // discard the spaces after vertex x
            i += chars_till_next_nonspace(rawdata + i);
            log_assert(rawdata[i] != ' ');
            
            // read vertex y
            new_vertex.y = string_to_float(rawdata + i);
            i += chars_till_next_space_or_slash(
                rawdata + i);
            log_assert(rawdata[i] == ' ');
            i += chars_till_next_nonspace(rawdata + i);
            log_assert(rawdata[i] != ' ');
            
            // read vertex z
            new_vertex.z = string_to_float(rawdata + i);
            i += chars_till_next_space_or_slash(
                rawdata + i);
            log_assert(rawdata[i] == '\n');
            i++;
            
            new_vertices[new_vertex_i] = new_vertex;
            log_assert(
                new_vertices[new_vertex_i].x == new_vertex.x);
            log_assert(
                new_vertices[new_vertex_i].y
                    == new_vertex.y);
            log_assert(
                new_vertices[new_vertex_i].z
                    == new_vertex.z);
            new_vertex_i++;
        } else if (
            rawdata[i] == 'v'
            && rawdata[i+1] == 't')
        {
            // discard the 'vt'
            i += 2;
            
            // skip the space(s) after the 'vt'
            log_assert(rawdata[i] == ' ');
            i += chars_till_next_nonspace(rawdata + i);
            log_assert(rawdata[i] != ' ');
            
            // read the u coordinate
            uv_u[new_uv_i] = string_to_float(rawdata + i);
            
            // discard the u coordinate
            i += chars_till_next_space_or_slash(
                rawdata + i);
            log_assert(rawdata[i] == ' ');
            
            // skip the space(s) after the u coord
            log_assert(rawdata[i] == ' ');
            i += chars_till_next_nonspace(rawdata + i);
            log_assert(rawdata[i] != ' ');
            
            // read the v coordinate
            uv_v[new_uv_i] = string_to_float(rawdata + i);
            
            // discard the v coordinate
            i += chars_till_next_space_or_slash(
                rawdata + i);
            log_assert(rawdata[i] == '\n');
            
            new_uv_i += 1;
            
        } else {
            if (rawdata[i] == 'f') {
                return_value.triangles_size += 1;
            }
            // skip until the next line break character 
            while (rawdata[i] != '\n' && rawdata[i] != '\0') {
                i++;
            }
            
            // skip the line break character
            i++;
        }
    }
    
    // pass through rawdata again to read all triangles 
    return_value.triangles =
        (zTriangle *)malloc_from_unmanaged(
            sizeof(zTriangle) * return_value.triangles_size);
    
    i = 0;
    uint32_t new_triangle_i = 0;
    int32_t using_texturearray_i = -1;
    int32_t using_texture_i = -1;
    float using_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    
    while (i < rawdata_size) {
        if (rawdata[i] == 'u') {
            
            uint32_t j = i + 1;
            while (rawdata[j] != '\n' && rawdata[j] != '\0') {
                j++;
            }
            uint32_t line_size = j - i;
            
            char usemtl_hint[line_size];
            
            for (j = 0; j < (line_size); j++) {
                usemtl_hint[j] = rawdata[i + j];
            }
            
            if (are_equal_strings_of_length(
                "usemtl Face",
                usemtl_hint,
                line_size))
            {
                using_texturearray_i = 1;
                using_texture_i = 2;
            }
            
            if (
                are_equal_strings_of_length(
                    "usemtl Back",
                    usemtl_hint,
                    line_size))
            {
                using_texturearray_i = 1;
                using_texture_i = 4;
            }
            
            if (are_equal_strings_of_length(
                    "usemtl Side",
                    usemtl_hint,
                    line_size))
            {
                using_texturearray_i = -1;
                using_texture_i = -1;
                using_color[0] = 0.5;
                using_color[1] = 0.5;
                using_color[2] = 0.5;
                using_color[3] = 1.0;
            }
            
            // skip until the next line break character 
            while (rawdata[i] != '\n' && rawdata[i] != '\0') {
                i++;
            }
            // skip the line break character
            i++;
            
        } else if (rawdata[i] == 'f') {
            // discard the 'f'
            i++;
            log_assert(rawdata[i] == ' ');
            
            // skip the space(s) after the 'f'
            i += chars_till_next_nonspace(rawdata + i);
            log_assert(rawdata[i] != ' ');
            
            // read triangle data
            zTriangle new_triangle;
            new_triangle.visible = 1;
            
            // read 1st vertex index
            int32_t vertex_i_0 = string_to_int32(rawdata + i);
            i += chars_till_next_space_or_slash(
                rawdata + i);
            
            int32_t uv_coord_i_0 = 0;
            if (rawdata[i] == '/')
            {
                // skip the slash
                i++;
                uv_coord_i_0 =
                    string_to_int32(rawdata + i);
                i += chars_till_next_space_or_slash(
                    rawdata + i);
            }
            
            log_assert(rawdata[i] == ' ');
            i += chars_till_next_nonspace(rawdata + i);
            log_assert(rawdata[i] != ' ');
            
            // read 2nd vertex index
            int32_t vertex_i_1 = string_to_int32(rawdata + i);
            i += chars_till_next_space_or_slash(
                rawdata + i);
            
            int32_t uv_coord_i_1 = 0;
            if (rawdata[i] == '/')
            {
                // skip the slash
                i++;
                uv_coord_i_1 =
                    string_to_int32(rawdata + i);
                i += chars_till_next_space_or_slash(
                    rawdata + i);
            }
            
            log_assert(rawdata[i] == ' ');
            i += chars_till_next_nonspace(rawdata + i);
            log_assert(rawdata[i] != ' ');
            
            // read 3rd vertex index
            int32_t vertex_i_2 = string_to_int32(rawdata + i);
            i += chars_till_next_space_or_slash(
                rawdata + i);
            int32_t uv_coord_i_2 = 0;
            if (rawdata[i] == '/')
            {
                // skip the slash
                i++;
                uv_coord_i_2 =
                    string_to_int32(rawdata + i);
                i += chars_till_next_space_or_slash(
                    rawdata + i);
            }
            log_assert(rawdata[i] == '\n');
            i++;
            
            log_assert(vertex_i_0 != vertex_i_1);
            log_assert(vertex_i_0 != vertex_i_2);
            log_assert(vertex_i_0 > 0);
            log_assert(vertex_i_1 > 0);
            log_assert(vertex_i_2 > 0);
            log_assert(uv_coord_i_0 < LOADING_OBJ_BUF_SIZE);
            log_assert(uv_coord_i_1 < LOADING_OBJ_BUF_SIZE);
            log_assert(uv_coord_i_2 < LOADING_OBJ_BUF_SIZE);
            
            new_triangle.vertices[0] =
                new_vertices[vertex_i_0 - 1];
            new_triangle.vertices[1] =
                new_vertices[vertex_i_1 - 1];
            new_triangle.vertices[2] =
                new_vertices[vertex_i_2 - 1];
            
            if (
                uv_coord_i_0 > 0
                && uv_coord_i_1 > 0
                && uv_coord_i_2 > 0)
            {
                new_triangle.vertices[0].uv[0] =
                    uv_u[uv_coord_i_0 - 1];
                new_triangle.vertices[0].uv[1] =
                    uv_v[uv_coord_i_0 - 1];
                new_triangle.vertices[1].uv[0] =
                    uv_u[uv_coord_i_1 - 1];
                new_triangle.vertices[1].uv[1] =
                    uv_v[uv_coord_i_1 - 1];
                new_triangle.vertices[2].uv[0] =
                    uv_u[uv_coord_i_2 - 1];
                new_triangle.vertices[2].uv[1] =
                    uv_v[uv_coord_i_2 - 1];
            }
            
            
            new_triangle.color[0] = using_color[0];
            new_triangle.color[1] = using_color[1];
            new_triangle.color[2] = using_color[2];
            new_triangle.color[3] = using_color[3];
            
            new_triangle.texturearray_i = using_texturearray_i;
            new_triangle.texture_i = using_texture_i;
            
            return_value.triangles[new_triangle_i] =
                new_triangle;
            new_triangle_i++;
        } else {
            // skip until the next line break character 
            while (rawdata[i] != '\n' && rawdata[i] != '\0') {
                i++;
            }
            
            // skip the line break character
            i++;
        }
    }
    
    free_from_managed((uint8_t *)new_vertices);
    
    return return_value;
}

void zpolygon_scale_to_width_given_z(
    zPolygon * to_scale,
    const float new_width,
    const float when_observed_at_z)
{
    float largest_width = 0.0f;
    for (uint32_t i = 0; i < to_scale->triangles_size; i++) {
        for (uint32_t j = 0; j < 3; j++)
        {
            float width =
                ((to_scale->triangles[i].vertices[j].x < 0) *  (to_scale->triangles[i].vertices[j].x * -1)) +
                ((to_scale->triangles[i].vertices[j].x >= 0) *  (to_scale->triangles[i].vertices[j].x)); 
            if (width > largest_width)
            {
                largest_width = width;
            }
        }
    }
    
    float target_width = new_width / when_observed_at_z;
    
    float scale_factor = target_width / largest_width;
    
    for (uint32_t i = 0; i < to_scale->triangles_size; i++) {
        for (uint32_t j = 0; j < 3; j++)
        {
            to_scale->triangles[i].vertices[j].x *= scale_factor;
            to_scale->triangles[i].vertices[j].y *= scale_factor;
            to_scale->triangles[i].vertices[j].z *= scale_factor;
        }
    }
}

void scale_zpolygon(
    zPolygon * to_scale,
    const float new_height)
{
    log_assert(to_scale != NULL);
    if (to_scale == NULL) { return; }
    
    float largest_height = 0.0f;
    for (uint32_t i = 0; i < to_scale->triangles_size; i++) {
        for (uint32_t j = 0; j < 3; j++)
        {
            float height =
                ((to_scale->triangles[i].vertices[j].y < 0) *  (to_scale->triangles[i].vertices[j].y * -1)) +
                ((to_scale->triangles[i].vertices[j].y >= 0) *  (to_scale->triangles[i].vertices[j].y)); 
            if (height > largest_height)
            {
                largest_height = height;
            }
        }
    }
    log_assert(largest_height > 0.0f);
    
    float scale_factor = new_height / largest_height;
    
    for (uint32_t i = 0; i < to_scale->triangles_size; i++) {
        for (uint32_t j = 0; j < 3; j++)
        {
            to_scale->triangles[i].vertices[j].x *= scale_factor;
            to_scale->triangles[i].vertices[j].y *= scale_factor;
            to_scale->triangles[i].vertices[j].z *= scale_factor;
        }
    }
}

void __attribute__((no_instrument_function))
ztriangle_apply_lighting(
    Vertex recipient[3],
    zTriangle * input,
    zLightSource * zlight_source)
{
    log_assert(zlight_source != NULL);
    if (zlight_source == NULL) { return; }
    
    // add lighting to the 3 vertices
    for (
        uint32_t m = 0;
        m < 3;
        m++)
    {
        zVertex light_source_pos;
        light_source_pos.x = zlight_source->x;
        light_source_pos.y = zlight_source->y;
        light_source_pos.z = zlight_source->z;
        
        float distance =
            distance_to_zvertex(
                light_source_pos,
                input->vertices[m]);
        
        float distance_mod = zlight_source->reach / distance;
        if (distance_mod < 0.0f) {
            distance_mod = 0.0f;
        }
        
        // *******************************************
        // add ambient lighting 
        for (uint32_t l = 0; l < 3; l++) {
            recipient[m].lighting[l] +=
                zlight_source->RGBA[l] *
                zlight_source->ambient *
                distance_mod;
        }
        
        // *******************************************
        // add diffuse lighting
        float diffuse_dot = get_visibility_rating(
            light_source_pos,
            input,
            m);
        
        if (diffuse_dot < 0.0f)
        {
            for (uint32_t l = 0; l < 3; l++) {
                recipient[m].lighting[l] +=
                    (diffuse_dot
                        * -1
                        * zlight_source->RGBA[l]
                        * distance_mod
                        * zlight_source->diffuse);
            }
        }
        // *******************************************
    }
}

void ztriangle_to_3d(
    Vertex recipient[3],
    zTriangle * input)
{
    ProjectionConstants * pjc = &projection_constants;
    
    for (uint32_t i = 0; i < 3; i++) {
        recipient[i].x =
            (pjc->aspect_ratio
            * pjc->field_of_view_modifier
            * input->vertices[i].x); 
        recipient[i].y = input->vertices[i].y;
        recipient[i].z = 0.0f;
        
        recipient[i].uv[0] = input->vertices[i].uv[0];
        recipient[i].uv[1] = input->vertices[i].uv[1];
        
        for (uint32_t j = 0; j < 3; j++) {
            recipient[i].RGBA[j] = input->color[j];
        }
        
        recipient[i].texturearray_i = input->texturearray_i;
        recipient[i].texture_i = input->texture_i;
    }
}

void ztriangle_to_2d(
    Vertex recipient[3],
    zTriangle * input)
{
    ProjectionConstants * pjc = &projection_constants;
    
    for (uint32_t i = 0; i < 3; i++) {
        
        recipient[i].x =
            input->vertices[i].x *
            pjc->aspect_ratio *
            pjc->field_of_view_modifier; 
        
        // note to self: y transformation
        // doesn't use aspect ratio
        recipient[i].y =
            input->vertices[i].y *
            pjc->field_of_view_modifier;
        
        recipient[i].z =
            input->vertices[i].z *
                pjc->far / (pjc->far - pjc->near) +
            1.0f * (-pjc->far * pjc->near) /
                (pjc->far - pjc->near);
        
        recipient[i].w = input->vertices[i].z == 0.0f ?
           0.0001f
           : input->vertices[i].z;
        
        // recipient[i].x /= recipient[i].w;
        // recipient[i].y /= recipient[i].w;
        // recipient[i].z /= recipient[i].w;
        
        recipient[i].uv[0] = input->vertices[i].uv[0];
        recipient[i].uv[1] = input->vertices[i].uv[1];
        
        for (uint32_t j = 0; j < 4; j++) {
            recipient[i].RGBA[j] = input->color[j];
        }
        
        recipient[i].texturearray_i = input->texturearray_i;
        recipient[i].texture_i = input->texture_i;
    }
}

zTriangle __attribute__((no_instrument_function))
x_rotate_ztriangle(
    const zTriangle * input,
    const float angle)
{
    zTriangle return_value = *input;
    
    if (angle == 0.0f) {
        return return_value;
    }
    
    for (
        uint32_t i = 0;
        i < 3;
        i++)
    {
        return_value.vertices[i] = x_rotate_zvertex(
            &return_value.vertices[i],
            angle);
    }
    
    return return_value;
}


zTriangle __attribute__((no_instrument_function))
z_rotate_ztriangle(
    const zTriangle * input,
    const float angle)
{
    zTriangle return_value = *input;
    
    if (angle == 0.0f) {
        return return_value;
    }
    
    for (
        uint32_t i = 0;
        i < 3;
        i++)
    {
        return_value.vertices[i] = z_rotate_zvertex(
            &return_value.vertices[i],
            angle);
    }
    
    return return_value;
}

zTriangle __attribute__((no_instrument_function))
y_rotate_ztriangle(
    const zTriangle * input,
    const float angle)
{
    zTriangle return_value = *input;
    
    if (angle == 0.0f) {
        return return_value;
    }
    
    for (
        uint32_t i = 0;
        i < 3;
        i++)
    {
        return_value.vertices[i] = y_rotate_zvertex(
            &return_value.vertices[i],
            angle);
    }
    
    return return_value;
}

zTriangle translate_ztriangle(
    const zTriangle * input,
    const float by_x,
    const float by_y,
    const float by_z)
{
    zTriangle return_value = *input;
    
    for (uint32_t i = 0; i < 3; i++) {
        return_value.vertices[i].x += by_x;
        return_value.vertices[i].y += by_y;
        return_value.vertices[i].z += by_z;
    }
    
    return return_value;
}

float get_avg_z(
    const zTriangle * of_triangle)
{
    return (
        of_triangle->vertices[0].z +
        of_triangle->vertices[1].z +
        of_triangle->vertices[2].z) / 3.0f;
}

int sorter_cmpr_lowest_z(
    const void * a,
    const void * b)
{
    return get_avg_z((zTriangle *)a) < get_avg_z((zTriangle *)b) ? -1 : 1;
}

static float get_magnitude(zVertex input) {
    float sum_squares =
        (input.x * input.x) +
        (input.y * input.y) +
        (input.z * input.z);
        
    // TODO: this square root is a performance bottleneck
    return sqrtf(sum_squares);
}

void normalize_zvertex(
    zVertex * to_normalize)
{
    float magnitude = get_magnitude(*to_normalize);
    to_normalize->x /= magnitude;
    to_normalize->y /= magnitude;
    to_normalize->z /= magnitude;
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

float distance_to_ztriangle(
    const zVertex p1,
    const zTriangle p2)
{
    return (
        get_distance(p1, p2.vertices[0]) +
        get_distance(p1, p2.vertices[1]) +
        get_distance(p1, p2.vertices[2])) / 3.0f;
}

float distance_to_zvertex(
    const zVertex p1,
    const zVertex p2)
{
    return (
        get_distance(p1, p2) +
        get_distance(p1, p2) +
        get_distance(p1, p2)) / 3.0f;
}

zVertex get_ztriangle_normal(
    const zTriangle * input,
    const uint32_t at_vertex_i)
{
    uint32_t vertex_0 = at_vertex_i % 3;
    log_assert(vertex_0 == at_vertex_i);
    uint32_t vertex_1 = (at_vertex_i + 1) % 3;
    uint32_t vertex_2 = (at_vertex_i + 2) % 3;
    
    zVertex normal; 
    zVertex vector1;
    zVertex vector2;
    
    vector1.x =
        input->vertices[vertex_1].x
            - input->vertices[vertex_0].x;
    vector1.y =
        input->vertices[vertex_1].y
            - input->vertices[vertex_0].y;
    vector1.z =
        input->vertices[vertex_1].z
            - input->vertices[vertex_0].z;
    
    vector2.x =
        input->vertices[vertex_2].x
            - input->vertices[vertex_0].x;
    vector2.y =
        input->vertices[vertex_2].y
            - input->vertices[vertex_0].y;
    vector2.z =
        input->vertices[vertex_2].z
            - input->vertices[vertex_0].z;
    
    normal.x =
        (vector1.y * vector2.z) - (vector1.z * vector2.y);
    normal.y =
        (vector1.z * vector2.x) - (vector1.x * vector2.z);
    normal.z =
        (vector1.x * vector2.y) - (vector1.y * vector2.x);
    
    return normal;
}

float get_visibility_rating(
    const zVertex observer,
    const zTriangle * observed,
    const uint32_t observed_vertex_i)
{
    // let's move everything so that observer is at {0,0,0}
    // we'll leave the observer as is and just use {0,0,0} where
    // we would have used it
    zTriangle observed_adj = *observed;
    for (uint32_t i = 0; i < 3; i++) {
        observed_adj.vertices[i].x =
            observed->vertices[i].x - observer.x;
        observed_adj.vertices[i].y =
            observed->vertices[i].y - observer.y;
        observed_adj.vertices[i].z =
            observed->vertices[i].z - observer.z;
    }
    
    zVertex normal = get_ztriangle_normal(
        observed,
        observed_vertex_i);
    
    // TODO: performance bottleneck
    normalize_zvertex(&normal);
    
    // compare normal's similarity to a vector straight
    // from observer to triangle location 
    zVertex triangle_minus_observer;
    triangle_minus_observer.x =
        observed_adj.vertices[observed_vertex_i].x;
    triangle_minus_observer.y =
        observed_adj.vertices[observed_vertex_i].y;
    triangle_minus_observer.z =
        observed_adj.vertices[observed_vertex_i].z;
        
    // TODO: normalize_zvertex is a performance bottleneck
    normalize_zvertex(&triangle_minus_observer);
    
    return dot_of_vertices(
        normal,
        triangle_minus_observer);
}

void zcamera_move_forward(
    zCamera * to_move,
    const float distance)
{
    // pick a point that would be in front of the camera
    // if it was not angled in any way, and if it was at
    // the origin
    zVertex forward_if_camera_was_unrotated_at_origin;
    forward_if_camera_was_unrotated_at_origin.x = 0.0f;
    forward_if_camera_was_unrotated_at_origin.y = 0.0f;
    forward_if_camera_was_unrotated_at_origin.z = distance;
    
    zVertex x_rotated = x_rotate_zvertex(
        &forward_if_camera_was_unrotated_at_origin,
        camera.x_angle);
    zVertex y_rotated = y_rotate_zvertex(
        &x_rotated,
        camera.y_angle);
    zVertex final = z_rotate_zvertex(
        &y_rotated,
        camera.z_angle);
    
    // add to the camera's current position
    to_move->x += final.x;
    to_move->y += final.y;
    to_move->z += final.z;
}

