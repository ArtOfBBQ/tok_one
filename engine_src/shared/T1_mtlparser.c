#include "T1_mtlparser.h"

#define PARSED_MATERIALS_CAP 100

#define ERROR_MSG_CAP 256
typedef struct MTLParserState {
    ParsedMaterial materials[PARSED_MATERIALS_CAP];
    uint32_t materials_size;
    char last_error_msg[ERROR_MSG_CAP];
    void * (* mtlparser_memset)(void *, int, size_t);
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
    MTLTOKEN_SPACE,
    MTLTOKEN_NEWMTL,
    MTLTOKEN_USEBASEMTL,
    MTLTOKEN_COMMENT,
    MTLTOKEN_T1_COMMENT,
    MTLTOKEN_NS, // Specular Exponent
    MTLTOKEN_AMBIENT_MAP,
    MTLTOKEN_DIFFUSE_MAP,
    MTLTOKEN_SPECULAR_MAP,
    MTLTOKEN_SPECULAR_EXPONENT_MAP,
    MTLTOKEN_ALPHA_MAP, // useless in our model
    MTLTOKEN_BUMP_OR_NORMAL_MAP,
    MTLTOKEN_BUMP_MAP_ARG_INTENSITY,
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
    MTLTOKEN_STRINGLITERAL = 100,
} MTLToken;

static void mtlparser_reset(void) {
    mtlparser_state->ambient_set_for_current_mtl = 0;
    mtlparser_state->diffuse_set_for_current_mtl = 0;
    mtlparser_state->specular_set_for_current_mtl = 0;
    mtlparser_state->emissive_set_for_current_mtl = 0;
    mtlparser_state->first_newmtl_found = 0;
}

void mtlparser_init(
    void * (* arg_memset_func)(void *, int, size_t),
    void * (* arg_malloc_func)(size_t),
    size_t (* arg_strlcat_func)(char *, const char *, size_t))
{
    mtlparser_state = arg_malloc_func(sizeof(MTLParserState));
    mtlparser_state->last_error_msg[0] = '\0';
    
    mtlparser_state->mtlparser_strlcat = arg_strlcat_func;
    mtlparser_state->mtlparser_memset = arg_memset_func;
    
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
    (void)material_name;
    
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
    token = T1_token_get_token_at(*i);
    
    while (
        token->enum_value == MTLTOKEN_SPACE &&
        *i + 1 < T1_token_get_token_count())
    {
        (*i)++;
        token = T1_token_get_token_at(*i);
    }
    
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
    (void)material_name;
    
    char initial_token_name[64];
    initial_token_name[0] = '\0';
    mtlparser_state->mtlparser_strlcat(
        initial_token_name,
        (
            token == NULL ||
            token->string_value ==
                NULL) ?
            "NULL" :
            token->string_value,
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
    token = T1_token_get_token_at(*i);
    
    while (token->enum_value == MTLTOKEN_SPACE) {
        *i += 1;
        token = T1_token_get_token_at(*i);
    }
    
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
    if (*i + 3 >= T1_token_get_token_count()) {
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
        token = T1_token_get_token_at(*i);
        
        while (
            token->enum_value == MTLTOKEN_SPACE &&
            *i + 1 < T1_token_get_token_count())
        {
            *i += 1;
            token = T1_token_get_token_at(*i);
        }
        
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

static void construct_material(ParsedMaterial * to_construct) {
    mtlparser_state->mtlparser_memset(to_construct, 0, sizeof(ParsedMaterial));
    
    to_construct->alpha = 1.0f;
    to_construct->ambient_rgb[0] = 0.1f;
    to_construct->ambient_rgb[1] = 0.1f;
    to_construct->ambient_rgb[2] = 0.1f;
    to_construct->bump_map_intensity = 1.0f;
    to_construct->diffuse_rgb[0] = 1.0f;
    to_construct->diffuse_rgb[1] = 1.0f;
    to_construct->diffuse_rgb[2] = 1.0f;
    to_construct->specular_rgb[0] = 0.1f;
    to_construct->specular_rgb[1] = 0.1f;
    to_construct->specular_rgb[2] = 0.1f;
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
    
    T1_token_reset(good);
    if (!*good) {
        return;
    }
    *good = 0;
    
    T1_token_set_reg_bitflags(
        T1_TOKEN_FLAG_LEAD_DOT_OK |
        T1_TOKEN_FLAG_SCIENTIFIC_OK |
        T1_TOKEN_FLAG_PRECISE);
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("newmtl ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_NEWMTL, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("USE_BASE_MATERIAL");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_USEBASEMTL, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("#T1");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_T1_COMMENT, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("#");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_COMMENT, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("\n");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_NEWLINE, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern(" ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_SPACE, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("Ns ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_NS, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("map_Ka ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_AMBIENT_MAP, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("map_Kd ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_DIFFUSE_MAP, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("map_Ks ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_SPECULAR_MAP, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("map_d ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_ALPHA_MAP, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("bump ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_BUMP_OR_NORMAL_MAP, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("map_bump ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_BUMP_OR_NORMAL_MAP, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("map_Bump ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_BUMP_OR_NORMAL_MAP, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("-bm ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_BUMP_MAP_ARG_INTENSITY, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("map_Ns ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_SPECULAR_EXPONENT_MAP, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("Ka ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_AMBIENT_KA, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("Kd ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_DIFFUSE_KD, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("Ks ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_SPECULAR_KS, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("Ke ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_EMISSIVE_KE, good);
    if (!*good) { return; }

    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("Ni ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_REFRACTION_NI, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("d ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_ALPHA_d, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("illum ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_ILLUM, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("Pr ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_ROUGHNESS, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("Pm ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_METALLIC, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("Ps ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_SHEEN, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("Pc ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_CLEARCOAT, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("Pcr ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_CLEARCOAT_ROUGHNESS, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("aniso ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_ANISOTROPY, good);
    if (!*good) { return; }
    
    T1_token_clear_start_pattern();
    T1_token_clear_stop_patterns();
    T1_token_set_reg_start_pattern("anisor ");
    T1_token_set_reg_middle_cap(0);
    T1_token_register(MTLTOKEN_ANISOTROPY_ROTATION, good);
    if (!*good) { return; }
    
    T1_token_set_string_literal(MTLTOKEN_STRINGLITERAL, good);
    if (!*good) { return; }
    
    
    *good = 0;
    T1_token_run(
        /* const char * input: */
            input,
        /* uint32_t * good: */
            good);
    if (!*good) {
        mtlparser_state->last_error_msg[0] = '\0';
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            "The tokenizer crashed in T1_token_run()!",
            ERROR_MSG_CAP);
        return;
    }
    *good = 0;
    
    ParsedMaterial * current_material = NULL;
    
    uint32_t tokens_count = T1_token_get_token_count();
    for (uint32_t i = 0; i < tokens_count; i++) {
        TokToken * token = T1_token_get_token_at(i);
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
                while (
                    !ignored ||
                    ignored->enum_value != MTLTOKEN_NEWLINE)
                {
                    i++;
                    ignored = T1_token_get_token_at(i);
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
                    mtlparser_state->last_error_msg[0] = '\0';
                    mtlparser_state->mtlparser_strlcat(
                    mtlparser_state->last_error_msg,
                    "A newmtl's name is too long",
                    ERROR_MSG_CAP);
                    return;
                }
                current_material = recipient + *recipient_size;
                construct_material(current_material);
                *recipient_size += 1;
                
                i++;
                token = T1_token_get_token_at(i);
                
                while (token->enum_value == MTLTOKEN_SPACE) {
                    i++;
                    token = T1_token_get_token_at(i);
                }
                
                if (
                    token->enum_value != MTLTOKEN_STRINGLITERAL ||
                    token->string_value_size < 1)
                {
                    *good = 0;
                    mtlparser_state->last_error_msg[0] = '\0';
                    mtlparser_state->mtlparser_strlcat(
                    mtlparser_state->last_error_msg,
                    "Expected a material name after newmtl",
                    ERROR_MSG_CAP);
                    return;
                }
                mtlparser_state->mtlparser_strlcat(
                    current_material->name,
                    token->string_value,
                    MATERIAL_NAME_CAP);
                
                token = T1_token_get_token_at(i + 1);
                
                while (
                    token->enum_value == MTLTOKEN_SPACE &&
                    (i + 2) < T1_token_get_token_count())
                {
                    i += 1;
                    token = T1_token_get_token_at(i + 1);
                }
                
                if (token->enum_value == MTLTOKEN_USEBASEMTL) {
                    current_material->use_base_mtl_flag = 1;
                    i++;
                }
                
                break;
            }
            case MTLTOKEN_T1_COMMENT: {
                
                i++;
                token = T1_token_get_token_at(i);
                if (token->enum_value != MTLTOKEN_SPACE) {
                    *good = 0;
                    mtlparser_state->last_error_msg[0] = '\0';
                    mtlparser_state->mtlparser_strlcat(
                    mtlparser_state->last_error_msg,
                    "Expected a space after special comment #T1",
                    ERROR_MSG_CAP);
                    return;
                }
                
                i++;
                token = T1_token_get_token_at(i);
                if (token->enum_value != MTLTOKEN_STRINGLITERAL) {
                    *good = 0;
                    mtlparser_state->last_error_msg[0] = '\0';
                    mtlparser_state->mtlparser_strlcat(
                        mtlparser_state->last_error_msg,
                        "Expected a string literal describing a special "
                        "property after special comment #T1, got: ",
                        ERROR_MSG_CAP);
                    mtlparser_state->mtlparser_strlcat(
                        mtlparser_state->last_error_msg,
                        token->string_value,
                        ERROR_MSG_CAP);
                    return;
                }
                
                float * target_float = NULL;
                if (
                    T1_std_are_equal_strings(
                        token->string_value,
                        "uv_scroll[0]"))
                {
                    target_float = &current_material->T1_uv_scroll[0];
                } else if (
                    T1_std_are_equal_strings(
                        token->string_value,
                        "uv_scroll[1]"))
                {
                    target_float = &current_material->T1_uv_scroll[1];
                } else {
                    *good = 0;
                    mtlparser_state->last_error_msg[0] = '\0';
                    mtlparser_state->mtlparser_strlcat(
                        mtlparser_state->last_error_msg,
                        "Unrecognized special embedded comment property: ",
                        ERROR_MSG_CAP);
                    mtlparser_state->mtlparser_strlcat(
                        mtlparser_state->last_error_msg,
                        token->string_value,
                        ERROR_MSG_CAP);
                    return;
                }
                
                parse_single_float_stat(
                    /* uint32_t * i: */
                        &i,
                    /* TokToken * token: */
                        token,
                    /* const char * material_name: */
                        current_material->name,
                    /* float * float_stat: */
                        target_float,
                    /* uint32_t * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
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
            case MTLTOKEN_SPECULAR_EXPONENT_MAP: {
                parse_single_string_stat(
                    /* uint32_t * i: */
                        &i,
                    /* TokToken * token: */
                        token,
                    /* const char * material_name: */
                        current_material->name,
                    /* char * string_stat: */
                        current_material->specular_exponent_map,
                    /* uint32_t * good: */
                        good);
                
                if (!*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_ALPHA_MAP: {
                // We use the alpha in textures as our alpha and ignore this
                i++;
                TokToken * ignored = T1_token_get_token_at(i);
                
                if (ignored->enum_value != MTLTOKEN_STRINGLITERAL) {
                    mtlparser_state->mtlparser_strlcat(
                        mtlparser_state->last_error_msg,
                        "Unexpected token type after map_d: ",
                        ERROR_MSG_CAP);
                    mtlparser_state->mtlparser_strlcat(
                        mtlparser_state->last_error_msg,
                        ignored->string_value,
                        ERROR_MSG_CAP);
                    *good = 0;
                    return;
                }
                
                break;
            }
            case MTLTOKEN_BUMP_OR_NORMAL_MAP: {
                TokToken * peek = T1_token_get_token_at(i+1);
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
                        current_material->bump_or_normal_map,
                    /* uint32_t * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
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
            case MTLTOKEN_SPACE:
                break;
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
