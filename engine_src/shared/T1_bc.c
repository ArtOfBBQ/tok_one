#include "T1_bc.h"

#include "T1_assert.h"

/*
One block's 16 pixels as columns of numbers, and the scratch its steps
work in: fields of one struct, so a loop over one never doubts another
(every loop here runs over the 16 pixels at once)
*/
typedef struct {
    s32 r[16];
    s32 g[16];
    s32 b[16];
    s32 a[16];
    s32 t[16];
    s32 d[16];
    s32 best_d[16];
    s32 idx[16];
    u32 bits[16];
    u64 bits64[16];
} T1BCBlock;

// the largest of t's 16 (t is overwritten)
static s32 T1_bc_max16(T1BCBlock * restrict k) {
    for (u32 i = 0; i < 8; i++) {
        const s32 m = -(s32)(k->t[i + 8] > k->t[i]);
        k->t[i] = (k->t[i + 8] & m) | (k->t[i] & ~m);
    }
    for (u32 i = 0; i < 4; i++) {
        const s32 m = -(s32)(k->t[i + 4] > k->t[i]);
        k->t[i] = (k->t[i + 4] & m) | (k->t[i] & ~m);
    }
    for (u32 i = 0; i < 2; i++) {
        const s32 m = -(s32)(k->t[i + 2] > k->t[i]);
        k->t[i] = (k->t[i + 2] & m) | (k->t[i] & ~m);
    }
    return k->t[1] > k->t[0] ? k->t[1] : k->t[0];
}

// the smallest of t's 16 (t is overwritten)
static s32 T1_bc_min16(T1BCBlock * restrict k) {
    for (u32 i = 0; i < 8; i++) {
        const s32 m = -(s32)(k->t[i + 8] < k->t[i]);
        k->t[i] = (k->t[i + 8] & m) | (k->t[i] & ~m);
    }
    for (u32 i = 0; i < 4; i++) {
        const s32 m = -(s32)(k->t[i + 4] < k->t[i]);
        k->t[i] = (k->t[i + 4] & m) | (k->t[i] & ~m);
    }
    for (u32 i = 0; i < 2; i++) {
        const s32 m = -(s32)(k->t[i + 2] < k->t[i]);
        k->t[i] = (k->t[i + 2] & m) | (k->t[i] & ~m);
    }
    return k->t[1] < k->t[0] ? k->t[1] : k->t[0];
}

// the sum of t's 16 (t is overwritten)
static s32 T1_bc_sum16(T1BCBlock * restrict k) {
    for (u32 i = 0; i < 8; i++) { k->t[i] += k->t[i + 8]; }
    for (u32 i = 0; i < 4; i++) { k->t[i] += k->t[i + 4]; }
    for (u32 i = 0; i < 2; i++) { k->t[i] += k->t[i + 2]; }
    return k->t[0] + k->t[1];
}

static void T1_bc_bounds(
    T1BCBlock * restrict k,
    const s32 * restrict channel,
    s32 * lo,
    s32 * hi)
{
    for (u32 i = 0; i < 16; i++) { k->t[i] = channel[i]; }
    *hi = T1_bc_max16(k);
    for (u32 i = 0; i < 16; i++) { k->t[i] = channel[i]; }
    *lo = T1_bc_min16(k);
}

// 16 times the covariance of two channels, its sign is what matters
static s32 T1_bc_covariance(
    T1BCBlock * restrict k,
    const s32 * restrict x,
    const s32 * restrict y)
{
    for (u32 i = 0; i < 16; i++) { k->t[i] = x[i]; }
    const s32 sum_x = T1_bc_sum16(k);
    for (u32 i = 0; i < 16; i++) { k->t[i] = y[i]; }
    const s32 sum_y = T1_bc_sum16(k);
    // (at most 4080 * 4080 * 16: fits)
    for (u32 i = 0; i < 16; i++) {
        k->t[i] = (x[i] * 16 - sum_x) * (y[i] * 16 - sum_y);
    }
    return T1_bc_sum16(k);
}

static u32 T1_bc_quantize(const s32 v, const s32 levels) {
    return (u32)((v * levels + 127) / 255);
}

static s32 T1_bc_expand5(const u32 v) { return (s32)((v << 3) | (v >> 2)); }
static s32 T1_bc_expand6(const u32 v) { return (s32)((v << 2) | (v >> 4)); }

static u32 T1_bc_565(const s32 * e) {
    return
        (T1_bc_quantize(e[0], 31) << 11) |
        (T1_bc_quantize(e[1], 63) << 5) |
        T1_bc_quantize(e[2], 31);
}

/*
Each pixel's nearest of the 4 colours c0 and c1 (c0 >= c1) make, as a GPU
decodes them: c0, c1, 2/3 c0 + 1/3 c1, then 1/3 c0 + 2/3 c1. Into idx,
returns the block's squared error
*/
static s32 T1_bc_fit(T1BCBlock * restrict k, const u32 c0, const u32 c1) {
    s32 pal[4][3];
    pal[0][0] = T1_bc_expand5(c0 >> 11);
    pal[0][1] = T1_bc_expand6((c0 >> 5) & 63);
    pal[0][2] = T1_bc_expand5(c0 & 31);
    pal[1][0] = T1_bc_expand5(c1 >> 11);
    pal[1][1] = T1_bc_expand6((c1 >> 5) & 63);
    pal[1][2] = T1_bc_expand5(c1 & 31);
    for (u32 c = 0; c < 3; c++) {
        pal[2][c] = (2 * pal[0][c] + pal[1][c] + 1) / 3;
        pal[3][c] = (pal[0][c] + 2 * pal[1][c] + 1) / 3;
    }
    // (one colour: every pixel the first)
    const s32 colours = c0 == c1 ? 1 : 4;
    
    for (u32 i = 0; i < 16; i++) {
        k->best_d[i] = 0x7FFFFFFF;
        k->idx[i] = 0;
    }
    for (s32 p = 0; p < colours; p++) {
        const s32 pr = pal[p][0];
        const s32 pg = pal[p][1];
        const s32 pb = pal[p][2];
        for (u32 i = 0; i < 16; i++) {
            const s32 dr = k->r[i] - pr;
            const s32 dg = k->g[i] - pg;
            const s32 db = k->b[i] - pb;
            k->d[i] = dr * dr + dg * dg + db * db;
        }
        for (u32 i = 0; i < 16; i++) {
            const s32 m = -(s32)(k->d[i] < k->best_d[i]);
            k->best_d[i] = (k->d[i] & m) | (k->best_d[i] & ~m);
            k->idx[i] = (p & m) | (k->idx[i] & ~m);
        }
    }
    for (u32 i = 0; i < 16; i++) { k->t[i] = k->best_d[i]; }
    return T1_bc_sum16(k);
}

// c0 and c1 in order (c0 the larger, see T1_bc_color_block())
static void T1_bc_order(u32 * c0, u32 * c1) {
    if (*c0 < *c1) {
        const u32 swap = *c0;
        *c0 = *c1;
        *c1 = swap;
    }
}

/*
The two colours that best fit the pixels' choices (least squares): each
pixel is a3 / 3 of the first and (3 - a3) / 3 of the second
*/
static void T1_bc_refit(T1BCBlock * restrict k, u32 * c0, u32 * c1) {
    // idx 0, 1, 2, 3: a3 3, 0, 2, 1
    for (u32 i = 0; i < 16; i++) {
        const s32 one = -(s32)(k->idx[i] == 1);
        const s32 zero = -(s32)(k->idx[i] == 0);
        k->d[i] = ((4 - k->idx[i]) & ~one & ~zero) | (3 & zero);
    }
    for (u32 i = 0; i < 16; i++) { k->t[i] = k->d[i] * k->d[i]; }
    const s32 aa = T1_bc_sum16(k);
    for (u32 i = 0; i < 16; i++) { k->t[i] = k->d[i] * (3 - k->d[i]); }
    const s32 ab = T1_bc_sum16(k);
    for (u32 i = 0; i < 16; i++) { k->t[i] = (3 - k->d[i]) * (3 - k->d[i]); }
    const s32 bb = T1_bc_sum16(k);
    const s32 det = aa * bb - ab * ab;
    if (det == 0) { return; }
    
    const s32 * channels[3] = { k->r, k->g, k->b };
    s32 e0[3];
    s32 e1[3];
    for (u32 c = 0; c < 3; c++) {
        const s32 * x = channels[c];
        for (u32 i = 0; i < 16; i++) { k->t[i] = k->d[i] * x[i]; }
        const s32 ax = T1_bc_sum16(k);
        for (u32 i = 0; i < 16; i++) { k->t[i] = (3 - k->d[i]) * x[i]; }
        const s32 bx = T1_bc_sum16(k);
        const f32 v0 = 3.0f * (f32)(bb * ax - ab * bx) / (f32)det;
        const f32 v1 = 3.0f * (f32)(aa * bx - ab * ax) / (f32)det;
        e0[c] = (s32)(v0 < 0.0f ? 0.0f : v0 > 255.0f ? 255.0f : v0 + 0.5f);
        e1[c] = (s32)(v1 < 0.0f ? 0.0f : v1 > 255.0f ? 255.0f : v1 + 0.5f);
    }
    *c0 = T1_bc_565(e0);
    *c1 = T1_bc_565(e1);
    T1_bc_order(c0, c1);
}

/*
The colour block, 8 bytes: two 565 colours, the first the larger (BC1, whose
colour blocks these are, would otherwise read 3 colours and a hole), and 2 bits a pixel choosing among
them. First guess: the corners of the pixels' bounding box, pulled in by a
sixteenth (the box's corners are rarely pixels), on the diagonal the pixels
lie along (a channel that falls while the widest one rises has its corners
swapped). Then the two that best fit the choices the guess made, kept if
they do better (blocks of two colours, text, come out exact)
*/
static void T1_bc_color_block(T1BCBlock * restrict k, u8 * restrict out) {
    s32 lo[3];
    s32 hi[3];
    const s32 * channels[3] = { k->r, k->g, k->b };
    for (u32 c = 0; c < 3; c++) {
        T1_bc_bounds(k, channels[c], lo + c, hi + c);
    }
    
    u32 widest = 0;
    for (u32 c = 1; c < 3; c++) {
        if (hi[c] - lo[c] > hi[widest] - lo[widest]) { widest = c; }
    }
    s32 e0[3];
    s32 e1[3];
    for (u32 c = 0; c < 3; c++) {
        const s32 inset = (hi[c] - lo[c]) >> 4;
        e0[c] = hi[c] - inset;
        e1[c] = lo[c] + inset;
    }
    for (u32 c = 0; c < 3; c++) {
        if (c == widest) { continue; }
        if (T1_bc_covariance(k, channels[widest], channels[c]) < 0) {
            const s32 swap = e0[c];
            e0[c] = e1[c];
            e1[c] = swap;
        }
    }
    
    u32 c0 = T1_bc_565(e0);
    u32 c1 = T1_bc_565(e1);
    T1_bc_order(&c0, &c1);
    s32 error = T1_bc_fit(k, c0, c1);
    
    u32 r0 = c0;
    u32 r1 = c1;
    T1_bc_refit(k, &r0, &r1);
    if (r0 != c0 || r1 != c1) {
        // (the guess's choices, for if the refit does worse)
        s32 kept[16];
        for (u32 i = 0; i < 16; i++) { kept[i] = k->idx[i]; }
        const s32 refit_error = T1_bc_fit(k, r0, r1);
        if (refit_error < error) {
            c0 = r0;
            c1 = r1;
            error = refit_error;
        } else {
            for (u32 i = 0; i < 16; i++) { k->idx[i] = kept[i]; }
        }
    }
    
    out[0] = (u8)(c0 & 0xFF);
    out[1] = (u8)(c0 >> 8);
    out[2] = (u8)(c1 & 0xFF);
    out[3] = (u8)(c1 >> 8);
    
    // 2 bits a pixel, the first pixel lowest
    for (u32 i = 0; i < 16; i++) { k->bits[i] = (u32)k->idx[i] << (2 * i); }
    for (u32 i = 0; i < 8; i++) { k->bits[i] |= k->bits[i + 8]; }
    for (u32 i = 0; i < 4; i++) { k->bits[i] |= k->bits[i + 4]; }
    for (u32 i = 0; i < 2; i++) { k->bits[i] |= k->bits[i + 2]; }
    const u32 bits = k->bits[0] | k->bits[1];
    out[4] = (u8)(bits & 0xFF);
    out[5] = (u8)((bits >> 8) & 0xFF);
    out[6] = (u8)((bits >> 16) & 0xFF);
    out[7] = (u8)(bits >> 24);
}

/*
The alpha block, 8 bytes: the largest and smallest alpha, and 3 bits a
pixel choosing among them and the 6 evenly between (index 0 the largest, 1
the smallest, 2 to 7 from the largest down)
*/
static void T1_bc_alpha_block(T1BCBlock * restrict k, u8 * restrict out) {
    s32 lo;
    s32 hi;
    T1_bc_bounds(k, k->a, &lo, &hi);
    out[0] = (u8)hi;
    out[1] = (u8)lo;
    if (hi == lo) {
        for (u32 i = 2; i < 8; i++) { out[i] = 0; }
        return;
    }

    // each pixel's step from the smallest (0) to the largest (7)
    const f32 scale = 7.0f / (f32)(hi - lo);
    for (u32 i = 0; i < 16; i++) {
        k->t[i] = (s32)((f32)(k->a[i] - lo) * scale + 0.5f);
    }
    // step 7: index 0, step 0: index 1, step s between: index 8 - s
    for (u32 i = 0; i < 16; i++) {
        const s32 top = -(s32)(k->t[i] == 7);
        const s32 bottom = -(s32)(k->t[i] == 0);
        const s32 between = 8 - k->t[i];
        k->idx[i] = (between & ~top & ~bottom) | (1 & bottom);
    }

    for (u32 i = 0; i < 16; i++) {
        k->bits64[i] = (u64)k->idx[i] << (3 * i);
    }
    for (u32 i = 0; i < 8; i++) { k->bits64[i] |= k->bits64[i + 8]; }
    for (u32 i = 0; i < 4; i++) { k->bits64[i] |= k->bits64[i + 4]; }
    for (u32 i = 0; i < 2; i++) { k->bits64[i] |= k->bits64[i + 2]; }
    const u64 bits = k->bits64[0] | k->bits64[1];
    for (u32 i = 0; i < 6; i++) { out[2 + i] = (u8)((bits >> (8 * i)) & 0xFF); }
}

void T1_bc3_encode(
    const u8 * restrict rgba,
    const u32 width,
    const u32 height,
    u8 * restrict out)
{
    T1_assert(width % 4 == 0 && height % 4 == 0);
    T1BCBlock k;
    for (u32 by = 0; by < height / 4; by++) {
        for (u32 bx = 0; bx < width / 4; bx++) {
            // the block's pixels, row by row (the gathers, alone)
            for (u32 ry = 0; ry < 4; ry++) {
                const u8 * row =
                    rgba + ((u64)(by * 4 + ry) * width + bx * 4) * 4;
                s32 * r = k.r + ry * 4;
                s32 * g = k.g + ry * 4;
                s32 * b = k.b + ry * 4;
                s32 * a = k.a + ry * 4;
                for (u32 x = 0; x < 4; x++) {
                    r[x] = row[x * 4 + 0];
                    g[x] = row[x * 4 + 1];
                    b[x] = row[x * 4 + 2];
                    a[x] = row[x * 4 + 3];
                }
            }
            T1_bc_alpha_block(&k, out);
            T1_bc_color_block(&k, out + 8);
            out += T1_BC3_BLOCK_BYTES;
        }
    }
}

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
