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

static float * normals_x_buffer;
static float * normals_y_buffer;
static float * normals_z_buffer;

void init_projection_constants() {
    
    if (window_height < 50.0f || window_width < 50.0f) {
        char unexpected_window_sz_msg[256];
        strcpy_capped(
            unexpected_window_sz_msg,
            256,
            "ERROR: unexpected window size [");
        strcat_int_capped(unexpected_window_sz_msg, 256, (int)window_height);
        strcat_capped(unexpected_window_sz_msg, 256, ",");
        strcat_int_capped(unexpected_window_sz_msg, 256, (int)window_width);
        strcat_capped(unexpected_window_sz_msg, 256, "]\n");
        log_append(unexpected_window_sz_msg);
        log_dump_and_crash(unexpected_window_sz_msg);
    }
    
    ProjectionConstants * pjc = &projection_constants;
    
    pjc->near = 0.01f;
    pjc->far = 1000.0f;
    
    float field_of_view = 90.0f;
    pjc->field_of_view_rad = ((field_of_view * 0.5f) / 180.0f) * 3.14159f;
    pjc->field_of_view_modifier = 1.0f / tanf(pjc->field_of_view_rad);
    
    distances_to_vertices = (float *)malloc_from_unmanaged(DISTANCES_TO_VERTICES_CAP);
    diffused_dots         = (float *)malloc_from_unmanaged(DIFFUSED_DOTS_CAP);
    normals_x_buffer      = (float *)malloc_from_unmanaged_aligned((SIMD_FLOAT_WIDTH * 4)+32, 32);
    normals_y_buffer      = (float *)malloc_from_unmanaged_aligned((SIMD_FLOAT_WIDTH * 4)+32, 32);
    normals_z_buffer      = (float *)malloc_from_unmanaged_aligned((SIMD_FLOAT_WIDTH * 4)+32, 32);
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
        
        char dbg_newline[30];
        strcpy_capped(dbg_newline, 30, rawdata + i);
        uint32_t dbg_i = 0;
        while (dbg_i < 29 && dbg_newline[dbg_i] != '\n') {
            dbg_i++;
        }
        dbg_newline[dbg_i] = '\0';
        
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
    
    if (return_value.triangles_size >= POLYGON_TRIANGLES_SIZE) {
        char error_msg[100];
        strcpy_capped(error_msg, 100, "Error: POLYGON_TRIANGLES_SIZE was ");
        strcat_uint_capped(error_msg, 100, POLYGON_TRIANGLES_SIZE);
        strcat_capped(error_msg, 100, ", but return_value.triangles_size is ");
        strcat_uint_capped(error_msg, 100, return_value.triangles_size);
        log_dump_and_crash(error_msg);
        assert(0);
    }
    
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
                        
            log_append("read 1st vertex index\n");
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
            
            log_append("read 2nd vertex index\n");
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
            
            log_append("read 3rd vertex index\n");
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
            
            while (rawdata[i] == ' ') { i++; }
            
            if (rawdata[i] != '\n') {
                log_append("read 4th vertex index\n");
                int32_t vertex_i_3 = string_to_int32(rawdata + i);
                i += chars_till_next_space_or_slash(
                    rawdata + i);
                int32_t uv_coord_i_3 = 0;
                if (rawdata[i] == '/')
                {
                    // skip the slash
                    i++;
                    uv_coord_i_3 =
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
                
                // there were 2 triangles in this face
                // the 1st triangle will be added anyway later, but
                // we do need to add the extra 2nd triangle here
                zTriangle new_triangle;
                new_triangle.visible = 1;
                
                log_assert(vertex_i_0 != vertex_i_1);
                log_assert(vertex_i_0 != vertex_i_2);
                log_assert(vertex_i_0 > 0);
                log_assert(vertex_i_1 > 0);
                log_assert(vertex_i_2 > 0);
                log_assert(uv_coord_i_0 < LOADING_OBJ_BUF_SIZE);
                log_assert(uv_coord_i_1 < LOADING_OBJ_BUF_SIZE);
                log_assert(uv_coord_i_2 < LOADING_OBJ_BUF_SIZE);
                
                uint32_t target_vertex_0 = 0;
                uint32_t target_vertex_1 = 1;
                uint32_t target_vertex_2 = 2;
                
                new_triangle.vertices[target_vertex_0] =
                    new_vertices[vertex_i_0 - 1];
                new_triangle.vertices[target_vertex_1] =
                    new_vertices[vertex_i_2 - 1];
                new_triangle.vertices[target_vertex_2] =
                    new_vertices[vertex_i_3 - 1];
                
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
                    uv_u[uv_coord_i_2 - 1];
                    new_triangle.vertices[target_vertex_1].uv[1] =
                    uv_v[uv_coord_i_2 - 1];
                    new_triangle.vertices[target_vertex_2].uv[0] =
                    uv_u[uv_coord_i_3 - 1];
                    new_triangle.vertices[target_vertex_2].uv[1] =
                    uv_v[uv_coord_i_3 - 1];
                }
                
                new_triangle.color[0] = using_color[0];
                new_triangle.color[1] = using_color[1];
                new_triangle.color[2] = using_color[2];
                new_triangle.color[3] = using_color[3];
                
                new_triangle.texturearray_i = using_texturearray_i;
                new_triangle.texture_i = using_texture_i;
                
                log_append("4 vertex face, return_value.triangles_size adjusted to: ");
                log_append_uint(return_value.triangles_size);
                log_append_char('\n');
                return_value.triangles_size += 1;
                log_assert(new_triangle_i < POLYGON_TRIANGLES_SIZE);
                
                return_value.triangles[new_triangle_i] = new_triangle;
                new_triangle_i++;
            } else {
                // there was only 1 triangle
            }
            
            // if you get here there was only 1 triangle OR
            // there were 2 triangles and you already did the other one
            zTriangle new_triangle;
            new_triangle.visible = 1;
            
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
            
            log_assert(new_triangle_i < ZPOLYGONS_TO_RENDER_ARRAYSIZE);
            return_value.triangles[new_triangle_i] = new_triangle;
            new_triangle_i++;
            log_append("triangles finished parsing: ");
            log_append_uint(new_triangle_i);
            log_append_char('\n');
            
            log_assert(rawdata[i] == '\n');
            i++;
            
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
    
    if (return_value.triangles_size >= POLYGON_TRIANGLES_SIZE) {
        char error_msg[100];
        strcpy_capped(error_msg, 100, "Error: POLYGON_TRIANGLES_SIZE was ");
        strcat_uint_capped(error_msg, 100, POLYGON_TRIANGLES_SIZE);
        strcat_capped(error_msg, 100, ", but return_value.triangles_size is ");
        strcat_uint_capped(error_msg, 100, return_value.triangles_size);
        strcat_capped(error_msg, 100, "\n");
        log_dump_and_crash(error_msg);
        assert(0);
    }
    
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
    const float new_size)
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
    log_assert(largest_width > 0.0f);
    
    float width_scale_factor = new_size / largest_width;
    float height_scale_factor = new_size / largest_height;
    float scale_factor =
        ((width_scale_factor > height_scale_factor) * height_scale_factor) +
        ((width_scale_factor <= height_scale_factor) * width_scale_factor);
    
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
    
    SIMD_FLOAT simd_light_source_x = simd_set_float(zlight_source->x);
    SIMD_FLOAT simd_light_source_y = simd_set_float(zlight_source->y);
    SIMD_FLOAT simd_light_source_z = simd_set_float(zlight_source->z);
    SIMD_FLOAT simd_light_reach    = simd_set_float(zlight_source->reach);
    
    distances_to_vertices_size = vertices_size;
    assert(distances_to_vertices_size < DISTANCES_TO_VERTICES_CAP);
    
    // get distance from triangle vertices to light_source    
    for (uint32_t i = 0; i < vertices_size; i += SIMD_FLOAT_WIDTH) {
        SIMD_FLOAT simd_vertices_x = simd_load_floats(vertices_x + i);
        SIMD_FLOAT simd_vertices_y = simd_load_floats(vertices_y + i);
        SIMD_FLOAT simd_vertices_z = simd_load_floats(vertices_z + i);
        
        SIMD_FLOAT simd_diff_x = simd_sub_floats(simd_vertices_x, simd_light_source_x);
        SIMD_FLOAT simd_diff_y = simd_sub_floats(simd_vertices_y, simd_light_source_y);
        SIMD_FLOAT simd_diff_z = simd_sub_floats(simd_vertices_z, simd_light_source_z);
        
        simd_diff_x = simd_mul_floats(simd_diff_x, simd_diff_x);
        simd_diff_y = simd_mul_floats(simd_diff_y, simd_diff_y);
        simd_diff_z = simd_mul_floats(simd_diff_z, simd_diff_z);
        
        SIMD_FLOAT total_dist = simd_add_floats(simd_diff_x, simd_diff_y);
        total_dist = simd_add_floats(total_dist, simd_diff_z);
        
        total_dist = simd_sqrt_floats(total_dist);
        
        total_dist = simd_div_floats(simd_light_reach, total_dist);
        
        simd_store_floats(distances_to_vertices + i, total_dist);
    }
        
    log_assert(vertices_size < DIFFUSED_DOTS_CAP);
    
    get_visibility_ratings(
        /* const zVertex observer         : */ light_source_pos,
        /* triangle vertices x            : */ vertices_x,
        /* triangle vertices y            : */ vertices_y,
        /* triangle vertices z            : */ vertices_z,
        /* const uint32_t observeds_size  : */ vertices_size,
        /* float * out_visibility_ratings : */ diffused_dots);
    
    // Reminder: it's necessary to have the float's address on ARM
    // for these macros to work. Don't delete the stack floats on your intel
    // machine - it will work but you will break the ARM build
    float minus_one_scalar = -1.0f;
    SIMD_FLOAT simd_minus_one_scalar = simd_set_float(minus_one_scalar);
    float zero_scalar = 0.0f;
    SIMD_FLOAT simd_zero_scalar = simd_set_float(zero_scalar);
    for (uint32_t i = 0; i < vertices_size; i += SIMD_FLOAT_WIDTH) {
        SIMD_FLOAT simd_diffused_dots = simd_load_floats(diffused_dots + i);
        
        simd_diffused_dots = simd_mul_floats(simd_diffused_dots, simd_minus_one_scalar);
        simd_diffused_dots = simd_max_floats(simd_diffused_dots, simd_zero_scalar);
        
        simd_store_floats(diffused_dots + i, simd_diffused_dots);
    }
    
    for (uint32_t col_i = 0; col_i < 3; col_i++) {
        
        float ambient_mod = zlight_source->RGBA[col_i] * zlight_source->ambient;
        float diffuse_mod = zlight_source->RGBA[col_i] * zlight_source->diffuse;
        log_assert(diffuse_mod >= 0.0f);
        log_assert(ambient_mod >= 0.0f);
        
        for (uint32_t vertex_i = 0; vertex_i < vertices_size; vertex_i++) {
            log_assert(vertex_i < recipients_size);
            
            // *******************************************
            // add ambient lighting   
            recipients[vertex_i].lighting[col_i] +=
                ambient_mod *
                distances_to_vertices[vertex_i] *
                lighting_multipliers[vertex_i];
            
            // *******************************************
            // add diffuse lighting                
            recipients[vertex_i].lighting[col_i] +=
                diffused_dots[vertex_i] *
                diffuse_mod *
                distances_to_vertices[vertex_i] *
                lighting_multipliers[vertex_i];
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

void ztriangles_to_2d_inplace(
    float * vertices_x,
    float * vertices_y,
    float * vertices_z,
    const uint32_t vertices_size)
{
    ProjectionConstants * pjc = &projection_constants;
    
    // Reminder: it's necessary to have the float's address on ARM
    // for these macros to work. Don't delete the stack floats on your intel
    // machine - it will work but you will break the ARM build
    float x_multiplier = aspect_ratio * pjc->field_of_view_modifier;
    SIMD_FLOAT simd_x_multiplier = simd_set_float(x_multiplier);
    float y_multiplier = pjc->field_of_view_modifier;
    SIMD_FLOAT simd_y_multiplier = simd_set_float(y_multiplier);
    float z_multiplier = (pjc->far / (pjc->far - pjc->near));
    SIMD_FLOAT simd_z_multiplier = simd_set_float(z_multiplier);
    float z_addition = (1.0f * (-pjc->far * pjc->near) /
        (pjc->far - pjc->near));
    SIMD_FLOAT simd_z_addition   = simd_set_float(z_addition);
    
    for (uint32_t i = 0; i < vertices_size; i += SIMD_FLOAT_WIDTH) {
        SIMD_FLOAT simd_vertices_x = simd_load_floats(vertices_x + i);
        SIMD_FLOAT simd_vertices_y = simd_load_floats(vertices_y + i);
        SIMD_FLOAT simd_vertices_z = simd_load_floats(vertices_z + i);
        
        simd_vertices_x = simd_mul_floats(simd_vertices_x, simd_x_multiplier);
        simd_vertices_y = simd_mul_floats(simd_vertices_y, simd_y_multiplier);
        simd_vertices_z = simd_mul_floats(simd_vertices_z, simd_z_multiplier);
        simd_vertices_z = simd_add_floats(simd_vertices_z, simd_z_addition);
        
        simd_store_floats(vertices_x + i, simd_vertices_x);
        simd_store_floats(vertices_y + i, simd_vertices_y);
        simd_store_floats(vertices_z + i, simd_vertices_z);
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

static SIMD_FLOAT simd_get_magnitudes(
    const SIMD_FLOAT vertices_x,
    const SIMD_FLOAT vertices_y,
    const SIMD_FLOAT vertices_z)
{    
    SIMD_FLOAT x_squared = simd_mul_floats(vertices_x, vertices_x);
    SIMD_FLOAT y_squared = simd_mul_floats(vertices_y, vertices_y);
    SIMD_FLOAT z_squared = simd_mul_floats(vertices_z, vertices_z);
    SIMD_FLOAT sum_squares = simd_add_floats(x_squared, y_squared);
    sum_squares = simd_add_floats(sum_squares, z_squared);
    
    return simd_sqrt_floats(sum_squares);
}

static void simd_normalize_zvertices_inplace(
    SIMD_FLOAT * simd_vertices_x,
    SIMD_FLOAT * simd_vertices_y,
    SIMD_FLOAT * simd_vertices_z)
{
    SIMD_FLOAT simd_magnitudes = simd_get_magnitudes(
        *simd_vertices_x,
        *simd_vertices_y,
        *simd_vertices_z);
    
    *simd_vertices_x = simd_div_floats(*simd_vertices_x, simd_magnitudes);
    *simd_vertices_y = simd_div_floats(*simd_vertices_y, simd_magnitudes);
    *simd_vertices_z = simd_div_floats(*simd_vertices_z, simd_magnitudes);
}

void normalize_zvertex(
    zVertex * to_normalize)
{
    float magnitude = get_magnitude(*to_normalize);
    to_normalize->x /= magnitude;
    to_normalize->y /= magnitude;
    to_normalize->z /= magnitude;
}

static SIMD_FLOAT simd_dots_of_vertices(
    const SIMD_FLOAT simd_vertices_1_x,
    const SIMD_FLOAT simd_vertices_1_y,
    const SIMD_FLOAT simd_vertices_1_z,
    const SIMD_FLOAT simd_vertices_2_x,
    const SIMD_FLOAT simd_vertices_2_y,
    const SIMD_FLOAT simd_vertices_2_z)
{
    SIMD_FLOAT result = simd_mul_floats(
        simd_vertices_1_x,
        simd_vertices_2_x);
    
    SIMD_FLOAT vertices_product =  simd_mul_floats(
        simd_vertices_1_y,
        simd_vertices_2_y);
    
    result = simd_add_floats(result, vertices_product);
    vertices_product =  simd_mul_floats(simd_vertices_1_z, simd_vertices_2_z);
    result = simd_add_floats(result, vertices_product);
    
    return result;
}

static void dots_of_vertices(
    const float * vertices_1_x,
    const float * vertices_1_y,
    const float * vertices_1_z,
    const float * vertices_2_x,
    const float * vertices_2_y,
    const float * vertices_2_z,
    const uint32_t vertices_size,
    float * out_dots)
{
    for (uint32_t i = 0; i < vertices_size; i++) {
        out_dots[i] = vertices_1_x[i];
    }
    
    for (uint32_t i = 0; i < vertices_size; i += SIMD_FLOAT_WIDTH) {
        SIMD_FLOAT simd_vertices_1_x = simd_load_floats(vertices_1_x + i);
        SIMD_FLOAT simd_vertices_1_y = simd_load_floats(vertices_1_y + i);
        SIMD_FLOAT simd_vertices_1_z = simd_load_floats(vertices_1_z + i);
        
        SIMD_FLOAT simd_vertices_2_x = simd_load_floats(vertices_2_x + i);
        SIMD_FLOAT simd_vertices_2_y = simd_load_floats(vertices_2_y + i);
        SIMD_FLOAT simd_vertices_2_z = simd_load_floats(vertices_2_z + i);
        
        SIMD_FLOAT result = simd_mul_floats(simd_vertices_1_x, simd_vertices_2_x);
        SIMD_FLOAT vertices_product =  simd_mul_floats(simd_vertices_1_y, simd_vertices_2_y);
        result = simd_add_floats(result, vertices_product);
        vertices_product =  simd_mul_floats(simd_vertices_1_z, simd_vertices_2_z);
        result = simd_add_floats(result, vertices_product);
        
        simd_store_floats(out_dots + i, result);
    }
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

static void simd_get_normals(
    const float * vertices_x,
    const float * vertices_y,
    const float * vertices_z,
    const int32_t first_tri_vertex_offset,
    SIMD_FLOAT * out_normals_x,
    SIMD_FLOAT * out_normals_y,
    SIMD_FLOAT * out_normals_z)
{
    log_assert(first_tri_vertex_offset < 1);
    
    zVertex vector1;
    zVertex vector2;
    
    for (
        int32_t vertex_i = first_tri_vertex_offset;
        vertex_i < SIMD_FLOAT_WIDTH;
        vertex_i += 3)
    {
        vector1.x = vertices_x[vertex_i + 1] - vertices_x[vertex_i + 0];
        vector1.y = vertices_y[vertex_i + 1] - vertices_y[vertex_i + 0];
        vector1.z = vertices_z[vertex_i + 1] - vertices_z[vertex_i + 0];
        
        vector2.x = vertices_x[vertex_i + 2] - vertices_x[vertex_i + 0];
        vector2.y = vertices_y[vertex_i + 2] - vertices_y[vertex_i + 0];
        vector2.z = vertices_z[vertex_i + 2] - vertices_z[vertex_i + 0];
        
        // TODO: calculate a normal for each vertex instead of copying 1st
        float cur_tri_normal_x = (vector1.y * vector2.z) - (vector1.z * vector2.y); 
        float cur_tri_normal_y = (vector1.z * vector2.x) - (vector1.x * vector2.z);
        float cur_tri_normal_z = (vector1.x * vector2.y) - (vector1.y * vector2.x); 
        
        log_assert(vertex_i + 2 >= 0);
        normals_x_buffer[vertex_i+2] = cur_tri_normal_x;
        normals_y_buffer[vertex_i+2] = cur_tri_normal_y;
        normals_z_buffer[vertex_i+2] = cur_tri_normal_z;
        
        if (vertex_i + 1 < 0) { continue; }
        normals_x_buffer[vertex_i+1] = cur_tri_normal_x;
        normals_y_buffer[vertex_i+1] = cur_tri_normal_y;
        normals_z_buffer[vertex_i+1] = cur_tri_normal_z;
        
        if (vertex_i < 0) { continue; }
        normals_x_buffer[vertex_i  ] = cur_tri_normal_x;
        normals_y_buffer[vertex_i  ] = cur_tri_normal_y;
        normals_z_buffer[vertex_i  ] = cur_tri_normal_z;        
    }
    
    *out_normals_x = simd_load_floats(normals_x_buffer);
    *out_normals_y = simd_load_floats(normals_y_buffer);
    *out_normals_z = simd_load_floats(normals_z_buffer);
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
    
    // get the normals of the triangles
    // log_assert(vertices_size + 7 < NORMALS_CAP);
    
    SIMD_FLOAT simd_observer_x = simd_set_float(observer.x);
    SIMD_FLOAT simd_observer_y = simd_set_float(observer.y);
    SIMD_FLOAT simd_observer_z = simd_set_float(observer.z);
    
    SIMD_FLOAT simd_normals_x;
    SIMD_FLOAT simd_normals_y;
    SIMD_FLOAT simd_normals_z;
    
    for (uint32_t i = 0; i < vertices_size; i += SIMD_FLOAT_WIDTH) {
        int32_t first_tri_vertex_offset = -1 * ((int32_t)i % 3);
        simd_get_normals(
            vertices_x + i,
            vertices_y + i,
            vertices_z + i,
            first_tri_vertex_offset,
            &simd_normals_x,
            &simd_normals_y,
            &simd_normals_z);
        simd_normalize_zvertices_inplace(
            &simd_normals_x,
            &simd_normals_y,
            &simd_normals_z);
        
        SIMD_FLOAT simd_adj_x = simd_load_floats(vertices_x + i);
        SIMD_FLOAT simd_adj_y = simd_load_floats(vertices_y + i);
        SIMD_FLOAT simd_adj_z = simd_load_floats(vertices_z + i);
        
        simd_adj_x = simd_sub_floats(simd_adj_x, simd_observer_x);
        simd_adj_y = simd_sub_floats(simd_adj_y, simd_observer_y);
        simd_adj_z = simd_sub_floats(simd_adj_z, simd_observer_z);
        
        simd_normalize_zvertices_inplace(
            &simd_adj_x,
            &simd_adj_y,
            &simd_adj_z);
        
        SIMD_FLOAT dot_result = simd_dots_of_vertices(
            simd_adj_x,
            simd_adj_y,
            simd_adj_z,
            simd_normals_x,
            simd_normals_y,
            simd_normals_z);
        
        simd_store_floats(out_visibility_ratings + i, dot_result);
    }
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
