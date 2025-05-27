#include "mtlparser.h"

#define PARSED_MATERIALS_CAP 100

#define ERROR_MSG_CAP 256
typedef struct MTLParserState {
    ParsedMaterial materials[PARSED_MATERIALS_CAP];
    uint32_t materials_size;
    char last_error_msg[ERROR_MSG_CAP];
    size_t (* mtlparser_strlcat)(char *, const char *, size_t);
    uint32_t ambient_set_for_current_mtl;
    uint32_t diffuse_set_for_current_mtl;
    uint32_t specular_set_for_current_mtl;
    uint32_t emissive_set_for_current_mtl;
    uint32_t first_newmtl_found;
} MTLParserState;

static MTLParserState * mtlparser_state = NULL;

typedef enum MTLToken {
    MTLTOKEN_NEWLINE,
    MTLTOKEN_NEWMTL,
    MTLTOKEN_COMMENT,
    MTLTOKEN_NS, // Specular Exponent
    MTLTOKEN_AMBIENT_MAP,
    MTLTOKEN_DIFFUSE_MAP,
    MTLTOKEN_SPECULAR_MAP,
    MTLTOKEN_BUMP_MAP,
    MTLTOKEN_BUMP_MAP_ARG_INTENSITY,
    MTLTOKEN_NORMAL_MAP,
    MTLTOKEN_AMBIENT_KA,
    MTLTOKEN_DIFFUSE_KD,
    MTLTOKEN_SPECULAR_KS,
    MTLTOKEN_EMISSIVE_KE,
    MTLTOKEN_REFRACTION_NI,
    MTLTOKEN_ALPHA_d,
    MTLTOKEN_ILLUM,
    MTLTOKEN_ROUGHNESS,
    MTLTOKEN_METALLIC,
    MTLTOKEN_SHEEN,
    MTLTOKEN_CLEARCOAT,
    MTLTOKEN_CLEARCOAT_ROUGHNESS,
    MTLTOKEN_ANISOTROPY,
    MTLTOKEN_ANISOTROPY_ROTATION,
    MTLTOKEN_STRINGLITERAL,
} MTLToken;

static void mtlparser_reset(void) {
    mtlparser_state->ambient_set_for_current_mtl = 0;
    mtlparser_state->diffuse_set_for_current_mtl = 0;
    mtlparser_state->specular_set_for_current_mtl = 0;
    mtlparser_state->emissive_set_for_current_mtl = 0;
    mtlparser_state->first_newmtl_found = 0;
}

void mtlparser_init(
    void * (* arg_malloc_func)(size_t),
    size_t (* arg_strlcat_func)(char *, const char *, size_t))
{
    mtlparser_state = arg_malloc_func(sizeof(MTLParserState));
    mtlparser_state->last_error_msg[0] = '\0';
    
    mtlparser_state->mtlparser_strlcat = arg_strlcat_func;
    
    mtlparser_reset();
}

const char * mtlparser_get_last_error_msg(void) {
    return mtlparser_state->last_error_msg;
}

static void
mtlparser_uint_to_string(
    const uint32_t input,
    char * recipient)
{
    if (input == 0) {
        recipient[0] = '0';
        recipient[1] = '\0';
        return;
    }
    
    uint32_t i = 0;
    uint32_t start_i = 0;
    uint32_t end_i;
    
    uint64_t decimal = 1;
    uint32_t input_div_dec = input;
    while (input_div_dec > 0) {
        uint32_t isolated_num = input % (decimal * 10);
        isolated_num /= decimal;
        recipient[i] = (char)('0' + isolated_num);
        i += 1;
        
        decimal *= 10;
        input_div_dec = input / decimal;
    }
    end_i = i - 1;
    
    recipient[end_i + 1] = '\0';
    
    // if we started with an input of -32, we should now have
    // the resut '-' '2' '3', with start_i at '2' and end_i at '3'
    // we want to swap from start_i to end_i
    while (start_i < end_i) {
        char swap = recipient[start_i];
        recipient[start_i] = recipient[end_i];
        recipient[end_i] = swap;
        end_i -= 1;
        start_i += 1; 
    }
}

static void parse_single_string_stat(
    uint32_t * i,
    TokToken * token,
    const char * material_name,
    char * string_stat,
    uint32_t * good)
{
    char initial_token_name[64];
    initial_token_name[0] = '\0';
    mtlparser_state->mtlparser_strlcat(
        initial_token_name,
        token->string_value == NULL ? "NULL" : token->string_value,
        64);
    
    char stack_string_64bytes[64];
    stack_string_64bytes[0] = '\0';
    
    if (!mtlparser_state->first_newmtl_found) {
        *good = 0;
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            "'",
            ERROR_MSG_CAP);
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            initial_token_name,
            ERROR_MSG_CAP);
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            "' call before newmtl!",
            ERROR_MSG_CAP);
        return;
    }
    
    (*i)++;
    token = toktoken_get_token_at(*i);
    if (
        token->enum_value != MTLTOKEN_STRINGLITERAL ||
        !token->string_value ||
        token->string_value_size < 1)
    {
        *good = 0;
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            "Expected a string literal after '",
            ERROR_MSG_CAP);
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            token->string_value,
            ERROR_MSG_CAP);
        return;
    }
    
    string_stat[0] = '\0';
    mtlparser_state->mtlparser_strlcat(
         string_stat,
         token->string_value,
         MATERIAL_NAME_CAP);
    *good = 1;
}

static void parse_single_float_stat(
    uint32_t * i,
    TokToken * token,
    const char * material_name,
    float * float_stat,
    uint32_t * good)
{
    char initial_token_name[64];
    initial_token_name[0] = '\0';
    mtlparser_state->mtlparser_strlcat(
        initial_token_name,
        token->string_value == NULL ? "NULL" : token->string_value,
        64);
    
    char stack_string_64bytes[64];
    stack_string_64bytes[0] = '\0';
    
    if (!mtlparser_state->first_newmtl_found) {
        *good = 0;
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            "'",
            ERROR_MSG_CAP);
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            initial_token_name,
            ERROR_MSG_CAP);
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            "' call before newmtl!",
            ERROR_MSG_CAP);
        return;
    }
    
    (*i)++;
    token = toktoken_get_token_at(*i);
    if (
        token->enum_value != MTLTOKEN_STRINGLITERAL ||
        !toktoken_is_number(token) ||
        !toktoken_fits_double(token) ||
        !token->number_value)
    {
        *good = 0;
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            "Expected a float after '",
            ERROR_MSG_CAP);
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            initial_token_name,
            ERROR_MSG_CAP);
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            "'.",
            ERROR_MSG_CAP);
        return;
    }
    
    *float_stat = (float)token->number_value->double_precision;
    *good = 1;
}

static void parse_rgb_token(
    uint32_t * i,
    TokToken * token,
    const char * material_name,
    float * rgb_stat,
    uint32_t * already_set_flag,
    uint32_t * good)
{
    char initial_token_name[64];
    initial_token_name[0] = '\0';
    mtlparser_state->mtlparser_strlcat(
        initial_token_name,
        token->string_value == NULL ? "NULL" : token->string_value,
        64);
    
    char stack_string_64bytes[64];
    stack_string_64bytes[0] = '\0';
    
    if (!mtlparser_state->first_newmtl_found) {
        *good = 0;
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            "'",
            ERROR_MSG_CAP);
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            initial_token_name,
            ERROR_MSG_CAP);
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            "' call before newmtl!",
            ERROR_MSG_CAP);
        return;
    }
    
    if (*already_set_flag) {
        *good = 0;
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            "Duplicate '",
            ERROR_MSG_CAP);
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            token->string_value == NULL ? "NULL" : token->string_value,
            ERROR_MSG_CAP);
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            "' entry for same material (",
            ERROR_MSG_CAP);
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            material_name,
            ERROR_MSG_CAP);
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            ")",
            ERROR_MSG_CAP);
        return;
    }
    *already_set_flag = 1;
    
    // we expect 3 float tokens to follow
    if (*i + 3 >= toktoken_get_token_count()) {
        *good = 0;
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            "Expected 3+ more tokens after '",
            ERROR_MSG_CAP);
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            token->string_value == NULL ? "NULL" : token->string_value,
            ERROR_MSG_CAP);
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            "' token",
            ERROR_MSG_CAP);
        return;
    }
    
    for (uint32_t rgb_i = 0; rgb_i < 3; rgb_i++) {
        (*i) = *i + 1;
        token = toktoken_get_token_at(*i);
        
        if (
            token->enum_value != MTLTOKEN_STRINGLITERAL ||
            !toktoken_is_number(token) ||
            !toktoken_fits_double(token))
        {
            *good = 0;
            stack_string_64bytes[0] = (char)('0' + rgb_i);
            stack_string_64bytes[1] = '\0';
            mtlparser_uint_to_string(
                rgb_i,
                stack_string_64bytes);
            
            mtlparser_state->mtlparser_strlcat(
                mtlparser_state->last_error_msg,
                "Expected 3 floating point values (rgb values)"
                " after '",
                ERROR_MSG_CAP);
            mtlparser_state->mtlparser_strlcat(
                mtlparser_state->last_error_msg,
                initial_token_name,
                ERROR_MSG_CAP);
            mtlparser_state->mtlparser_strlcat(
                mtlparser_state->last_error_msg,
                "', got: ",
                ERROR_MSG_CAP);
            mtlparser_state->mtlparser_strlcat(
                mtlparser_state->last_error_msg,
                stack_string_64bytes,
                ERROR_MSG_CAP);
            return;
        }
        
        assert(token->number_value != NULL);
        
        rgb_stat[rgb_i] =
            (float)token->number_value->double_precision;
    }
    *good = 1;
}

void mtlparser_parse(
    ParsedMaterial * recipient,
    uint32_t * recipient_size,
    const uint32_t recipient_cap,
    const char * input,
    uint32_t * good)
{
    *good = 0;
    *recipient_size = 0;
    
    toktoken_reset(good);
    if (!*good) {
        return;
    }
    *good = 0;
    
    toktoken_register_token("newmtl", MTLTOKEN_NEWMTL, good);
    if (!*good) { return; }
    
    toktoken_register_token("#", MTLTOKEN_COMMENT, good);
    if (!*good) { return; }
    
    toktoken_register_token("\n", MTLTOKEN_NEWLINE, good);
    if (!*good) { return; }
    
    toktoken_register_token("Ns", MTLTOKEN_NS, good);
    if (!*good) { return; }
    
    toktoken_register_token("map_Ka", MTLTOKEN_AMBIENT_MAP, good);
    if (!*good) { return; }
    
    toktoken_register_token("map_Kd", MTLTOKEN_DIFFUSE_MAP, good);
    if (!*good) { return; }
    
    toktoken_register_token("map_Ks", MTLTOKEN_SPECULAR_MAP, good);
    if (!*good) { return; }
    
    toktoken_register_token("bump", MTLTOKEN_BUMP_MAP, good);
    if (!*good) { return; }
    
    toktoken_register_token("map_bump", MTLTOKEN_BUMP_MAP, good);
    if (!*good) { return; }
    
    toktoken_register_token("map_Bump", MTLTOKEN_BUMP_MAP, good);
    if (!*good) { return; }
    
    toktoken_register_token("-bm", MTLTOKEN_BUMP_MAP_ARG_INTENSITY, good);
    if (!*good) { return; }
    
    toktoken_register_token("map_Ns", MTLTOKEN_NORMAL_MAP, good);
    if (!*good) { return; }
    
    toktoken_register_token("Ka", MTLTOKEN_AMBIENT_KA, good);
    if (!*good) { return; }
    
    toktoken_register_token("Kd", MTLTOKEN_DIFFUSE_KD, good);
    if (!*good) { return; }

    toktoken_register_token("Ks", MTLTOKEN_SPECULAR_KS, good);
    if (!*good) { return; }
    
    toktoken_register_token("Ke", MTLTOKEN_EMISSIVE_KE, good);
    if (!*good) { return; }

    toktoken_register_token("Ni", MTLTOKEN_REFRACTION_NI, good);
    if (!*good) { return; }
    
    toktoken_register_token("d", MTLTOKEN_ALPHA_d, good);
    if (!*good) { return; }
    
    toktoken_register_token("illum", MTLTOKEN_ILLUM, good);
    if (!*good) { return; }
    
    toktoken_register_token("Pr", MTLTOKEN_ROUGHNESS, good);
    if (!*good) { return; }
    
    toktoken_register_token("Pm", MTLTOKEN_METALLIC, good);
    if (!*good) { return; }
    
    toktoken_register_token("Ps", MTLTOKEN_SHEEN, good);
    if (!*good) { return; }
    
    toktoken_register_token("Pc", MTLTOKEN_CLEARCOAT, good);
    if (!*good) { return; }
    
    toktoken_register_token("Pcr", MTLTOKEN_CLEARCOAT_ROUGHNESS, good);
    if (!*good) { return; }
    
    toktoken_register_token("aniso", MTLTOKEN_ANISOTROPY, good);
    if (!*good) { return; }
    
    toktoken_register_token("anisor", MTLTOKEN_ANISOTROPY_ROTATION, good);
    if (!*good) { return; }
    
    toktoken_register_newline_enum(MTLTOKEN_NEWLINE, good);
    if (!*good) { return; }
    
    toktoken_register_string_literal_enum(MTLTOKEN_STRINGLITERAL, good);
    if (!*good) { return; }
    
    
    *good = 0;
    toktoken_client_settings->allow_scientific_notation = 0;
    
    toktoken_run(
        /* const char * input: */
            input,
        /* uint32_t * good: */
            good);
    if (!*good) {
        return;
    }
    *good = 0;
    
    ParsedMaterial * current_material = NULL;
    
    uint32_t tokens_count = toktoken_get_token_count();
    for (uint32_t i = 0; i < tokens_count; i++) {
        TokToken * token = toktoken_get_token_at(i);
        mtlparser_state->last_error_msg[0] = '\0';
        char stack_string_64bytes[64];
        mtlparser_uint_to_string(
            token->line_number,
            stack_string_64bytes);
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            stack_string_64bytes,
            ERROR_MSG_CAP);
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            ":: ",
            ERROR_MSG_CAP);
        
        switch (token->enum_value) {
            case MTLTOKEN_COMMENT: {
                TokToken * ignored = NULL;
                while (!ignored || ignored->enum_value != MTLTOKEN_NEWLINE) {
                    i++;
                    ignored = toktoken_get_token_at(i);
                }
                break;
            }
            case MTLTOKEN_NEWLINE: {
                assert(token->number_value == NULL);
                break;
            }
            case MTLTOKEN_NEWMTL: {
                mtlparser_state->ambient_set_for_current_mtl = 0;
                mtlparser_state->diffuse_set_for_current_mtl = 0;
                mtlparser_state->specular_set_for_current_mtl = 0;
                mtlparser_state->emissive_set_for_current_mtl = 0;
                mtlparser_state->first_newmtl_found = 1;
                
                if (*recipient_size + 1 >= recipient_cap) {
                    *good = 0;
                    return;
                }
                current_material = recipient + *recipient_size;
                *recipient_size += 1;
                
                i++;
                token = toktoken_get_token_at(i);
                if (
                    token->enum_value != MTLTOKEN_STRINGLITERAL ||
                    token->string_value_size < 1)
                {
                    *good = 0;
                    return;
                }
                mtlparser_state->mtlparser_strlcat(
                    current_material->name,
                    token->string_value,
                    MATERIAL_NAME_CAP);
                break;
            }
            case MTLTOKEN_NS: {
                parse_single_float_stat(
                    /* uint32_t * i: */
                        &i,
                    /* TokToken * token: */
                        token,
                    /* const char * material_name: */
                        current_material->name,
                    /* float * float_stat: */
                        &current_material->specular_exponent,
                    /* uint32_t * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                break;
            }
            case MTLTOKEN_ALPHA_d: {
                parse_single_float_stat(
                    /* uint32_t * i: */
                        &i,
                    /* TokToken * token: */
                        token,
                    /* const char * material_name: */
                        current_material->name,
                    /* float * float_stat: */
                        &current_material->alpha,
                    /* uint32_t * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_ILLUM: {
                parse_single_float_stat(
                    /* uint32_t * i: */
                        &i,
                    /* TokToken * token: */
                        token,
                    /* const char * material_name: */
                        current_material->name,
                    /* float * float_stat: */
                        &current_material->illum,
                    /* uint32_t * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_REFRACTION_NI: {
                // Reminder: 'Ni' means "optical density" or "refraction"
                // in .mtl files
                
                parse_single_float_stat(
                    /* uint32_t * i: */
                        &i,
                    /* TokToken * token: */
                        token,
                    /* const char * material_name: */
                        current_material->name,
                    /* float * float_stat: */
                        &current_material->refraction,
                    /* uint32_t * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_AMBIENT_KA: {

                parse_rgb_token(
                    /* uint32_t * i: */
                        &i,
                    /* TokToken * token: */
                        token,
                    /* const char * material_name: */
                        current_material->name,
                    /* float * rgb_stat: */
                        current_material->ambient_rgb,
                    /* uint32_t * already_set_flag: */
                        &mtlparser_state->ambient_set_for_current_mtl,
                    /* uint32_t * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_DIFFUSE_KD: {
                parse_rgb_token(
                    /* uint32_t * i: */
                        &i,
                    /* TokToken * token: */
                        token,
                    /* const char * material_name: */
                        current_material->name,
                    /* float * rgb_stat: */
                        current_material->diffuse_rgb,
                    /* uint32_t * already_set_flag: */
                        &mtlparser_state->diffuse_set_for_current_mtl,
                    /* uint32_t * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_SPECULAR_KS: {
                parse_rgb_token(
                    /* uint32_t * i: */
                        &i,
                    /* TokToken * token: */
                        token,
                    /* const char * material_name: */
                        current_material->name,
                    /* float * rgb_stat: */
                        current_material->specular_rgb,
                    /* uint32_t * already_set_flag: */
                        &mtlparser_state->specular_set_for_current_mtl,
                    /* uint32_t * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_EMISSIVE_KE: {
                parse_rgb_token(
                    /* uint32_t * i: */
                        &i,
                    /* TokToken * token: */
                        token,
                    /* const char * material_name: */
                        current_material->name,
                    /* float * rgb_stat: */
                        current_material->emissive_rgb,
                    /* uint32_t * already_set_flag: */
                        &mtlparser_state->emissive_set_for_current_mtl,
                    /* uint32_t * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_STRINGLITERAL: {
                *good = 0;
                mtlparser_state->mtlparser_strlcat(
                    mtlparser_state->last_error_msg,
                    "Unexpected string literal: '",
                    ERROR_MSG_CAP);
                mtlparser_state->mtlparser_strlcat(
                    mtlparser_state->last_error_msg,
                    token->string_value,
                    ERROR_MSG_CAP);
                mtlparser_state->mtlparser_strlcat(
                    mtlparser_state->last_error_msg,
                    "' (",
                    ERROR_MSG_CAP);
                stack_string_64bytes[0] = '\0';
                mtlparser_uint_to_string(
                    token->string_value_size,
                    stack_string_64bytes);
                mtlparser_state->mtlparser_strlcat(
                    mtlparser_state->last_error_msg,
                    stack_string_64bytes,
                    ERROR_MSG_CAP);
                mtlparser_state->mtlparser_strlcat(
                    mtlparser_state->last_error_msg,
                    " bytes)",
                    ERROR_MSG_CAP);
                return;
                break;
            }
            case MTLTOKEN_AMBIENT_MAP: {
                
                parse_single_string_stat(
                    /* uint32_t * i: */
                        &i,
                    /* TokToken * token: */
                        token,
                    /* const char * material_name: */
                        current_material->name,
                    /* char * string_stat: */
                        current_material->ambient_map,
                    /* uint32_t * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_DIFFUSE_MAP: {
                
                parse_single_string_stat(
                    /* uint32_t * i: */
                        &i,
                    /* TokToken * token: */
                        token,
                    /* const char * material_name: */
                        current_material->name,
                    /* char * string_stat: */
                        current_material->diffuse_map,
                    /* uint32_t * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_SPECULAR_MAP: {
                assert(0);
                
                break;
            }
            case MTLTOKEN_BUMP_MAP: {
                TokToken * peek = toktoken_get_token_at(i+1);
                if (peek->enum_value == MTLTOKEN_BUMP_MAP_ARG_INTENSITY) {
                    i++;
                    
                    parse_single_float_stat(
                        /* uint32_t * i: */
                            &i,
                        /* TokToken * token: */
                            token,
                        /* const char * material_name: */
                            current_material->name,
                        /* float * float_stat: */
                            &current_material->bump_map_intensity,
                        /* uint32_t * good: */
                            good);
                } else {
                    current_material->bump_map_intensity = 1.0f;
                }
                
                parse_single_string_stat(
                    /* uint32_t * i: */
                        &i,
                    /* TokToken * token: */
                        token,
                    /* const char * material_name: */
                        current_material->name,
                    /* char * string_stat: */
                        current_material->bump_map,
                    /* uint32_t * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_NORMAL_MAP: {
                assert(0); // TODO: implement me
                break;
            }
            case MTLTOKEN_ROUGHNESS: {
                parse_single_float_stat(
                    /* uint32_t * i: */
                        &i,
                    /* TokToken * token: */
                        token,
                    /* const char * material_name: */
                        current_material->name,
                    /* float * float_stat: */
                        &current_material->roughness,
                    /* uint32_t * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_METALLIC: {
                parse_single_float_stat(
                    /* uint32_t * i: */
                        &i,
                    /* TokToken * token: */
                        token,
                    /* const char * material_name: */
                        current_material->name,
                    /* float * float_stat: */
                        &current_material->metallic,
                    /* uint32_t * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_SHEEN: {
                parse_single_float_stat(
                    /* uint32_t * i: */
                        &i,
                    /* TokToken * token: */
                        token,
                    /* const char * material_name: */
                        current_material->name,
                    /* float * float_stat: */
                        &current_material->sheen,
                    /* uint32_t * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_CLEARCOAT: {
                parse_single_float_stat(
                    /* uint32_t * i: */
                        &i,
                    /* TokToken * token: */
                        token,
                    /* const char * material_name: */
                        current_material->name,
                    /* float * float_stat: */
                        &current_material->clearcoat,
                    /* uint32_t * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_CLEARCOAT_ROUGHNESS: {
                parse_single_float_stat(
                    /* uint32_t * i: */
                        &i,
                    /* TokToken * token: */
                        token,
                    /* const char * material_name: */
                        current_material->name,
                    /* float * float_stat: */
                        &current_material->clearcoat_roughness,
                    /* uint32_t * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_ANISOTROPY: {
                parse_single_float_stat(
                    /* uint32_t * i: */
                        &i,
                    /* TokToken * token: */
                        token,
                    /* const char * material_name: */
                        current_material->name,
                    /* float * float_stat: */
                        &current_material->anisotropy,
                    /* uint32_t * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_ANISOTROPY_ROTATION: {
                parse_single_float_stat(
                    /* uint32_t * i: */
                        &i,
                    /* TokToken * token: */
                        token,
                    /* const char * material_name: */
                        current_material->name,
                    /* float * float_stat: */
                        &current_material->anisotropy_rotation,
                    /* uint32_t * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            default:
                *good = 0;
                mtlparser_state->mtlparser_strlcat(
                    mtlparser_state->last_error_msg,
                    "Unhandled token type: ",
                    ERROR_MSG_CAP);
                stack_string_64bytes[0] = '\0';
                mtlparser_uint_to_string(
                    token->enum_value,
                    stack_string_64bytes);
                mtlparser_state->mtlparser_strlcat(
                    mtlparser_state->last_error_msg,
                    stack_string_64bytes,
                    ERROR_MSG_CAP);
                mtlparser_state->mtlparser_strlcat(
                    mtlparser_state->last_error_msg,
                    " with value: ",
                    ERROR_MSG_CAP);
                mtlparser_state->mtlparser_strlcat(
                    mtlparser_state->last_error_msg,
                    token->string_value == NULL ?
                        "NULL" : token->string_value,
                    ERROR_MSG_CAP);
                return;
        }
    }
    
    *good = 1;
}
