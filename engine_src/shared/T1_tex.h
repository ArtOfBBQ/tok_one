#ifndef T1_TEX_H
#define T1_TEX_H

#include <stdint.h>

#include "T1_types_public.h"

/*
Can we use 2 bytes to specify a texture?

Currently we have these:
#define T1_TEXARRAYS_CAP 29
#define T1_TEX_SLICES_CAP 250

64 32 16 08 04 02 01
 0  0  1  1  1  1  1  16 + 8 + 04 + 02 + 01 = 31
 0  0  0  1  1  1  1       8 + 04 + 02 + 01 = 15
 
So we could have 5 bits for the array, then use an 11-bit value
for the slice

11 bits = 2048 max

when the 3 rightmost bits of the first byte are all set, their value is 7,
so the mask 0x7F will be key

when the 5 leftmost bits of the first byte are all set, their value is 0xF8
*/
s16 T1_tex_to_array_i(T1Tex tex);
s16 T1_tex_to_slice_i(T1Tex tex);
void T1_tex_set_array_i(
    T1Tex * recip,
    s16 newval);

void T1_tex_set_slice_i(
    T1Tex * recip,
    s16 newval);

#endif // T1_TEX_H
