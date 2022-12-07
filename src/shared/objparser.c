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
    #define LOADING_OBJ_BUF_SIZE 16000
    zVertex * new_vertices = (zVertex *)malloc_from_managed(
        sizeof(zVertex) * LOADING_OBJ_BUF_SIZE);
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
