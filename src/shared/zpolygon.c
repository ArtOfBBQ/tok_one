#include "zpolygon.h"

ProjectionConstants projection_constants = {};

// If you want to draw 3D objects to the screen, you need
// to set them up here
zPolygon zpolygons_to_render[ZPOLYGONS_TO_RENDER_ARRAYSIZE];
uint32_t zpolygons_to_render_size = 0;

// Pre-allocated arrays for apply_lighting(), a performance bottleneck
#define DISTANCES_TO_VERTICES_CAP 1000000
static float * distances_to_vertices;
static uint32_t distances_to_vertices_size = 0;
#define DIFFUSED_DOTS_CAP 1000000
static float * diffused_dots;
static uint32_t diffused_dots_size = 0;

// Pre-allocated arrays for get_visiblity_ratings() 
#define OBSERVEDS_ADJ_CAP 1000000
static float * observeds_adj_x;
static float * observeds_adj_y;
static float * observeds_adj_z;
#define NORMALS_CAP 1000000
static float * normals_x;
static float * normals_y;
static float * normals_z;

// Pre-allocated arrays for get_magnitudes()
#define MAGNITUDES_CAP 1000000
static float * magnitudes_working_memory;
static float * magnitudes;

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
    pjc->field_of_view_rad = ((field_of_view * 0.5f) / 180.0f) * 3.14159f;
    pjc->field_of_view_modifier = 1.0f / tanf(pjc->field_of_view_rad);
    
    pjc->aspect_ratio = window_height / window_width; 
    
    distances_to_vertices = (float *)malloc_from_unmanaged(DISTANCES_TO_VERTICES_CAP);
    diffused_dots = (float *)malloc_from_unmanaged(DIFFUSED_DOTS_CAP);
    observeds_adj_x = (float *)malloc_from_unmanaged(OBSERVEDS_ADJ_CAP);
    observeds_adj_y = (float *)malloc_from_unmanaged(OBSERVEDS_ADJ_CAP);
    observeds_adj_z = (float *)malloc_from_unmanaged(OBSERVEDS_ADJ_CAP);
    normals_x = (float *)malloc_from_unmanaged(NORMALS_CAP);
    normals_y = (float *)malloc_from_unmanaged(NORMALS_CAP);
    normals_z = (float *)malloc_from_unmanaged(NORMALS_CAP);
    
    magnitudes_working_memory = (float *)malloc_from_unmanaged(MAGNITUDES_CAP);
    magnitudes = (float *)malloc_from_unmanaged(MAGNITUDES_CAP);
}

void request_zpolygon_to_render(zPolygon * to_add)
{
    log_assert(to_add->triangles != NULL);
    log_assert(to_add->triangles_size > 0);
    
    for (uint32_t tri_i = 0; tri_i < to_add->triangles_size; tri_i++) {
        if (to_add->triangles[tri_i].texturearray_i < 0) { log_assert(to_add->triangles[tri_i].texture_i < 0); }
        if (to_add->triangles[tri_i].texture_i < 0) { log_assert(to_add->triangles[tri_i].texturearray_i < 0); }
        
        if (to_add->triangles[tri_i].texturearray_i >= 0) {
            register_high_priority_if_unloaded(
                to_add->triangles[tri_i].texturearray_i,
                to_add->triangles[tri_i].texture_i);
        }
    }
    
    for (
        uint32_t i = 0;
        i < zpolygons_to_render_size;
        i++)
    {
        if (zpolygons_to_render[i].deleted)
        {
            zpolygons_to_render[i] = *to_add;
            return;
        }
    }
    
    log_assert(zpolygons_to_render_size + 1 < ZPOLYGONS_TO_RENDER_ARRAYSIZE);
    zpolygons_to_render[zpolygons_to_render_size] = *to_add;
    zpolygons_to_render_size += 1;
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

/*
Ducktaped together parser to read my .obj files from Blender
I export to 'legacy obj' in Blender like this:

Step 1:
File -> Export -> Wavefront (.obj) (Legacy)

Step 2:
Maximize the 'geometry' panel and check:
[x] Apply Modifiers
[x] Include UVs
[x] Write Materials (but don't use 'material groups' in the panel above)
[x] Triangulate faces
[x] Keep vertex order
Uncheck everything else
*/
zPolygon parse_obj(
    char * rawdata,
    uint64_t rawdata_size,
    const bool32_t flip_winding)
{
    return parse_obj_expecting_materials(rawdata, rawdata_size, NULL, 0, flip_winding);
}
zPolygon parse_obj_expecting_materials(
    char * rawdata,
    uint64_t rawdata_size,
    ExpectedObjMaterials * expected_materials,
    const uint32_t expected_materials_size,
    const bool32_t flip_winding)
{
    zPolygon return_value;
    construct_zpolygon(&return_value);
    
    // TODO: think about buffer size 
    // pass through buffer once to read all vertices 
    #define LOADING_OBJ_BUF_SIZE 16000
    zVertex * new_vertices = (zVertex *)malloc_from_managed(
        sizeof(zVertex) * LOADING_OBJ_BUF_SIZE);
    float uv_u[LOADING_OBJ_BUF_SIZE];
    float uv_v[LOADING_OBJ_BUF_SIZE];
    uint32_t new_uv_i = 0;
    
    uint32_t i = 0;
    uint32_t first_material_or_face_i = UINT32_MAX;
    uint32_t new_vertex_i = 0;
    
    while (i < rawdata_size) {
        // read the 1st character, which denominates the type
        // of information
        if (
            rawdata[i] == 'v' &&
            rawdata[i+1] == ' ')
        {
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
            
            new_uv_i += 1;
            
            // discard the v coordinate
            i += chars_till_next_space_or_slash(
                rawdata + i);
            log_assert(rawdata[i] == '\n');
            
            // discard the line break
            i++;
                        
        } else {
            if (
                rawdata[i] == 'f' ||
                (
                    rawdata[i] == 'u' &&
                    rawdata[i+1] == 's' &&
                    rawdata[i+2] == 'e' &&
                    rawdata[i+3] == 'm'))
            {
                if (i < first_material_or_face_i) {
                    first_material_or_face_i = i;
                }
            }
            
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
    
    log_assert(return_value.triangles_size > 0);
    
    // pass through rawdata again to read all triangles 
    //    return_value.triangles =
    //        (zTriangle *)malloc_from_unmanaged(
    //            sizeof(zTriangle) * return_value.triangles_size);
    log_assert(return_value.triangles_size < POLYGON_TRIANGLES_SIZE);
    
    i = first_material_or_face_i;
    uint32_t new_triangle_i = 0;
    int32_t using_texturearray_i = -1;
    int32_t using_texture_i = -1;
    float using_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    
    while (i < rawdata_size) {
        if (rawdata[i] == 'u' &&
            rawdata[i+1] == 's' &&
            rawdata[i+2] == 'e' &&
            rawdata[i+3] == 'm')
        {
            uint32_t j = i + 1;
            while (
                rawdata[j] != '\n' &&
                rawdata[j] != '\0')
            {
                j++;
            }
            uint32_t line_size = j - i;
            
            if (expected_materials != NULL) {
                char usemtl_hint[line_size];
                char expected_mtl[line_size * 2];
                
                for (j = 0; j < (line_size); j++) {
                    usemtl_hint[j] = rawdata[i + j];
                }
                
                for (
                    uint32_t mtl_i = 0;
                    mtl_i < expected_materials_size;
                    mtl_i++)
                {
                    strcpy_capped(
                        expected_mtl,
                        line_size*2,
                        "usemtl ");
                    strcat_capped(
                        expected_mtl,
                        line_size*2,
                        expected_materials[mtl_i].material_name);
                    
                    if (
                        are_equal_strings_of_length(
                            expected_mtl,
                            usemtl_hint,
                            line_size))
                    {
                        log_append("Now using material: ");
                        log_append(expected_mtl);
                        log_append_char('\n');
                        using_texturearray_i =
                            expected_materials[mtl_i].texturearray_i;
                        using_texture_i =
                            expected_materials[mtl_i].texture_i;
                        using_color[0] =
                            expected_materials[mtl_i].rgba[0];
                        using_color[1] =
                            expected_materials[mtl_i].rgba[1];
                        using_color[2] =
                            expected_materials[mtl_i].rgba[2];
                        using_color[3] =
                            expected_materials[mtl_i].rgba[3];
                        break;
                    }
                }
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
                uv_coord_i_0 = string_to_int32(rawdata + i);
                i += chars_till_next_space_or_slash(rawdata + i);
            }
            // skip any id's of normals
            if (rawdata[i] == '/') {
                i++;
                log_assert(rawdata[i] != ' ');
                i += chars_till_next_space_or_slash(rawdata + i);
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
            // skip any id's of normals
            if (rawdata[i] == '/') {
                i++;
                log_assert(rawdata[i] != ' ');
                i += chars_till_next_space_or_slash(rawdata + i);
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
            
            // skip any id's of normals
            if (rawdata[i] == '/') {
                i++;
                log_assert(rawdata[i] != ' ');
                while (rawdata[i] <= '9' && rawdata[i] >= '0') {
                    i++;
                }
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
            
            uint32_t target_vertex_0 = flip_winding ? 2 : 0;
            uint32_t target_vertex_1 = 1;
            uint32_t target_vertex_2 = flip_winding ? 0 : 2;
            
            new_triangle.vertices[target_vertex_0] =
                new_vertices[vertex_i_0 - 1];
            new_triangle.vertices[target_vertex_1] =
                new_vertices[vertex_i_1 - 1];
            new_triangle.vertices[target_vertex_2] =
                new_vertices[vertex_i_2 - 1];
            
            if (
                uv_coord_i_0 > 0 &&
                uv_coord_i_1 > 0 &&
                uv_coord_i_2 > 0)
            {
                new_triangle.vertices[target_vertex_0].uv[0] =
                uv_u[uv_coord_i_0 - 1];
                new_triangle.vertices[target_vertex_0].uv[1] =
                uv_v[uv_coord_i_0 - 1];
                new_triangle.vertices[target_vertex_1].uv[0] =
                uv_u[uv_coord_i_1 - 1];
                new_triangle.vertices[target_vertex_1].uv[1] =
                uv_v[uv_coord_i_1 - 1];
                new_triangle.vertices[target_vertex_2].uv[0] =
                uv_u[uv_coord_i_2 - 1];
                new_triangle.vertices[target_vertex_2].uv[1] =
                uv_v[uv_coord_i_2 - 1];
            }
            
            new_triangle.color[0] = using_color[0];
            new_triangle.color[1] = using_color[1];
            new_triangle.color[2] = using_color[2];
            new_triangle.color[3] = using_color[3];
            
            new_triangle.texturearray_i = using_texturearray_i;
            new_triangle.texture_i = using_texture_i;
            
            return_value.triangles[new_triangle_i] = new_triangle;
            new_triangle_i++;
            log_append("triangles finished parsing: ");
            log_append_uint(new_triangle_i);
            log_append_char('\n');
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

void center_zpolygon_offsets(
    zPolygon * to_center)
{
    log_assert(to_center != NULL);
    if (to_center == NULL) { return; }
    
    float smallest_y = FLOAT32_MAX;
    float largest_y = FLOAT32_MIN;
    float smallest_x = FLOAT32_MAX;
    float largest_x = FLOAT32_MIN;
    float smallest_z = FLOAT32_MAX;
    float largest_z = FLOAT32_MIN;
    
    for (uint32_t i = 0; i < to_center->triangles_size; i++) {
        for (uint32_t j = 0; j < 3; j++)
        {
            float x = to_center->triangles[i].vertices[j].x;
            float y = to_center->triangles[i].vertices[j].y;
            float z = to_center->triangles[i].vertices[j].z;
            
            if (y > largest_y)  { largest_y = y;  }
            if (y < smallest_y) { smallest_y = y; }
            if (x > largest_x)  { largest_x = x;  }
            if (x < smallest_x) { smallest_x = x; }
            if (z > largest_z)  { largest_z = z;  }
            if (z < smallest_z) { smallest_z = z; }
        }
    }
    log_assert(largest_y > 0.0f);
    log_assert(largest_y > smallest_y);
    log_assert(largest_x > smallest_x);
    log_assert(largest_z > smallest_z);
    
    float x_delta = (smallest_x + largest_x) / 2.0f;
    float y_delta = (smallest_y + largest_y) / 2.0f;
    float z_delta = (smallest_z + largest_z) / 2.0f;
    
    for (uint32_t i = 0; i < to_center->triangles_size; i++) {
        for (uint32_t j = 0; j < 3; j++)
        {
            to_center->triangles[i].vertices[j].x -= x_delta;
            to_center->triangles[i].vertices[j].y -= y_delta;
            to_center->triangles[i].vertices[j].z -= z_delta;
        }
    }
}

void ztriangles_apply_lighting(
    float * vertices_x,
    float * vertices_y,
    float * vertices_z,
    float * lighting_multipliers,
    const uint32_t vertices_size,
    Vertex * recipients,
    const uint32_t recipients_size,
    zLightSource * zlight_source)
{
    log_assert(zlight_source != NULL);
    if (zlight_source == NULL) { return; }
    
    zVertex light_source_pos;
        light_source_pos.x = zlight_source->x;
        light_source_pos.y = zlight_source->y;
        light_source_pos.z = zlight_source->z;
    
    distances_to_vertices_size = (vertices_size * 3);
    assert(distances_to_vertices_size < DISTANCES_TO_VERTICES_CAP);

    // get distance from triangle vertices to light_source    
    for (
        uint32_t i = 0;
        i < vertices_size;
        i++)
    {
        distances_to_vertices[i] =
            ((light_source_pos.x - vertices_x[i]) *
             (light_source_pos.x - vertices_x[i])) +
            ((light_source_pos.y - vertices_y[i]) *
             (light_source_pos.y - vertices_y[i])) +
            ((light_source_pos.z - vertices_z[i]) *
             (light_source_pos.z - vertices_z[i])); 
    }
    platform_256_sqrt(distances_to_vertices, distances_to_vertices_size);
    
    // convert distances_to_triangles to store distance modifiers instead
    platform_256_div_scalar_by_input(
        /* divisors: */ distances_to_vertices,
        /* divisors_size: */ distances_to_vertices_size,
        /* numerator: */ zlight_source->reach);
    
    // this is seperated to set the stage for converting to simd
    diffused_dots_size = vertices_size / 3;
    log_assert(diffused_dots_size < DIFFUSED_DOTS_CAP);
    
    get_visibility_ratings(
        /* const zVertex observer         : */ light_source_pos,
        /* triangle vertices x            : */ vertices_x,
        /* triangle vertices y            : */ vertices_y,
        /* triangle vertices z            : */ vertices_z,
        /* const uint32_t observeds_size  : */ vertices_size,
        /* float * out_visibility_ratings : */ diffused_dots);
    
    for (uint32_t col_i = 0; col_i < 3; col_i++) {
        
        float ambient_mod = zlight_source->RGBA[col_i] * zlight_source->ambient;
        float diffuse_mod = zlight_source->RGBA[col_i] * zlight_source->diffuse;
        
        for (uint32_t triangle_i = 0; triangle_i < diffused_dots_size; triangle_i++) {
            for (
                uint32_t m = 0;
                m < 3;
                m++)
            {
                uint32_t vertex_i = (triangle_i * 3) + m;
                log_assert(vertex_i < recipients_size);
                recipients[vertex_i].lighting[col_i] +=
                    ambient_mod *
                    distances_to_vertices[vertex_i] *
                    lighting_multipliers[vertex_i];
                
                // *******************************************
                // add diffuse lighting                
                if (diffused_dots[triangle_i] < 0.0f)
                {
                    recipients[vertex_i].lighting[col_i] +=
                        (diffused_dots[triangle_i] * -1) *
                        diffuse_mod *
                        distances_to_vertices[vertex_i] *
                        lighting_multipliers[vertex_i];
                }
            }
        }
    }
}

void construct_zpolygon(zPolygon * to_construct) {
    to_construct->object_id = -1;
    to_construct->touchable_id = -1;
    to_construct->triangles_size = 0;
    to_construct->x = 0.0f;
    to_construct->y = 0.0f;
    to_construct->z = 1.0f;
    to_construct->x_angle = 0.0f;
    to_construct->y_angle = 0.0f;
    to_construct->z_angle = 0.0f;
    to_construct->scale_factor = 1.0f;
    to_construct->ignore_lighting = false;
    to_construct->ignore_camera = false;
    to_construct->deleted = false;
}

void __attribute__((no_instrument_function))
ztriangle_apply_lighting(
    Vertex recipient[3],
    zTriangle * input,
    zLightSource * zlight_source)
{
    log_assert(zlight_source != NULL);
    if (zlight_source == NULL) { return; }
    
    zVertex light_source_pos;
        light_source_pos.x = zlight_source->x;
        light_source_pos.y = zlight_source->y;
        light_source_pos.z = zlight_source->z;
    
    // add lighting to the 3 vertices
    for (
        uint32_t m = 0;
        m < 3;
        m++)
    {
        float distance =
            get_distance(
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
            input);
        
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

void ztriangles_to_2d_inplace(
    float * vertices_x,
    float * vertices_y,
    float * vertices_z,
    const uint32_t vertices_size)
{
    ProjectionConstants * pjc = &projection_constants;
    
    float x_multiplier = pjc->aspect_ratio *
        pjc->field_of_view_modifier;
    float z_multiplier = (pjc->far / (pjc->far - pjc->near));
    float z_addition = (1.0f * (-pjc->far * pjc->near) /
        (pjc->far - pjc->near));
    
    platform_256_mul_scalar(vertices_x, vertices_size, x_multiplier);
    
    // note to self: y transformation doesn't use aspect ratio
    platform_256_mul_scalar(vertices_y, vertices_size, pjc->field_of_view_modifier);
    
    platform_256_mul_scalar(vertices_z, vertices_size, z_multiplier);
    platform_256_add_scalar(vertices_z, vertices_size, z_addition);    
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

static void get_magnitudes_inplace(
    const float * vertices_x,
    const float * vertices_y,
    const float * vertices_z,
    float * recipient,
    float * working_memory,
    const uint32_t vertices_and_recipient_size)
{
    log_assert(vertices_and_recipient_size < MAGNITUDES_CAP);
    
    for (uint32_t i = 0; i < vertices_and_recipient_size; i++) {
        // check for NaN
        log_assert(!(vertices_x[i] != vertices_x[i]));
    }
    
    for (uint32_t i = 0; i < vertices_and_recipient_size; i++) {
        working_memory[i] = vertices_x[i];
        recipient[i] = vertices_y[i];
        
        // check for NaN
        log_assert(!(working_memory[i] != working_memory[i]));
        log_assert(!(recipient[i] != recipient[i]));        
    }
    
    // working_memory = x * x
    platform_256_mul(working_memory, vertices_x, vertices_and_recipient_size);
    
    for (uint32_t i = 0; i < vertices_and_recipient_size; i++) {
        // check for NaN
        log_assert(!(working_memory[i] != working_memory[i]));
        log_assert(!(recipient[i] != recipient[i]));        
    }
    
    // recipient = y * y
    platform_256_mul(recipient, vertices_y, vertices_and_recipient_size);
    
    for (uint32_t i = 0; i < vertices_and_recipient_size; i++) {
        // check for NaN
        log_assert(!(working_memory[i] != working_memory[i]));
        log_assert(!(recipient[i] != recipient[i]));        
    }
    
    // after this recipient is (x*x)+(y*y), and working memory is ready to be re-used
    platform_256_add(recipient, working_memory, vertices_and_recipient_size);
    
    for (uint32_t i = 0; i < vertices_and_recipient_size; i++) {
        // check for NaN
        log_assert(!(working_memory[i] != working_memory[i]));
        log_assert(!(recipient[i] != recipient[i]));        
    }
    
    for (uint32_t i = 0; i < vertices_and_recipient_size; i++) {
        working_memory[i] = vertices_z[i];
    }
    
    for (uint32_t i = 0; i < vertices_and_recipient_size; i++) {
        // check for NaN
        log_assert(!(working_memory[i] != working_memory[i]));
        log_assert(!(recipient[i] != recipient[i]));        
    }
    
    // working_memory = z * z
    platform_256_mul(working_memory, vertices_z, vertices_and_recipient_size);
    
    
    for (uint32_t i = 0; i < vertices_and_recipient_size; i++) {
        // check for NaN
        log_assert(!(working_memory[i] != working_memory[i]));
        log_assert(!(recipient[i] != recipient[i]));        
    }
    
    // after this, recipient becomes (x*x)+(y*y)+(z*z) 
    platform_256_add(recipient, working_memory, vertices_and_recipient_size);
    
    for (uint32_t i = 0; i < vertices_and_recipient_size; i++) {
        // check for NaN
        log_assert(!(working_memory[i] != working_memory[i]));
        log_assert(!(recipient[i] != recipient[i]));        
    }
    
    // now we can take the root of the sum of squares
    platform_256_sqrt(recipient, vertices_and_recipient_size);
    
    for (uint32_t i = 0; i < vertices_and_recipient_size; i++) {
        // check for NaN
        log_assert(!(working_memory[i] != working_memory[i]));
        log_assert(!(recipient[i] != recipient[i]));        
    }
}

static void normalize_zvertices_inplace(
    float * vertices_x,
    float * vertices_y,
    float * vertices_z,
    float * working_memory,
    const uint32_t vertices_size)
{
    // TODO: remove debug asserts
    zVertex first_tri;
    first_tri.x = vertices_x[0];
    first_tri.y = vertices_y[0];
    first_tri.z = vertices_z[0];
    normalize_zvertex(&first_tri);
    zVertex second_tri;
    second_tri.x = vertices_x[1];
    second_tri.y = vertices_y[1];
    second_tri.z = vertices_z[1];
    normalize_zvertex(&second_tri);
    // // TODO: end of debug code
    
    // TODO: remove debug asserts
    for (uint32_t i = 0; i < vertices_size; i++) {
        // check for NaN
        log_assert(!(vertices_x[i] != vertices_x[i]));
    }
    
    log_assert(vertices_size < MAGNITUDES_CAP);
    get_magnitudes_inplace(
        vertices_x,
        vertices_y,
        vertices_z,
        magnitudes,
        working_memory,
        vertices_size);
    
    // TODO: remove debug asserts
    for (uint32_t i = 0; i < vertices_size; i++) {
        // check for NaN
        log_assert(!(vertices_x[i] != vertices_x[i]));
        zVertex vertex;
        vertex.x = vertices_x[i];
        vertex.y = vertices_y[i];
        vertex.z = vertices_z[i];
        log_assert(magnitudes[i] == get_magnitude(vertex));
    }
    
    platform_256_div(vertices_x, magnitudes, vertices_size);
    platform_256_div(vertices_y, magnitudes, vertices_size);
    platform_256_div(vertices_z, magnitudes, vertices_size);
    
    // TODO: find a better way to deal with NaN
    for (uint32_t i = 0; i < vertices_size; i++) {
        if (vertices_x[i] != vertices_x[i]) {
            vertices_x[i] = 0.0f;
        }
    }
}

void normalize_zvertex(
    zVertex * to_normalize)
{
    float magnitude = get_magnitude(*to_normalize);
    to_normalize->x /= magnitude;
    to_normalize->y /= magnitude;
    to_normalize->z /= magnitude;
}

static void dots_of_vertices(
    const float * vertices_1_x,
    const float * vertices_1_y,
    const float * vertices_1_z,
    const float * vertices_2_x,
    const float * vertices_2_y,
    const float * vertices_2_z,
    const uint32_t vertices_size,
    float * working_memory,
    float * out_dots)
{
    for (uint32_t i = 0; i < vertices_size; i++) {
        out_dots[i] = vertices_1_x[i];
        working_memory[i] = vertices_1_y[i];
    }
    
    platform_256_mul(out_dots, vertices_2_x, vertices_size);
    platform_256_mul(working_memory, vertices_2_y, vertices_size);
    platform_256_add(out_dots, working_memory, vertices_size);
    
    for (uint32_t i = 0; i < vertices_size; i++) {
        working_memory[i] = vertices_1_z[i];
    }
    platform_256_mul(working_memory, vertices_2_z, vertices_size);
    
    platform_256_add(out_dots, working_memory, vertices_size);
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

static void get_ztriangles_normals(
    const float * vertices_x,
    const float * vertices_y,
    const float * vertices_z,
    const uint32_t vertices_size,
    float * out_normals_x,
    float * out_normals_y,
    float * out_normals_z,
    const uint32_t out_normals_size)
{
    zVertex vector1;
    zVertex vector2;
    
    for (uint32_t triangle_i = 0; triangle_i < out_normals_size; triangle_i++) {
        uint32_t vertex_i = triangle_i * 3;
        
        vector1.x = vertices_x[vertex_i + 1] - vertices_x[vertex_i + 0];
        vector1.y = vertices_y[vertex_i + 1] - vertices_y[vertex_i + 0];
        vector1.z = vertices_z[vertex_i + 1] - vertices_z[vertex_i + 0];
        
        vector2.x = vertices_x[vertex_i + 2] - vertices_x[vertex_i + 0];
        vector2.y = vertices_y[vertex_i + 2] - vertices_y[vertex_i + 0];
        vector2.z = vertices_z[vertex_i + 2] - vertices_z[vertex_i + 0];
        
        out_normals_x[triangle_i] = (vector1.y * vector2.z) - (vector1.z * vector2.y);
        out_normals_y[triangle_i] = (vector1.z * vector2.x) - (vector1.x * vector2.z);
        out_normals_z[triangle_i] = (vector1.x * vector2.y) - (vector1.y * vector2.x);
    }
    
    // TODO: understand how to deal with NaN values without all this branching
    for (uint32_t triangle_i = 0; triangle_i < out_normals_size; triangle_i++) {
        if (out_normals_x[triangle_i] != out_normals_x[triangle_i]) {
            out_normals_x[triangle_i] = 0.0f;
        }
        if (out_normals_y[triangle_i] != out_normals_y[triangle_i]) {
            out_normals_y[triangle_i] = 0.0f;
        }
        if (out_normals_z[triangle_i] != out_normals_z[triangle_i]) {
            out_normals_z[triangle_i] = 0.0f;
        }
    }
}

zVertex get_ztriangle_normal(
    const zTriangle * input)
{
    uint32_t vertex_0 = 0;
    uint32_t vertex_1 = 1;
    uint32_t vertex_2 = 2;
    
    zVertex normal;
    zVertex vector1;
    zVertex vector2;
    
    vector1.x = input->vertices[vertex_1].x - input->vertices[vertex_0].x;
    vector1.y = input->vertices[vertex_1].y - input->vertices[vertex_0].y;
    vector1.z = input->vertices[vertex_1].z - input->vertices[vertex_0].z;
    
    vector2.x = input->vertices[vertex_2].x - input->vertices[vertex_0].x;
    vector2.y = input->vertices[vertex_2].y - input->vertices[vertex_0].y;
    vector2.z = input->vertices[vertex_2].z - input->vertices[vertex_0].z;
    
    normal.x = (vector1.y * vector2.z) - (vector1.z * vector2.y);
    normal.y = (vector1.z * vector2.x) - (vector1.x * vector2.z);
    normal.z = (vector1.x * vector2.y) - (vector1.y * vector2.x);
    
    return normal;
}

void get_visibility_ratings(
    const zVertex observer,
    const float * vertices_x,
    const float * vertices_y,
    const float * vertices_z,
    const uint32_t vertices_size,
    float * out_visibility_ratings)
{
    log_assert(vertices_size % 3 == 0);
    
    // find the imaginary position of the observed triangles
    // if we move the entire world so that the observer is at {0,0,0}
    
    for (uint32_t i = 0; i < vertices_size; i++) {
        observeds_adj_x[i] = vertices_x[i];
        observeds_adj_y[i] = vertices_y[i];
        observeds_adj_z[i] = vertices_z[i];
    }
    
    platform_256_sub_scalar(
        observeds_adj_x,
        vertices_size,
        observer.x);
    platform_256_sub_scalar(
        observeds_adj_y,
        vertices_size,
        observer.y);
    platform_256_sub_scalar(
        observeds_adj_z,
        vertices_size,
        observer.z);
    
    // get the normals of the imaginary triangles
    const uint32_t normals_size = vertices_size / 3;
    log_assert(normals_size + 7 < NORMALS_CAP);
    
    get_ztriangles_normals(
        observeds_adj_x,
        observeds_adj_y,
        observeds_adj_z,
        vertices_size,
        normals_x,
        normals_y,
        normals_z,
        normals_size);
    
    normalize_zvertices_inplace(
        normals_x,
        normals_y,
        normals_z,
        magnitudes_working_memory,
        normals_size);
    
    // normalize the adjusted observed triangles
    // we actually only need vertex 0 from each triangle, and
    // we'll be using them again, so let's copy first
    // REMINDER: this changes the observeds_adj_xyz arrays to
    // be effectively 3x smaller!
    for (uint32_t triangle_i = 1; triangle_i < normals_size; triangle_i++) {
        observeds_adj_x[triangle_i] =
            observeds_adj_x[triangle_i * 3];
        observeds_adj_y[triangle_i] =
            observeds_adj_y[triangle_i * 3];
        observeds_adj_z[triangle_i] =
            observeds_adj_z[triangle_i * 3];
    }
    normalize_zvertices_inplace(
        observeds_adj_x,
        observeds_adj_y,
        observeds_adj_z,
        magnitudes_working_memory,
        normals_size);
    
    // finally, get the dot product of each triangle's normal
    // and the 'normalized triangle minus observer' (so a vector
    // pointing from the light or observer to the triangle's 0th vertex)
    dots_of_vertices(
        observeds_adj_x,
        observeds_adj_y,
        observeds_adj_z,
        normals_x,
        normals_y,
        normals_z,
        normals_size,
        magnitudes_working_memory,
        out_visibility_ratings);
}

float get_visibility_rating(
    const zVertex observer,
    const zTriangle * observed)
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
    zVertex normal = get_ztriangle_normal(&observed_adj);
    // TODO: performance bottleneck
    normalize_zvertex(&normal);
    
    // store the 1st vertex as a zVertex so we can
    // use the normalize function
    zVertex triangle_minus_observer;
    triangle_minus_observer.x =
        observed_adj.vertices[0].x;
    triangle_minus_observer.y =
        observed_adj.vertices[0].y;
    triangle_minus_observer.z =
        observed_adj.vertices[0].z;
    
    // TODO: normalize_zvertex is a performance bottleneck
    normalize_zvertex(&triangle_minus_observer);
    
    float return_value = dot_of_vertices(
        normal,
        triangle_minus_observer);
    
    return return_value;
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

float screen_y_to_3d_y(const float screen_y)
{
    float return_value = screen_y / (window_height * 0.5f);
    return_value -= 1.0f;
    
    return return_value;
}

float screen_x_to_3d_x(const float screen_x)
{
    float return_value = screen_x / (window_width * 0.5f);
    return_value -= 1.0f;

    return_value /= (projection_constants.aspect_ratio *
        projection_constants.field_of_view_modifier);
    
    return return_value;
}
