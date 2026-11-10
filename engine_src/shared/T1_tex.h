#ifndef T1_TEX_H
#define T1_TEX_H

#include "T1_stdint.h"

#include "T1_types_public.h"

/*
How T1 keeps textures, its own business: each T1Tex is a slice of a texture
array of images of one size, at most T1_TEXARRAYS_CAP arrays (each bound to
every pass, see T1_types_gpucpu.h) of T1_TEX_SLICES_CAP slices. A T1Tex packs
the array index (5 bits, so T1_TEXARRAYS_CAP <= 32) and the slice index (11
bits, so T1_TEX_SLICES_CAP <= 2048) into 16 bits
*/
#define T1_TEXARRAYS_CAP 29
#define T1_TEX_SLICES_CAP 250
// the cap as text, for the shaders' preludes
#define T1_TEX_STR_(x) #x
#define T1_TEX_STR(x) T1_TEX_STR_(x)
s16 T1_tex_to_array_i(T1Tex tex);
s16 T1_tex_to_slice_i(T1Tex tex);
void T1_tex_set_array_i(T1Tex * recip, s16 newval);

void T1_tex_set_slice_i(T1Tex * recip, s16 newval);

#endif // T1_TEX_H
