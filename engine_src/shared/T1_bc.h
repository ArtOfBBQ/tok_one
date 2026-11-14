#ifndef T1_BC_H
#define T1_BC_H

#include "T1_stdint.h"

/*
BLOCK COMPRESSION: BC1 (8 bytes a 4x4 block: two colours and one of 4
points between them a pixel, or 3 and holes) and BC3 (16: BC1's colours
after two alphas and one of 8 points between them a pixel), the formats
every desktop GPU samples as they are.

T1_bc_decode(): BC1 (bc3 0) or BC3 blocks of a width * height image into
its rgba8, as a GPU decodes them, for a GPU that can't sample them; the
edge's blocks may stick out (width and height any size)
*/
#define T1_BC3_BLOCK_BYTES 16
#define T1_BC1_BLOCK_BYTES 8

void T1_bc_decode(
    const u8 * restrict blocks,
    const u32 width,
    const u32 height,
    const b8 bc3,
    u8 * restrict rgba);

#endif // T1_BC_H
