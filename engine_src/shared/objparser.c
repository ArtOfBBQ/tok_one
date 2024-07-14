#include "objparser.h"

static void * (* objparser_malloc_func)(size_t);
static void   (*   objparser_free_func)(void *);

void init_obj_parser(
    void * (* arg_objparser_malloc_func)(size_t),
    void (* arg_objparser_free_func)(void *))
{
    objparser_malloc_func = arg_objparser_malloc_func;
    objparser_free_func   = arg_objparser_free_func;
}

static unsigned int consume_uint(
    char ** raw_buffer,
    unsigned int * good)
{
    #ifndef OBJ_PARSER_IGNORE_ASSERTS
    assert(*raw_buffer[0] >= '0');
    assert(*raw_buffer[0] <= '9');
    #endif
    
    if ((*raw_buffer)[0] < '0' || (*raw_buffer)[0] > '9') {
        *good = 0;
        return 0.0f;
    }
    
    unsigned int return_value = 0;
    
    while ((*raw_buffer)[0] >= '0' && (*raw_buffer)[0] <= '9') {
        unsigned int new_digit = (unsigned int)((*raw_buffer)[0] - '0');
        return_value *= 10;
        return_value += new_digit;
        (*raw_buffer)++;
    }
    
    return return_value;
}

static float consume_float(
    char ** raw_buffer,
    unsigned int * good)
{
    float final_multiplier = 1.0f;
    if ((*raw_buffer)[0] == '-') {
        final_multiplier = -1.0f;
        (*raw_buffer)++;
    }
    
    #ifndef OBJ_PARSER_IGNORE_ASSERTS
    assert((*raw_buffer)[0] >= '0');
    assert((*raw_buffer)[0] <= '9');
    #endif
    
    if ((*raw_buffer)[0] < '0' || (*raw_buffer)[0] > '9') {
        *good = 0;
        return 0.0f;
    }
    
    unsigned int above_comma = consume_uint(raw_buffer, good);
    unsigned int below_comma = 0;
    float below_comma_adj = 0.0f;
    unsigned int below_comma_leading_zeros = 0;
    
    if ((*raw_buffer)[0] == '.') {
        (*raw_buffer)++;
        
        while ((*raw_buffer)[0] == '0') {
            below_comma_leading_zeros += 1;
            (*raw_buffer)++;
        }
        
        if (*raw_buffer[0] >= '0' && *raw_buffer[0] <= '9') {
            below_comma = consume_uint(raw_buffer, good);
        }
        if (!*good) { return above_comma; }
    }
    
    if (below_comma != 0) {
        
        below_comma_adj = below_comma;
        while (below_comma_adj >= 1.0f) {
            below_comma_adj /= 10.0f;
        }
        
        for (unsigned int _ = 0; _ < below_comma_leading_zeros; _++) {
            below_comma_adj /= 10.0f;
        }
    }

    float return_value = ((float)above_comma + below_comma_adj) *
        final_multiplier;
    
    if ((*raw_buffer)[0] == 'e') {
        // 'to the power of 10' adjustment
        (*raw_buffer)++;
        
        unsigned int div_instead = 0;
        if ((*raw_buffer)[0] == '+') {
            (*raw_buffer)++;
        } else if ((*raw_buffer)[0] == '-') {
            div_instead = 1;
            (*raw_buffer)++;
        }
        
        #ifndef OBJ_PARSER_IGNORE_ASSERTS
        assert((*raw_buffer)[0] >= '0');
        assert((*raw_buffer)[0] <= '9');
        #endif
        
        unsigned int e_num = consume_uint(raw_buffer, good);
        if (!good) { return 0.0f; }
        
        unsigned int extracted_mod = 10;
        for (unsigned int _ = 1; _ < e_num; _++) {
            extracted_mod *= 10;
        }
        
        if (e_num == 0) {
            // 0.22fe+0 is just 0.2ff, so just pass
            // kind of weird that ppl would write this but i run into it
        } else if (!div_instead) {
            return_value *= extracted_mod;
        } else {
            return_value /= extracted_mod;
        }
    }
    
    return return_value;
}

static void consume_separated_uints(
    char ** from_buffer,
    int * recipient,
    unsigned int * success)
{
    unsigned int new_num = consume_uint(
        from_buffer,
        success);
    
    if (!*success) { return; }
    
    #ifndef OBJ_PARSER_IGNORE_ASSERTS
    assert(new_num < 2147483647);
    #endif
    recipient[0] = (int)new_num;
    
    unsigned int recipient_i = 0;
    while ((*from_buffer)[0] == '/') {
        
        (*from_buffer)++; // ditch the '/'
        recipient_i += 1;
        
        if ((*from_buffer)[0] == '/') {
            recipient[recipient_i] = -1;
            continue;
        } else {
            new_num = consume_uint(
                from_buffer,
                success);
        }
        
        if (!success) { return; }
        
        #ifndef OBJ_PARSER_IGNORE_ASSERTS
        assert(new_num < 2147483647);
        #endif
        recipient[recipient_i] = (int)new_num;
    }
}

/*
spacenum = space followed by a "number" that may include '/'

this is for counting the number of faces in a line

for example, the string:
"f 3/2/1 3/4/1 2/3/2
this text doesnt matter"

...has 3 faces because there are 3 spaces followed by numbers before the next
line break.
*/
static unsigned int count_upcoming_spacenums(
    const char * in_buffer)
{
    unsigned int return_value = 0;
    
    unsigned int i = 0;
    while (in_buffer[i] != '\n' && in_buffer[i] != '\0') {
        if (in_buffer[i] == ' ') {
            unsigned int j = i;
            while (in_buffer[j] == ' ') {
                j++;
            }
            if (in_buffer[j] >= '0' && in_buffer[j] <= '9') {
                return_value += 1;
                while (
                    (in_buffer[j] >= '0' && in_buffer[j] <= '9') ||
                    in_buffer[j] == '/')
                {
                    j++;
                }
            } else {
                break;
            }
        }
        i++;
    }
    
    return return_value;
}

static int get_material_i_or_register_new(
    ParsedObj * in_obj,
    char * name)
{
    int already_exist_i = -1;
    
    #ifndef OBJ_PARSER_IGNORE_ASSERTS
    assert(name[0] != '\0');
    #endif
    
    unsigned int char_i;
    for (
        unsigned int mat_i = 0;
        mat_i < in_obj->materials_count;
        mat_i++)
    {
        char_i = 0;
        unsigned int match = 1;
        while (name[char_i] != '\0') {
            if (in_obj->materials[mat_i].name[char_i] !=
                name[char_i])
            {
                match = 0;
                break;
            }
            char_i++;
        }
        
        if (!match) {
            continue;
        } else {
            already_exist_i = (int)mat_i;
            break;
        }
    }
    
    if (already_exist_i >= 0) {
        return already_exist_i;
    }
    
    int first_unused_material = -1;
    for (int _ = 0; _ <= (int)in_obj->materials_count; _++) {
        if (in_obj->materials[_].name[0] == '\0') {
            first_unused_material = _;
            break;
        }
    }
    
    #ifndef OBJ_PARSER_IGNORE_ASSERTS
    // no room for more materials? impossible
    assert(first_unused_material >= 0);
    #endif
    
    char_i = 0;
    while (name[char_i] != '\0') {
        in_obj->materials[first_unused_material].
            name[char_i] =
                name[char_i];
        char_i++;
    }
    in_obj->materials[first_unused_material].name[char_i] = '\0';
    
    return first_unused_material;
}

void parse_obj(
    ParsedObj * recipient,
    const char * raw_buf,
    unsigned int * success)
{
    char * raw_buffer = (char *)raw_buf;
    
    #ifndef OBJ_PARSER_IGNORE_ASSERTS
    assert(recipient != 0);
    assert(raw_buffer != 0);
    #endif
    
    *success = 1;
    
    recipient->triangles = 0;
    recipient->triangle_textures = 0;
    recipient->triangle_normals = 0;
    recipient->quads = 0;
    recipient->quad_textures = 0;
    recipient->quad_normals = 0;
    recipient->textures = 0;
    recipient->normals = 0;
    recipient->materials = objparser_malloc_func(sizeof(ParsedMaterial) * 200);
    for (unsigned int i = 0; i < 200; i++) {
        recipient->materials[i].name[0] = '\0';
    }
    
    recipient->vertices_count = 0;
    recipient->textures_count = 0;
    recipient->normals_count = 0;
    recipient->materials_count = 0;
    
    recipient->triangles_count = 0;
    recipient->quads_count = 0;
    
    /*
    1st run: count the number of vertices and faces so we know how much memory
    to allocate.
    */
    unsigned int i = 0;
    while (raw_buffer[i] != '\0') {
        if (raw_buffer[i] == '#') {
            // Ignore comment
            while (raw_buffer[i] != '\n' && raw_buffer[i] != '\0') {
                i++;
            }
            if (raw_buffer[i] == '\n') { i++; };
            continue;
        }
        if (raw_buffer[i] == 'v' && raw_buffer[i + 1] == ' ') {
            recipient->vertices_count += 1;
        }
        if (
            raw_buffer[i + 0] == 'u' &&
            raw_buffer[i + 1] == 's' &&
            raw_buffer[i + 2] == 'e' &&
            raw_buffer[i + 3] == 'm' &&
            raw_buffer[i + 4] == 't' &&
            raw_buffer[i + 5] == 'l' &&
            raw_buffer[i + 6] == ' ')
        {
            i += 7;
            
            char material_name[64];
            unsigned int char_i = 0;
            while (
                raw_buffer[i] != '\n' &&
                raw_buffer[i] != '\r' &&
                raw_buffer[i] != '\0')
            {
                material_name[char_i++] = raw_buffer[i];
                i++;
            }
            material_name[char_i] = '\0';
            
            int new_material_i = get_material_i_or_register_new(
                recipient,
                material_name);
            if (new_material_i >= (int)recipient->materials_count) {
                recipient->materials_count += 1;
            }
            
            #ifndef OBJ_PARSER_IGNORE_ASSERTS
            assert(new_material_i < (int)recipient->materials_count);
            #endif
        }
        if (raw_buffer[i] == 'v' && raw_buffer[i + 1] == 'n') {
            recipient->normals_count += 1;
        }
        if (raw_buffer[i] == 'v' && raw_buffer[i + 1] == 't') {
            recipient->textures_count += 1;
        }
        if (raw_buffer[i] == 'f' && raw_buffer[i + 1] == ' ') {
            /*
            peek ahead to decide if triangle, quad, or whatever
            we are essentially looking for a space(s) followed by a number(s)
            or a 'spacenum'
            */
            unsigned int spacenums_before_lb = count_upcoming_spacenums(
                raw_buffer + i);
            
            if (spacenums_before_lb == 3) {
                recipient->triangles_count += 1;
            } else if (spacenums_before_lb == 4) {
                recipient->quads_count += 1;
            } else {
                // We're not supporting faces with more than 4 vertices for now
                // (we just count spaces, so this can also be triggered by
                // consecutive spaces)
                #ifndef OBJ_PARSER_IGNORE_ASSERTS
                assert(0);
                #endif
                
                *success = 0;
                return;
            }
        }
        i++;
    }
    
    if (recipient->vertices_count < 3) {
        // We didn't even find 1 triangle's worth of vertices?
        #ifndef OBJ_PARSER_IGNORE_ASSERTS
        assert(0);
        #endif
        *success = 0;
        return;
    }
    if (recipient->materials != 0) {
        objparser_free_func(recipient->materials);
        recipient->materials = 0;
    }
    
    recipient->vertices = objparser_malloc_func(
        sizeof(unsigned int[6]) * recipient->vertices_count);
    if (recipient->materials_count > 0) {
        recipient->materials = objparser_malloc_func(
            sizeof(ParsedMaterial) * recipient->materials_count);
        for (unsigned int j = 0; j < recipient->materials_count; j++) {
            recipient->materials[j].name[0] = '\0';
        }
    }
    
    if (recipient->normals_count > 0) {
        recipient->normals = objparser_malloc_func(
            sizeof(unsigned int[3]) * recipient->normals_count);
        
        #ifndef OBJ_PARSER_IGNORE_ASSERTS
        assert(
            recipient->triangles_count + recipient->quads_count > 0);
        #endif
        
        if (recipient->triangles_count > 0) {
            recipient->triangle_normals = objparser_malloc_func(
                sizeof(unsigned int[3]) * recipient->triangles_count);
        }
        
        if (recipient->quads_count > 0) {
            recipient->quad_normals = objparser_malloc_func(
                sizeof(unsigned int[4]) * recipient->quads_count);
        }
    }
    if (recipient->textures_count > 0) {
        recipient->textures = objparser_malloc_func(
            sizeof(float[2]) * recipient->textures_count);
        
        #ifndef OBJ_PARSER_IGNORE_ASSERTS
        assert(
            recipient->triangles_count + recipient->quads_count > 0);
        #endif
        
        if (recipient->triangles_count > 0) {
            recipient->triangle_textures = objparser_malloc_func(
                sizeof(unsigned int[3]) * recipient->triangles_count);
            for (
                unsigned int tri_i = 0;
                tri_i < recipient->triangles_count;
                tri_i++)
            {
                recipient->triangle_textures[tri_i][0] = 1;
                recipient->triangle_textures[tri_i][1] = 1;
                recipient->triangle_textures[tri_i][2] = 1;
            }
        }
        
        if (recipient->quads_count > 0) {
            recipient->quad_textures = objparser_malloc_func(
                sizeof(unsigned int[4]) * recipient->quads_count);
            for (
                unsigned int quad_i = 0;
                quad_i < recipient->quads_count;
                quad_i++)
            {
                recipient->quad_textures[quad_i][0] = 1;
                recipient->quad_textures[quad_i][1] = 1;
                recipient->quad_textures[quad_i][2] = 1;
                recipient->quad_textures[quad_i][3] = 1;
            }
        }
    }
    
    if (recipient->triangles_count > 0) {
        recipient->triangles = objparser_malloc_func(
            sizeof(unsigned int) *
            5 *
            recipient->triangles_count);
    }
    for (unsigned int j = 0; j < recipient->triangles_count; j++) {
        recipient->triangles[j][0] = 0; // vertex index 1
        recipient->triangles[j][1] = 0; // vertex index 2
        recipient->triangles[j][2] = 0; // vertex index 3
        recipient->triangles[j][3] = 0; // smooth shading default = 0
        recipient->triangles[j][4] = 0; // use material 0
    }
    
    if (recipient->quads_count > 0) {
        recipient->quads = objparser_malloc_func(
            sizeof(unsigned int) *
            6 *
            recipient->quads_count);
    }
    for (unsigned int j = 0; j < recipient->quads_count; j++) {
        recipient->quads[j][0] = 0; // vertex index 1
        recipient->quads[j][1] = 0; // vertex index 2
        recipient->quads[j][2] = 0; // vertex index 3
        recipient->quads[j][3] = 0; // vertex index 4
        recipient->quads[j][4] = 0; // smooth shading default = 0
        recipient->quads[j][5] = 0; // use material 0
    }
    
    // run 2: read in the data
    unsigned int cur_vertex_i = 0;
    unsigned int cur_texture_i = 0;
    unsigned int cur_normal_i = 0;
    unsigned int cur_material_i = 0;
    unsigned int cur_triangle_i = 0;
    unsigned int cur_quad_i = 0;
    unsigned int smooth_shading = 0;
    
    while (raw_buffer[0] != '\0') {
        while (raw_buffer[0] == ' ' || raw_buffer[0] == '\n') {
            raw_buffer++;
        }
        
        if (raw_buffer[0] == '\0') {
            #ifndef OBJ_PARSER_IGNORE_ASSERTS
            assert(0);
            #endif
            return;
        }
        
        if (raw_buffer[0] == '#') {
            // comment, ignore
            while (raw_buffer[0] != '\n' && raw_buffer[0] != '\0') {
                raw_buffer++;
            }
        } else if (raw_buffer[0] == 'o' && raw_buffer[1] == ' ') {
            // object spec, ignore
            while (raw_buffer[0] != '\n' && raw_buffer[0] != '\0') {
                raw_buffer++;
            }
        } else if (raw_buffer[0] == 'g' && raw_buffer[1] == ' ') {
            // group spec, ignore
            while (raw_buffer[0] != '\n' && raw_buffer[0] != '\0') {
                raw_buffer++;
            }
        } else if (raw_buffer[0] == 'l' && raw_buffer[1] == ' ') {
            // line spec, ignore for now
            while (raw_buffer[0] != '\n' && raw_buffer[0] != '\0') {
                raw_buffer++;
            }
        } else if (raw_buffer[0] == 's' && raw_buffer[1] == ' ') {
            // change smooth shading status
            
            // ditch the "s "
            raw_buffer += 2;
            
            if (raw_buffer[0] == '1' ||
                (raw_buffer[0] == 'o' && raw_buffer[1] == 'n'))
            {
                smooth_shading = 1;
            } else if (
                raw_buffer[0] == '0' ||
                (
                    raw_buffer[0] == 'o' &&
                    raw_buffer[1] == 'f' &&
                    raw_buffer[2] == 'f'))
            {
                smooth_shading = 0;
            } else {
                // expected 's on', 's off', 's 1', or 's 0'
                #ifndef OBJ_PARSER_IGNORE_ASSERTS
                assert(0);
                #endif
                
                *success = 0;
                return;
            }
            
            while (raw_buffer[0] != '\n' && raw_buffer[0] != '\0') {
                raw_buffer++;
            }
        } else if (
            raw_buffer[0] == 'm' &&
            raw_buffer[1] == 't' &&
            raw_buffer[2] == 'l' &&
            raw_buffer[3] == 'l' &&
            raw_buffer[4] == 'i' &&
            raw_buffer[5] == 'b')
        {
            // mttlib spec, ignore
            while (raw_buffer[0] != '\n' && raw_buffer[0] != '\0') {
                raw_buffer++;
            }
        } else if (raw_buffer[0] == 'v' && raw_buffer[1] == ' ') {
            // vertex data
            
            raw_buffer++; // skip the 'v'
            while (raw_buffer[0] == ' ') {
                raw_buffer++;
            }
            
            for (unsigned int axis_i = 0; axis_i < 3; axis_i++) {
                #ifndef OBJ_PARSER_IGNORE_ASSERTS
                assert(cur_vertex_i < recipient->vertices_count);
                #endif
                recipient->vertices[cur_vertex_i][axis_i] =
                    consume_float(&raw_buffer, success);
                if (!*success) {
                    #ifndef OBJ_PARSER_IGNORE_ASSERTS
                    assert(0);
                    #endif
                    return;
                }
                
                while (raw_buffer[0] == ' ') {
                    raw_buffer++;
                }
            }
            
            cur_vertex_i += 1;
            #ifndef OBJ_PARSER_IGNORE_ASSERTS
            // actually should be < vertices_count, but this may be the last one
            assert(cur_vertex_i <= recipient->vertices_count);
            #endif
        } else if (raw_buffer[0] == 'f' && raw_buffer[1] == ' ') {
            // face data
            
            raw_buffer++; // skip the 'f'
            
            // peek ahead to decide how many vertices in this face
            unsigned int vertices_in_face_count = count_upcoming_spacenums(
                raw_buffer);
            
            while (raw_buffer[0] == ' ') {
                raw_buffer++;
            }
            
            #ifndef OBJ_PARSER_IGNORE_ASSERTS
            assert(vertices_in_face_count > 2);
            #endif
            
            int indexes[4];
            switch (vertices_in_face_count) {
                case 3: {
                    #ifndef OBJ_PARSER_IGNORE_ASSERTS
                    assert(recipient->triangles != 0);
                    assert(recipient->triangles_count > 0);
                    #endif
                    
                    unsigned int consec_entry_i = 0;
                    while (raw_buffer[0] >= '0' && raw_buffer[0] <= '9') {
                        indexes[0] = -1;
                        indexes[1] = -1;
                        indexes[2] = -1;
                        indexes[3] = -1;
                        consume_separated_uints(
                            &raw_buffer,
                            indexes,
                            success);
                        
                        if (!success) {
                            #ifndef OBJ_PARSER_IGNORE_ASSERTS
                            assert(0);
                            #endif
                            return;
                        }
                        
                        if (indexes[0] < 0) {
                            #ifndef OBJ_PARSER_IGNORE_ASSERTS
                            assert(0);
                            #endif
                            success = 0;
                            return;
                        }
                        
                        recipient->triangles[cur_triangle_i][consec_entry_i] =
                            (unsigned int)indexes[0];
                        
                        if (indexes[1] >= 0) {
                            #ifndef OBJ_PARSER_IGNORE_ASSERTS
                            assert(recipient->triangle_textures != 0);
                            assert(indexes[1] >= 1);
                            assert(
                                indexes[1] <= (int)recipient->textures_count);
                            #endif
                            recipient->triangle_textures[cur_triangle_i]
                                [consec_entry_i] = (unsigned int)indexes[1];
                        }
                        
                        if (indexes[2] >= 0) {
                            recipient->triangle_normals[cur_triangle_i]
                                [consec_entry_i] = (unsigned int)indexes[2];
                            #ifndef OBJ_PARSER_IGNORE_ASSERTS
                            assert(recipient->triangle_normals[cur_triangle_i]
                                [consec_entry_i] > 0);
                            assert(recipient->triangle_normals[cur_triangle_i]
                                [consec_entry_i] <= recipient->normals_count);
                            #endif
                        }
                        
                        #ifndef OBJ_PARSER_IGNORE_ASSERTS
                        assert(indexes[3] == -1);
                        #endif
                        
                        if (indexes[3] != -1) {
                            #ifndef OBJ_PARSER_IGNORE_ASSERTS
                            assert(0);
                            #endif
                            *success = 0;
                            return;
                        }
                        
                        while (raw_buffer[0] == ' ') {
                            raw_buffer++;
                        }
                        consec_entry_i += 1;
                    }
                    
                    recipient->triangles[cur_triangle_i][3] = smooth_shading;
                    
                    recipient->triangles[cur_triangle_i][4] = cur_material_i;
                    
                    cur_triangle_i += 1;
                    break;
                }
                case 4: {
                    #ifndef OBJ_PARSER_IGNORE_ASSERTS
                    assert(recipient->quads != 0);
                    assert(recipient->quads_count > 0);
                    assert(cur_quad_i < recipient->quads_count);
                    #endif
                    
                    unsigned int consec_entry_i = 0;
                    while (raw_buffer[0] >= '0' && raw_buffer[0] <= '9') {
                        indexes[0] = -1;
                        indexes[1] = -1;
                        indexes[2] = -1;
                        indexes[3] = -1;
                        consume_separated_uints(
                            &raw_buffer,
                            indexes,
                            success);
                        
                        if (!success) { return; }
                        
                        if (indexes[0] < 0) {
                            #ifndef OBJ_PARSER_IGNORE_ASSERTS
                            assert(0);
                            #endif
                            success = 0;
                            return;
                        }
                        
                        recipient->quads[cur_quad_i][consec_entry_i] =
                            (unsigned int)indexes[0];
                        
                        if (indexes[1] >= 0) {
                            #ifndef OBJ_PARSER_IGNORE_ASSERTS
                            assert(recipient->quad_textures != 0);
                            assert(indexes[1] >= 1);
                            assert(
                                indexes[1] <= (int)recipient->textures_count);
                            #endif
                            recipient->quad_textures[cur_quad_i]
                                [consec_entry_i] = (unsigned int)indexes[1];
                        }
                        
                        if (indexes[2] >= 0) {
                            #ifndef OBJ_PARSER_IGNORE_ASSERTS
                            assert(recipient->quad_normals != 0);
                            #endif
                            recipient->quad_normals[cur_quad_i]
                                [consec_entry_i] = (unsigned int)indexes[2];
                        }
                        
                        while (raw_buffer[0] == ' ') {
                            raw_buffer++;
                        }
                        consec_entry_i += 1;
                    }
                    
                    recipient->quads[cur_quad_i][4] = smooth_shading;
                    
                    recipient->quads[cur_quad_i][5] = cur_material_i;
                    
                    #ifndef OBJ_PARSER_IGNORE_ASSERTS
                    if (recipient->quad_textures != 0) {
                        assert(recipient->quad_textures[cur_quad_i][0] >= 0);
                        assert(recipient->quad_textures[cur_quad_i][1] >= 0);
                        assert(recipient->quad_textures[cur_quad_i][2] >= 0);
                        assert(recipient->quad_textures[cur_quad_i][3] >= 0);
                        assert(recipient->quad_textures[cur_quad_i][0] <=
                            recipient->textures_count);
                        assert(recipient->quad_textures[cur_quad_i][1] <=
                            recipient->textures_count);
                        assert(recipient->quad_textures[cur_quad_i][2] <=
                            recipient->textures_count);
                        assert(recipient->quad_textures[cur_quad_i][3] <=
                            recipient->textures_count);
                    }
                    #endif
                    cur_quad_i += 1;
                    break;
                }
                default: {
                    #ifndef OBJ_PARSER_IGNORE_ASSERTS
                    assert(0);
                    #endif
                }
            }
            
        } else if (raw_buffer[0] == 'v' && raw_buffer[1] == 't') {
            // vertex texture data
            
            raw_buffer++; // skip the 'v'
            raw_buffer++; // skip the 't'
            
            while (raw_buffer[0] == ' ') {
                raw_buffer++;
            }
            
            for (unsigned int uv_i = 0; uv_i < 2; uv_i++) {
                recipient->textures[cur_texture_i][uv_i] =
                    consume_float(&raw_buffer, success);
                if (!*success) { return; }
                
                while (raw_buffer[0] == ' ') {
                    raw_buffer++;
                }
            }
            
            if (raw_buffer[0] >= '0' && raw_buffer[0] <= '9') {
                // w coordinate of uv coordinate
                float w_coordinate = consume_float(&raw_buffer, success);
                (void)w_coordinate;
                
                if (!*success) {
                    #ifndef OBJ_PARSER_IGNORE_ASSERTS
                    assert(0);
                    #endif
                    return;
                }
                
                while (raw_buffer[0] == ' ') {
                    raw_buffer++;
                }
            }
            
            cur_texture_i += 1;
            #ifndef OBJ_PARSER_IGNORE_ASSERTS
            // actually should be < vertices_count, but this may be the last one
            assert(cur_texture_i <= recipient->textures_count);
            #endif
        }  else if (raw_buffer[0] == 'v' && raw_buffer[1] == 'n') {
            
            raw_buffer++; // skip the 'v'
            raw_buffer++; // skip the 'n'
            
            while (raw_buffer[0] == ' ') {
                raw_buffer++;
            }
            
            for (unsigned int axis_i = 0; axis_i < 3; axis_i++) {
                recipient->normals[cur_normal_i][axis_i] =
                    consume_float(&raw_buffer, success);
                if (!*success) { return; }
                
                while (raw_buffer[0] == ' ') {
                    raw_buffer++;
                }
            }
            
            cur_normal_i += 1;
            #ifndef OBJ_PARSER_IGNORE_ASSERTS
            // actually should be < normals_count, but this may be the last one
            assert(cur_normal_i <= recipient->normals_count);
            #endif
            
        } else if (
            raw_buffer[0] == 'u' &&
            raw_buffer[1] == 's' &&
            raw_buffer[2] == 'e' &&
            raw_buffer[3] == 'm' &&
            raw_buffer[4] == 't' &&
            raw_buffer[5] == 'l' &&
            raw_buffer[6] == ' ')
        {
            raw_buffer += 7;
            
            char material_name[64];
            unsigned int char_i = 0;
            while (
                raw_buffer[0] != '\n' &&
                raw_buffer[0] != '\r' &&
                raw_buffer[0] != '\0')
            {
                material_name[char_i++] = raw_buffer[0];
                raw_buffer++;
            }
            material_name[char_i] = '\0';
            
            cur_material_i = (unsigned int)get_material_i_or_register_new(
                recipient,
                material_name);
        } else {
            #ifndef OBJ_PARSER_IGNORE_ASSERTS
            assert(0);
            #endif
            *success = 0;
            return;
        }
        
        if (raw_buffer[0] == '\0') {
            break;
        }
        
        if (raw_buffer[0] != '\n' && raw_buffer[0] != '\r') {
            *success = 0;
            #ifndef OBJ_PARSER_IGNORE_ASSERTS
            assert(0);
            #endif
            return;
        }
        
        while (
            raw_buffer[0] == '\n' ||
            raw_buffer[0] == '\r' ||
            raw_buffer[0] == '\x01' ||
            raw_buffer[0] == '\x04' ||
            raw_buffer[0] == '\xff')
        {
            raw_buffer++; // discard the newline
        }
    }
    
    #ifndef OBJ_PARSER_IGNORE_ASSERTS
    for (unsigned int tri_i = 0; tri_i < recipient->triangles_count; tri_i++) {
        if (recipient->triangle_normals != 0) {
            assert(recipient->triangle_normals[tri_i][0] >= 1);
            assert(recipient->triangle_normals[tri_i][1] >= 1);
            assert(recipient->triangle_normals[tri_i][2] >= 1);
            assert(recipient->triangle_normals[tri_i][0] <=
                recipient->normals_count);
            assert(recipient->triangle_normals[tri_i][1] <=
                recipient->normals_count);
            assert(recipient->triangle_normals[tri_i][2] <=
                recipient->normals_count);
        }
    }
    for (unsigned int quad_i = 0; quad_i < recipient->quads_count; quad_i++) {
        if (recipient->quad_textures != 0) {
            for (unsigned int m = 0; m < 4; m++) {
                assert(recipient->quad_textures[quad_i][m] >= 1);
                assert(
                    (recipient->quad_textures[quad_i][m] - 1) <
                    recipient->textures_count);
            }
        }
    }
    #endif
    
    return;
}

void free_obj(ParsedObj * to_free) {
    
    if (to_free->quads != 0) {
        objparser_free_func(to_free->quads);
        to_free->quads = 0;
    }
    if (to_free->quad_textures != 0) {
        objparser_free_func(to_free->quad_textures);
        to_free->quad_textures = 0;
    }
    if (to_free->triangle_textures != 0) {
        objparser_free_func(to_free->triangle_textures);
    }
    if (to_free->textures != 0) {
        objparser_free_func(to_free->textures);
        to_free->textures = 0;
    }
    if (to_free->quad_normals != 0) {
        objparser_free_func(to_free->quad_normals);
        to_free->quad_normals = 0;
    }
    if (to_free->triangle_normals != 0) {
        objparser_free_func(to_free->triangle_normals);
        to_free->triangle_normals = 0;
    }
    if (to_free->normals != 0) {
        objparser_free_func(to_free->normals);
        to_free->normals = 0;
    }
    if (to_free->materials != 0) {
        objparser_free_func(to_free->materials);
        to_free->materials = 0;
    }
    if (to_free->vertices != 0) {
        objparser_free_func(to_free->vertices);
        to_free->vertices = 0;
    }
}

