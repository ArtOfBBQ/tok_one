//== common
/*
The shaders of shadertest.c, in sections: a line "//== name" starts one.
Every case's section is compiled on its own after this common one (so a
case that doesn't compile on a platform fails alone), then all of them
together. The math case's section is made by shadertest.c from its tables.
*/

// shadertest.c writes these, see T1TInput there
typedef struct {
    float x;
    float y;
    int i;
    int j;
} T1TInput;

struct T1TPixel {
    float4 position [[ position ]];
    float2 uv; // 0 to 1, the top left is 0, 0
};

struct T1TColorPixel {
    float4 position [[ position ]];
    float4 color;
};

/*
A struct with every kind of member whose place in a buffer differs between
APIs: float3 (16 bytes in MSL), float2, matrices, an array, a struct
*/
typedef struct {
    float2 p;
    uint q;
} T1TInner;

typedef struct {
    float a;
    float3 b;
    float2 c;
    uint d;
    float4x4 m;
    float arr[3];
    T1TInner inner;
    float3x3 n;
    int e;
} T1TLayout;

// One triangle covering the whole target
vertex T1TPixel t1t_full_vertex(uint vid [[ vertex_id ]]) {
    const float2 corners[3] = {
        float2(-1.0f, -1.0f), float2(3.0f, -1.0f), float2(-1.0f, 3.0f),
    };
    const float2 c = corners[vid];
    T1TPixel out;
    out.position = float4(c, 0.5f, 1.0f);
    out.uv = float2(c.x + 1.0f, 1.0f - c.y) * 0.5f;
    return out;
}

/*
The quadrant params[0] of the target (0 top left, 1 top right, 2 bottom
left, 3 bottom right) as 2 triangles, uv 0 to 1 inside it
*/
vertex T1TPixel t1t_quadrant_vertex(
    uint vid [[ vertex_id ]],
    constant u32 * params [[ buffer(T1_PARAMS_I) ]])
{
    const float2 corners[6] = {
        float2(0.0f, 0.0f), float2(1.0f, 0.0f), float2(1.0f, 1.0f),
        float2(0.0f, 0.0f), float2(1.0f, 1.0f), float2(0.0f, 1.0f),
    };
    const float2 c = corners[vid];
    const u32 q = params[0];
    const float2 uv =
        float2((float)(q % 2), (float)(q / 2)) * 0.5f + c * 0.5f;
    T1TPixel out;
    out.position = float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, 0.5f, 1.0f);
    out.uv = c;
    return out;
}

fragment float4 t1t_color_fragment(T1TColorPixel in [[ stage_in ]]) {
    return in.color;
}

//== orientation
// Pixel centers, which way y goes in a target and in clip space
fragment float4 t1t_orientation_fragment(T1TPixel in [[ stage_in ]]) {
    // red, green: the pixel's position, blue, alpha: the interpolated uv
    return float4(
        in.position.x / 32.0f,
        in.position.y / 32.0f,
        in.uv.x,
        in.uv.y);
}

// White, in the top left corner if clip space y goes up
vertex T1TColorPixel t1t_corner_vertex(uint vid [[ vertex_id ]]) {
    const float2 corners[3] = {
        float2(-1.0f, 1.0f), float2(0.0f, 1.0f), float2(-1.0f, 0.0f),
    };
    T1TColorPixel out;
    out.position = float4(corners[vid], 0.5f, 1.0f);
    out.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    return out;
}

//== cull
/*
2 triangles in the top half (params[0] 0) or the bottom half (1): the red
one on the left counterclockwise on screen, the green one clockwise
*/
vertex T1TColorPixel t1t_cull_vertex(
    uint vid [[ vertex_id ]],
    constant u32 * params [[ buffer(T1_PARAMS_I) ]])
{
    const float2 corners[6] = {
        float2(-0.9f, -0.9f), float2(-0.1f, -0.9f), float2(-0.5f, 0.9f),
        float2(0.1f, -0.9f), float2(0.5f, 0.9f), float2(0.9f, -0.9f),
    };
    const float2 c = corners[vid];
    const float center_y = params[0] == 0 ? 0.5f : -0.5f;
    T1TColorPixel out;
    out.position = float4(c.x, center_y + c.y * 0.5f, 0.5f, 1.0f);
    out.color = vid < 3 ?
        float4(1.0f, 0.0f, 0.0f, 1.0f) :
        float4(0.0f, 1.0f, 0.0f, 1.0f);
    return out;
}

//== perspective
// Perspective-correct interpolation (w 1, 2 and 4) and a flat value
struct T1TPerspectivePixel {
    float4 position [[ position ]];
    float4 color;
    float flat_value [[ flat ]];
};

vertex T1TPerspectivePixel t1t_perspective_vertex(uint vid [[ vertex_id ]]) {
    const float2 corners[3] = {
        float2(-0.9f, -0.9f), float2(0.9f, -0.9f), float2(0.0f, 0.9f),
    };
    const float ws[3] = { 1.0f, 2.0f, 4.0f };
    const float4 colors[3] = {
        float4(1.0f, 0.0f, 0.0f, 1.0f),
        float4(0.0f, 1.0f, 0.0f, 1.0f),
        float4(0.0f, 0.0f, 1.0f, 1.0f),
    };
    const float w = ws[vid];
    T1TPerspectivePixel out;
    out.position = float4(corners[vid] * w, 0.5f * w, w);
    out.color = colors[vid];
    out.flat_value = (float)vid * 0.25f + 0.25f;
    return out;
}

fragment float4 t1t_perspective_fragment(
    T1TPerspectivePixel in [[ stage_in ]])
{
    return float4(in.color.rgb, in.flat_value);
}

//== depth
/*
Quads (6 vertices each, pick them with "vertex_start"): 0 green, the top
half at z 0.5; 1 red, top left at z 0.7 (behind the green); 2 blue, top
right at z 0.3 (in front); 3 yellow, bottom left at z -0.25 and 4
magenta, bottom right at z 1.25 (both outside the 0 to 1 depth range)
*/
vertex T1TColorPixel t1t_depth_vertex(uint vid [[ vertex_id ]]) {
    const float2 corners[6] = {
        float2(0.0f, 0.0f), float2(1.0f, 0.0f), float2(1.0f, 1.0f),
        float2(0.0f, 0.0f), float2(1.0f, 1.0f), float2(0.0f, 1.0f),
    };
    // x0, y0, x1, y1 in clip space
    const float4 rects[5] = {
        float4(-1.0f, 0.0f, 1.0f, 1.0f),
        float4(-0.9f, 0.1f, -0.1f, 0.9f),
        float4(0.1f, 0.1f, 0.9f, 0.9f),
        float4(-0.9f, -0.9f, -0.1f, -0.1f),
        float4(0.1f, -0.9f, 0.9f, -0.1f),
    };
    const float zs[5] = { 0.5f, 0.7f, 0.3f, -0.25f, 1.25f };
    const float4 colors[5] = {
        float4(0.0f, 1.0f, 0.0f, 1.0f),
        float4(1.0f, 0.0f, 0.0f, 1.0f),
        float4(0.0f, 0.0f, 1.0f, 1.0f),
        float4(1.0f, 1.0f, 0.0f, 1.0f),
        float4(1.0f, 0.0f, 1.0f, 1.0f),
    };
    const uint quad = vid / 6;
    const float2 c = corners[vid % 6];
    const float4 r = rects[quad];
    T1TColorPixel out;
    out.position = float4(
        mix(r.x, r.z, c.x), mix(r.y, r.w, c.y), zs[quad], 1.0f);
    out.color = colors[quad];
    return out;
}

//== mrt
// Two color targets at once
struct T1TTwoColors {
    float4 a [[ color(0) ]];
    float4 b [[ color(1) ]];
};

fragment T1TTwoColors t1t_mrt_fragment(T1TPixel in [[ stage_in ]]) {
    T1TTwoColors out;
    out.a = float4(in.uv, 0.0f, 1.0f);
    out.b = float4(1.0f - in.uv.x, 0.25f, in.uv.y, 0.5f);
    return out;
}

//== sampling
/*
params[0] is a 4x4 texture, sampled from -0.25 to 1.25 (past both edges):
nearest top left, linear top right, linear repeating bottom left, and
T1_read() bottom right
*/
fragment float4 t1t_sampling_fragment(
    T1TPixel in [[ stage_in ]],
    T1Textures textures [[ texture(0) ]],
    constant u32 * params [[ buffer(T1_PARAMS_I) ]])
{
    const float2 q = floor(in.uv * 2.0f);
    const float2 local = in.uv * 2.0f - q;
    const float2 uv = local * 1.5f - 0.25f;
    const u32 tex = params[0];
    const int which = (int)q.x + 2 * (int)q.y;
    float4 r = float4(0.0f, 0.0f, 0.0f, 0.0f);
    if (which == 0) {
        r = T1_sample_nearest(textures, tex, uv);
    } else if (which == 1) {
        r = T1_sample_linear(textures, tex, uv);
    } else if (which == 2) {
        r = T1_sample_linear_repeat(textures, tex, uv);
    } else {
        const uint2 p = uint2(clamp(local * 4.0f, 0.0f, 3.0f));
        r = T1_read(textures, tex, p);
    }
    return r;
}

//== blend
// The same color with every blend mode, one quadrant each
fragment float4 t1t_blend_fragment(T1TPixel in [[ stage_in ]]) {
    return float4(0.6f, 0.3f, 0.1f, 0.5f);
}

//== discard
// Squares of 4x4 pixels, every other one discarded
fragment float4 t1t_discard_fragment(T1TPixel in [[ stage_in ]]) {
    const int2 p = int2(in.position.xy);
    if (((p.x / 4 + p.y / 4) & 1) == 0) {
        discard_fragment();
    }
    return float4(1.0f, 0.5f, 0.0f, 1.0f);
}

//== derivatives
// fwidth() of linear functions (the same everywhere) and of a curve
fragment float4 t1t_derivatives_fragment(T1TPixel in [[ stage_in ]]) {
    const float a = in.position.x * 0.25f + in.position.y * 0.125f;
    const float b = in.uv.x * 4.0f - in.uv.y * 2.0f;
    return float4(fwidth(a), fwidth(b), fwidth(a * a) * 0.01f, 1.0f);
}

//== compute_write
// A compute pass writing every pixel of its write_tex
kernel void t1t_compute_write_kernel(
    T1WriteTex out_tex [[ texture(T1_WRITE_TEX_I) ]],
    uint2 pos [[ thread_position_in_grid ]])
{
    const uint2 size = T1_write_tex_size(out_tex);
    const float4 c = float4(
        (float)pos.x / (float)(size.x - 1),
        (float)pos.y / (float)(size.y - 1),
        ((float)(pos.x * 8) + 0.5f) / 255.0f, // halfway between 2 levels
        1.0f);
    T1_write_tex_write(out_tex, pos, c);
}

//== compute_read_write
// Reads what a clear pass left in write_tex and writes it back changed
kernel void t1t_compute_read_write_kernel(
    T1WriteTex tex [[ texture(T1_WRITE_TEX_I) ]],
    uint2 pos [[ thread_position_in_grid ]])
{
    const float4 c = T1_write_tex_read(tex, pos);
    T1_write_tex_write(
        tex,
        pos,
        float4(1.0f - c.r, c.g + (float)pos.x / 64.0f, c.b, c.a * 0.5f));
}

//== layout
/*
Reads T1TLayouts byte for byte as MSL lays them out (the C side wrote
numbers into every word), writes each member to flat, and writes changed
copies back, whole and member by member
*/
kernel void t1t_layout_kernel(
    uint t [[ thread_position_in_grid ]],
    const device T1TLayout * in [[ buffer(T1_BUFS_I + 0) ]],
    device float * flat [[ buffer(T1_BUFS_I + 1) ]],
    device T1TLayout * out [[ buffer(T1_BUFS_I + 2) ]])
{
    T1TLayout l = in[t];
    const uint b = t * 64;
    flat[b + 0] = l.a;
    flat[b + 1] = l.b.x;
    flat[b + 2] = l.b.y;
    flat[b + 3] = l.b.z;
    flat[b + 4] = l.c.x;
    flat[b + 5] = l.c.y;
    flat[b + 6] = (float)l.d;
    for (uint col = 0; col < 4; col++) {
        for (uint row = 0; row < 4; row++) {
            flat[b + 7 + col * 4 + row] = l.m[col][row];
        }
    }
    flat[b + 23] = l.arr[0];
    flat[b + 24] = l.arr[1];
    flat[b + 25] = l.arr[2];
    flat[b + 26] = l.inner.p.x;
    flat[b + 27] = l.inner.p.y;
    flat[b + 28] = (float)l.inner.q;
    for (uint col = 0; col < 3; col++) {
        flat[b + 29 + col * 3 + 0] = l.n[col].x;
        flat[b + 29 + col * 3 + 1] = l.n[col].y;
        flat[b + 29 + col * 3 + 2] = l.n[col].z;
    }
    flat[b + 38] = (float)l.e;

    l.b.y = l.b.y + 100.0f;
    l.m[2][1] = -5.0f;
    l.inner.q = l.inner.q + 7;
    l.arr[1] = 0.5f;
    out[t] = l;
    out[t].a = 42.0f;
    out[t].arr[2] = -1.0f;
    out[t].inner.p = float2(3.0f, 4.0f);
}

//== control
// Statements: loops, switch, local arrays, swizzles, helpers, macros
#define T1T_SQUARE(v) ((v) * (v))

// constants at program scope, read by a helper and by the kernel
constant float t1t_table[4] = { 1.5f, -2.0f, 0.25f, 8.0f };
constant float2 t1t_offsets[3] = {
    float2(1.0f, 2.0f), float2(-3.0f, 0.5f), float2(10.0f, -20.0f),
};
constant int t1t_scale = 3;

static float t1t_table_at(int k) {
    return t1t_table[k % 4] * (float)t1t_scale;
}

static int t1t_fib(int n) {
    int a = 0;
    int b = 1;
    for (int k = 0; k < n; k++) {
        const int sum = a + b;
        a = b;
        b = sum;
    }
    return a;
}

kernel void t1t_control_kernel(
    uint t [[ thread_position_in_grid ]],
    device int * out [[ buffer(T1_BUFS_I + 0) ]])
{
    const int n = (int)t;
    const uint b = t * 20;

    // for with break and continue
    int sum = 0;
    for (int k = 0; k < 20; k++) {
        if (k == n) { break; }
        if ((k & 1) != 0) { continue; }
        sum += k;
    }
    out[b + 0] = sum;

    // while
    int w = n;
    int steps = 0;
    while (w > 1 && steps < 100) {
        w = (w & 1) != 0 ? 3 * w + 1 : w / 2;
        steps++;
    }
    out[b + 1] = steps;

    // switch, no fallthrough
    int s = 0;
    switch (n % 4) {
        case 0: s = 10; break;
        case 1: s = 20; break;
        case 2: { s = 30; s += n; } break;
        default: s = -1; break;
    }
    out[b + 2] = s;

    // a local array, indexed by a runtime value
    float arr[8];
    for (int k = 0; k < 8; k++) { arr[k] = (float)k * 1.5f; }
    out[b + 3] = (int)(arr[n % 8] * 2.0f);

    // swizzles, read and written
    float4 v = float4(1.0f, 2.0f, 3.0f, 4.0f);
    v.zx = float2(9.0f, 8.0f);
    v.y += 0.5f;
    const float4 v2 = v.wzyx + v.xxyy;
    out[b + 4] = (int)(v2.x * 10.0f + v2.y * 100.0f + v2.z * 1000.0f);
    out[b + 5] = (int)(v2.w * 10.0f);

    // ?: between vectors, bools
    const float2 p = n > 3 ? float2(1.0f, 2.0f) : float2(3.0f, 4.0f);
    const bool flag = n > 2 && n < 9;
    out[b + 6] = (int)(p.x * 10.0f + p.y) + (flag ? 100 : 0);

    // ++, --, compound operators
    int c = n;
    c++;
    c *= 3;
    c -= 2;
    c <<= 1;
    c %= 7;
    c |= 8;
    c ^= 3;
    --c;
    out[b + 7] = c;

    // helpers, macros, constants at program scope
    const float2 offset = t1t_offsets[n % 3];
    out[b + 8] = t1t_fib(n % 20) + T1T_SQUARE(n);
    out[b + 9] =
        (int)(t1t_table_at(n) * 4.0f) +
        (int)(offset.x * 100.0f + offset.y * 10000.0f);

    // matrices: m[i] is column i
    const float3x3 m = float3x3(
        float3(1.0f, 2.0f, 3.0f),
        float3(4.0f, 5.0f, 6.0f),
        float3(7.0f, 8.0f, 9.0f));
    out[b + 10] = (int)m[1][2] * 100 + (int)m[2].x;
    const float3 row_times_m = mul(float3(1.0f, (float)n, 0.0f), m);
    out[b + 11] = (int)(
        row_times_m.x + row_times_m.y * 100.0f + row_times_m.z * 10000.0f);
    const float3x3 mt = transpose(m);
    out[b + 12] = (int)mt[1][2] * 100 + (int)mt[2].x;
    const float3x3 mm = mul(m, mt);
    out[b + 13] = (int)(mm[0][1] + mm[2][0] * 1000.0f);

    // integer vectors
    const int2 iv = int2(n, -n) * int2(3, 5) + int2(1, 1);
    const uint3 uv3 = uint3(t, t * 2, t * 3) >> uint3(0, 1, 2);
    out[b + 14] = iv.x * 1000 + iv.y;
    out[b + 15] = (int)(uv3.x + uv3.y * 100 + uv3.z * 10000);

    // bool vectors in variables, from comparisons of vectors
    const float3 p3 = float3((float)n, 2.0f, (float)(n % 3));
    const bool3 bigger = p3 > float3(2.0f);
    const bool3 same = p3 == p3.yxz;
    bool2 flags = bool2(n > 4, false);
    flags = !flags;
    out[b + 16] =
        (bigger.x ? 1 : 0) + (bigger.y ? 2 : 0) + (bigger.z ? 4 : 0);
    out[b + 17] = (same.x ? 1 : 0) + (same.z ? 2 : 0);
    out[b + 18] = (flags.x ? 1 : 0) + (flags.y ? 2 : 0);
    out[b + 19] = any(int2(n, -n) > int2(5)) ? 1 : 0;
}

//== switch_fallthrough
// A case that runs on into the next one (FXC rejects these)
kernel void t1t_switch_fallthrough_kernel(
    uint t [[ thread_position_in_grid ]],
    device int * out [[ buffer(T1_BUFS_I + 0) ]])
{
    int s = 0;
    switch ((int)t % 3) {
        case 0: s = 10;
        case 1: s += 5; break;
        default: s = -1; break;
    }
    out[t] = s;
}

//== perf
/*
The performance cases: each runs alone for a few frames, its GPU time
compared to t1t_perf_alu's (see PERFORMANCE in shadertest.c)
*/
#define T1T_PERF_SIDE 1024

kernel void t1t_perf_alu(
    uint2 t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    float v = (float)(t.x ^ t.y) * 0.001f;
    for (int k = 0; k < 1024; k++) {
        v = v * 0.9999f + 0.25f;
    }
    out[t.y * T1T_PERF_SIDE + t.x] = v;
}

kernel void t1t_perf_transcendental(
    uint2 t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    float v = (float)(t.x ^ t.y) * 0.001f;
    for (int k = 0; k < 64; k++) {
        v = sin(v) * 1.5f + exp2(v * 0.1f) - sqrt(abs(v)) + pow(abs(v), 0.7f);
    }
    out[t.y * T1T_PERF_SIDE + t.x] = v;
}

kernel void t1t_perf_branches(
    uint2 t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    float v = (float)(t.x ^ t.y) * 0.001f;
    for (uint k = 0; k < 256; k++) {
        switch ((k + t.x) % 5) {
            case 0: v = v * 1.01f; break;
            case 1: v = v + 0.5f; break;
            case 2: v = v - 0.25f; break;
            case 3: v = abs(v) * 0.99f; break;
            default: v = -v; break;
        }
    }
    out[t.y * T1T_PERF_SIDE + t.x] = v;
}

// A table at program scope, indexed by values known only at runtime
constant float t1t_perf_table[16] = {
    0.1f, 0.7f, 0.3f, 0.9f, 0.2f, 0.8f, 0.4f, 0.6f,
    0.15f, 0.75f, 0.35f, 0.95f, 0.25f, 0.85f, 0.45f, 0.65f,
};

kernel void t1t_perf_global_table(
    uint2 t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    float v = 0.0f;
    uint k = t.x ^ t.y;
    for (int r = 0; r < 256; r++) {
        v = v * 0.5f + t1t_perf_table[k & 15];
        k = k * 1664525 + 1013904223;
    }
    out[t.y * T1T_PERF_SIDE + t.x] = v;
}

// The tables' loop without a table: what they're compared to
kernel void t1t_perf_hash_loop(
    uint2 t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    float v = 0.0f;
    uint k = t.x ^ t.y;
    for (int r = 0; r < 256; r++) {
        v = v * 0.5f + (float)(k & 15u) * 0.0625f;
        k = k * 1664525 + 1013904223;
    }
    out[t.y * T1T_PERF_SIDE + t.x] = v;
}

// The same table local to the function, to compare with the global one
kernel void t1t_perf_local_table(
    uint2 t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    const float table[16] = {
        0.1f, 0.7f, 0.3f, 0.9f, 0.2f, 0.8f, 0.4f, 0.6f,
        0.15f, 0.75f, 0.35f, 0.95f, 0.25f, 0.85f, 0.45f, 0.65f,
    };
    float v = 0.0f;
    uint k = t.x ^ t.y;
    for (int r = 0; r < 256; r++) {
        v = v * 0.5f + table[k & 15];
        k = k * 1664525 + 1013904223;
    }
    out[t.y * T1T_PERF_SIDE + t.x] = v;
}

// As struct_loads, plain floats at the same places: what it's compared to
kernel void t1t_perf_flat_loads(
    uint2 t [[ thread_position_in_grid ]],
    const device float * in [[ buffer(T1_BUFS_I + 0) ]],
    device float * out [[ buffer(T1_BUFS_I + 1) ]])
{
    float acc = 0.0f;
    const uint start = t.y * T1T_PERF_SIDE + t.x;
    for (uint k = 0; k < 16; k++) {
        const uint i = ((start + k * 97) & 1023) * 64;
        acc += in[i + 5] + in[i + 14] + in[i + 34] + in[i + 4] + in[i + 28];
    }
    out[start] = acc;
}

// 1024 T1TLayouts in buf[0]
kernel void t1t_perf_struct_loads(
    uint2 t [[ thread_position_in_grid ]],
    const device T1TLayout * in [[ buffer(T1_BUFS_I + 0) ]],
    device float * out [[ buffer(T1_BUFS_I + 1) ]])
{
    float acc = 0.0f;
    const uint start = t.y * T1T_PERF_SIDE + t.x;
    for (uint k = 0; k < 16; k++) {
        const T1TLayout l = in[(start + k * 97) & 1023];
        acc += l.b.y + l.m[1][2] + l.n[2].x + l.c.x + l.arr[1];
    }
    out[start] = acc;
}

// As struct_stores, plain floats: what it's compared to
kernel void t1t_perf_flat_stores(
    uint2 t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    const uint i = ((t.y * T1T_PERF_SIDE + t.x) & 1023) * 64;
    for (uint w = 0; w < 44; w++) {
        out[i + w] = (float)(t.x + w);
    }
}

// 1024 T1TLayouts in buf[0]
kernel void t1t_perf_struct_stores(
    uint2 t [[ thread_position_in_grid ]],
    device T1TLayout * out [[ buffer(T1_BUFS_I + 0) ]])
{
    const uint i = (t.y * T1T_PERF_SIDE + t.x) & 1023;
    T1TLayout l = out[i];
    l.a = (float)t.x;
    l.b = float3(1.0f, 2.0f, (float)t.y);
    l.m[3][3] = 1.0f;
    out[i] = l;
}

// params[0] is a 256x256 texture: 16 linear samples a pixel
fragment float4 t1t_perf_samples_fragment(
    T1TPixel in [[ stage_in ]],
    T1Textures textures [[ texture(0) ]],
    constant u32 * params [[ buffer(T1_PARAMS_I) ]])
{
    float4 acc = float4(0.0f, 0.0f, 0.0f, 0.0f);
    for (int k = 0; k < 16; k++) {
        const float2 offset = float2((float)k * 0.013f, (float)k * 0.007f);
        acc += T1_sample_linear(textures, params[0], in.uv + offset);
    }
    return acc / 16.0f;
}

kernel void t1t_perf_write_tex(
    T1WriteTex tex [[ texture(T1_WRITE_TEX_I) ]],
    uint2 pos [[ thread_position_in_grid ]])
{
    const float4 c = T1_write_tex_read(tex, pos);
    T1_write_tex_write(tex, pos, c * 0.5f + 0.25f);
}

fragment float4 t1t_perf_blend_fragment(T1TPixel in [[ stage_in ]]) {
    return float4(in.uv, 0.5f, 0.25f);
}

//== atomics
// Every atomic on 1024 threads: only what doesn't depend on who came first
static void t1t_atomics_tally(
    device uint * out,
    uint at)
{
    T1_atomic_add(out, at, 1u);
}

kernel void t1t_atomics_kernel(
    uint t [[ thread_position_in_grid ]],
    device uint * out [[ buffer(T1_BUFS_I + 0) ]])
{
    T1_atomic_add(out, 0, 1u);
    T1_atomic_min(out, 1, t + 5u);
    T1_atomic_max(out, 2, t + 5u);
    T1_atomic_or(out, 3, 1u << (t % 32u));
    T1_atomic_and(out, 4, ~(1u << (t % 32u)));
    T1_atomic_xor(out, 5, t * 3u);
    // all that was exchanged plus what's left is everything put in
    const uint old = T1_atomic_exchange(out, 6, t + 1u);
    T1_atomic_add(out, 7, old);
    // each of 32 slots lets exactly one thread in
    const uint slot = 8u + t % 32u;
    if (T1_atomic_compare_exchange(out, slot, 0u, t + 1u) == 0u) {
        T1_atomic_add(out, 40, 1u);
    }
    // through a helper, and read back through the old values: 1024 distinct
    t1t_atomics_tally(out, 41u + t % 16u);
    T1_atomic_store(out, 57, 77u);
    T1_atomic_add(out, 58, T1_atomic_load(out, 59));
}

kernel void t1t_atomics_settle_kernel(
    uint t [[ thread_position_in_grid ]],
    device uint * out [[ buffer(T1_BUFS_I + 0) ]])
{
    out[7] = out[7] + out[6];
    out[6] = 0u;
    for (uint s = 8u; s < 40u; s++) { out[s] = out[s] > 0u ? 1u : 0u; }
}

//== threadgroup_1d
// Each thread writes its slot, then reads its neighbour's after the barrier
kernel void t1t_threadgroup_1d_kernel(
    uint t [[ thread_position_in_grid ]],
    uint local [[ thread_position_in_threadgroup ]],
    uint group [[ threadgroup_position_in_grid ]],
    device uint * out [[ buffer(T1_BUFS_I + 0) ]])
{
    threadgroup uint slots[256];
    slots[local] = group * 1000u + local;
    T1_threadgroup_barrier();
    out[t] = slots[(local + 1u) % 256u];
}

//== threadgroup_2d
kernel void t1t_threadgroup_2d_kernel(
    uint2 t [[ thread_position_in_grid ]],
    uint2 local [[ thread_position_in_threadgroup ]],
    uint2 group [[ threadgroup_position_in_grid ]],
    device uint * out [[ buffer(T1_BUFS_I + 0) ]])
{
    threadgroup float2 slots[256];
    slots[local.y * 16u + local.x] = float2(
        (float)group.x, (float)(group.y * 100u + local.x));
    T1_threadgroup_barrier();
    const float2 v = slots[(15u - local.y) * 16u + (15u - local.x)];
    out[t.y * 32u + t.x] = (uint)v.x + (uint)v.y * 2u;
}

//== indirect
// One pass says how many groups the next ones run, 4 bytes in
kernel void t1t_indirect_args_kernel(
    uint t [[ thread_position_in_grid ]],
    device uint * args [[ buffer(T1_BUFS_I + 0) ]])
{
    args[1] = 3u; args[2] = 1u; args[3] = 1u; // 3 groups of 256
    args[5] = 2u; args[6] = 1u; args[7] = 1u; // 2 groups of 16 x 16
}

kernel void t1t_indirect_1d_kernel(
    uint t [[ thread_position_in_grid ]],
    device uint * out [[ buffer(T1_BUFS_I + 0) ]])
{
    out[t] = t + 1u;
}

kernel void t1t_indirect_2d_kernel(
    uint2 t [[ thread_position_in_grid ]],
    device uint * out [[ buffer(T1_BUFS_I + 0) ]])
{
    out[t.y * 32u + t.x] = t.y * 1000u + t.x + 1u;
}

//== indexed_writes
// Writes through indices the compiler doesn't know: FXC can't do these, so
// T1 sets the whole vector or matrix
typedef struct {
    float3 v;
    float4x4 m;
} T1TIndexed;

kernel void t1t_indexed_writes_kernel(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]],
    device float3 * out3 [[ buffer(T1_BUFS_I + 1) ]],
    device T1TIndexed * outs [[ buffer(T1_BUFS_I + 2) ]])
{
    uint i = t % 3u;
    uint j = (t + 1u) % 3u;
    uint b = t * 64u;

    float3 v = float3(1.0f, 2.0f, 3.0f);
    v[i] = 5.0f;
    v[j] += 2.0f;
    v[i]++;
    out[b + 0u] = v.x; out[b + 1u] = v.y; out[b + 2u] = v.z;

    float3x3 m = float3x3(2.0f);
    m[i] = float3(7.0f, 8.0f, 9.0f);
    m[j] += float3(1.0f, 1.0f, 1.0f);
    m[i][j] = 11.0f;
    m[1][i] *= 2.0f;
    m[j][1] = 13.0f;
    m[i].x = 17.0f;
    for (uint c = 0u; c < 3u; c++) {
        out[b + 3u + c * 3u] = m[c].x;
        out[b + 4u + c * 3u] = m[c].y;
        out[b + 5u + c * 3u] = m[c].z;
    }

    float3 a[3];
    a[0] = float3(0.0f, 1.0f, 2.0f);
    a[1] = float3(3.0f, 4.0f, 5.0f);
    a[2] = float3(6.0f, 7.0f, 8.0f);
    a[1][i] = 20.0f;
    a[j][i] += 30.0f;
    for (uint c = 0u; c < 3u; c++) {
        out[b + 12u + c * 3u] = a[c].x;
        out[b + 13u + c * 3u] = a[c].y;
        out[b + 14u + c * 3u] = a[c].z;
    }

    T1TIndexed s;
    s.v = float3(1.0f, 1.0f, 1.0f);
    s.m = float4x4(3.0f);
    s.v[i] += 40.0f;
    s.m[i] = float4(50.0f, 51.0f, 52.0f, 53.0f);
    s.m[j][i] = 60.0f;
    out[b + 21u] = s.v.x; out[b + 22u] = s.v.y; out[b + 23u] = s.v.z;
    for (uint c = 0u; c < 4u; c++) {
        out[b + 24u + c * 4u] = s.m[c].x;
        out[b + 25u + c * 4u] = s.m[c].y;
        out[b + 26u + c * 4u] = s.m[c].z;
        out[b + 27u + c * 4u] = s.m[c].w;
    }

    out3[t] = float3(1.0f, 2.0f, 3.0f);
    out3[t][i] = 70.0f;
    out3[t][j] += 80.0f;

    outs[t].v = float3(4.0f, 5.0f, 6.0f);
    outs[t].m = float4x4(1.0f);
    outs[t].v[i] = 90.0f;
    outs[t].m[i] = float4(100.0f, 101.0f, 102.0f, 103.0f);
    outs[t].m[j][i] += 110.0f;
    outs[t].m[i].y = 120.0f;

    // (read back here: a float3's fourth word isn't anyone's)
    out[b + 40u] = out3[t].x;
    out[b + 41u] = out3[t].y;
    out[b + 42u] = out3[t].z;
    out[b + 43u] = outs[t].v.x;
    out[b + 44u] = outs[t].v.y;
    out[b + 45u] = outs[t].v.z;
    for (uint c = 0u; c < 4u; c++) {
        out[b + 46u + c * 4u] = outs[t].m[c].x;
        out[b + 47u + c * 4u] = outs[t].m[c].y;
        out[b + 48u + c * 4u] = outs[t].m[c].z;
        out[b + 49u + c * 4u] = outs[t].m[c].w;
    }
    out[b + 62u] = 1.0f;
    out[b + 63u] = 2.0f;
}

//== small_named_buffers
// Writable uint buffers of the smallest sizes, named like the locals of T1's
// atomics helpers. Every buffer's first word counts the threads
kernel void t1t_small_named_buffers_kernel(
    uint t [[ thread_position_in_grid ]],
    device uint * o [[ buffer(T1_BUFS_I + 0) ]],
    device uint * i [[ buffer(T1_BUFS_I + 1) ]],
    device uint * v [[ buffer(T1_BUFS_I + 2) ]],
    device uint * e [[ buffer(T1_BUFS_I + 3) ]])
{
    T1_atomic_add(o, 0u, 1u);
    T1_atomic_add(i, 0u, 2u);
    T1_atomic_add(v, 0u, 3u);
    T1_atomic_add(e, 0u, t + 1u);
}

//== threadgroup_struct
// threadgroup memory of a struct: its declaration must come after the struct
typedef struct {
    float a;
    float3 b;
} T1TSlot;

kernel void t1t_threadgroup_struct_kernel(
    uint t [[ thread_position_in_grid ]],
    uint local [[ thread_position_in_threadgroup ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    threadgroup T1TSlot slots[256];
    slots[local].a = (float)local;
    slots[local].b = float3(1.0f, 2.0f, (float)local);
    slots[local].b[1] = 7.0f;
    T1_threadgroup_barrier();
    T1TSlot other = slots[(local + 1u) % 256u];
    out[t] = other.a + other.b.y + other.b.z * 2.0f;
}
