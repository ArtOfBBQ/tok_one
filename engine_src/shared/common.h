#ifndef COMMON_H
#define COMMON_H

#include <inttypes.h>
#include <stddef.h>

// #define COMMON_IGNORE_ASSERTS
#ifndef COMMON_IGNORE_ASSERTS
#include <assert.h>
#endif

#include "simd.h"

#ifndef __cplusplus
#define true 1
#define false 0
#endif

#ifndef NULL
#define NULL 0
#endif

#define bool8_t  uint8_t
#define bool32_t uint32_t

#ifdef __cplusplus
extern "C" {
#endif

#define FLOAT32_MAX 2147483647
#define FLOAT32_MIN 1.1754943508222875e-38F

#define common_abs(a) ((((a) > 0)*(a))+(((a) < 0)*-(a)))
#define common_fabs(a) ((((a) > 0.0f)*(a))+(((a) < 0.0f)*-(a)))

void common_memset_char(
    void * input,
    char value,
    unsigned int size_bytes);
void common_memset_int16(
    void * input,
    int16_t value,
    unsigned int size_bytes);
void common_memset_int32(
    void * input,
    int32_t value,
    unsigned int size_bytes);
void common_memset_float(
    void * input,
    float value,
    unsigned int size_bytes);
void * common_memcpy(
    void * dest,
    const void * src,
    size_t n_bytes);

// float common__sqrtf(const float in);

int   common_mini(const int x, const int y);
int   common_maxi(const int x, const int y);
float common_minf(const float x, const float y);
float common_maxf(const float x, const float y);


#ifndef COMMON_IGNORE_ASSERTS
#define common_strcat_capped(recip, recipsize, to_append) common_internal_strcat_capped(recip,recipsize,to_append);
void
common_internal_strcat_capped(
    char * recipient,
    const uint32_t recipient_size,
    const char * to_append);
#else
#define common_strcat_capped(recip, recipsize, to_append) common_internal_strcat_capped(recip,to_append);
void
common_internal_strcat_capped(
    char * recipient,
    const char * to_append);
#endif

#ifndef COMMON_IGNORE_ASSERTS
#define common_strcat_int_capped(recip, recipsize, to_append) common_internal_strcat_int_capped(recip, recipsize, to_append);
void
common_internal_strcat_int_capped(
    char * recipient,
    const uint32_t recipient_size,
    const int32_t to_append);
#else
#define common_strcat_int_capped(recip, recipsize, to_append) common_internal_strcat_int_capped(recip, to_append);
void
common_internal_strcat_int_capped(
    char * recipient,
    const int32_t to_append);
#endif

#ifndef COMMON_IGNORE_ASSERTS
#define common_strcat_uint_capped(recip, recipsize, to_append) common_internal_strcat_uint_capped(recip, recipsize, to_append);
void
common_internal_strcat_uint_capped(
    char * recipient,
    const uint32_t recipient_size,
    const uint32_t to_append);
#else
#define common_strcat_uint_capped(recip, recipsize, to_append) common_internal_strcat_uint_capped(recip, to_append);
void
common_internal_strcat_uint_capped(
    char * recipient,
    const uint32_t to_append);
#endif

#ifndef COMMON_IGNORE_ASSERTS
#define common_strcat_float_capped(recip, recipsize, to_append) common_internal_strcat_float_capped(recip, recipsize, to_append);
void
common_internal_strcat_float_capped(
    char * recipient,
    const uint32_t recipient_size,
    const float to_append);
#else
#define common_strcat_float_capped(recip, recipsize, to_append) common_internal_strcat_float_capped(recip, to_append);
void
common_internal_strcat_float_capped(
    char * recipient,
    const float to_append);
#endif

#ifndef COMMON_IGNORE_ASSERTS
#define common_strcpy_capped(recip, recipsize, to_append) common_internal_strcpy_capped(recip, recipsize, to_append);
void common_internal_strcpy_capped(
    char * recipient,
    const uint32_t recipient_size,
    const char * origin);
#else
#define common_strcpy_capped(recip, recipsize, to_append) common_internal_strcpy_capped(recip, to_append);
void common_internal_strcpy_capped(
    char * recipient,
    const char * origin);
#endif

void
common_copy_strings(
    char * recipient,
    const uint32_t recipient_size,
    const char * origin,
    const uint32_t origin_size);

uint32_t
common_get_string_length(   
    const char * null_terminated_string);

bool32_t common_string_starts_with(
    const char * str_to_check,
    const char * start);

bool32_t common_string_ends_with(
    const char * str_to_check,
    const char * ending);

bool32_t common_are_equal_strings(
    const char * str1,
    const char * str2);

bool32_t
common_are_equal_until_nullterminator(
    const char * str1,
    const char * str2);

bool32_t common_are_equal_strings_of_length(
    const char * str1,
    const char * str2,
    const uint64_t length);

void
common_int_to_string(
    const int32_t input,
    char * recipient);

void
common_uint_to_string(
    const uint32_t input,
    char * recipient);

void
common_float_to_string(
    const float input,
    char * recipient,
    const uint32_t recipient_size);

int32_t
common_string_to_int32_validate(
    const char * input,
    bool32_t * good);

int32_t
common_string_to_int32(const char * input);

uint32_t
common_string_to_uint32_validate(
    const char * input,
    bool32_t * good);

uint32_t
common_string_to_uint32(const char * input);

float
common_string_to_float_validate(
    const char * input,
    bool32_t * good);

float
common_string_to_float(const char * input);

void
common_float_to_string(
    const float input,
    char * recipient,
    const uint32_t recipient_size);

#ifdef __cplusplus
}
#endif

#endif // COMMON_H
