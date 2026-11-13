#ifndef T1_BC_H
#define T1_BC_H

#include "T1_stdint.h"

/*
BLOCK COMPRESSION: BC3, a format every desktop GPU samples as it is, 4x4
pixels a block of 16 bytes (a quarter of rgba8). A block keeps two colours
and picks one of 4 points between them for each pixel, and two alphas with
one of 8 points between them, so sharp edges between more than two colours
(text) smear, while noisy surfaces look nearly the same.

T1_bc3_encode() packs width * height rgba8 pixels (row by row, width and
height multiples of 4) into out, block row by block row, each row's blocks
left to right: width / 4 * height / 4 blocks of T1_BC3_BLOCK_BYTES.
*/
#define T1_BC3_BLOCK_BYTES 16
#define T1_BC1_BLOCK_BYTES 8

void T1_bc3_encode(
    const u8 * restrict rgba,
    const u32 width,
    const u32 height,
    u8 * restrict out);

/*
T1_bc_decode(): BC1 (bc3 0: 8 bytes a block, colours only, or holes) or BC3
blocks of a width * height image into its rgba8, as a GPU decodes them;
the edge's blocks may stick out (width and height any size)
*/
void T1_bc_decode(
    const u8 * restrict blocks,
    const u32 width,
    const u32 height,
    const b8 bc3,
    u8 * restrict rgba);

#endif // T1_BC_H
