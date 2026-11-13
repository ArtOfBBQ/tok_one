#include "T1_etc.h"

#include "T1_assert.h"

/*
ETC1's brightness steps: a half's table gives a small and a large step,
and each pixel takes one of +small, +large, -small, -large (its 2 bits:
0, 1, 2, 3)
*/
static const s32 T1_etc1_tables[8][2] = {
    {  2,   8}, {  5,  17}, {  9,  29}, { 13,  42},
    { 18,  60}, { 24,  80}, { 33, 106}, { 47, 183},
};

// EAC's alpha steps, times the block's multiplier (3 bits a pixel)
static const s32 T1_eac_tables[16][8] = {
    {-3, -6,  -9, -15, 2, 5, 8, 14},
    {-3, -7, -10, -13, 2, 6, 9, 12},
    {-2, -5,  -8, -13, 1, 4, 7, 12},
    {-2, -4,  -6, -13, 1, 3, 5, 12},
    {-3, -6,  -8, -12, 2, 5, 7, 11},
    {-3, -7,  -9, -11, 2, 6, 8, 10},
    {-4, -7,  -8, -11, 3, 6, 7, 10},
    {-3, -5,  -8, -11, 2, 4, 7, 10},
    {-2, -6,  -8, -10, 1, 5, 7,  9},
    {-2, -5,  -8, -10, 1, 4, 7,  9},
    {-2, -4,  -8, -10, 1, 3, 7,  9},
    {-2, -5,  -7, -10, 1, 4, 6,  9},
    {-3, -4,  -7, -10, 2, 3, 6,  9},
    {-1, -2,  -3, -10, 0, 1, 2,  9},
    {-4, -6,  -8,  -9, 3, 5, 7,  8},
    {-3, -5,  -7,  -9, 2, 4, 6,  8},
};

// A half block's 8 pixels and the scratch fitting them works in
typedef struct {
    s32 r[8];
    s32 g[8];
    s32 b[8];
    s32 d[8];
    s32 best_d[8];
    s32 idx[8];
} T1ETCHalf;

/*
A block's 16 pixels, column by column (pixel x, y at x * 4 + y, the order
ETC's bits take them in), and the scratch: fields of one struct, so a loop
over one never doubts another
*/
typedef struct {
    s32 r[16];
    s32 g[16];
    s32 b[16];
    s32 a[16];
    s32 d[16];
    s32 best_d[16];
    s32 idx[16];
    s32 best_idx[16];
    u64 bits64[16];
    T1ETCHalf half;
} T1ETCBlock;

static s32 T1_etc_clamp255(const s32 v) {
    return v < 0 ? 0 : v > 255 ? 255 : v;
}

static s32 T1_etc_sum8(const s32 * restrict x) {
    s32 s = 0;
    // (integers: the compiler may split it into lanes)
    for (u32 i = 0; i < 8; i++) { s += x[i]; }
    return s;
}

/*
The best table for a half with this base colour: each pixel its nearest
of the table's 4 colours. Into idx (the 2 bit values), returns the squared
error, *table_out the table
*/
static s32 T1_etc_half_fit(
    T1ETCHalf * restrict h,
    const s32 base[3],
    u32 * table_out,
    s32 idx_out[8])
{
    s32 best_error = 0x7FFFFFFF;
    for (u32 t = 0; t < 8; t++) {
        const s32 steps[4] = {
            T1_etc1_tables[t][0], T1_etc1_tables[t][1],
            -T1_etc1_tables[t][0], -T1_etc1_tables[t][1],
        };
        for (u32 i = 0; i < 8; i++) {
            h->best_d[i] = 0x7FFFFFFF;
            h->idx[i] = 0;
        }
        for (s32 m = 0; m < 4; m++) {
            const s32 cr = T1_etc_clamp255(base[0] + steps[m]);
            const s32 cg = T1_etc_clamp255(base[1] + steps[m]);
            const s32 cb = T1_etc_clamp255(base[2] + steps[m]);
            for (u32 i = 0; i < 8; i++) {
                const s32 dr = h->r[i] - cr;
                const s32 dg = h->g[i] - cg;
                const s32 db = h->b[i] - cb;
                h->d[i] = dr * dr + dg * dg + db * db;
            }
            for (u32 i = 0; i < 8; i++) {
                const s32 mask = -(s32)(h->d[i] < h->best_d[i]);
                h->best_d[i] = (h->d[i] & mask) | (h->best_d[i] & ~mask);
                h->idx[i] = (m & mask) | (h->idx[i] & ~mask);
            }
        }
        const s32 error = T1_etc_sum8(h->best_d);
        if (error < best_error) {
            best_error = error;
            *table_out = t;
            for (u32 i = 0; i < 8; i++) { idx_out[i] = h->idx[i]; }
        }
    }
    return best_error;
}

// which of the block's pixels (x * 4 + y) are in half `half` of `flip`
static void T1_etc_half_pixels(const u32 flip, const u32 half, u32 out[8]) {
    for (u32 i = 0; i < 8; i++) {
        if (flip == 0) {
            // side by side: 2 columns each
            out[i] = half * 8 + i;
        } else {
            // one above the other: 2 rows each, of every column
            out[i] = (i / 2) * 4 + half * 2 + (i % 2);
        }
    }
}

/*
The colour block, 8 bytes, in ETC1's modes (which ETC2 reads the same as
long as a differential base stays in 0..31, as here). For both ways of
halving the block (2x4 side by side, 4x2 one above the other) each half's
base is its mean colour: two 4-bit colours ("individual"), or a 5-bit one
and the other's difference in 3 bits ("differential", finer, when the
halves are close). The halving and mode with the least error is kept
*/
static void T1_etc_color_block(T1ETCBlock * restrict k, u8 * restrict out) {
    s32 best_error = 0x7FFFFFFF;
    u32 best_word0 = 0;
    for (u32 flip = 0; flip < 2; flip++) {
        u32 pixels[2][8];
        s32 mean[2][3];
        for (u32 half = 0; half < 2; half++) {
            T1_etc_half_pixels(flip, half, pixels[half]);
            // (the gathers, alone)
            for (u32 i = 0; i < 8; i++) {
                k->half.r[i] = k->r[pixels[half][i]];
                k->half.g[i] = k->g[pixels[half][i]];
                k->half.b[i] = k->b[pixels[half][i]];
            }
            mean[half][0] = (T1_etc_sum8(k->half.r) + 4) / 8;
            mean[half][1] = (T1_etc_sum8(k->half.g) + 4) / 8;
            mean[half][2] = (T1_etc_sum8(k->half.b) + 4) / 8;
        }

        for (u32 diff = 0; diff < 2; diff++) {
            s32 q[2][3];
            s32 base[2][3];
            b8 fits = 1;
            for (u32 half = 0; half < 2; half++) {
                for (u32 c = 0; c < 3; c++) {
                    if (diff) {
                        q[half][c] = (mean[half][c] * 31 + 127) / 255;
                        base[half][c] = (q[half][c] << 3) | (q[half][c] >> 2);
                    } else {
                        q[half][c] = (mean[half][c] * 15 + 127) / 255;
                        base[half][c] = q[half][c] * 17;
                    }
                }
            }
            for (u32 c = 0; c < 3 && diff; c++) {
                const s32 delta = q[1][c] - q[0][c];
                fits &= delta >= -4 && delta <= 3;
            }
            if (!fits) { continue; }

            u32 tables[2];
            s32 idx[2][8];
            s32 error = 0;
            for (u32 half = 0; half < 2; half++) {
                for (u32 i = 0; i < 8; i++) {
                    k->half.r[i] = k->r[pixels[half][i]];
                    k->half.g[i] = k->g[pixels[half][i]];
                    k->half.b[i] = k->b[pixels[half][i]];
                }
                error += T1_etc_half_fit(
                    &k->half, base[half], tables + half, idx[half]);
            }
            if (error >= best_error) { continue; }
            best_error = error;

            u32 word0;
            if (diff) {
                word0 =
                    ((u32)q[0][0] << 27) | (((u32)(q[1][0] - q[0][0]) & 7) << 24) |
                    ((u32)q[0][1] << 19) | (((u32)(q[1][1] - q[0][1]) & 7) << 16) |
                    ((u32)q[0][2] << 11) | (((u32)(q[1][2] - q[0][2]) & 7) << 8);
            } else {
                word0 =
                    ((u32)q[0][0] << 28) | ((u32)q[1][0] << 24) |
                    ((u32)q[0][1] << 20) | ((u32)q[1][1] << 16) |
                    ((u32)q[0][2] << 12) | ((u32)q[1][2] << 8);
            }
            word0 |= (tables[0] << 5) | (tables[1] << 2) | (diff << 1) | flip;
            best_word0 = word0;
            for (u32 half = 0; half < 2; half++) {
                for (u32 i = 0; i < 8; i++) {
                    k->best_idx[pixels[half][i]] = idx[half][i];
                }
            }
        }
    }

    // a pixel's 2 bits: the high one at bit 16 + its place, the low at its place
    u32 word1 = 0;
    for (u32 i = 0; i < 16; i++) {
        const u32 v = (u32)k->best_idx[i];
        word1 |= ((v >> 1) << (16 + i)) | ((v & 1) << i);
    }
    out[0] = (u8)(best_word0 >> 24);
    out[1] = (u8)(best_word0 >> 16);
    out[2] = (u8)(best_word0 >> 8);
    out[3] = (u8)best_word0;
    out[4] = (u8)(word1 >> 24);
    out[5] = (u8)(word1 >> 16);
    out[6] = (u8)(word1 >> 8);
    out[7] = (u8)word1;
}

/*
The alpha block, 8 bytes: a base, a multiplier (1 to 15) and one of 16
tables, each pixel base + its step times the multiplier. Every table is
tried, its multiplier from the alphas' spread (and one more), the base in
the middle; the one with the least error is kept
*/
static void T1_etc_alpha_block(T1ETCBlock * restrict k, u8 * restrict out) {
    s32 lo = 255;
    s32 hi = 0;
    for (u32 i = 0; i < 16; i++) {
        lo = k->a[i] < lo ? k->a[i] : lo;
        hi = k->a[i] > hi ? k->a[i] : hi;
    }

    // one alpha: table 13 has a step of 0 (its 5th)
    s32 best_error = 0x7FFFFFFF;
    u32 best_base = (u32)lo;
    u32 best_mult = 1;
    u32 best_table = 13;
    for (u32 i = 0; i < 16; i++) { k->best_idx[i] = 4; }

    for (u32 t = 0; t < 16 && hi > lo; t++) {
        const s32 * steps = T1_eac_tables[t];
        const s32 span = steps[7] - steps[3];
        s32 mult0 = ((hi - lo) + span / 2) / span;
        mult0 = mult0 < 1 ? 1 : mult0 > 15 ? 15 : mult0;
        for (s32 mult = mult0; mult <= mult0 + 1 && mult <= 15; mult++) {
            const s32 base = T1_etc_clamp255(
                (hi + lo - (steps[7] + steps[3]) * mult + 1) / 2);
            for (u32 i = 0; i < 16; i++) {
                k->best_d[i] = 0x7FFFFFFF;
                k->idx[i] = 0;
            }
            for (s32 j = 0; j < 8; j++) {
                const s32 v = T1_etc_clamp255(base + steps[j] * mult);
                for (u32 i = 0; i < 16; i++) {
                    const s32 d = k->a[i] - v;
                    k->d[i] = d * d;
                }
                for (u32 i = 0; i < 16; i++) {
                    const s32 mask = -(s32)(k->d[i] < k->best_d[i]);
                    k->best_d[i] = (k->d[i] & mask) | (k->best_d[i] & ~mask);
                    k->idx[i] = (j & mask) | (k->idx[i] & ~mask);
                }
            }
            s32 error = 0;
            for (u32 i = 0; i < 16; i++) { error += k->best_d[i]; }
            if (error < best_error) {
                best_error = error;
                best_base = (u32)base;
                best_mult = (u32)mult;
                best_table = t;
                for (u32 i = 0; i < 16; i++) { k->best_idx[i] = k->idx[i]; }
            }
        }
    }

    // the first pixel's 3 bits highest
    for (u32 i = 0; i < 16; i++) {
        k->bits64[i] = (u64)k->best_idx[i] << (45 - 3 * i);
    }
    u64 bits =
        ((u64)best_base << 56) | ((u64)best_mult << 52) | ((u64)best_table << 48);
    for (u32 i = 0; i < 16; i++) { bits |= k->bits64[i]; }
    for (u32 i = 0; i < 8; i++) { out[i] = (u8)(bits >> (56 - 8 * i)); }
}

void T1_etc2_eac_encode(
    const u8 * restrict rgba,
    const u32 width,
    const u32 height,
    u8 * restrict out)
{
    T1_assert(width % 4 == 0 && height % 4 == 0);
    T1ETCBlock k;
    for (u32 by = 0; by < height / 4; by++) {
        for (u32 bx = 0; bx < width / 4; bx++) {
            // the block's pixels, column by column (the gathers, alone)
            for (u32 y = 0; y < 4; y++) {
                const u8 * row =
                    rgba + ((u64)(by * 4 + y) * width + bx * 4) * 4;
                for (u32 x = 0; x < 4; x++) {
                    k.r[x * 4 + y] = row[x * 4 + 0];
                    k.g[x * 4 + y] = row[x * 4 + 1];
                    k.b[x * 4 + y] = row[x * 4 + 2];
                    k.a[x * 4 + y] = row[x * 4 + 3];
                }
            }
            T1_etc_alpha_block(&k, out);
            T1_etc_color_block(&k, out + 8);
            out += T1_ETC2_EAC_BLOCK_BYTES;
        }
    }
}
