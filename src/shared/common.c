#include "common.h"

bool32_t are_equal_strings(
    char * str1,
    char * str2,
    uint64_t len)
{
    for (uint64_t i = 0; i < len; i++) {
        if (str1[i] != str2[i]) {
            return false;
        }
    }
    
    return true;
}

