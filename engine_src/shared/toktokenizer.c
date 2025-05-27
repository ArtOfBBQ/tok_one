#include "toktokenizer.h"

TokTokenClientSettings * toktoken_client_settings = NULL;

typedef struct RegisteredToken {
    uint32_t enum_value;
    char * ascii_value;
    uint8_t ignores_case;
    uint8_t requires_trailing_whitespace;
} RegisteredToken;

#define ASCII_STORE_CAP 1000000
#define REGISTERED_TOKENS_CAP 2000
#define TOKENS_CAP 10000
#define NUMBERS_CAP 10000
typedef struct TokTokenState {
    RegisteredToken registered_tokens[REGISTERED_TOKENS_CAP];
    TokToken tokens[TOKENS_CAP];
    TokTokenNumber numbers[NUMBERS_CAP];
    uint32_t string_literal_enum_value;
    uint32_t newline_enum_value;
    uint32_t registered_tokens_size;
    uint32_t tokens_size;
    uint32_t numbers_size;
    char * ascii_store;
    uint32_t ascii_store_next_i; // index to write the next string at
    uint32_t good;
} TokTokenState;

static TokTokenState * toktoken_state = NULL;

#ifndef NDEBUG
void toktoken_debug_print_state(
    int (*arg_printf)(const char *, ... ))
{
    arg_printf("%s\n", "*****");
    arg_printf("%s\n", "toktokenizer state (debug-only):");
    arg_printf("%s\n", "*****");
    if (
        !toktoken_client_settings ||
        !toktoken_state ||
        !toktoken_state->ascii_store)
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
    
    if (toktoken_state->good) {
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
    
    if (toktoken_state->string_literal_enum_value < UINT32_MAX) {
        arg_printf(
            "String literals will be assigned to enum value: %u\n",
            toktoken_state->string_literal_enum_value);
    } else {
        arg_printf(
            "%s\n\n",
            "The enum value for string literals is not set. This is a hard "
            "requirement before tokenizing.\n"
            "Call toktoken_register_string_literal_enum(ENUM_VAL, &good) to "
            "set the enum you want to associate with random string "
            "literals.");
    }
    
    if (toktoken_state->registered_tokens_size > 0) {
        for (uint32_t i = 0; i < toktoken_state->registered_tokens_size; i++) {
            arg_printf(
                "Token %u's ASCII value: \"%s\", enum value: %u\n",
                i,
                toktoken_state->registered_tokens[i].ascii_value,
                toktoken_state->registered_tokens[i].enum_value);
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

void toktoken_init(
    void*(*arg_malloc_func)(size_t),
    uint32_t * good)
{
    *good = 0;
    
    toktoken_client_settings = arg_malloc_func(sizeof(TokTokenClientSettings));
    if (!toktoken_client_settings) {
        toktoken_state->good = 0;
        return;
    }
    
    toktoken_state = arg_malloc_func(sizeof(TokTokenState));
    if (!toktoken_state) {
        toktoken_state->good = 0;
        return;
    }
    
    toktoken_state->ascii_store = arg_malloc_func(ASCII_STORE_CAP);
    if (toktoken_state->ascii_store == NULL) {
        toktoken_state->good = 0;
        return;
    }
    
    toktoken_reset(good);
    if (!*good) {
        toktoken_state->good = 0;
        return;
    }
    
    *good = 1;
    toktoken_state->good = 1;
}

void toktoken_reset(uint32_t * good) {
    *good = 0;
    if (toktoken_client_settings == NULL || toktoken_state == NULL) {
        // init() failed or wasn't called yet
        return;
    }
    
    toktoken_client_settings->next_token_ignore_case = 0;
    toktoken_client_settings->next_token_ignore_whitespace = 0;
    toktoken_client_settings->allow_scientific_notation = 1;
    toktoken_client_settings->allow_leading_dot = 1;
    toktoken_client_settings->allow_high_precision = 1;
    
    toktoken_state->string_literal_enum_value = UINT32_MAX;
    toktoken_state->newline_enum_value = UINT32_MAX;
    toktoken_state->registered_tokens_size = 0;
    toktoken_state->good = 1;
    
    *good = 1;
}

void toktoken_register_newline_enum(
    const uint32_t enum_value,
    uint32_t * good)
{
    *good = 0;
    if (
        toktoken_client_settings == NULL ||
        toktoken_state == NULL ||
        !toktoken_state->good ||
        enum_value == UINT32_MAX)
    {
        return;
    }
    
    toktoken_state->newline_enum_value = enum_value;
    
    *good = 1;
}

void toktoken_register_string_literal_enum(
    const uint32_t enum_value,
    uint32_t * good)
{
    *good = 0;
    if (
        toktoken_client_settings == NULL ||
        toktoken_state == NULL ||
        !toktoken_state->good ||
        enum_value == UINT32_MAX)
    {
        return;
    }
    
    toktoken_state->string_literal_enum_value = enum_value;
    
    *good = 1;
}

void toktoken_register_token(
    const char * ascii_value,
    const uint32_t enum_value,
    uint32_t * good)
{
    *good = 0;
    if (
        toktoken_client_settings == NULL ||
        toktoken_state == NULL ||
        !toktoken_state->good ||
        ascii_value == NULL ||
        ascii_value[0] == '\0')
    {
        return;
    }
    
    // keep the token's data
    if (toktoken_state->registered_tokens_size + 1 >= REGISTERED_TOKENS_CAP) {
        toktoken_state->good = 0;
        return;
    }
    RegisteredToken * new = &toktoken_state->
        registered_tokens[toktoken_state->registered_tokens_size];
    toktoken_state->registered_tokens_size += 1;
    new->ascii_value = toktoken_state->ascii_store + toktoken_state->ascii_store_next_i;
    new->enum_value = enum_value;
    new->requires_trailing_whitespace = !toktoken_client_settings->next_token_ignore_whitespace;
    new->ignores_case =
        toktoken_client_settings->next_token_ignore_case;
    
    if (new->ignores_case) {
        // TODO: implement ignore case tokens
        *good = 0;
        return;
    }
    
    // keep the ascii value in our persistent local store
    uint32_t i = 0;
    while (
        ascii_value[i] != '\0' &&
        toktoken_state->ascii_store_next_i < ASCII_STORE_CAP)
    {
        toktoken_state->ascii_store[toktoken_state->ascii_store_next_i++] = ascii_value[i];
        i += 1;
    }
    if (toktoken_state->ascii_store_next_i + 1 >= ASCII_STORE_CAP) {
        toktoken_state->good = 0;
        return;
    }
    toktoken_state->ascii_store[toktoken_state->ascii_store_next_i++] = '\0';
    
    
    *good = 1;
}

static uint32_t toktoken_strmatch(
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
    
    for (uint32_t i = 0; i < toktoken_state->registered_tokens_size; i++) {
        uint32_t matching_chars =
            toktoken_strmatch(
                input,
                toktoken_state->registered_tokens[i].ascii_value);
        
        if (matching_chars > 0)
        {
            if (
                !toktoken_state->registered_tokens[i].
                    requires_trailing_whitespace ||
                input[matching_chars] == '\0' ||
                input[matching_chars] == '\n' ||
                input[matching_chars] == ' ' ||
                input[matching_chars] == '\t')
            {
                *matching_token_i = i;
                *data_len = matching_chars;
                return;
            } else {
                // matched a token, but it's a substring
            }
        }
    }
    
    return;
}

static void toktoken_set_number_flags(TokToken *token) {
    token->number_value = NULL;
    
    if (
        token->string_value_size == 0 ||
        toktoken_state->numbers_size + 1 >= NUMBERS_CAP)
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
    uint32_t leading_digit_count = 0;
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
        leading_digit_count++;
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
        i < token->string_value_size && (token->string_value[i] == 'e' ||
        token->string_value[i] == 'E'))
    {
        if (!toktoken_client_settings->allow_scientific_notation) {
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
        token->number_value = &toktoken_state->numbers[toktoken_state->numbers_size];
        toktoken_state->numbers_size += 1;
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

void toktoken_run(
    const char * input,
    uint32_t * good)
{
    *good = 0;
    if (
        input == NULL ||
        toktoken_client_settings == NULL ||
        toktoken_state == NULL ||
        !toktoken_state->good ||
        toktoken_state->string_literal_enum_value == UINT32_MAX ||
        toktoken_state->string_literal_enum_value == UINT32_MAX)
    {
        return;
    }
    
    uint32_t i = 0;
    uint32_t matching_token_i;
    uint32_t data_len;
    uint32_t line_number = 1;
    
    while (input[i] != '\0') {
        
        while (
            input[i] == ' ' ||
            input[i] == '\t' ||
            input[i] == '\n')
        {
            if (input[i] == '\n') {
                line_number += 1;
                if (toktoken_state->newline_enum_value != UINT32_MAX) {
                    if (toktoken_state->tokens_size + 1 >= TOKENS_CAP) {
                        *good = 0;
                        toktoken_state->good = 0;
                        return;
                    }
                    toktoken_state->tokens[toktoken_state->tokens_size].enum_value =
                        toktoken_state->newline_enum_value;
                    toktoken_state->tokens[toktoken_state->tokens_size].line_number =
                        line_number;
                    toktoken_state->tokens[toktoken_state->tokens_size].castable_flags = 0;
                    toktoken_state->tokens[toktoken_state->tokens_size].number_value = NULL;
                    toktoken_state->tokens[toktoken_state->tokens_size].string_value = NULL;
                    toktoken_state->tokens[toktoken_state->tokens_size].string_value_size = 0;
                    toktoken_state->tokens_size += 1;
                }
            }
            i++;
        }
        
        if (input[i] == '\0') { continue; }
        
        toktoken_string_match_tokens(
            input + i,
            &matching_token_i,
            &data_len);
        
        TokToken * new = &toktoken_state->tokens[toktoken_state->tokens_size];
        toktoken_state->tokens_size += 1;
        if (toktoken_state->tokens_size + 1 >= TOKENS_CAP) {
            *good = 0;
            toktoken_state->good = 0;
            return;
        }
        new->line_number = line_number;
        
        if (matching_token_i == UINT32_MAX) {
            new->enum_value = toktoken_state->string_literal_enum_value;
            new->string_value =
                toktoken_state->ascii_store + toktoken_state->ascii_store_next_i;
            
            uint32_t j = i;
            new->string_value[0] = input[j];
            j++;
            
            while (
                input[j] != ' ' &&
                input[j] != '\0' &&
                input[j] != '\n')
            {
                new->string_value[j-i] = input[j];
                j++;
            }
            new->string_value[j-i] = '\0';
            
            new->string_value_size = (uint16_t)(j-i);
            toktoken_state->ascii_store_next_i += (j-i) + 1;
            
            i = j;
        } else {
            new->enum_value = toktoken_state->
                registered_tokens[matching_token_i].enum_value;
            new->string_value =
                toktoken_state->registered_tokens[matching_token_i].ascii_value;
            new->string_value_size = (uint16_t)data_len;
            
            i += data_len;
        }
        
        toktoken_set_number_flags(new);
        if (!toktoken_state->good) { return; }
    }
    
    *good = 1;
}

uint32_t toktoken_get_token_count(void) {
    return toktoken_state->tokens_size;
}

TokToken * toktoken_get_token_at(
    const uint32_t token_i)
{
    return &toktoken_state->tokens[token_i];
}

