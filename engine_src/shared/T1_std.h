#ifndef T1_STD_H
#define T1_STD_H

#include <inttypes.h>
#include <stddef.h>

// #define COMMON_IGNORE_ASSERTS
#ifndef COMMON_IGNORE_ASSERTS
#include <assert.h>
#endif

#include "T1_simd.h"

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

// The common_memset_char signature matches libc's,
// the others are my preference
void * T1_std_memset(
    void * input,
    int value,
    size_t size_bytes);
void T1_std_memset_i16(
    void * input,
    int16_t value,
    unsigned int size_bytes);
void T1_std_memset_i32(
    void * input,
    int32_t value,
    unsigned int size_bytes);
void T1_std_memset_f32(
    void * input,
    float value,
    unsigned int size_bytes);
void * T1_std_memcpy(
    void * dest,
    const void * src,
    size_t n_bytes);

// float common__sqrtf(const float in);

int   T1_std_mini(const int x, const int y);
int   T1_std_maxi(const int x, const int y);
float T1_std_minf(const float x, const float y);
float T1_std_maxf(const float x, const float y);


#ifndef COMMON_IGNORE_ASSERTS
#define T1_std_strcat_cap(recip, recipsize, to_append) T1_std_internal_strcat_cap(recip,recipsize,to_append);
#else
#define T1_std_strcat_cap(recip, recipsize, to_append) T1_std_internal_strcat_cap(recip, UINT32_MAX, to_append);
#endif
void
T1_std_internal_strcat_cap(
    char * recipient,
    const uint32_t recipient_size,
    const char * to_append);

void
T1_std_strcat(
    char * recipient,
    const char * to_append);

void T1_std_strcat_char_cap(
    char * recipient,
    char to_append);

#ifndef COMMON_IGNORE_ASSERTS
#define T1_std_strcat_int_cap(recip, recipsize, to_append) T1_std_internal_strcat_int_cap(recip, recipsize, to_append);
void
T1_std_internal_strcat_int_cap(
    char * recipient,
    const uint32_t recipient_size,
    const int32_t to_append);
#else
#define T1_std_strcat_int_cap(recip, recipsize, to_append) T1_std_internal_strcat_int_cap(recip, to_append);
void
T1_std_internal_strcat_int_cap(
    char * recipient,
    const int32_t to_append);
#endif

#ifndef COMMON_IGNORE_ASSERTS
#define T1_std_strcat_uint_cap(recip, recipsize, to_append) T1_std_internal_strcat_uint_cap(recip, recipsize, to_append);
void
T1_std_internal_strcat_uint_cap(
    char * recipient,
    const uint32_t recipient_size,
    const uint32_t to_append);
#else
#define T1_std_strcat_uint_cap(recip, recipsize, to_append) T1_std_internal_strcat_uint_cap(recip, to_append);
void
T1_std_internal_strcat_uint_cap(
    char * recipient,
    const uint32_t to_append);
#endif

#ifndef COMMON_IGNORE_ASSERTS
#define T1_std_strcat_float_cap(recip, recipsize, to_append) T1_std_internal_strcat_float_cap(recip, recipsize, to_append);
#else
#define T1_std_strcat_float_cap(recip, recipsize, to_append) T1_std_internal_strcat_float_cap(recip, UINT32_MAX, to_append);
#endif
void
T1_std_internal_strcat_float_cap(
    char * recipient,
    const uint32_t recipient_size,
    const float to_append);

#ifndef COMMON_IGNORE_ASSERTS
#define T1_std_strcpy_cap(recip, recipsize, to_append) T1_std_internal_strcpy_cap(recip, recipsize, to_append);
void T1_std_internal_strcpy_cap(
    char * recipient,
    const uint32_t recipient_size,
    const char * origin);
#else
#define T1_std_strcpy_cap(recip, recipsize, to_append) T1_std_internal_strcpy_cap(recip, to_append);
void T1_std_internal_strcpy_cap(
    char * recipient,
    const char * origin);
#endif

void
T1_std_copy_strings(
    char * recipient,
    const uint32_t recipient_size,
    const char * origin,
    const uint32_t origin_size);

size_t
T1_std_strlen(
    const char * null_terminated_string);

bool32_t T1_std_string_starts_with(
    const char * str_to_check,
    const char * start);

bool32_t T1_std_string_ends_with(
    const char * str_to_check,
    const char * ending);

void T1_std_strsub(
    char * in,
    const char * to_match,
    const char * replacement);

bool32_t
T1_std_are_equal_strings(
    const char * str1,
    const char * str2);

bool32_t
T1_std_are_equal_until_nullterminator(
    const char * str1,
    const char * str2);

bool32_t
T1_std_are_equal_strings_of_length(
    const char * str1,
    const char * str2,
    const uint64_t length);

void
T1_std_int_to_string(
    const int32_t input,
    char * recipient);

void
T1_std_uint_to_string(
    const uint32_t input,
    char * recipient);

void
T1_std_float_to_string(
    const float input,
    char * recipient,
    const uint32_t recipient_size);

int32_t
T1_std_string_to_int32_validate(
    const char * input,
    bool32_t * good);

int32_t
T1_std_string_to_int32(const char * input);

uint32_t
T1_std_string_to_uint32_validate(
    const char * input,
    bool32_t * good);

uint32_t
T1_std_string_to_uint32(const char * input);

float
T1_std_string_to_float_validate(
    const char * input,
    bool32_t * good);

float
T1_std_string_to_float(const char * input);

void
T1_std_float_to_string(
    const float input,
    char * recipient,
    const uint32_t recipient_size);

#ifdef __cplusplus
}
#endif

#endif // T1_STD_H
