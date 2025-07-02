#include "T1_tokenizer.h"

static void T1_tokenizer_strcat(
    char * recipient,
    const char * to_append)
{
    uint32_t i = 0;
    while (recipient[i] != '\0') {
        i++;
    }
    
    uint32_t j = 0;
    while (to_append[j] != '\0') {
        recipient[i++] = to_append[j++];
    }
    
    recipient[i] = '\0';
}


/*
These options apply to the next token you register.
*/
#define STRING_LITERAL_CAP 1024
#define PATTERN_ASCII_CAP 128
typedef struct T1TokenPattern {
    char ascii[PATTERN_ASCII_CAP];
    uint8_t active;
} T1TokenPattern;

#define PATTERNS_CAP 5
typedef struct T1TokenNextReg {
    T1TokenPattern start_pattern;
    T1TokenPattern stop_patterns[PATTERNS_CAP];
    uint32_t middle_cap;
    uint8_t bitflags;
} T1TokenNextReg;

typedef struct RegisteredToken {
    uint32_t enum_value;
    char * start_pattern;
    char * stop_patterns[PATTERNS_CAP];
    uint32_t middle_cap;
    uint8_t bitflags;
} RegisteredToken;

#define ASCII_STORE_CAP 1000000
#define REGISTERED_TOKENS_CAP 2000
#define TOKENS_CAP 10000
#define NUMBERS_CAP 10000
typedef struct TokTokenState {
    void * (* memset)(void *, int, size_t);
    size_t (* strlen)(const char *);
    char * ascii_store;
    T1TokenNextReg next_reg;
    RegisteredToken regs[REGISTERED_TOKENS_CAP];
    TokToken tokens[TOKENS_CAP];
    TokTokenNumber numbers[NUMBERS_CAP];
    uint32_t string_literal_enum_value;
    uint32_t regs_size;
    uint32_t tokens_size;
    uint32_t numbers_size;
    uint32_t ascii_store_next_i; // index to write the next string at
    uint32_t good;
    uint8_t string_literal_bitflags;
} TokTokenState;

static TokTokenState * tts = NULL;

#ifndef NDEBUG
void T1_token_print_debug_state(
    int (*arg_printf)(const char *, ... ))
{
    arg_printf("%s\n", "*****");
    arg_printf("%s\n", "toktokenizer state (debug-only):");
    arg_printf("%s\n", "*****");
    if (
        !tts ||
        !tts->ascii_store)
    {
        arg_printf(
            "%s\n",
            "Not initialized - the tokenizer has no memory to work with.\n\n"
            "To provide memory, call toktoken_init(malloc, &good), or "
            "if you already did, the initialization must have failed "
            "because your malloc() function returned NULL)");
        arg_printf("%s\n", "*****");
        return;
    }
    
    arg_printf(
        "%s\n\n",
        "Initialized - the tokenizer has memory to work with.");
    
    if (tts->good) {
        arg_printf(
            "%s\n\n",
            "The tokenizer's 'good' flag is set (no errors encountered).");
    } else {
        arg_printf(
            "%s\n\n",
            "The tokenizer's 'good' flag is not set (previously encountered"
            " errors)."
            "Make sure you pass a uint to 'good' params and make sure they"
            " return 1!");
    }
    
    if (tts->string_literal_enum_value < UINT32_MAX) {
        arg_printf(
            "String literals will be assigned to enum value: %u\n",
            tts->string_literal_enum_value);
    } else {
        arg_printf(
            "%s\n\n",
            "The enum value for string literals is not set. This is a hard "
            "requirement before tokenizing.\n"
            "Call toktoken_register_string_literal_enum(ENUM_VAL, &good) to "
            "set the enum you want to associate with random string "
            "literals.");
    }
    
    if (tts->regs_size > 0) {
        for (uint32_t i = 0; i < tts->regs_size; i++) {
            arg_printf(
                "Token %u's start value: \"%s\", enum value: %u\n",
                i,
                tts->regs[i].start_pattern,
                tts->regs[i].enum_value);
        }
    } else {
        arg_printf(
            "%s\n",
            "No tokens have been registered. Use toktoken_register_token("
            "str, enumval, &good) to register tokens. Before registering "
            "tokens, you can set optional flags in toktoken_client_settings "
            "if you need non-default behavior (e.g. you don't want your "
            "token to be case sensitive, see toktokenizer.h)");
    }
    arg_printf("%s\n", "*****");
}
#endif

void T1_token_init(
    void * (* arg_memset_func)(void *, int, size_t),
    size_t (* arg_strlen_func)(const char *),
    void * (* arg_malloc_func)(size_t),
    uint32_t * good)
{
    *good = 0;
        
    tts = arg_malloc_func(sizeof(TokTokenState));
    if (!tts) {
        tts->good = 0;
        return;
    }
    
    tts->memset = arg_memset_func;
    tts->strlen = arg_strlen_func;
    
    tts->ascii_store = arg_malloc_func(ASCII_STORE_CAP);
    if (tts->ascii_store == NULL) {
        tts->good = 0;
        return;
    }
    
    T1_token_reset(good);
    if (!*good) {
        tts->good = 0;
        return;
    }
    
    *good = 1;
    tts->good = 1;
}

void T1_token_reset(uint32_t * good) {
    *good = 0;
    if (tts == NULL) {
        // init() failed or wasn't called yet
        return;
    }
    
    if (tts->ascii_store) {
        tts->memset(
            tts->ascii_store,
            0,
            ASCII_STORE_CAP);
    }
    tts->memset(
        tts->regs,
        0,
        sizeof(TokTokenState) - offsetof(TokTokenState, regs));
    
    tts->next_reg.bitflags =
            T1_TOKEN_FLAG_IGNORE_CASE |
            T1_TOKEN_FLAG_SCIENTIFIC_OK |
            T1_TOKEN_FLAG_LEAD_DOT_OK |
            T1_TOKEN_FLAG_PRECISE;
    
    tts->string_literal_enum_value = UINT32_MAX;
    tts->good = 1;
    
    *good = 1;
}

void T1_token_set_reg_bitflags(
    const uint8_t bitflags)
{
    if (
        tts == NULL ||
        !tts->good)
    {
        return;
    }
    
    tts->next_reg.bitflags = bitflags;
}

void T1_token_clear_start_pattern(void) {
    if (
        tts == NULL ||
        !tts->good)
    {
        return;
    }
    
    tts->memset(
        tts->next_reg.start_pattern.ascii,
        0,
        PATTERN_ASCII_CAP);
    
    tts->next_reg.start_pattern.active = 0;
}

void T1_token_set_reg_start_pattern(
    const char * start_pattern)
{
    if (
        tts == NULL ||
        !tts->good ||
        start_pattern == NULL ||
        tts->strlen(start_pattern) >= PATTERN_ASCII_CAP)
    {
        return;
    }
    
    tts->next_reg.start_pattern.ascii[0] = '\0';
    T1_tokenizer_strcat(
        /* char * recipient: */
            tts->next_reg.start_pattern.ascii,
        /* const char * to_append: */
            start_pattern);
    tts->next_reg.start_pattern.active = 1;
}

void T1_token_clear_stop_patterns(void) {
    if (
        tts == NULL ||
        !tts->good)
    {
        return;
    }
    
    for (uint32_t i = 0; i < PATTERNS_CAP; i++) {
        
        tts->memset(
            tts->next_reg.stop_patterns[i].ascii,
            0,
            PATTERN_ASCII_CAP);
        
        tts->next_reg.stop_patterns[i].active = 0;
    }
}

void T1_token_set_reg_stop_pattern(
    const char * stop_pattern,
    const uint32_t pattern_index)
{
    if (
        tts == NULL ||
        !tts->good ||
        stop_pattern == NULL)
    {
        return;
    }
    
    tts->next_reg.stop_patterns[pattern_index].ascii[0] = '\0';
    T1_tokenizer_strcat(
        /* char * recipient: */
            tts->next_reg.stop_patterns[pattern_index].ascii,
        /* const char * to_append: */
            stop_pattern);
    tts->next_reg.stop_patterns[pattern_index].active = 1;
}

void T1_token_set_reg_middle_cap(
    const uint32_t middle_cap)
{
    if (
        tts == NULL ||
        !tts->good)
    {
        return;
    }
    
    tts->next_reg.middle_cap = middle_cap;
}

void toktoken_register_string_literal_enum(
    const uint32_t enum_value,
    uint32_t * good)
{
    *good = 0;
    if (
        tts == NULL ||
        !tts->good ||
        enum_value == UINT32_MAX)
    {
        return;
    }
    
    tts->string_literal_enum_value = enum_value;
    tts->string_literal_bitflags = tts->next_reg.bitflags;
    
    *good = 1;
}

void T1_token_set_string_literal(
    const uint32_t enum_value,
    uint32_t * good)
{
    *good = 0;
    
    if (
        tts == NULL ||
        !tts->good)
    {
        return;
    }
    
    tts->string_literal_enum_value = enum_value;
    *good = 1;
}

static char * copy_string_to_ascii_store(
    const char * to_copy,
    uint32_t * good)
{
    *good = 0;
    
    char * return_value = tts->ascii_store + tts->ascii_store_next_i;
    
    size_t new_len = tts->strlen(to_copy);
    
    if (tts->ascii_store_next_i + new_len >= ASCII_STORE_CAP) {
        *good = 0;
        return NULL;
    } else {
        tts->ascii_store_next_i += (new_len + 1);
    }
    
    return_value[0] = '\0';
    T1_tokenizer_strcat(return_value, to_copy);
    
    *good = 1;
    return return_value;
}

void T1_token_register(
    const uint32_t enum_value,
    uint32_t * good)
{
    *good = 0;
    
    if (
        tts == NULL ||
        !tts->good)
    {
        return;
    }
    
    // keep the token's data
    if (
        tts->regs_size + 1 >= REGISTERED_TOKENS_CAP)
    {
        tts->good = 0;
        return;
    }
    RegisteredToken * new = &tts->regs[tts->regs_size];
    tts->regs_size += 1;
    
    if (tts->next_reg.start_pattern.active) {
        new->start_pattern =
            copy_string_to_ascii_store(
                tts->next_reg.start_pattern.ascii,
            good);
    }
    
    for (uint32_t i = 0; i < PATTERNS_CAP; i++) {
        if (tts->next_reg.stop_patterns[i].active) {
            new->stop_patterns[i] =
                copy_string_to_ascii_store(
                    tts->next_reg.stop_patterns[i].ascii,
                good);
            if (*good) { *good = 0; } else { return; }
        }
    }
    
    
    new->enum_value = enum_value;
    new->middle_cap = tts->next_reg.middle_cap;
    new->bitflags = tts->next_reg.bitflags;
    
    if ((new->bitflags & T1_TOKEN_FLAG_IGNORE_CASE) > 0)
    {
        // TODO: implement ignore case tokens
        *good = 0;
        return;
    }
    
    // keep the ascii value in our persistent local store
    //    uint32_t i = 0;
    //    while (
    //        ascii_value[i] != '\0' &&
    //        tts->ascii_store_next_i < ASCII_STORE_CAP)
    //    {
    //        tts->ascii_store[tts->ascii_store_next_i++] = ascii_value[i];
    //        i += 1;
    //    }
    //    if (tts->ascii_store_next_i + 1 >= ASCII_STORE_CAP) {
    //        tts->good = 0;
    //        return;
    //    }
    //    tts->ascii_store[tts->ascii_store_next_i++] = '\0';
    
    
    *good = 1;
}

static uint32_t T1_token_strmatch(
    const char * stream,
    const char * ascii_val)
{ 
    uint32_t i = 0;
    while (ascii_val[i] != '\0') {
        if (stream[i] != ascii_val[i]) { return 0; }
        i++;
    }
    
    if (ascii_val[i] != '\0') { return 0; }
    
    return i;
}

static void toktoken_string_match_tokens(
    const char * input,
    uint32_t * matching_token_i,
    uint32_t * data_len)
{
    *matching_token_i = UINT32_MAX;
    *data_len = 0;
    
    for (uint32_t i = 0; i < tts->regs_size; i++) {
        if (tts->regs[i].start_pattern == NULL) { continue; }
        
        uint32_t matching_chars =
            T1_token_strmatch(
                input,
                tts->regs[i].start_pattern);
        
        if (
            matching_chars > 0 &&
            tts->regs[i].start_pattern[matching_chars] == '\0')
        {
            *matching_token_i = i;
            *data_len = matching_chars;
        }
        
        if (*matching_token_i == UINT32_MAX) { continue; }
        
        // If there are no stop conditions registered, this is automatically
        // a hit. If so, there must also be 0 middle characters
        if (tts->regs[i].stop_patterns[0] == NULL) {
            #ifndef T1_TOKEN_NO_ASSERTS
            for (uint32_t pat_i = 0; pat_i < PATTERNS_CAP; pat_i++) {
                assert(tts->regs[i].stop_patterns[pat_i] == NULL);
            }
            assert(tts->regs[i].middle_cap == 0);
            #endif
            return;
        }
        
        // look for the first ending match
        for (
            uint32_t middle_chars = 0;
            middle_chars <= tts->regs[i].middle_cap;
            middle_chars++)
        {
            for (uint32_t pat_i = 0; pat_i < PATTERNS_CAP; pat_i++) {
                if (tts->regs[i].stop_patterns[pat_i] == NULL) { continue; }
                
                uint32_t matching_chars =
                    T1_token_strmatch(
                        input + *data_len + middle_chars,
                        tts->regs[i].stop_patterns[pat_i]);
                
                if (
                    matching_chars > 0 &&
                    tts->regs[i].stop_patterns[pat_i][matching_chars] == '\0')
                {
                    *data_len += middle_chars;
                    *data_len += matching_chars;
                    return;
                }
            }
        }
        
        *matching_token_i = UINT32_MAX;
        *data_len = 0;
    }
}

static void T1_token_set_number_flags(
    TokToken * token,
    const uint8_t bitflags)
{
    //    const uint8_t scientific_ok =
    //        (bitflags & T1_TOKEN_FLAG_SCIENTIFIC_OK) > 0;
    //    const uint8_t lead_dot_ok =
    //        (bitflags & T1_TOKEN_FLAG_LEAD_DOT_OK) > 0;
    //    const uint8_t precise =
    //        (bitflags & T1_TOKEN_FLAG_PRECISE) > 0;
    
    token->number_value = NULL;
    
    if (
        token->string_value_size == 0 ||
        tts->numbers_size + 1 >= NUMBERS_CAP)
    {
        return;
    }
    
    uint32_t i = 0;
    uint32_t has_leading_minus = 0;
    uint32_t has_leading_nums = 0;
    uint64_t leading_num_u64 = 0;
    uint32_t has_dot = 0;
    uint64_t trailing_num_u64 = 0;
    uint32_t trailing_digit_count = 0;
    uint32_t trailing_num_leading_zeros = 0;
    uint32_t has_exponent = 0;
    
    // UINT64_MAX as string (20 digits)
    const char *u64_max_str = "18446744073709551615";
    
    // Handle leading sign
    if (token->string_value[i] == '-') {
        has_leading_minus = 1;
        i++;
    } else if (token->string_value[i] == '+') {
        i++;
    }

    // Validate size (conservative, adjust for exponent later)
    // +10 for . + fraction + e-xxx
    if (
        token->string_value_size > (has_leading_minus ? 21 : 20) + 10)
    {
        return;
    }
    
    // Validate first character after sign (digit or dot)
    if (
        i < token->string_value_size &&
        token->string_value[i] != '.' && 
        (token->string_value[i] < '0' || token->string_value[i] > '9'))
    {
        return;
    }
    
    // Numerical characters (leading)
    uint32_t mult = 1;
    // uint32_t leading_digit_count = 0;
    while (
        i < token->string_value_size && token->string_value[i] >= '0' &&
        token->string_value[i] <= '9')
    {
        if (!has_leading_nums) {
            has_leading_nums = 1;
            if (token->string_value[i] == '0') {
                trailing_num_leading_zeros += 1;
            }
            
            if (
                !has_dot &&
                token->string_value_size == (has_leading_minus ? 21 : 20))
            {
                uint32_t start = has_leading_minus ? 1 : 0;
                for (
                    uint32_t j = start;
                    j < token->string_value_size;
                    j++)
                {
                    if (
                        token->string_value[j] > u64_max_str[j - start])
                    {
                        return; // Exceeds UINT64_MAX
                    } else if (
                        token->string_value[j] < u64_max_str[j - start])
                    {
                        break; // Safe, number is smaller
                    }
                }
            }
        }
        
        leading_num_u64 *= mult;
        leading_num_u64 += (uint64_t)(token->string_value[i] - '0');
        mult = 10;
        // leading_digit_count++;
        i++;
    }
    
    if (trailing_num_leading_zeros > 0 && leading_num_u64 != 0) {
        return; // Reject "-01" but allow "-0"
    }
    
    // Decimal point and trailing digits
    if (i < token->string_value_size && token->string_value[i] == '.')
    {
        if (has_dot) {
            return; // Multiple dots
        }
        has_dot = 1;
        i++;
        
        mult = 1;
        while (
            i < token->string_value_size &&
            token->string_value[i] >= '0' &&
            token->string_value[i] <= '9')
        {
            trailing_num_u64 *= mult;
            trailing_num_u64 += (uint64_t)(token->string_value[i] - '0');
            mult = 10;
            trailing_digit_count += 1;
            i++;
        }
    }
    
    // Exponent (e.g., e-4, E+05)
    if (
        i < token->string_value_size &&
            (
                token->string_value[i] == 'e' ||
                token->string_value[i] == 'E')
        )
    {
        if ((tts->next_reg.bitflags & T1_TOKEN_FLAG_SCIENTIFIC_OK) == 0)
        {
            return; // Reject exponent
        }
        
        has_exponent = 1;
        i++;
        if (
            i < token->string_value_size &&
            (token->string_value[i] == '+' || token->string_value[i] == '-'))
        {
            i++;
        }
        
        uint32_t has_exponent_digits = 0;
        while (
            i < token->string_value_size &&
            token->string_value[i] >= '0' &&
            token->string_value[i] <= '9')
        {
            has_exponent_digits = 1;
            i++;
        }
        if (!has_exponent_digits) {
            return; // Invalid exponent (e.g., "1.2e", "1.2e-")
        }
    }
    
    // Ensure all characters consumed
    if (
        i != token->string_value_size ||
        (has_dot && trailing_digit_count == 0 && !has_exponent))
    {
        return; // Invalid trailing characters or lone dot
    }
    
    // Finally, set flags
    if (has_leading_nums || trailing_digit_count || has_exponent)
    {
        token->castable_flags |= 1; // is number
        token->number_value = &tts->numbers[tts->numbers_size];
        tts->numbers_size += 1;
        token->number_value->unsigned_int     = leading_num_u64;
        token->number_value->signed_int       =
            ((int)leading_num_u64) * (has_leading_minus ? -1 : 1);
        token->number_value->double_precision =
            (double)leading_num_u64 * (has_leading_minus ? -1.0 : 1.0);
        if (has_dot) {
            double divisor = 1.0;
            for (uint32_t _ = 0; _ < trailing_digit_count; _++) {
                divisor *= 10.0;
            }
            //            while ((double)trailing_num_u64 / divisor > 1.0) {
            //                divisor *= 10.0;
            //            }
            token->number_value->double_precision +=
                ((double)trailing_num_u64 / divisor);
        }
        
        if (!has_dot && !has_exponent) {
            // Integer: double always fits, float if ≤ 2²⁴
            token->castable_flags |= 2; // fits double
            if (leading_num_u64 <= (1ULL << 24)) { // 16,777,216
                token->castable_flags |= 4; // fits float
            }
            
            if (has_leading_minus) {
                // Negative integer
                if (leading_num_u64 <= ((uint64_t)INT8_MAX + 1)) {
                    token->castable_flags |= 128; // fits i8
                }
                if (leading_num_u64 <= ((uint64_t)INT16_MAX + 1)) {
                    token->castable_flags |= 256; // fits i16
                }
                if (leading_num_u64 <= ((uint64_t)INT32_MAX + 1)) {
                    token->castable_flags |= 512; // fits i32
                }
                if (leading_num_u64 <= ((uint64_t)INT64_MAX + 1)) {
                    token->castable_flags |= 1024; // fits i64
                }
            } else {
                // Positive integer
                if (leading_num_u64 <= UINT8_MAX) {
                    token->castable_flags |= 8; // fits u8
                }
                if (leading_num_u64 <= UINT16_MAX) {
                    token->castable_flags |= 16; // fits u16
                }
                if (leading_num_u64 <= UINT32_MAX) {
                    token->castable_flags |= 32; // fits u32
                }
                token->castable_flags |= 64; // fits u64
                if (leading_num_u64 <= INT8_MAX) {
                    token->castable_flags |= 128; // fits i8
                }
                if (leading_num_u64 <= INT16_MAX) {
                    token->castable_flags |= 256; // fits i16
                }
                if (leading_num_u64 <= INT32_MAX) {
                    token->castable_flags |= 512; // fits i32
                }
                if (leading_num_u64 <= INT64_MAX) {
                    token->castable_flags |= 1024; // fits i64
                }
            }
        } else {
            // Floating-point: count significant digits
            uint32_t significant_digits = 0;
            // Leading digits
            uint64_t temp = leading_num_u64;
            while (temp > 0) {
                if (temp % 10 != 0 || significant_digits > 0) {
                    significant_digits++;
                }
                temp /= 10;
            }
            // Trailing digits
            temp = trailing_num_u64;
            while (temp > 0) {
                if (temp % 10 != 0 || significant_digits > 0) {
                    significant_digits++;
                }
                temp /= 10;
            }
            // For numbers like ".123", leading_num_u64 = 0,
            // so check trailing digits
            if (significant_digits == 0 && trailing_num_u64 > 0) {
                temp = trailing_num_u64;
                while (temp > 0) {
                    significant_digits++;
                    temp /= 10;
                }
            }
            
            // double: ≤ 16 significant digits
            if (
                significant_digits <= 16 || has_exponent)
            {
                token->castable_flags |= 2; // fits double
            }
            // float: ≤ 7 significant digits
            if (
                significant_digits <= 7 ||
                (has_exponent && significant_digits <= 8))
            {
                token->castable_flags |= 4; // fits float
            }
        }
    }    
}

void T1_token_run(
    const char * input,
    uint32_t * good)
{
    tts->tokens_size = 0;
    tts->numbers_size = 0;
    
    #ifndef T1_TOKEN_IGNORE_ASSERTS
    assert(good != NULL);
    #endif
    
    *good = 0;
    if (
        input == NULL ||
        tts == NULL ||
        !tts->good ||
        tts->string_literal_enum_value == UINT32_MAX ||
        tts->string_literal_enum_value == UINT32_MAX)
    {
        return;
    }
    
    uint32_t i = 0;
    uint32_t matching_token_i = UINT32_MAX;
    uint32_t data_len;
    uint32_t line_number = 1;
    
    TokToken * previous_lit_token = NULL;
    uint32_t previous_lit_ascii_i = 0;

    while (input[i] != '\0') {
        
        toktoken_string_match_tokens(
            input + i,
            &matching_token_i,
            &data_len);
        
        if (matching_token_i == UINT32_MAX) {
            
            if (previous_lit_token == NULL) {
                previous_lit_token = &tts->tokens[tts->tokens_size];
                if (tts->tokens_size + 1 >= TOKENS_CAP) {
                    *good = 0;
                    tts->good = 0;
                    return;
                }
                tts->tokens_size += 1;
                previous_lit_token->enum_value =
                    tts->string_literal_enum_value;
                previous_lit_token->string_value =
                    tts->ascii_store + tts->ascii_store_next_i;
                tts->ascii_store_next_i += STRING_LITERAL_CAP;
                tts->memset(
                    previous_lit_token->string_value,
                    0,
                    STRING_LITERAL_CAP);
                previous_lit_ascii_i = 0;
            }
            
            previous_lit_token->string_value[previous_lit_ascii_i] =
                input[i];
            assert(previous_lit_ascii_i+1 < STRING_LITERAL_CAP);;
            previous_lit_token->string_value[previous_lit_ascii_i+1] =
                '\0';
            previous_lit_ascii_i += 1;
            i += 1;
            
            continue;
        } else {
            TokToken * new = &tts->tokens[tts->tokens_size];
            if (tts->tokens_size + 1 >= TOKENS_CAP) {
                *good = 0;
                tts->good = 0;
                return;
            }
            
            // Commit the previous string literal if it was going
            previous_lit_token = NULL;
            previous_lit_ascii_i = 0;
            
            tts->tokens_size += 1;
            new->line_number = line_number;
            new->enum_value = tts->regs[matching_token_i].enum_value;
            new->string_value = copy_string_to_ascii_store(input + i, good);
            assert(*good); // TODO: don't copy huge strings, copy what's needed
            new->string_value[data_len] = '\0';
            new->string_value_size = (uint16_t)data_len;
            
            T1_token_set_number_flags(
                new,
                tts->regs[matching_token_i].bitflags);
            
            i += data_len;
        }
        
        if (!tts->good) { return; }
    }
    
    for (uint32_t i = 0; i < tts->tokens_size; i++) {
        if (tts->tokens[i].enum_value == tts->string_literal_enum_value) {
            tts->tokens[i].string_value_size =
                tts->strlen(tts->tokens[i].string_value);
            T1_token_set_number_flags(
                &tts->tokens[i],
                tts->string_literal_bitflags);
        }
    }
    
    *good = 1;
}

uint32_t T1_token_get_token_count(void) {
    return tts->tokens_size;
}

TokToken * T1_token_get_token_at(
    const uint32_t token_i)
{
    return &tts->tokens[token_i];
}

