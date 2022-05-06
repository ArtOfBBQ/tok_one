#ifndef COMMON_H
#define COMMON_H

#include <inttypes.h>
#include "assert.h"

#ifndef __cplusplus
#define true 1
#define false 0
#endif

#define bool32_t uint32_t

#define local_only static

void concat_strings(
    const char * string_1,
    const char * string_2,
    char * output,
    const uint64_t output_size);

bool32_t are_equal_strings(
    const char * str1,
    const char * str2,
    const uint64_t length);

#endif

