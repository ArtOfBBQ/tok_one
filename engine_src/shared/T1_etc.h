#ifndef T1_ETC_H
#define T1_ETC_H

#include "T1_stdint.h"

/*
ETC2 + EAC (RGBA8): the compressed format of GPUs without BC3 (Apple's,
from the iPhone 5s on), 4x4 pixels a block of 16 bytes (a quarter of
rgba8), like BC3. A block's first 8 bytes are its alpha (EAC: a base, a
multiplier and one of 16 tables, then 3 bits a pixel choosing one of the
table's 8 steps), the last 8 its colour (ETC1's modes, which ETC2 reads
too: two halves of 2x4 or 4x2 pixels, each a base colour and one of 8
tables of brightness steps, then 2 bits a pixel choosing one of 4 steps).
Both are big-endian, their pixels taken column by column.

T1_etc2_eac_encode() packs width * height rgba8 pixels (row by row, width
and height multiples of 4) into out, block row by block row, each row's
blocks left to right: width / 4 * height / 4 blocks of
T1_ETC2_EAC_BLOCK_BYTES.
*/
#define T1_ETC2_EAC_BLOCK_BYTES 16

void T1_etc2_eac_encode(
    const u8 * restrict rgba,
    const u32 width,
    const u32 height,
    u8 * restrict out);

#endif // T1_ETC_H
