#include "T1_bc.h"

#include "T1_assert.h"

static s32 T1_bc_expand5(const u32 v) { return (s32)((v << 3) | (v >> 2)); }
static s32 T1_bc_expand6(const u32 v) { return (s32)((v << 2) | (v >> 4)); }

/*
DECODING, as a GPU does: a colour block's 4 colours (c0 > c1), or for BC1
with c0 <= c1 three (c0, c1, their middle) and transparent black. BC3's
colour half is always the 4 colours. 16 rgba pixels, row by row
*/
static void T1_bc_decode_color(
    const u8 * restrict block,
    const b8 bc1,
    u8 * restrict px)
{
    const u32 c0 = (u32)block[0] | ((u32)block[1] << 8);
    const u32 c1 = (u32)block[2] | ((u32)block[3] << 8);
    s32 pal[4][4];
    pal[0][0] = T1_bc_expand5(c0 >> 11);
    pal[0][1] = T1_bc_expand6((c0 >> 5) & 63);
    pal[0][2] = T1_bc_expand5(c0 & 31);
    pal[1][0] = T1_bc_expand5(c1 >> 11);
    pal[1][1] = T1_bc_expand6((c1 >> 5) & 63);
    pal[1][2] = T1_bc_expand5(c1 & 31);
    pal[0][3] = pal[1][3] = pal[2][3] = pal[3][3] = 255;
    if (!bc1 || c0 > c1) {
        for (u32 c = 0; c < 3; c++) {
            pal[2][c] = (2 * pal[0][c] + pal[1][c] + 1) / 3;
            pal[3][c] = (pal[0][c] + 2 * pal[1][c] + 1) / 3;
        }
    } else {
        for (u32 c = 0; c < 3; c++) {
            pal[2][c] = (pal[0][c] + pal[1][c]) / 2;
            pal[3][c] = 0;
        }
        pal[3][3] = 0;
    }
    const u32 bits =
        (u32)block[4] | ((u32)block[5] << 8) |
        ((u32)block[6] << 16) | ((u32)block[7] << 24);
    for (u32 i = 0; i < 16; i++) {
        const u32 k = (bits >> (2 * i)) & 3;
        for (u32 c = 0; c < 4; c++) { px[i * 4 + c] = (u8)pal[k][c]; }
    }
}

// BC3's alpha half into the 16 pixels' alpha
static void T1_bc_decode_alpha(const u8 * restrict block, u8 * restrict px) {
    s32 a[8];
    a[0] = block[0];
    a[1] = block[1];
    if (a[0] > a[1]) {
        for (s32 i = 1; i < 7; i++) {
            a[i + 1] = ((7 - i) * a[0] + i * a[1] + 3) / 7;
        }
    } else {
        for (s32 i = 1; i < 5; i++) {
            a[i + 1] = ((5 - i) * a[0] + i * a[1] + 2) / 5;
        }
        a[6] = 0;
        a[7] = 255;
    }
    u64 bits = 0;
    for (u32 i = 0; i < 6; i++) { bits |= (u64)block[2 + i] << (8 * i); }
    for (u32 i = 0; i < 16; i++) {
        px[i * 4 + 3] = (u8)a[(bits >> (3 * i)) & 7];
    }
}

void T1_bc_decode(
    const u8 * restrict blocks,
    const u32 width,
    const u32 height,
    const b8 bc3,
    u8 * restrict rgba)
{
    const u32 block_bytes = bc3 ? T1_BC3_BLOCK_BYTES : T1_BC1_BLOCK_BYTES;
    const u32 blocks_w = (width + 3) / 4;
    const u32 blocks_h = (height + 3) / 4;
    u8 px[16 * 4];
    for (u32 by = 0; by < blocks_h; by++) {
        for (u32 bx = 0; bx < blocks_w; bx++) {
            const u8 * block = blocks + ((u64)by * blocks_w + bx) * block_bytes;
            if (bc3) {
                T1_bc_decode_color(block + 8, 0, px);
                T1_bc_decode_alpha(block, px);
            } else {
                T1_bc_decode_color(block, 1, px);
            }
            // (the edge's blocks may stick out of the image)
            for (u32 y = 0; y < 4 && by * 4 + y < height; y++) {
                u8 * row = rgba + ((u64)(by * 4 + y) * width + bx * 4) * 4;
                const u32 xs = width - bx * 4 < 4 ? width - bx * 4 : 4;
                for (u32 i = 0; i < xs * 4; i++) { row[i] = px[y * 16 + i]; }
            }
        }
    }
}
