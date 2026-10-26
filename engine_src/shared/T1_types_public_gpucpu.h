#ifndef T1_TYPES_PUBLIC_GPUCPU_H
#define T1_TYPES_PUBLIC_GPUCPU_H

#ifdef __METAL_VERSION__
#else
#include <stdint.h>
#endif

#define T1_TEX_NONE 32109
// as a pass's "color_tex[0]": the window
#define T1_TEX_WINDOW 32110
// a T1Tex is the index of a texture array and of a slice in it
#define T1_TEX_ARRAY_I(tex) ((tex) >> 11)
#define T1_TEX_SLICE_I(tex) ((tex) & 0x07FF)

#endif // T1_TYPES_PUBLIC_GPUCPU_H
