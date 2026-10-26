#ifndef T1_TEX_H
#define T1_TEX_H

#include "T1_stdint.h"

#include "T1_types_public.h"

/*
A T1Tex packs a texture array index (5 bits, so T1_TEXARRAYS_CAP <= 32) and
a slice index (11 bits, so T1_TEX_SLICES_CAP <= 2048) into 16 bits
*/
s16 T1_tex_to_array_i(T1Tex tex);
s16 T1_tex_to_slice_i(T1Tex tex);
void T1_tex_set_array_i(T1Tex * recip, s16 newval);

void T1_tex_set_slice_i(T1Tex * recip, s16 newval);

#endif // T1_TEX_H
