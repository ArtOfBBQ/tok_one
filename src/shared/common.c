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
    
    while (str1_at[0] != '\0') {
        *output_at++ = *str1_at++;
        assert(output_at - output < output_size);
    }
    
    while (str2_at[0] != '\0') {
        *output_at++ = *str2_at++;
        assert(output_at - output < output_size);
    }
    
    *output_at = '\0';
    assert(output_at - output < output_size);
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

