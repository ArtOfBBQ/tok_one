#ifndef T1_STDINT_H
#define T1_STDINT_H

#ifdef __METAL_VERSION__
// For Metal Shading Language
typedef long   s64;
typedef int    s32;
typedef short  s16;
typedef char   s8;

typedef ulong  u64;
typedef uint   u32;
typedef ushort u16;
typedef uchar   u8;

typedef float  f32;
typedef half   f16;
#else
#include <stdint.h>

typedef int64_t  s64;
typedef int32_t  s32;
typedef int16_t  s16;
typedef int8_t    s8;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t   u8;
typedef uint8_t   b8;
typedef uint8_t u4x2;
typedef char      c8;

typedef double   f64;
typedef float    f32;
#endif

#endif // T1_STDINT_H
