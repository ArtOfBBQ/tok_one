#ifndef T1_STD_H
#define T1_STD_H

#include "T1_stdint.h"

#ifndef __cplusplus
#define true 1
#define false 0
#endif

#ifndef NULL
#define NULL 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define T1_F32_MAX 3.402823466e+38f
#define T1_F32_MIN -T1_F32_MAX

#define T1_std_abs(a) ((((a) > 0)*(a))+(((a) < 0)*-(a)))
#define T1_std_fabs(a) ((((a) > 0.0f)*(a))+(((a) < 0.0f)*-(a)))

void *
T1_std_memset(
    void * input,
    s32 value,
    u64 size_bytes);

void
T1_std_memset_i16(
    void * input,
    s16 value,
    u32 size_bytes);

void
T1_std_memset_s32(
    void * input,
    s32 value,
    u32 size_bytes);

void
T1_std_memset_f32(
    void * input,
    f32 value,
    u32 size_bytes);

void *
T1_std_memcpy(
    void * dest,
    const void * src,
    u64 n_bytes);

s32 T1_std_mini(s32 x, s32 y);
s32 T1_std_maxi(s32 x, s32 y);
f32 T1_std_minf(f32 x, f32 y);
f32 T1_std_maxf(f32 x, f32 y);

void T1_std_strcat_cap(
    c8 * recipient,
    const u32 recipient_size,
    const c8 * to_append);

void T1_std_strcat(
    c8 * recipient,
    const c8 * to_append);

u64 T1_std_strlcat(c8 * a, const c8 * b, u64 c);

void T1_std_strcat_c8_cap(c8 * recipient, c8 to_append);

void
T1_std_strcat_s32_cap(
    c8 * recipient,
    u32 recipient_size,
    s32 to_append);

void
T1_std_strcat_u32_cap(
    c8 * recip,
    u32 recip_size,
    u32 to_append);

void
T1_std_strcat_f32_cap(
    c8 * recipient,
    const u32 recipient_size,
    const f32 to_append);

void T1_std_strcpy_cap(
    c8 * recipient,
    const u32 recipient_cap,
    const c8 * origin);

void
T1_std_copy_strings(
    c8 * recipient,
    const u32 recipient_size,
    const c8 * origin,
    const u32 origin_size);

u64 T1_std_strlen(const c8 * nullterm_str);

void
T1_std_strtolower(c8 * in);

u8
T1_std_string_starts_with(
    const c8 * str_to_check,
    const c8 * start);

u8
T1_std_string_ends_with(
    const c8 * str_to_check,
    const c8 * ending);

void
T1_std_strsub(
    c8 * in,
    const c8 * to_match,
    const c8 * replacement);

b8
T1_std_are_equal_strings(
    const c8 * str1,
    const c8 * str2);

u8
T1_std_are_equal_until_nullterminator(
    const c8 * str1,
    const c8 * str2);

u8
T1_std_are_equal_strings_of_length(
    const c8 * str1,
    const c8 * str2,
    const u64 length);

void
T1_std_s32_to_string(
    const s32 input,
    c8 * recipient);

void
T1_std_u32_to_string(
    const u32 input,
    c8 * recipient);

void
T1_std_f32_to_string(
    const f32 input,
    c8 * recipient,
    const u32 recipient_size);

s32
T1_std_string_to_s32_validate(
    const c8 * input,
    u8 * good);

s32
T1_std_string_to_s32(const c8 * input);

u32
T1_std_string_to_u32_validate(
    const c8 * input,
    u8 * good);

u32
T1_std_string_to_u32(const c8 * input);

f32
T1_std_string_to_f32_validate(
    const c8 * input,
    u8 * good);

f32
T1_std_string_to_f32(const c8 * input);

void
T1_std_f32_to_string(
    const f32 input,
    c8 * recipient,
    const u32 recipient_size);

#ifdef __cplusplus
}
#endif

#endif // T1_STD_H
