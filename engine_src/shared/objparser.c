#include "objparser.h"

static void * (* malloc_function)(size_t);
static void   (*   free_function)(void *);

void init_obj_parser(
    void * (* arg_malloc_function)(size_t),
    void (* arg_free_function)(void *))
{
    malloc_function = arg_malloc_function;
    free_function = arg_free_function;
}

static uint32_t consume_uint(
    char ** raw_buffer,
    uint32_t * good)
{
    #ifndef OBJ_PARSER_NO_ASSERTS
    assert(*raw_buffer[0] >= '0');
    assert(*raw_buffer[0] <= '9');
    #endif
    
    if ((*raw_buffer)[0] < '0' || (*raw_buffer)[0] > '9') {
        *good = 0;
        return 0.0f;
    }
    
    uint32_t return_value = 0;
    
    while ((*raw_buffer)[0] >= '0' && (*raw_buffer)[0] <= '9') {
        uint32_t new_digit = (*raw_buffer)[0] - '0';
        return_value *= 10;
        return_value += new_digit;
        (*raw_buffer)++;
    }
    
    return return_value;
}

static float consume_float(
    char ** raw_buffer,
    uint32_t * good)
{
    float final_multiplier = 1.0f;
    if ((*raw_buffer)[0] == '-') {
        final_multiplier = -1.0f;
        (*raw_buffer)++;
    }
    
    #ifndef OBJ_PARSER_NO_ASSERTS
    assert((*raw_buffer)[0] >= '0');
    assert((*raw_buffer)[0] <= '9');
    #endif
    
    if ((*raw_buffer)[0] < '0' || (*raw_buffer)[0] > '9') {
        *good = 0;
        return 0.0f;
    }
    
    uint32_t above_comma = consume_uint(raw_buffer, good);
    uint32_t below_comma = 0;
    float below_comma_adj = 0.0f;
    uint32_t below_comma_leading_zeros = 0;
    
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
        
        for (uint32_t _ = 0; _ < below_comma_leading_zeros; _++) {
            below_comma_adj /= 10.0f;
        }
    }

    float return_value = ((float)above_comma + below_comma_adj) *
        final_multiplier;
    
    if ((*raw_buffer)[0] == 'e') {
        // 'to the power of 10' adjustment
        (*raw_buffer)++;
        
        uint32_t div_instead = 0;
        if ((*raw_buffer)[0] == '+') {
            (*raw_buffer)++;
        } else if ((*raw_buffer)[0] == '-') {
            div_instead = 1;
            (*raw_buffer)++;
        }
        
        #ifndef OBJ_PARSER_NO_ASSERTS
        assert((*raw_buffer)[0] >= '0');
        assert((*raw_buffer)[0] <= '9');
        #endif
        
        uint32_t e_num = consume_uint(raw_buffer, good);
        if (!good) { return 0.0f; }
        
        uint32_t extracted_mod = 10;
        for (uint32_t _ = 1; _ < e_num; _++) {
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
    int32_t * recipient,
    uint32_t * success)
{
    uint32_t new_num = consume_uint(
        from_buffer,
        success);
    
    if (!*success) { return; }
    
    #ifndef OBJ_PARSER_NO_ASSERTS
    assert(new_num < INT32_MAX);
    #endif
    recipient[0] = (int32_t)new_num;
    
    uint32_t recipient_i = 0;
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
        
        #ifndef OBJ_PARSER_NO_ASSERTS
        assert(new_num < INT32_MAX);
        #endif
        recipient[recipient_i] = (int32_t)new_num;
    }
}

void parse_obj(
    ParsedObj * recipient,
    char * raw_buffer,
    uint32_t * success)
{
    #ifndef OBJ_PARSER_NO_ASSERTS
    assert(recipient != NULL);
    assert(raw_buffer != NULL);
    #endif
    
    *success = 1;
    
    recipient->triangles = NULL;
    recipient->triangle_textures = NULL;
    recipient->triangle_normals = NULL;
    recipient->quads = NULL;
    recipient->quad_textures = NULL;
    recipient->quad_normals = NULL;
    recipient->textures = NULL;
    recipient->normals = NULL;
    
    recipient->vertices_count = 0;
    recipient->textures_count = 0;
    recipient->normals_count = 0;
    
    recipient->triangles_count = 0;
    recipient->quads_count = 0;
    
    /*
    1st run: count the number of vertices and faces so we know how much memory
    to allocate.
    */
    uint32_t i = 0;
    while (raw_buffer[i] != '\0') {
        if (raw_buffer[i] == 'v' && raw_buffer[i + 1] == ' ') {
            recipient->vertices_count += 1;
        }
        if (raw_buffer[i] == 'v' && raw_buffer[i + 1] == 'n') {
            recipient->normals_count += 1;
        }
        if (raw_buffer[i] == 'v' && raw_buffer[i + 1] == 't') {
            recipient->textures_count += 1;
        }
        if (raw_buffer[i] == 'f' && raw_buffer[i + 1] == ' ') {
            // peek ahead to decide if triangle, quad, or whatever
            uint32_t spaces_before_lb = 0;
            uint32_t j = i;
            while (raw_buffer[j] != '\n' && raw_buffer[j] != '\0') {
                if (raw_buffer[j] == ' ') { spaces_before_lb += 1; }
                j++;
            }
            
            if (spaces_before_lb == 3) {
                recipient->triangles_count += 1;
            } else if (spaces_before_lb == 4) {
                recipient->quads_count += 1;
            } else {
                // We're not supporting faces with more than 4 vertices for now
                #ifndef OBJ_PARSER_NO_ASSERTS
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
        #ifndef OBJ_PARSER_NO_ASSERTS
        assert(0);
        #endif
        *success = 0;
        return;
    }
    recipient->vertices = malloc_function(
        sizeof(uint32_t[6]) * recipient->vertices_count);
    
    if (recipient->normals_count > 0) {
        recipient->normals = malloc_function(
            sizeof(uint32_t[3]) * recipient->normals_count);
        
        #ifndef OBJ_PARSER_NO_ASSERTS
        assert(
            recipient->triangles_count + recipient->quads_count > 0);
        #endif
        
        if (recipient->triangles_count > 0) {
            recipient->triangle_normals = malloc_function(
                sizeof(uint32_t[3]) * recipient->triangles_count);
        }
        
        if (recipient->quads_count > 0) {
            recipient->quad_normals = malloc_function(
                sizeof(uint32_t[4]) * recipient->quads_count);
        }
    }
    if (recipient->textures_count > 0) {
        recipient->textures = malloc_function(
            sizeof(float[2]) * recipient->textures_count);
        
        #ifndef OBJ_PARSER_NO_ASSERTS
        assert(
            recipient->triangles_count + recipient->quads_count > 0);
        #endif
        
        if (recipient->triangles_count > 0) {
            recipient->triangle_textures = malloc_function(
                sizeof(uint32_t[3]) * recipient->triangles_count);
        }
        
        if (recipient->quads_count > 0) {
            recipient->quad_textures = malloc_function(
                sizeof(uint32_t[4]) * recipient->quads_count);
        }
    }
    
    if (recipient->triangles_count > 0) {
        recipient->triangles = malloc_function(
            sizeof(uint32_t[3]) * recipient->triangles_count);
    }
    for (uint32_t i = 0; i < recipient->triangles_count; i++) {
        recipient->triangles[i][0] = 0;
        recipient->triangles[i][1] = 0;
        recipient->triangles[i][2] = 0;
    }
    
    if (recipient->quads_count > 0) {
        recipient->quads = malloc_function(
            sizeof(uint32_t) *
            4 *
            recipient->quads_count);
    }
    
    // run 2: read in the data
    uint32_t cur_vertex_i = 0;
    uint32_t cur_texture_i = 0;
    uint32_t cur_normal_i = 0;
    uint32_t cur_triangle_i = 0;
    uint32_t cur_quad_i = 0;
    
    while (raw_buffer[0] != '\0') {
        while (raw_buffer[0] == ' ' || raw_buffer[0] == '\n') {
            raw_buffer++;
        }
        
        if (raw_buffer[0] == '\0') { return; }
        
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
            
            for (uint32_t axis_i = 0; axis_i < 3; axis_i++) {
                recipient->vertices[cur_vertex_i][axis_i] =
                    consume_float(&raw_buffer, success);
                if (!*success) { return; }
                
                while (raw_buffer[0] == ' ') {
                    raw_buffer++;
                }
            }
            
            cur_vertex_i += 1;
        } else if (raw_buffer[0] == 'f' && raw_buffer[1] == ' ') {
            // face data
            
            raw_buffer++; // skip the 'f'
            
            while (raw_buffer[0] == ' ') {
                raw_buffer++;
            }
            
            // peek ahead to decide how many vertices in this face
            uint32_t verties_in_face_count = 1;
            uint32_t peek_i = 0;
            while (raw_buffer[peek_i] != '\n' && raw_buffer[peek_i] != '\0') {
                if (raw_buffer[peek_i] == ' ') {
                    verties_in_face_count += 1;
                }
                peek_i += 1;
            }
            
            #ifndef OBJ_PARSER_NO_ASSERTS
            assert(verties_in_face_count > 2);
            #endif
            
            int32_t indexes[4];
            switch (verties_in_face_count) {
                case 3: {
                    uint32_t consec_entry_i = 0;
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
                        
                        if (indexes[0] == -1) {
                            success = 0;
                            return;
                        }
                        
                        recipient->triangles[cur_triangle_i][consec_entry_i] =
                            indexes[0];
                        
                        if (indexes[1] != -1) {
                            recipient->triangle_textures[cur_triangle_i]
                                [consec_entry_i] = indexes[1];
                        }
                        
                        if (indexes[2] != -1) {
                            recipient->triangle_normals[cur_triangle_i]
                                [consec_entry_i] = indexes[2];
                        }
                        
                        #ifndef OBJ_PARSER_NO_ASSERTS
                        assert(indexes[3] == -1);
                        #endif
                        
                        if (indexes[3] != -1) {
                            *success = 0;
                            return;
                        }
                        
                        while (raw_buffer[0] == ' ') {
                            raw_buffer++;
                        }
                        consec_entry_i += 1;
                    }
                    cur_triangle_i += 1;
                    break;
                }
                case 4: {
                    uint32_t consec_entry_i = 0;
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
                        
                        if (indexes[0] == -1) {
                            success = 0;
                            return;
                        }
                        
                        recipient->quads[cur_quad_i][consec_entry_i] =
                            indexes[0];
                        
                        if (indexes[1] != -1) {
                            #ifndef OBJ_PARSER_NO_ASSERTS
                            assert(recipient->quad_textures != NULL);
                            #endif
                            recipient->quad_textures[cur_quad_i]
                                [consec_entry_i] = indexes[1];
                        }
                        
                        if (indexes[2] != -1) {
                            #ifndef OBJ_PARSER_NO_ASSERTS
                            assert(recipient->quad_normals != NULL);
                            #endif
                            recipient->quad_normals[cur_quad_i]
                                [consec_entry_i] = indexes[2];
                        }
                        
                        while (raw_buffer[0] == ' ') {
                            raw_buffer++;
                        }
                        consec_entry_i += 1;
                    }
                    cur_quad_i += 1;
                    break;
                }
                default: {
                    #ifndef OBJ_PARSER_NO_ASSERTS
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
            
            for (uint32_t uv_i = 0; uv_i < 2; uv_i++) {
                recipient->textures[cur_texture_i][uv_i] =
                    consume_float(&raw_buffer, success);
                if (!*success) { return; }
                
                while (raw_buffer[0] == ' ') {
                    raw_buffer++;
                }
            }
            
            cur_vertex_i += 1;
        }  else if (raw_buffer[0] == 'v' && raw_buffer[1] == 'n') {
            
            raw_buffer++; // skip the 'v'
            raw_buffer++; // skip the 'n'
            
            while (raw_buffer[0] == ' ') {
                raw_buffer++;
            }
            
            for (uint32_t axis_i = 0; axis_i < 3; axis_i++) {
                recipient->normals[cur_normal_i][axis_i] =
                    consume_float(&raw_buffer, success);
                if (!*success) { return; }
                
                while (raw_buffer[0] == ' ') {
                    raw_buffer++;
                }
            }
            
            cur_normal_i += 1;
            
        } else if (
            raw_buffer[0] == 'u' &&
            raw_buffer[1] == 's' &&
            raw_buffer[2] == 'e' &&
            raw_buffer[3] == 'm' &&
            raw_buffer[4] == 't' &&
            raw_buffer[5] == 'l')
        {
            while (
                raw_buffer[0] != '\n' &&
                raw_buffer[0] != '\r' &&
                raw_buffer[0] != '\0')
            {
                raw_buffer++;
            }
        } else {
            #ifndef OBJ_PARSER_NO_ASSERTS
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
            #ifndef OBJ_PARSER_NO_ASSERTS
            assert(0);
            #endif
            return;
        }
        
        while (raw_buffer[0] == '\n' || raw_buffer[0] == '\r') {
            raw_buffer++; // discard the newline
        }
    }
    
    return;
}

