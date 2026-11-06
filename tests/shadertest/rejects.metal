//== about
/*
Shaders T1 must refuse: each section is one, alone, with the start of the
message it must give (a "// why:" line). Most are fine MSL, so Metal alone
would take them, but T1 can't translate them for the other GPU APIs: the
subset must catch them on every platform, before the shader ever runs.

A case that's accepted means the subset grew: if that's on purpose, it
must be translated for every API too (and tested in shadertest.metal),
then its case here goes. (What Metal itself refuses isn't here: it
can't slip through.)

shadertest.c runs these first, see REJECTS there.
*/

//== include
// why: no #include
#include <metal_stdlib>
kernel void k(uint t [[ thread_position_in_grid ]]) {}

//== using_namespace
// why: no using
using namespace metal;
kernel void k(uint t [[ thread_position_in_grid ]]) {}

//== half
// why: floats only, no half
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I) ]])
{
    half h = 1.0h;
    out[t] = (float)h;
}

//== double
// why: no double
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I) ]])
{
    double d = 1.0;
    out[t] = (float)d;
}

//== short
// why: 32-bit numbers only
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I) ]])
{
    short s = 3;
    out[t] = (float)s;
}

//== uchar
// why: 32-bit numbers only
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device uchar * out [[ buffer(T1_BUFS_I) ]])
{
    out[t] = 3;
}

//== own_sampler
// why: no samplers of your own
constexpr sampler s(filter::linear);
kernel void k(uint t [[ thread_position_in_grid ]]) {}

//== texture_type
// why: use T1Textures or T1WriteTex
fragment float4 f(
    float4 p [[ position ]],
    texture2d<float> tex [[ texture(0) ]])
{
    return float4(1.0f);
}

//== array_template
// why: no array<>
fragment float4 f(
    float4 p [[ position ]],
    array<texture2d<float>, 2> texs [[ texture(0) ]])
{
    return float4(1.0f);
}

//== threadgroup_memory_nested
// why: threadgroup memory only as threadgroup Type name[N];
kernel void k(uint t [[ thread_position_in_grid ]]) {
    if (t > 0) {
        threadgroup float shared[64];
        shared[t % 64] = 1.0f;
    }
}

//== threadgroup_memory_in_helper
// why: threadgroup memory only as threadgroup Type name[N];
static float f(uint t) {
    threadgroup float shared[64];
    return shared[t % 64];
}
kernel void k(uint t [[ thread_position_in_grid ]]) { f(t); }

//== threadgroup_barrier_in_if
// why: T1_threadgroup_barrier() only at the top
kernel void k(uint t [[ thread_position_in_grid ]]) {
    if (t > 0) { T1_threadgroup_barrier(); }
}

//== atomic_in_fragment
// why: T1_atomic_*() only in kernels and their helpers
fragment float4 f(
    float4 p [[ position ]],
    device uint * c [[ buffer(T1_BUFS_I) ]])
{
    return float4((float)T1_atomic_add(c, 0, 1u));
}

//== atomic_on_floats
// why: T1_atomic_*() takes a writable buffer of uints
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * c [[ buffer(T1_BUFS_I) ]])
{
    T1_atomic_add(c, 0, 1u);
}

//== atomic_on_read_only
// why: T1_atomic_*() takes a writable buffer of uints
kernel void k(
    uint t [[ thread_position_in_grid ]],
    const device uint * c [[ buffer(T1_BUFS_I) ]])
{
    T1_atomic_add(c, 0, 1u);
}

//== thread_address_space
// why: no address spaces except buffer parameters
static float twice(thread float & x) { return x * 2.0f; }
kernel void k(uint t [[ thread_position_in_grid ]]) {}

//== device_local
// why: address spaces only for buffer parameters
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I) ]])
{
    device float * p = out;
    p[t] = 1.0f;
}

//== constant_in_function
// why: address spaces only for buffer parameters
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I) ]])
{
    constant float table[2] = { 1.0f, 2.0f };
    out[t] = table[t % 2];
}

//== template
// why: no templates
template <typename T> T twice(T x) { return x * 2; }
kernel void k(uint t [[ thread_position_in_grid ]]) {}

//== class
// why: C, not C++
class Thing { float x; };
kernel void k(uint t [[ thread_position_in_grid ]]) {}

//== static_cast
// why: C, not C++, cast with (type)
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I) ]])
{
    out[t] = static_cast<float>(t);
}

//== reinterpret_cast
// why: C, not C++, cast with as_type<type>()
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I) ]])
{
    out[t] = reinterpret_cast<float>(t);
}

//== auto
// why: C, not C++
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I) ]])
{
    auto v = 1.0f;
    out[t] = v;
}

//== enum
// why: no enums, use #define
enum Mode { MODE_A, MODE_B };
kernel void k(uint t [[ thread_position_in_grid ]]) {}

//== union
// why: no unions
union Bits { float f; uint u; };
kernel void k(uint t [[ thread_position_in_grid ]]) {}

//== goto
// why: no goto
kernel void k(uint t [[ thread_position_in_grid ]]) {
    goto done;
    done:
    return;
}

//== sizeof
// why: no sizeof
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device uint * out [[ buffer(T1_BUFS_I) ]])
{
    out[t] = sizeof(float4);
}

//== typedef
// why: no typedefs except typedef struct
typedef float my_float;
kernel void k(uint t [[ thread_position_in_grid ]]) {}

//== unknown_function
// why: unknown function, not one T1 can translate
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device uint * out [[ buffer(T1_BUFS_I) ]])
{
    out[t] = popcount(t);
}

//== msl_derivative
// why: unknown function, not one T1 can translate
fragment float4 f(float4 p [[ position ]]) {
    return float4(dfdx(p.x), 0.0f, 0.0f, 1.0f);
}

//== method
// why: no methods, use the T1_ helpers
fragment float4 f(
    float4 p [[ position ]],
    T1Textures textures [[ texture(0) ]])
{
    return textures[0].read(uint2(0, 0), 0);
}

//== arrow
// why: no pointers, use buffer[i].field
typedef struct { float x; } Thing;
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device Thing * things [[ buffer(T1_BUFS_I) ]])
{
    things->x = 1.0f;
}

//== namespace_qualified
// why: no namespaces
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I) ]])
{
    out[t] = metal::abs(-1.0f);
}

//== local_pointer
// why: pointers only as device or constant buffer parameters
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I) ]])
{
    float x = 1.0f;
    float * p = &x;
    out[t] = x;
}

//== address_of
// why: no & (addresses or references)
static void set(thread float & x) { x = 1.0f; }
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I) ]])
{
    out[t] = 0.0f;
}

//== matrix_star
// why: multiply matrices with mul(v, m), not *
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I) ]])
{
    float2x2 m = float2x2(float2(1.0f, 0.0f), float2(0.0f, 1.0f));
    float2 v = m * float2(1.0f, 2.0f);
    out[t] = v.x;
}

//== unknown_attribute
// why: unsupported attribute
kernel void k(
    uint t [[ thread_position_in_grid ]],
    uint g [[ threads_per_threadgroup ]])
{
}

//== preprocessor_line
// why: unsupported preprocessor line
#line 5
kernel void k(uint t [[ thread_position_in_grid ]]) {}

//== buffer_dereference
// why: use a buffer as name[i] or pass it on whole
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I) ]])
{
    float * p = out;
}

//== helper_buffer_name
// why: a helper's buffer
static float first(const device float * values) { return values[0]; }
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I) ]])
{
    out[t] = first(out);
}

//== vertex_returns_float4
// why: a vertex function returns a struct
vertex float4 v(uint vid [[ vertex_id ]]) {
    return float4(0.0f, 0.0f, 0.0f, 1.0f);
}

//== as_type_64_bits
// why: unsupported as_type<>, use a 32-bit type
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I) ]])
{
    float2 f = as_type<float2>(uint2(t, t));
    out[t] = f.x;
}

//== buffer_increment
// why: write buffer elements as name[i] op= value;
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I) ]])
{
    out[t]++;
}

//== buffer_increment_before
// why: write buffer elements as name[i] op= value;
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I) ]])
{
    ++out[t];
}

//== buffer_write_in_expression
// why: write buffer elements as name[i] op= value;
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I) ]])
{
    float v = (out[t] = 2.0f);
}

//== buffer_index_outside_pass
// why: buffer(n) needs T1_FRAME_CONSTS_I, T1_PARAMS_I or a constant
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(3) ]])
{
    out[t] = 1.0f;
}

//== eight_writable_buffers
// why: at most 7 writable buffers per kernel
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * a [[ buffer(T1_BUFS_I + 0) ]],
    device float * b [[ buffer(T1_BUFS_I + 1) ]],
    device float * c [[ buffer(T1_BUFS_I + 2) ]],
    device float * d [[ buffer(T1_BUFS_I + 3) ]],
    device float * e [[ buffer(T1_BUFS_I + 4) ]],
    device float * f [[ buffer(T1_BUFS_I + 5) ]],
    device float * g [[ buffer(T1_BUFS_I + 6) ]],
    device float * h [[ buffer(T1_BUFS_I + 7) ]])
{
    a[t] = 1.0f; b[t] = 1.0f; c[t] = 1.0f; d[t] = 1.0f;
    e[t] = 1.0f; f[t] = 1.0f; g[t] = 1.0f; h[t] = 1.0f;
}

//== write_tex_in_fragment
// why: only kernels have a T1WriteTex
fragment float4 f(
    float4 p [[ position ]],
    T1WriteTex out_tex [[ texture(T1_WRITE_TEX_I) ]])
{
    return T1_write_tex_read(out_tex, uint2(0, 0));
}

//== threadgroup_same_place
// why: every thread writes the same threadgroup memory
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    threadgroup float s[256];
    s[0] = (float)t;
    T1_threadgroup_barrier();
    out[t] = s[0];
}

//== threadgroup_same_for_the_group
// why: every thread writes the same threadgroup memory
kernel void k(
    uint t [[ thread_position_in_grid ]],
    uint group [[ threadgroup_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    threadgroup float s[256];
    s[group % 256u] = 1.0f;
    out[t] = s[0];
}

//== threadgroup_loop_over_all
// why: every thread writes the same threadgroup memory
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    threadgroup float s[256];
    for (uint i = 0u; i < 256u; i++) { s[i] = 0.0f; }
    out[t] = s[0];
}

//== threadgroup_vector_variable_index
// why: threadgroup memory's vectors take literal indices only
kernel void k(
    uint t [[ thread_position_in_grid ]],
    uint local [[ thread_position_in_threadgroup ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    threadgroup float3 s[256];
    s[local][t % 3u] = 1.0f;
    out[t] = 1.0f;
}

//== threadgroup_never_written
// why: nothing writes this threadgroup memory
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device uint * out [[ buffer(T1_BUFS_I + 0) ]])
{
    threadgroup uint s[256];
    T1_threadgroup_barrier();
    out[t] = s[0];
}

//== index_with_side_effect_before_component
// why: keep the indices before it plain
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    float3 a[3];
    uint i = 0u;
    a[i++][t % 3u] = 2.0f;
    out[t] = a[0].x;
}

//== side_effect_after_and
// why: HLSL runs both sides of && and ||
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    uint i = 0u;
    if (t > 3u && (i++ > 2u)) { out[t] = 1.0f; }
}

//== writing_call_after_or
// why: HLSL runs both sides of && and ||
float write_it(device float * out, uint t) {
    out[t] = 9.0f;
    return 1.0f;
}
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    if (t > 3u || write_it(out, t) > 0.0f) { out[t] = 2.0f; }
}

//== side_effect_in_ternary
// why: HLSL runs both sides of ?:
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    uint i = 0u;
    uint j = t > 3u ? i++ : 7u;
    out[t] = (float)(i + j);
}

//== ternary_between_structs
// why: ?: between numbers and vectors only
typedef struct { float a; } S;
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    S a; S b; a.a = 1.0f; b.a = 2.0f;
    S r = t > 3u ? a : b;
    out[t] = r.a;
}

//== ternary_vector_condition
// why: ?: takes one bool
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    float3 v = float3(1.0f, 2.0f, 3.0f);
    float3 w = v > float3(1.0f, 1.0f, 1.0f) ? v : v;
    out[t] = w.x;
}

//== for_variable_over_another
// why: a for's variable can't have the name of one around it
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    uint i = 5u;
    for (uint i = 0u; i < 3u; i++) { out[t] = 1.0f; }
    out[t] = (float)i;
}

//== for_variable_before_another
// why: a for's variable can't have the name of one around it
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    for (uint i = 0u; i < 3u; i++) { out[t] = 1.0f; }
    float i = 2.0f;
    out[t] = i;
}

//== for_variable_over_parameter
// why: a for's variable can't have the name of one around it
float f(uint i) { for (uint i = 0u; i < 2u; i++) { } return 1.0f; }
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    out[t] = f(t);
}

//== for_variable_inside_itself
// why: a for's variable can't have the name of one around it
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    for (uint i = 0u; i < 2u; i++) {
        for (uint i = 0u; i < 2u; i++) { out[t] = 1.0f; }
    }
}

//== initializer_short_of_elements
// why: an initializer lists every element
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    float a[4] = {0.0f};
    out[t] = a[0];
}

//== initializer_empty
// why: an initializer lists every element
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    float a[4] = {};
    out[t] = a[0];
}

//== initializer_short_of_members
// why: an initializer lists every element
typedef struct { float a; float b; } Q;
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    Q q = {1.0f};
    out[t] = q.a;
}

//== array_assigned
// why: arrays can't be assigned
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    float a[2];
    a[0] = 0.0f; a[1] = 0.0f;
    float b[2];
    b = a;
    out[t] = b[0];
}

//== array_parameter
// why: no arrays as parameters
float f(float a[2]) { return a[0]; }
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    float a[2];
    a[0] = 0.0f; a[1] = 0.0f;
    out[t] = f(a);
}

//== missing_return
// why: every path must return a value
float f(int x) { if (x > 0) { return 1.0f; } }
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    out[t] = f((int)t);
}

//== recursion
// why: no recursion
float f(int x) { return x > 0 ? f(x - 1) : 0.0f; }
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    out[t] = f(3);
}

//== recursion_through_another
// why: no recursion
float g(int x);
float f(int x) { return g(x); }
float g(int x) { return f(x); }
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    out[t] = f(3);
}

//== vector_cut_short
// why: a vector or matrix takes only its own type
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    float4 w = float4(1.0f, 2.0f, 3.0f, 4.0f);
    float3 v = w;
    out[t] = v.x;
}

//== vector_converted
// why: a vector or matrix takes only its own type
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    int3 a = int3(1, 2, 3);
    float3 v = a;
    out[t] = v.x;
}

//== vector_condition
// why: a condition is one bool
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    float3 v = float3(1.0f, 2.0f, 3.0f);
    if (v == v) { out[t] = 1.0f; }
}

//== number_condition
// why: a condition is a bool
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    if (t) { out[t] = 1.0f; }
}

//== not_on_number
// why: ! takes a bool
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    bool b = !t;
    out[t] = b ? 1.0f : 0.0f;
}

//== math_on_bool
// why: no math on bools
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    bool b = t > 3u;
    uint i = b + 1u;
    out[t] = (float)i;
}

//== float_into_integer
// why: a float doesn't become an integer unasked
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    int i = 1.5f;
    out[t] = (float)i;
}

//== percent_on_floats
// why: % is for integers
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    float x = 3.0f;
    x = x % 2.0f;
    out[t] = x;
}

//== swizzle_on_number
// why: no swizzles on a single number
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    float x = 1.0f;
    float3 v = x.xxx;
    out[t] = v.x;
}

//== swizzle_write_repeats
// why: a swizzle written to can't repeat a component
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    float3 v = float3(1.0f, 2.0f, 3.0f);
    v.xx = float2(1.0f, 2.0f);
    out[t] = v.x;
}

//== struct_in_function
// why: structs at program scope only
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    struct L { float a; };
    L l;
    l.a = 1.0f;
    out[t] = l.a;
}

//== fwidth_in_kernel
// why: only fragment functions have this
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    out[t] = fwidth(1.0f);
}

//== discard_through_helper
// why: only fragment functions have this
void h() { discard_fragment(); }
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    h();
    out[t] = 1.0f;
}

//== name_metal_refuses
// why: not a name Metal takes
kernel void k(
    uint t [[ thread_position_in_grid ]],
    device float * out [[ buffer(T1_BUFS_I + 0) ]])
{
    float register = 1.0f;
    out[t] = register;
}
