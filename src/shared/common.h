#ifndef COMMON_H
#define COMMON_H

#include <inttypes.h>

#ifndef __cplusplus
#define true 1
#define false 0
#endif

#ifndef NULL
#define NULL 0
#endif

#define COMMON_SILENCE
#ifndef COMMON_SILENCE
#include <stdio.h>
#endif

#define COMMON_IGNORE_ASSERTS
#ifndef COMMON_IGNORE_ASSERTS
#include <assert.h>
#endif

#define bool32_t uint32_t

#define local_only static

void __attribute__((no_instrument_function))
concat_strings(
    const char * string_1,
    const char * string_2,
    char * output,
    const uint32_t output_size);

void __attribute__((no_instrument_function))
copy_0term_string_to(
    char * recipient,
    const uint32_t recipient_size,
    const char * origin);

void __attribute__((no_instrument_function))
copy_strings(
    char * recipient,
    const uint32_t recipient_size,
    const char * origin,
    const uint32_t origin_size);

uint32_t __attribute__((no_instrument_function))
get_string_length(   
    const char * null_terminated_string);

bool32_t are_equal_strings(
    const char * str1,
    const char * str2);

bool32_t are_equal_strings_of_length(
    const char * str1,
    const char * str2,
    const uint64_t length);

void __attribute__((no_instrument_function))
int_to_string(
    const int32_t input,
    char * recipient,
    const uint32_t recipient_size);

void __attribute__((no_instrument_function))
uint_to_string(
    const uint32_t input,
    char * recipient,
    const uint32_t recipient_size);

int32_t
string_to_int32_validate(
    const char * input,
    const uint32_t input_size,
    bool32_t * good);

int32_t
string_to_int32(
    const char * input,
    const uint32_t input_size);

uint32_t
string_to_uint32_validate(
    const char * input,
    const uint32_t input_size,
    bool32_t * good);

uint32_t
string_to_uint32(
    const char * input,
    const uint32_t input_size);

float
string_to_float_validate(
    const char * input,
    const uint32_t input_size,
    bool32_t * good);

float
string_to_float(
    const char * input,
    const uint32_t input_size);

void __attribute__((no_instrument_function))
float_to_string(
    const float input,
    char * recipient,
    const uint32_t recipient_size);

#endif

