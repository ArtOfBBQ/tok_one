#include "common.h"

char * concat_strings(char * str1, char * str2)
{
    char buffer[5000];

    uint32_t buffer_i = 0;
    while (str1[0] != '\0') {
        buffer[buffer_i] = str1[0];
        buffer_i += 1;
        str1++;
        assert(buffer_i < 5000);
    }
    
    while (str2[0] != '\0') {
        buffer[buffer_i] = str2[0];
        buffer_i += 1;
        str2++;
        assert(buffer_i < 5000);
    }
    
    char return_value[buffer_i + 1];
    for (
        uint32_t i = 0;
        i < buffer_i;
        i++)
    {
        return_value[i] = buffer[i];
    }
    return_value[buffer_i] = '\0';
    
    return &return_value;
}

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

