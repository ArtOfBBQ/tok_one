#include "objmodel.h"

zTriangle * all_meshes;
uint32_t all_meshes_size = 0;

void init_all_meshes(void) {
    all_meshes = (zTriangle *)malloc_from_unmanaged(
        sizeof(zTriangle) * ALL_MESH_TRIANGLES_SIZE);
    
    // Let's hardcode a basic quad since that's a mesh that will be used by
    // pretty much every app and has negligible cost
    // this also illustrates what will happen if we read a .obj file with the
    // parser: we grab the data below from the file and fill in all_meshes
    // with the triangles
    const float left_vertex     = -1.0f;
    const float right_vertex    = 1.0f;
    const float top_vertex      = 1.0f;
    const float bottom_vertex   = -1.0f;
    const float left_uv_coord   = 0.0f;
    const float right_uv_coord  = 1.0f;
    const float bottom_uv_coord = 1.0f;
    const float top_uv_coord    = 0.0f;
    
    all_meshes[0].normal.x = 0.0f;
    all_meshes[0].normal.y = 0.0f;
    all_meshes[0].normal.z = -1.0f;
    all_meshes[0].parent_material_i = 0;
    // top left vertex
    all_meshes[0].vertices[0].x     = left_vertex;
    all_meshes[0].vertices[0].y     = top_vertex;
    all_meshes[0].vertices[0].z     = 0.0f;
    all_meshes[0].vertices[0].uv[0] = left_uv_coord;
    all_meshes[0].vertices[0].uv[1] = top_uv_coord;
    // top right vertex
    all_meshes[0].vertices[1].x     = right_vertex;
    all_meshes[0].vertices[1].y     = top_vertex;
    all_meshes[0].vertices[1].z     = 0.0f;
    all_meshes[0].vertices[1].uv[0] = right_uv_coord;
    all_meshes[0].vertices[1].uv[1] = top_uv_coord;
    // bottom left vertex
    all_meshes[0].vertices[2].x     = left_vertex;
    all_meshes[0].vertices[2].y     = bottom_vertex;
    all_meshes[0].vertices[2].z     = 0.0f;
    all_meshes[0].vertices[2].uv[0] = left_uv_coord;
    all_meshes[0].vertices[2].uv[1] = bottom_uv_coord;
    
    all_meshes[1].normal.x = 0.0f;
    all_meshes[1].normal.y = 0.0f;
    all_meshes[1].normal.z = -1.0f;
    all_meshes[1].parent_material_i = 0;
    // top right vertex
    all_meshes[1].vertices[0].x = right_vertex;
    all_meshes[1].vertices[0].y = top_vertex;
    all_meshes[1].vertices[0].z = 0.0f;
    all_meshes[1].vertices[0].uv[0] = right_uv_coord;
    all_meshes[1].vertices[0].uv[1] = top_uv_coord;
    // bottom right vertex
    all_meshes[1].vertices[1].x = right_vertex;
    all_meshes[1].vertices[1].y = bottom_vertex;
    all_meshes[1].vertices[1].z = 0.0f;
    all_meshes[1].vertices[1].uv[0] = right_uv_coord;
    all_meshes[1].vertices[1].uv[1] = bottom_uv_coord;
    // bottom left vertex
    all_meshes[1].vertices[2].x = left_vertex;
    all_meshes[1].vertices[2].y = bottom_vertex;
    all_meshes[1].vertices[2].z = 0.0f;
    all_meshes[1].vertices[2].uv[0] = left_uv_coord;
    all_meshes[1].vertices[2].uv[1] = bottom_uv_coord;
    
    all_meshes_size = 2;
}

static uint32_t chars_till_next_space_or_slash(
    char * buffer)
{
    uint32_t i = 0;
    
    while (
        buffer[i] != '\n' &&
        buffer[i] != ' ' &&
        buffer[i] != '/')
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
void parse_obj(
    char * rawdata,
    uint64_t rawdata_size,
    const bool32_t flip_winding,
    zTriangle * recipient,
    uint32_t * recipient_size)
{
    parse_obj_expecting_materials(
        rawdata,
        rawdata_size,
        NULL,
        0,
        flip_winding,
        recipient,
        recipient_size);
}

void parse_obj_expecting_materials(
    char * rawdata,
    uint64_t rawdata_size,
    ExpectedObjMaterials * expected_materials,
    const uint32_t expected_materials_size,
    const bool32_t flip_winding,
    zTriangle * recipient,
    uint32_t * recipient_size)
{
    log_assert(rawdata != NULL);
    log_assert(rawdata_size > 0);
    
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
                *recipient_size += 1;
            }
            // skip until the next line break character
            while (rawdata[i] != '\n' && rawdata[i] != '\0') {
                i++;
            }
            
            // skip the line break character
            i++;
        }
    }
    
    log_assert(*recipient_size > 0);
    
    if (*recipient_size >= ALL_MESH_TRIANGLES_SIZE) {
        char error_msg[100];
        strcpy_capped(error_msg, 100, "Error: POLYGON_TRIANGLES_SIZE was ");
        strcat_uint_capped(error_msg, 100, ALL_MESH_TRIANGLES_SIZE);
        strcat_capped(error_msg, 100, ", but recipient->triangles_size is ");
        strcat_uint_capped(error_msg, 100, *recipient_size);
        log_dump_and_crash(error_msg);
        assert(0);
    }
    
    i = first_material_or_face_i;
    uint32_t new_triangle_i = 0;
    int32_t using_material_i = 0;
    
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
                            get_string_length(expected_mtl)))
                    {
                        using_material_i = mtl_i;
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
            
            if (rawdata[i] != '\n' && rawdata[i] != '\r') {
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
                
                new_triangle.parent_material_i = using_material_i;
                
                *recipient_size += 1;
                log_assert(new_triangle_i < ALL_MESH_TRIANGLES_SIZE);
                
                recipient[new_triangle_i] = new_triangle;
                new_triangle_i++;
            } else {
                // there was only 1 triangle
            }
            
            // if you get here there was only 1 triangle OR
            // there were 2 triangles and you already did the other one
            zTriangle new_triangle;
            
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
            
            new_triangle.parent_material_i = using_material_i;
            
            recipient[new_triangle_i] = new_triangle;
            new_triangle_i++;
            
            log_assert(rawdata[i] == '\n' || rawdata[i] == '\r');
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
}

int32_t new_mesh_head_id_from_resource(
    const char * filename)
{
    int32_t new_mesh_head_id = all_meshes_size;
    
    FileBuffer obj_file;
    
    obj_file.size = platform_get_resource_size(filename);
    obj_file.contents = (char *)malloc_from_managed(obj_file.size);
    obj_file.good = false;
    
    platform_read_resource_file(
        /* char * filename: */
            filename,
        /* FileBuffer *out_preallocatedbuffer: */
            &obj_file);
    
    log_assert(obj_file.good);
    
    parse_obj(
        /* char * rawdata: */ obj_file.contents,
        /* uint64_t rawdata_size: */ obj_file.size,
        /* const uint32_t flip_winding: */ false,
        /* zTriangle recipient: */ all_meshes + new_mesh_head_id,
        /* uint32_t * recipient_size: */ &all_meshes_size);
    
    log_assert((int32_t)all_meshes_size > new_mesh_head_id);
    
    return new_mesh_head_id;
}
