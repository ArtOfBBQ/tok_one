#include "T1_mtlparser.h"

#include "T1_std.h"
#include "T1_token.h"

#define PARSED_MATERIALS_CAP 100

#define ERROR_MSG_CAP 256
typedef struct MTLParserState {
    ParsedMaterial materials[PARSED_MATERIALS_CAP];
    u32 materials_size;
    char last_error_msg[ERROR_MSG_CAP];
    void * (* mtlparser_memset)(void *, int, u64);
    u64 (* mtlparser_strlcat)(char *, const char *, u64);
    u32 ambient_set_for_current_mtl;
    u32 diffuse_set_for_current_mtl;
    u32 specular_set_for_current_mtl;
    u32 emissive_set_for_current_mtl;
    u32 first_newmtl_found;
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
    void * (* arg_memset_func)(void *, int, u64),
    void * (* arg_malloc_func)(u64),
    u64 (* arg_strlcat_func)(char *, const char *, u64))
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
    const u32 input,
    char * recipient)
{
    if (input == 0) {
        recipient[0] = '0';
        recipient[1] = '\0';
        return;
    }
    
    u32 i = 0;
    u32 start_i = 0;
    u32 end_i;
    
    u64 decimal = 1;
    u32 input_div_dec = input;
    while (input_div_dec > 0) {
        u32 isolated_num = input % (decimal * 10);
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
    u16 * i,
    const char * material_name,
    char * string_stat,
    u8 * good)
{
    (void)material_name;
    
    char initial_token_name[64];
    initial_token_name[0] = '\0';
    mtlparser_state->mtlparser_strlcat(
        initial_token_name,
        T1_token_get_string_value(*i) == NULL ?
            "NULL" : T1_token_get_string_value(*i),
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
    
    while (
        T1_token_get_enum_value(*i) == MTLTOKEN_SPACE &&
        *i + 1 < T1_token_get_token_count())
    {
        (*i)++;
    }
    
    if (
        T1_token_get_enum_value(*i) != MTLTOKEN_STRINGLITERAL ||
        !T1_token_get_string_value(*i) ||
        T1_token_get_string_value_size(*i) < 1)
    {
        *good = 0;
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            "Expected a string literal after '",
            ERROR_MSG_CAP);
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            T1_token_get_string_value(*i),
            ERROR_MSG_CAP);
        return;
    }
    
    string_stat[0] = '\0';
    mtlparser_state->mtlparser_strlcat(
         string_stat,
         T1_token_get_string_value(*i),
         T1_MATERIAL_NAME_CAP);
    *good = 1;
}

static void parse_single_f32_stat(
    u16 * i,
    const char * material_name,
    f32 * f32_stat,
    u8 * good)
{
    (void)material_name;
    
    char initial_token_name[64];
    initial_token_name[0] = '\0';
    mtlparser_state->mtlparser_strlcat(
        initial_token_name,
        (
            T1_token_get_string_value(*i) ==
                NULL) ?
                    "NULL" :
                    T1_token_get_string_value(*i),
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
    
    while (T1_token_get_enum_value(*i) == MTLTOKEN_SPACE) {
        *i += 1;
    }
    
    if (
        T1_token_get_enum_value(*i) != MTLTOKEN_STRINGLITERAL ||
        !T1_token_is_number(*i) ||
        !T1_token_fits_f64(*i))
    {
        *good = 0;
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            "Expected a f32 after '",
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
    
    *f32_stat = (f32)T1_token_as_number_floating(*i);
    *good = 1;
}

static void parse_rgb_token(
    u16 * i,
    const char * material_name,
    f32 * rgb_stat,
    u32 * already_set_flag,
    u8 * good)
{
    char initial_token_name[64];
    initial_token_name[0] = '\0';
    mtlparser_state->mtlparser_strlcat(
        initial_token_name,
        T1_token_get_string_value(*i) == NULL ?
            "NULL" :
            T1_token_get_string_value(*i),
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
            T1_token_get_string_value(*i) == NULL ?
                "NULL" :
                T1_token_get_string_value(*i),
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
    
    // we expect 3 f32 tokens to follow
    if (*i + 3 >= T1_token_get_token_count()) {
        *good = 0;
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            "Expected 3+ more tokens after '",
            ERROR_MSG_CAP);
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            T1_token_get_string_value(*i) == NULL ?
                "NULL" : T1_token_get_string_value(*i),
            ERROR_MSG_CAP);
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            "' token",
            ERROR_MSG_CAP);
        return;
    }
    
    for (u32 rgb_i = 0; rgb_i < 3; rgb_i++) {
        (*i) = *i + 1;
        
        while (
            T1_token_get_enum_value(*i) == MTLTOKEN_SPACE &&
            *i + 1 < T1_token_get_token_count())
        {
            *i += 1;
        }
        
        if (
            T1_token_get_enum_value(*i) != MTLTOKEN_STRINGLITERAL ||
            !T1_token_is_number(*i) ||
            !T1_token_fits_f64(*i))
        {
            *good = 0;
            stack_string_64bytes[0] = (char)('0' + rgb_i);
            stack_string_64bytes[1] = '\0';
            mtlparser_uint_to_string(
                rgb_i,
                stack_string_64bytes);
            
            mtlparser_state->mtlparser_strlcat(
                mtlparser_state->last_error_msg,
                "Expected 3 f32 values (rgb values)"
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
        
        rgb_stat[rgb_i] = (f32)T1_token_as_number_floating(*i);
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
    u32 * recipient_size,
    const u32 recipient_cap,
    const char * input,
    u8 * good)
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
        /* u8 * good: */
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
    
    u32 tokens_count = T1_token_get_token_count();
    for (u16 i = 0; i < tokens_count; i++) {
        mtlparser_state->last_error_msg[0] = '\0';
        char stack_string_64bytes[64];
        mtlparser_uint_to_string(
            T1_token_get_line_num(i),
            stack_string_64bytes);
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            stack_string_64bytes,
            ERROR_MSG_CAP);
        mtlparser_state->mtlparser_strlcat(
            mtlparser_state->last_error_msg,
            ":: ",
            ERROR_MSG_CAP);
        
        switch (T1_token_get_enum_value(i)) {
            case MTLTOKEN_COMMENT: {
                while (
                    T1_token_get_enum_value(i) != MTLTOKEN_NEWLINE)
                {
                    i++;
                }
                break;
            }
            case MTLTOKEN_NEWLINE: {
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
                
                while (T1_token_get_enum_value(i) == MTLTOKEN_SPACE) {
                    i++;
                }
                
                if (
                    T1_token_get_enum_value(i) != MTLTOKEN_STRINGLITERAL ||
                    T1_token_get_string_value_size(i) < 1)
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
                    T1_token_get_string_value(i),
                    T1_MATERIAL_NAME_CAP);
                
                while (
                    T1_token_get_enum_value(i+1) == MTLTOKEN_SPACE &&
                    (i + 2) < T1_token_get_token_count())
                {
                    i += 1;
                }
                
                if (T1_token_get_enum_value(i+1) == MTLTOKEN_USEBASEMTL) {
                    current_material->use_base_mtl_flag = 1;
                    i++;
                }
                
                break;
            }
            case MTLTOKEN_T1_COMMENT: {
                
                i++;
                if (T1_token_get_enum_value(i) != MTLTOKEN_SPACE) {
                    *good = 0;
                    mtlparser_state->last_error_msg[0] = '\0';
                    mtlparser_state->mtlparser_strlcat(
                    mtlparser_state->last_error_msg,
                    "Expected a space after special comment #T1",
                    ERROR_MSG_CAP);
                    return;
                }
                
                i++;
                if (T1_token_get_enum_value(i) != MTLTOKEN_STRINGLITERAL) {
                    *good = 0;
                    mtlparser_state->last_error_msg[0] = '\0';
                    mtlparser_state->mtlparser_strlcat(
                        mtlparser_state->last_error_msg,
                        "Expected a string literal describing a special "
                        "property after special comment #T1, got: ",
                        ERROR_MSG_CAP);
                    mtlparser_state->mtlparser_strlcat(
                        mtlparser_state->last_error_msg,
                        T1_token_get_string_value(i),
                        ERROR_MSG_CAP);
                    return;
                }
                
                f32 * target_f32 = NULL;
                if (
                    T1_std_are_equal_strings(
                        T1_token_get_string_value(i),
                        "uv_scroll[0]"))
                {
                    target_f32 = &current_material->T1_uv_scroll[0];
                } else if (
                    T1_std_are_equal_strings(
                        T1_token_get_string_value(i),
                        "uv_scroll[1]"))
                {
                    target_f32 = &current_material->T1_uv_scroll[1];
                } else {
                    *good = 0;
                    mtlparser_state->last_error_msg[0] = '\0';
                    mtlparser_state->mtlparser_strlcat(
                        mtlparser_state->last_error_msg,
                        "Unrecognized special embedded comment property: ",
                        ERROR_MSG_CAP);
                    mtlparser_state->mtlparser_strlcat(
                        mtlparser_state->last_error_msg,
                        T1_token_get_string_value(i),
                        ERROR_MSG_CAP);
                    return;
                }
                
                parse_single_f32_stat(
                    /* u32 * i: */
                        &i,
                    /* const char * material_name: */
                        current_material->name,
                    /* f32 * f32_stat: */
                        target_f32,
                    /* u8 * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_NS: {
                parse_single_f32_stat(
                    /* u32 * i: */
                        &i,
                    /* const char * material_name: */
                        current_material->name,
                    /* f32 * f32_stat: */
                        &current_material->specular_exponent,
                    /* u8 * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                break;
            }
            case MTLTOKEN_ALPHA_d: {
                parse_single_f32_stat(
                    /* u32 * i: */
                        &i,
                    /* const char * material_name: */
                        current_material->name,
                    /* f32 * f32_stat: */
                        &current_material->alpha,
                    /* u8 * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_ILLUM: {
                parse_single_f32_stat(
                    /* u32 * i: */
                        &i,
                    /* const char * material_name: */
                        current_material->name,
                    /* f32 * f32_stat: */
                        &current_material->illum,
                    /* u8 * good: */
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
                
                parse_single_f32_stat(
                    /* u32 * i: */
                        &i,
                    /* const char * material_name: */
                        current_material->name,
                    /* f32 * f32_stat: */
                        &current_material->refraction,
                    /* u8 * good: */
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
                    /* u32 * i: */
                        &i,
                    /* const char * material_name: */
                        current_material->name,
                    /* f32 * rgb_stat: */
                        current_material->ambient_rgb,
                    /* u32 * already_set_flag: */
                        &mtlparser_state->ambient_set_for_current_mtl,
                    /* u8 * good: */
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
                    /* u32 * i: */
                        &i,
                    /* const char * material_name: */
                        current_material->name,
                    /* f32 * rgb_stat: */
                        current_material->diffuse_rgb,
                    /* u32 * already_set_flag: */
                        &mtlparser_state->diffuse_set_for_current_mtl,
                    /* u8 * good: */
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
                    /* u32 * i: */
                        &i,
                    /* const char * material_name: */
                        current_material->name,
                    /* f32 * rgb_stat: */
                        current_material->specular_rgb,
                    /* u32 * already_set_flag: */
                        &mtlparser_state->specular_set_for_current_mtl,
                    /* u8 * good: */
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
                    /* u32 * i: */
                        &i,
                    /* const char * material_name: */
                        current_material->name,
                    /* f32 * rgb_stat: */
                        current_material->emissive_rgb,
                    /* u32 * already_set_flag: */
                        &mtlparser_state->emissive_set_for_current_mtl,
                    /* u8 * good: */
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
                    T1_token_get_string_value(i),
                    ERROR_MSG_CAP);
                mtlparser_state->mtlparser_strlcat(
                    mtlparser_state->last_error_msg,
                    "' (",
                    ERROR_MSG_CAP);
                stack_string_64bytes[0] = '\0';
                mtlparser_uint_to_string(
                    T1_token_get_string_value_size(i),
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
                    /* u32 * i: */
                        &i,
                    /* const char * material_name: */
                        current_material->name,
                    /* char * string_stat: */
                        current_material->ambient_map,
                    /* u8 * good: */
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
                    /* u32 * i: */
                        &i,
                    /* const char * material_name: */
                        current_material->name,
                    /* char * string_stat: */
                        current_material->diffuse_map,
                    /* u8 * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_SPECULAR_MAP: {
                break;
            }
            case MTLTOKEN_SPECULAR_EXPONENT_MAP: {
                parse_single_string_stat(
                    /* u32 * i: */
                        &i,
                    /* const char * material_name: */
                        current_material->name,
                    /* char * string_stat: */
                        current_material->specular_exponent_map,
                    /* u8 * good: */
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
                
                if (T1_token_get_enum_value(i) != MTLTOKEN_STRINGLITERAL) {
                    mtlparser_state->mtlparser_strlcat(
                        mtlparser_state->last_error_msg,
                        "Unexpected token type after map_d: ",
                        ERROR_MSG_CAP);
                    mtlparser_state->mtlparser_strlcat(
                        mtlparser_state->last_error_msg,
                        T1_token_get_string_value(i),
                        ERROR_MSG_CAP);
                    *good = 0;
                    return;
                }
                
                break;
            }
            case MTLTOKEN_BUMP_OR_NORMAL_MAP: {
                if (T1_token_get_enum_value(i+1) == MTLTOKEN_BUMP_MAP_ARG_INTENSITY) {
                    i++;
                    
                    parse_single_f32_stat(
                        /* u32 * i: */
                            &i,
                        /* const char * material_name: */
                            current_material->name,
                        /* f32 * f32_stat: */
                            &current_material->bump_map_intensity,
                        /* u8 * good: */
                            good);
                } else {
                    current_material->bump_map_intensity = 1.0f;
                }
                
                parse_single_string_stat(
                    /* u32 * i: */
                        &i,
                    /* const char * material_name: */
                        current_material->name,
                    /* char * string_stat: */
                        current_material->bump_or_normal_map,
                    /* u8 * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_ROUGHNESS: {
                parse_single_f32_stat(
                    /* u32 * i: */
                        &i,
                    /* const char * material_name: */
                        current_material->name,
                    /* f32 * f32_stat: */
                        &current_material->roughness,
                    /* u8 * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_METALLIC: {
                parse_single_f32_stat(
                    /* u32 * i: */
                        &i,
                    /* const char * material_name: */
                        current_material->name,
                    /* f32 * f32_stat: */
                        &current_material->metallic,
                    /* u8 * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_SHEEN: {
                parse_single_f32_stat(
                    /* u32 * i: */
                        &i,
                    /* const char * material_name: */
                        current_material->name,
                    /* f32 * f32_stat: */
                        &current_material->sheen,
                    /* u8 * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_CLEARCOAT: {
                parse_single_f32_stat(
                    /* u32 * i: */
                        &i,
                    /* const char * material_name: */
                        current_material->name,
                    /* f32 * f32_stat: */
                        &current_material->clearcoat,
                    /* u8 * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_CLEARCOAT_ROUGHNESS: {
                parse_single_f32_stat(
                    /* u32 * i: */
                        &i,
                    /* const char * material_name: */
                        current_material->name,
                    /* f32 * f32_stat: */
                        &current_material->clearcoat_roughness,
                    /* u8 * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_ANISOTROPY: {
                parse_single_f32_stat(
                    /* u32 * i: */
                        &i,
                    /* const char * material_name: */
                        current_material->name,
                    /* f32 * f32_stat: */
                        &current_material->anisotropy,
                    /* u8 * good: */
                        good);
                
                if (*good) {
                    *good = 0;
                } else {
                    return;
                }
                
                break;
            }
            case MTLTOKEN_ANISOTROPY_ROTATION: {
                parse_single_f32_stat(
                    /* u32 * i: */
                        &i,
                    /* const char * material_name: */
                        current_material->name,
                    /* f32 * f32_stat: */
                        &current_material->anisotropy_rotation,
                    /* u8 * good: */
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
                    T1_token_get_enum_value(i),
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
                    T1_token_get_string_value(i) == NULL ?
                        "NULL" : T1_token_get_string_value(i),
                    ERROR_MSG_CAP);
                return;
        }
    }
    
    *good = 1;
}
