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

void memset_char(
    void * input,
    char value,
    unsigned int size_bytes);
void memset_int16(
    void * input,
    int16_t value,
    unsigned int size_bytes);
void memset_float(
    void * input,
    float value,
    unsigned int size_bytes);
void * tok_memcpy(
    void * dest,
    const void * src,
    size_t n_bytes);

// float tok_sqrtf(const float in);

int   tok_mini(const int x, const int y);
int   tok_maxi(const int x, const int y);
float tok_minf(const float x, const float y);
float tok_maxf(const float x, const float y);


#ifndef COMMON_IGNORE_ASSERTS
#define strcat_capped(recip, recipsize, to_append) internal_strcat_capped(recip,recipsize,to_append);
void
internal_strcat_capped(
    char * recipient,
    const uint32_t recipient_size,
    const char * to_append);
#else
#define strcat_capped(recip, recipsize, to_append) internal_strcat_capped(recip,to_append);
void
internal_strcat_capped(
    char * recipient,
    const char * to_append);
#endif

#ifndef COMMON_IGNORE_ASSERTS
#define strcat_int_capped(recip, recipsize, to_append) internal_strcat_int_capped(recip, recipsize, to_append);
void
internal_strcat_int_capped(
    char * recipient,
    const uint32_t recipient_size,
    const int32_t to_append);
#else
#define strcat_int_capped(recip, recipsize, to_append) internal_strcat_int_capped(recip, to_append);
void
internal_strcat_int_capped(
    char * recipient,
    const int32_t to_append);
#endif

#ifndef COMMON_IGNORE_ASSERTS
#define strcat_uint_capped(recip, recipsize, to_append) internal_strcat_uint_capped(recip, recipsize, to_append);
void
internal_strcat_uint_capped(
    char * recipient,
    const uint32_t recipient_size,
    const uint32_t to_append);
#else
#define strcat_uint_capped(recip, recipsize, to_append) internal_strcat_uint_capped(recip, to_append);
void
internal_strcat_uint_capped(
    char * recipient,
    const uint32_t to_append);
#endif

#ifndef COMMON_IGNORE_ASSERTS
#define strcat_float_capped(recip, recipsize, to_append) internal_strcat_float_capped(recip, recipsize, to_append);
void
internal_strcat_float_capped(
    char * recipient,
    const uint32_t recipient_size,
    const float to_append);
#else
#define strcat_float_capped(recip, recipsize, to_append) internal_strcat_float_capped(recip, to_append);
void
internal_strcat_float_capped(
    char * recipient,
    const float to_append);
#endif

#ifndef COMMON_IGNORE_ASSERTS
#define strcpy_capped(recip, recipsize, to_append) internal_strcpy_capped(recip, recipsize, to_append);
void internal_strcpy_capped(
    char * recipient,
    const uint32_t recipient_size,
    const char * origin);
#else
#define strcpy_capped(recip, recipsize, to_append) internal_strcpy_capped(recip, to_append);
void internal_strcpy_capped(
    char * recipient,
    const char * origin);
#endif

void
copy_strings(
    char * recipient,
    const uint32_t recipient_size,
    const char * origin,
    const uint32_t origin_size);

uint32_t
get_string_length(   
    const char * null_terminated_string);

bool32_t are_equal_strings(
    const char * str1,
    const char * str2);

bool32_t
are_equal_until_nullterminator(
    const char * str1,
    const char * str2);

bool32_t are_equal_strings_of_length(
    const char * str1,
    const char * str2,
    const uint64_t length);

void
int_to_string(
    const int32_t input,
    char * recipient);

void
uint_to_string(
    const uint32_t input,
    char * recipient);

void
float_to_string(
    const float input,
    char * recipient,
    const uint32_t recipient_size);

int32_t
string_to_int32_validate(
    const char * input,
    bool32_t * good);

int32_t
string_to_int32(const char * input);

uint32_t
string_to_uint32_validate(
    const char * input,
    bool32_t * good);

uint32_t
string_to_uint32(const char * input);

float
string_to_float_validate(
    const char * input,
    bool32_t * good);

float
string_to_float(const char * input);

void
float_to_string(
    const float input,
    char * recipient,
    const uint32_t recipient_size);

#ifdef __cplusplus
}
#endif

#endif // COMMON_H
