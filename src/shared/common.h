#ifndef COMMON_H
#define COMMON_H

#include <inttypes.h>
#include "assert.h"

#ifndef __cplusplus
#define true 1
#define false 0
#endif

#ifndef FUINT64
#define FUINT64 "%llu"
#endif

#define bool32_t uint32_t

#define local_only static

void concat_strings(
    const char * string_1,
    const char * string_2,
    char * output,
    const uint64_t output_size);

void copy_strings(
    char * recipient,
    const uint32_t recipient_size,
    const char * origin,
    const uint32_t origin_size);

uint32_t get_string_length(   
    const char * null_terminated_string);

bool32_t are_equal_strings(
    const char * str1,
    const char * str2,
    const uint64_t length);

#endif
