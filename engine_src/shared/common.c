#include "common.h"

float tok_fminf(const float x, const float y)
{
    return ((x <= y) * x) + ((y < x) * y);
}

float tok_fmaxf(const float x, const float y)
{
    return ((x >  y) * x) + ((y <= x) * y);
}

int tok_imini(const int x, const int y)
{
    return ((x <= y) * x) + ((y < x) * y);
}

int tok_imaxi(const int x, const int y)
{
    return ((x <= y) * y) + ((x > y) * x);
}

void
strcat_capped(
    char * recipient,
    const uint32_t recipient_size,
    const char * to_append)
{
    uint32_t i = 0;
    while (recipient[i] != '\0') {
        #ifndef COMMON_IGNORE_ASSERTS
        assert(i < recipient_size);
        #endif
        i++;
    }
    
    uint32_t j = 0;
    while (to_append[j] != '\0') {
        #ifndef COMMON_IGNORE_ASSERTS
        assert(i < recipient_size - 1);
        #endif
        recipient[i++] = to_append[j++];
    }
    
    recipient[i] = '\0';
}

void
strcat_int_capped(
    char * recipient,
    const uint32_t recipient_size,
    const int32_t to_append)
{
    uint32_t i = 0;
    while (recipient[i] != '\0') {
        #ifndef COMMON_IGNORE_ASSERTS
        assert(i < recipient_size);
        #endif
        i++;
    }
    
    int_to_string(to_append, recipient + i, recipient_size - i);
}

void
strcat_uint_capped(
    char * recipient,
    const uint32_t recipient_size,
    const uint32_t to_append)
{
    uint32_t i = 0;
    while (recipient[i] != '\0') {
        #ifndef COMMON_IGNORE_ASSERTS
        assert(i < recipient_size);
        #endif
        i++;
    }
    
    uint_to_string(to_append, recipient + i, recipient_size - i);
}

void
strcat_float_capped(
    char * recipient,
    const uint32_t recipient_size,
    const float to_append)
{
    float positive_append = to_append >= 0.0f ? to_append : -1.0f * to_append;
    
    uint32_t before_comma = (uint32_t)positive_append;
    uint32_t after_comma =
        ((uint32_t)(positive_append * 1000) - (before_comma * 1000));
    if (to_append < 0.0f) {
        strcat_capped(recipient, recipient_size, "-");
    }
    strcat_uint_capped(recipient, recipient_size, before_comma);
    strcat_capped(recipient, recipient_size, ".");
    strcat_uint_capped(recipient, recipient_size, after_comma);
    strcat_capped(recipient, recipient_size, "f");
}

void strcpy_capped(
    char * recipient,
    const uint32_t recipient_size,
    const char * origin)
{
    uint32_t i = 0;
    while (
        origin[i] != '\0'
        && i < recipient_size - 1)
    {
        #ifndef COMMON_IGNORE_ASSERTS
        assert(i < recipient_size - 1);
        #endif
        recipient[i] = origin[i];
        i += 1;
    }
    
    #ifndef COMMON_IGNORE_ASSERTS
    assert(i < recipient_size);
    #endif
    recipient[i] = '\0';
}

void
copy_strings(
    char * recipient,
    const uint32_t recipient_size,
    const char * origin,
    const uint32_t origin_size)
{
    uint32_t i = 0;
    for (; i < origin_size; i++) {
        recipient[i] = origin[i];
    }
    
    for (; i < recipient_size; i++) {
        recipient[i] = '\0';
    }
}

uint32_t
get_string_length(   
    const char * null_terminated_string)
{
    uint32_t return_value = 0;
    while (
        null_terminated_string[return_value] != '\0')
    {
        return_value++;
    }
    
    return return_value;
}

bool32_t
are_equal_strings(
    const char * str1,
    const char * str2)
{
    if (str1 == NULL || str2 == NULL) {
        return false;
    }
    
    if (
        str1[0] == '\0'
        && str2[0] != '\0')
    {
        return false;
    }
    
    uint32_t i = 0;
    while (str1[i] != '\0') {
        if (str1[i] != str2[i]) {
            return false;
        }
        i++;
    }
    
    if (str1[i] != '\0' || str2[i] != '\0') {
        return false;
    }

    return true;
}

bool32_t
are_equal_until_nullterminator(
    const char * str1,
    const char * str2)
{
    if (str1 == NULL || str2 == NULL) {
        return false;
    }
    
    uint32_t i = 0;
    while (
        str1[i] != '\0' &&
        str2[i] != '\0')
    {
        if (str1[i] != str2[i]) {
            return false;
        }
        i++;
    }
    
    return true;
}

bool32_t
are_equal_strings_of_length(
    const char * str1,
    const char * str2,
    const uint64_t len)
{
    for (uint64_t i = 0; i < len; i++) {
        if (str1[i] != str2[i]) {
            return false;
        }
    }
    
    return true;
}

void float_to_string(
    const float input,
    char * recipient,
    const uint32_t recipient_size)
{
    (void)recipient_size;
    
    float temp_above_decimal = (float)(int32_t)input;
    int32_t below_decimal =
        (int32_t)((input - temp_above_decimal) * 100000);
    int32_t above_decimal = (int32_t)temp_above_decimal;
    
    int_to_string(
        /* const int32_t input: */
            above_decimal,
        /* char * recipient: */
            recipient,
        /* const uint32_t recipient_size: */
            1000);
    
    uint32_t count = 0;
    while (recipient[count] != '\0') {
        count++;
    }
    
    recipient[count] = '.';
    count++;
    
    // count the number of leading 0's after the comma
    float mod = 10.0f;
    while (
        (below_decimal > 0) &&
        (uint32_t)((input - temp_above_decimal) * mod) < 1)
    {
        mod *= 10.0f;
        recipient[count++] = '0';
    }
    
    int_to_string(
        /* const int32_t input: */
            below_decimal,
        /* char * recipient: */
            recipient + count,
        /* const uint32_t recipient_size: */
            1000);
}

void int_to_string(
    const int32_t input,
    char * recipient,
    const uint32_t recipient_size)
{
    if (input < 0) {
        recipient[0] = '-';
        int32_t positive_to_append = (input + (input == INT32_MIN)) * -1;
        uint_to_string(
            /* input: */ (uint32_t)positive_to_append,
            /* recipient: */ recipient + 1,
            /* recipient_size: */ recipient_size - 1); 
    } else {
        uint_to_string(
            /* input: */ (uint32_t)input,
            /* recipient: */ recipient,
            /* recipient_size: */ recipient_size);
    }
}

void
uint_to_string(
    const uint32_t input,
    char * recipient,
    const uint32_t recipient_size)
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
        assert(i < recipient_size);
        
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

int32_t
string_to_int32_validate(
    const char * input,
    bool32_t * good)
{
    if (input[0] == '\0') {
        #ifndef COMMON_SILENCE
        printf("ERROR : string_to_int32 with input_size < 1\n");
        #endif
        *good = false;
        return 0;
    }
    
    // the maximum int32_t is 2147483647
    
    if (input[0] == '-') {
        uint32_t temp = string_to_uint32(
            /* input: */
                input + 1);
        
        if (temp > 2147483646) {
            #ifndef COMMON_SILENCE
            printf(
                "ERROR : string_to_int32 below than INT_MIN\n");
            #endif
            *good = false;
            return 0;
        }
        
        *good = true;
        
        return (int32_t)temp * -1;
    }
    
    uint32_t unsigned_return = string_to_uint32_validate(input, good);
    
    if (!*good) {
        return 0;
    }
    
    return (int32_t)unsigned_return;
}

int32_t
string_to_int32(const char * input)
{
    bool32_t result_good = false;
    int32_t result = string_to_int32_validate(
        input,
        &result_good);
    #ifndef COMMON_IGNORE_ASSERTS
    assert(result_good);
    #endif
    return result;
}

uint32_t
string_to_uint32_validate(
    const char * input,
    bool32_t * good)
{
    if (input[0] < '0' || input[0] > '9') {
        *good = false;
        return 0;
    } else {
        *good = true;
    }
    
    uint32_t return_value = 0;
    
    // the maximum uint32_t is 4294967295
    // so the decimal should   1000000000
    
    uint32_t decimal = 1;
    int32_t i = 0;
    while (input[i] >= '0' && input[i] <= '9') {
        i++;
    }
    i--;
    
    while (i >= 0)
    {
        if (input[i] < '0' || input[i] > '9') {
            return return_value;
        }
        
        uint32_t current_digit = (uint32_t)(input[i] - '0');
        return_value += decimal * current_digit;
        decimal *= 10;
        
        if (decimal > 1000000000) {
            #ifndef COMMON_SILENCE
            printf(
                "ERROR: overflowing uint32\n");
            #endif
            *good = false;
            return return_value;
        }
        
        i--;
    }
    
    return return_value;
}

uint32_t
string_to_uint32(
    const char * input)
{
    bool32_t result_good = false;
    uint32_t result = string_to_uint32_validate(
        input,
        &result_good);
    #ifndef COMMON_IGNORE_ASSERTS
    assert(result_good);
    #endif
    return result;
}

float
string_to_float_validate(
    const char * input,
    bool32_t * good)
{
    if (input[0] == '\0') {
        #ifndef COMMON_SILENCE
        printf("ERROR: string_to_float but input[0] is nullterminator\n");
        #endif
        *good = false;
        return 0;
    }
    
    float return_value = 0;
    
    uint32_t i = 0;
    bool32_t found_num = false;
    bool32_t used_dot = false;
    char first_part[20];
    uint32_t first_part_size = 0;
    char second_part[20];
    uint32_t second_part_size = 0;
   
    bool32_t found_exponent = false;
    int32_t exponent_modifier = 1; 
    int32_t exponent = 0;
    
    if (input[0] == '-') {
        i++;
    }
    
    while (
        input[i] != '\0' &&
        input[i] != ' ' &&
        input[i] != '\n' &&
        input[i] != '\r')
    {
        if (!used_dot && found_num && input[i] == '.') {
            i++;
            used_dot = true;
            continue;
        }
        
        if (input[i] >= '0' && input[i] <= '9') {
            found_num = true;
            if (found_exponent) {
                if (exponent == 0) {
                    exponent = input[i] - '0';
                } else {
                    // TODO: exponents of 10 or higher
                    exponent = 10;
                }
            } else if (!used_dot) {
                first_part[first_part_size++] = input[i];
            } else {
                second_part[second_part_size++] = input[i];
            }
        } else if (
            input[i] == 'e' || input[i] == 'E')
        {
            if (found_exponent) {
                *good = false;
                return return_value;
            }
            found_exponent = true;
            if (input[i+1] == '-') {
                exponent_modifier = -1;
                i++;
            } else if (input[i+1] == '+') {
                i++;
            } else {
                *good = false;
                return return_value;
            }
        } else {
            *good = false;
            return return_value;
        }
        
        i++;
    }
    
    first_part[first_part_size] = '\0';
    
    bool32_t first_part_valid = false;
    int first_part_int = string_to_int32_validate(
        /* const char input: */ first_part,
        &first_part_valid);
    if (!first_part_valid) {
        *good = false;
        return return_value;
    }

    if (second_part_size > 0) {
    
        if (second_part_size > 6) { second_part_size = 6; }
        
        second_part[second_part_size] = '\0';
        bool32_t second_part_valid = false;
        int second_part_int = string_to_int32_validate(
            /* const char input: */ second_part,
            &second_part_valid);
        if (!second_part_valid) {
            *good = false;
            return return_value;
        }
        
        // throw away 0's at the end after the comma, they're useless
        while (second_part_int % 10 == 0 && second_part_size > 0) {
            second_part_int /= 10;
            second_part_size -= 1;
        }
	
        return_value += (float)first_part_int;
        float divisor = 1;
        for (uint32_t _ = 0; _ < second_part_size; _++) {
            divisor *= 10;
        }
        return_value += ((float)second_part_int / divisor);
        
        if (input[0] == '-') {
            return_value *= -1;
        }
    }
    
    // apply the 'scientific notation' exponent
    // e-4 means we want to multiply by -1 * (10^4)
    float scinot_modifier = 1;
    for (int32_t _ = 0; _ < exponent; _++) {
        scinot_modifier *= (exponent_modifier < 0 ? 0.1f : 10.0f);
    }
    return_value *= scinot_modifier;
    
    *good = true;
    return return_value;
}

float
string_to_float(
    const char * input)
{
    bool32_t result_good = false;
    float result = string_to_float_validate(
        input,
        &result_good);
    
    #ifndef COMMON_IGNORE_ASSERTS
    #ifndef COMMON_SILENCE
    if (!result_good) {
        printf(
            "string_to_float failed to parse input: %c%c%c%c%c%c%c\n",
            input[0],
            input[1],
            input[2],
            input[3],
            input[4],
            input[5],
            input[6]);
    }
    #endif
    assert(result_good);
    #endif
    return result;
}
