#include "common.h"

void __attribute__((no_instrument_function))
concat_strings(
    const char * string_1,
    const char * string_2,
    char * output,
    const uint32_t output_size)
{
    char * str1_at = (char *)string_1;
    char * str2_at = (char *)string_2;
    char * output_at = output;
    
    uint32_t chars_used = 0;
    
    while (str1_at[0] != '\0') {
        *output_at++ = *str1_at++;
        chars_used++;
    }
    
    while (str2_at[0] != '\0') {
        *output_at++ = *str2_at++;
        chars_used++;
    }
    
    *output_at = '\0';
    chars_used++;
}

void __attribute__((no_instrument_function))
copy_strings(
    char * recipient,
    const uint32_t recipient_size,
    const char * origin)
{
    uint32_t i = 0;
    while (
        origin[i] != '\0'
        && i < recipient_size - 1)
    {
        assert(i < recipient_size - 1);
        recipient[i] = origin[i];
        i++;
    }
    assert(i < recipient_size);
    recipient[i] = '\0';
}

void __attribute__((no_instrument_function))
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

uint32_t __attribute__((no_instrument_function))
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
    float temp_above_decimal =
        (float)(int32_t)input;
    int32_t below_decimal =
        (int32_t)((input - temp_above_decimal) * 10000);
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
        int32_t positive_to_append = input * -1;
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

void __attribute__((no_instrument_function))
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
    uint32_t input_div_dec = input / decimal;
    while (input_div_dec > 0)
    {
        uint64_t isolated_num =
            input % (decimal * 10);
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

int32_t
string_to_int32(
    const char * input,
    const uint32_t input_size,
    bool32_t * good)
{
    if (input_size < 1) {
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
                input + 1,
            /* input_size: */
                input_size - 1);
        
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
    
    uint32_t unsigned_return =
        string_to_uint32(
            /* input: */
                input,
            /* input_size: */
                input_size);
    
    if (unsigned_return > 2147483647) {
        #ifndef COMMON_SILENCE
        printf("ERROR : string_to_int32 exceeded INT_MAX\n");
        #endif
        *good = false;
        return 0;
    }
    
    *good = true; 
    return (int32_t)unsigned_return;
}

int32_t
string_to_int32(
    const char * input,
    const uint32_t input_size)
{
    bool32_t result_good = false;
    int32_t result = string_to_int32(
        input,
        input_size,
        &result_good);
    assert(result_good);
    return result;
}

uint32_t
string_to_uint32(
    const char * input,
    const uint32_t input_size,
    bool32_t * good)
{
    if (input_size < 1) {
        #ifndef COMMON_SILENCE
        printf("ERROR : string_to_uint32 with input_size < 1\n");
        #endif
        *good = false;
        return 0;
    }
    
    if (input[0] == '\0') {
        #ifndef COMMON_SILENCE
        printf(
            "ERR: string_to_uint32 but input[0] is nullterminator\n");
        #endif
        *good = false;
        return 0;
    }
    
    if (input_size >= 2147483647) {
        #ifndef COMMON_SILENCE
        printf(
            "ERROR: string_to_uint32 has input > signed int max");
        #endif
        *good = false;
        return 0;
    }
    
    uint32_t return_value = 0;
    
    // the maximum uint32_t is 4294967295
    // so the decimal should   1000000000
    
    uint32_t decimal = 1; 
    for (
        int32_t i = (int32_t)input_size - 1;
        i >= 0;
        i--)
    {
        if (
            input[i] < '0'
            || input[i] > '9')
        {
            *good = false;
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
    }
    
    *good = true;
    return return_value;
}

uint32_t
string_to_uint32(
    const char * input,
    const uint32_t input_size)
{
    bool32_t result_good = false;
    uint32_t result = string_to_uint32(
        input,
        input_size,
        &result_good);
    assert(result_good);
    return result;
}
