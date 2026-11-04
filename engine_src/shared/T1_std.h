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

// dest and src may not overlap (asserted)
void * T1_std_memcpy(
    void * restrict dest,
    const void * restrict src,
    u64 n_bytes);
typedef union { f32 f; s32 s; } T1StdF32Bits;

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

void T1_std_strcat_u32_cap(
    c8 * recip,
    u32 recip_size,
    u32 to_append);

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

b8 T1_std_are_equal_strings(
    const c8 * str1,
    const c8 * str2);

u8 T1_std_are_equal_strings_of_length(
    const c8 * str1,
    const c8 * str2,
    u64 length);

void T1_std_u32_to_string(
    u32 input,
    c8 * recipient);

#endif // T1_STD_H
