#include "common.h"

void concat_strings(
    const char * string_1,
    const char * string_2,
    char * output,
    const uint64_t output_size)
{
    char * str1_at = (char *)string_1;
    char * str2_at = (char *)string_2;
    char * output_at = output;
    
    uint32_t chars_used = 0;
    
    while (str1_at[0] != '\0') {
        *output_at++ = *str1_at++;
        chars_used++;
        assert(chars_used < output_size);
    }
    
    while (str2_at[0] != '\0') {
        *output_at++ = *str2_at++;
        chars_used++;
        assert(chars_used < output_size);
    }
    
    *output_at = '\0';
    chars_used++;
    assert(chars_used <= output_size);
}

void copy_strings(
    char * recipient,
    const uint32_t recipient_size,
    const char * origin,
    const uint32_t origin_size)
{
    uint32_t i = 0;
    for (; i < origin_size; i++) {
        assert(recipient_size > i);

        recipient[i] = origin[i];
    }
    
    for (; i < recipient_size; i++) {
        recipient[i] = '\0';
    }
}

uint32_t get_string_length(   
    const char * null_terminated_string)
{
    uint32_t return_value = 0;
    while (
        null_terminated_string[return_value] != '\0'
)
    {
        assert(return_value < 2000000000);
        return_value++;
    }
    
    return return_value;
}

bool32_t are_equal_strings(
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
