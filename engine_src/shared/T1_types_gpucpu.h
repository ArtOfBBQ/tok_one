#ifndef T1_TYPES_GPUCPU_H
#define T1_TYPES_GPUCPU_H

#include "T1_stdint.h"
#include "T1_types_public_gpucpu.h"

/*
Where shaders find what T1 binds for every pass:
- texture(0) to texture(T1_TEXARRAYS_CAP - 1): every texture array, so a
  shader can sample any T1Tex (see T1_TEX_ARRAY_I and T1_TEX_SLICE_I)
- buffer(T1_PARAMS_I): the pass's "params[0]" and up, a constant array of
  T1_PARAMS_CAP uint32_t for anything (e.g. T1Texs to sample)
- texture(T1_WRITE_TEX_I): a compute pass's "write_tex", one thread per
  pixel of it (uint2 [[thread_position_in_grid]])
- buffer(T1_FRAME_CONSTS_I): a T1GPUFrameConsts
- buffer(T1_BUFS_I + k): the pass's "buf[k]", a T1Buf the shader declares
  as whatever type it holds, e.g. device MyParticle * [[buffer(T1_BUFS_I)]]
Draw passes bind these for both their vertex and fragment functions.
*/
// T1_FRAME_CONSTS_I etc. are in T1_types_public_gpucpu.h

// What T1 tells shaders about the frame, see T1_FRAME_CONSTS_I
typedef struct
{
    uint32_t timestamp; // microseconds
    uint32_t padding[3];
} T1GPUFrameConsts;

#endif // T1_TYPES_GPUCPU_H
