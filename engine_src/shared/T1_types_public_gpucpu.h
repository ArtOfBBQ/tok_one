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

// where shaders find what passes bind, see T1_types_gpucpu.h
#define T1_FRAME_CONSTS_I 7
#define T1_PARAMS_I 9
#define T1_PARAMS_CAP 8
#define T1_BUFS_I 16
#define T1_PASS_BUFS_CAP 12
#define T1_WRITE_TEX_I 30

#endif // T1_TYPES_PUBLIC_GPUCPU_H
