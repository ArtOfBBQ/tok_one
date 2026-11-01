#ifndef T1_STD_H
#define T1_STD_H

#include "T1_stdint.h"

#ifndef NULL
#define NULL 0
#endif

#define T1_F32_MAX 3.402823466e+38f
#define T1_F32_MIN -T1_F32_MAX

#define T1_std_abs(a) ((((a) > 0)*(a))+(((a) < 0)*-(a)))

void * T1_std_memset(
    void * input,
    s32 value,
    u64 size_bytes);

void T1_std_memset_i16(
    void * input,
    s16 value,
    u32 size_bytes);

void T1_std_memset_s32(
    void * input,
    s32 value,
    u32 size_bytes);

void T1_std_memset_f32(
    void * input,
    f32 value,
    u32 size_bytes);

// dest and src may not overlap (asserted)
void * T1_std_memcpy(
    void * restrict dest,
    const void * restrict src,
    u64 n_bytes);

s32 T1_std_mini(s32 x, s32 y);
s32 T1_std_maxi(s32 x, s32 y);
typedef union { f32 f; s32 s; } T1StdF32Bits;

// |a|: the sign bit cleared
static inline f32 T1_std_fabs(const f32 a) {
    T1StdF32Bits bits;
    bits.f = a;
    bits.s &= 0x7FFFFFFF;
    return bits.f;
}

// x if x <= y, else y
static inline f32 T1_std_minf(const f32 x, const f32 y) {
    T1StdF32Bits ux, uy, picked;
    ux.f = x;
    uy.f = y;
    const s32 x_wins = -(s32)(x <= y); // all 1 bits or all 0
    picked.s = (ux.s & x_wins) | (uy.s & ~x_wins);
    return picked.f;
}

// x if x > y, else y
static inline f32 T1_std_maxf(const f32 x, const f32 y) {
    T1StdF32Bits ux, uy, picked;
    ux.f = x;
    uy.f = y;
    const s32 x_wins = -(s32)(x > y); // all 1 bits or all 0
    picked.s = (ux.s & x_wins) | (uy.s & ~x_wins);
    return picked.f;
}

static inline f32 T1_std_clampf(const f32 x, const f32 lo, const f32 hi) {
    return T1_std_minf(T1_std_maxf(x, lo), hi);
}

void T1_std_strcat_cap(
    c8 * recipient,
    u32 recipient_size,
    const c8 * to_append);

void T1_std_strcat(
    c8 * recipient,
    const c8 * to_append);

u64 T1_std_strlcat(c8 * a, const c8 * b, u64 c);

void T1_std_strcat_c8_cap(c8 * recipient, c8 to_append);

void T1_std_strcat_s32_cap(
    c8 * recip,
    u32 recip_size,
    s32 to_append);

void T1_std_strcat_u32_cap(
    c8 * recip,
    u32 recip_size,
    u32 to_append);

void T1_std_strcat_f32_cap(
    c8 * recip,
    u32 recip_size,
    f32 to_append);

void T1_std_strcpy_cap(
    c8 * recip,
    const u32 recip_cap,
    const c8 * origin);

#if 0
void T1_std_copy_strings(
    c8 * recipient,
    u32 recip_size,
    const c8 * origin,
    u32 origin_size);
#endif

u64 T1_std_strlen(const c8 * nullterm_str);

void T1_std_strtolower(c8 * str);

u8 T1_std_string_starts_with(
    const c8 * str_to_check,
    const c8 * start);

u8 T1_std_string_ends_with(
    const c8 * str_to_check,
    const c8 * ending);

void T1_std_strsub(
    c8 * str,
    const c8 * to_match,
    const c8 * replacement);

b8 T1_std_are_equal_strings(
    const c8 * str1,
    const c8 * str2);

u8 T1_std_are_equal_until_nullterminator(
    const c8 * str1,
    const c8 * str2);

u8 T1_std_are_equal_strings_of_length(
    const c8 * str1,
    const c8 * str2,
    u64 length);

void T1_std_s32_to_string(
    s32 input,
    c8 * recipient);

void T1_std_u32_to_string(
    u32 input,
    c8 * recipient);

void T1_std_f32_to_string(
    f32 input,
    c8 * recipient,
    u32 recipient_size);

s32 T1_std_string_to_s32_validate(
    const c8 * input,
    u8 * good);

s32 T1_std_string_to_s32(const c8 * input);

u32 T1_std_string_to_u32_validate(
    const c8 * input,
    u8 * good);

u32 T1_std_string_to_u32(const c8 * input);

f32 T1_std_string_to_f32_validate(
    const c8 * input,
    u8 * good);

f32 T1_std_string_to_f32(const c8 * input);

void T1_std_f32_to_string(
    f32 input,
    c8 * recipient,
    u32 recipient_size);

#endif // T1_STD_H
