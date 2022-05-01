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

char * concat_strings(
    char * str1,
    char * str2);

bool32_t are_equal_strings(
    char * str1,
    char * str2,
    uint64_t len);

#endif

