/*
SHADERTEST: checks a GPU backend of T1 against Metal's.

T1's shaders are written once in its MSL subset and translated for the other
GPU APIs. One app looking right on a new backend doesn't prove much: it
only uses some of the subset. This runs a case for every part of it and
compares the results with the ones Metal gave (reference.bin, next to this
file), so a port can tell what still differs:
- math: every builtin function and operator, on tricky inputs (negatives,
  halfway values, overflow), compared number by number with a tolerance
  each (GPUs may round transcendentals differently, never floor() or ints)
- statements: loops, switch, local arrays, swizzles, matrices (columns!),
  helpers, macros, constants at program scope
- layout: a struct with float3s, matrices, an array and a struct in it,
  read from a buffer and written back, byte for byte as MSL lays it out
- pictures (32x32, read back): pixel centers and which way y goes, culling,
  perspective-correct and flat interpolation, the depth test and depth
  range, 2 color targets, the 3 samplers and T1_read() past the edges,
  every blend mode, discard_fragment(), fwidth(), compute passes writing
  and reading their write_tex. A channel may differ by a little (each
  case's tolerance, of 255), and failures are saved as .ppm pictures in
  failures/ (got and Metal's) to look at
- REJECTS: shaders outside the subset (rejects.metal), which T1 must
  refuse on every platform with the right message, though Metal would run
  most of them: the subset may only grow on purpose, translated everywhere
- PERFORMANCE: a few heavy passes (math, transcendentals, branches, tables
  at program scope and local ones, struct loads and stores, texture samples, write_tex, blending) each run alone for
  some frames, timed on the GPU (T1_gpu_frame_us()), divided by the time of
  a plain arithmetic pass on the same GPU. That ratio is about the same on
  any machine, unlike the times: a case more than SLOW_FACTOR times
  Metal's ratio is reported SLOW (a translation that's much worse)

Every case's shaders are compiled on their own first, so a case that
doesn't compile on a backend fails alone (with the compiler's message) and
the others still run.

Build and run (the shaders and the reference are read from
tests/shadertest/, found from the tok_one folder or from the program's
place in build/tests/, so it runs from anywhere):
    gcc concat.c -o concat && ./concat
    ./build/tests/shadertest            all of it
    ./build/tests/shadertest noperf     without the performance cases
    ./build/tests/shadertest record     makes reference.bin (on Metal only,
                                        a Mac that isn't busy: perf too)
It ends by itself, printing each case and a summary, and returns 0 if
everything passed. Its window shows nothing.

(concat builds it like the examples, with
    gcc -std=c11 -O3 tests/shadertest/shadertest.c tests/shadertest/T1.c
        -o build/tests/shadertest)

Adding a case: its shaders in a section of shadertest.metal, a setup
function below that makes its passes and says what to read back (expect_*),
listed in check_setups[] (or a perf_setups[] entry), then record again on
a Mac.
*/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#if defined(_WIN32)
#include <direct.h>
#define t1t_mkdir(path) _mkdir(path)
#else
#define t1t_mkdir(path) mkdir(path, 0777)
#endif

#include "T1.h"

/*
Its files, in tests/shadertest/ of the tok_one folder: found from where
you are (the tok_one folder) or else from where the program is (in
build/tests/)
*/
static char shaders_path[1024];
static char rejects_path[1024];
static char reference_path[1024];
static char failures_dir[1024];

static uint8_t file_exists(const char * path) {
    FILE * f = fopen(path, "rb");
    if (f == NULL) { return 0; }
    fclose(f);
    return 1;
}

static uint8_t find_files(const char * argv0) {
    char roots[2][1024];
    snprintf(roots[0], sizeof(roots[0]), "%s", "");
    // the program's folder (build/tests/), then 2 folders up
    snprintf(roots[1], sizeof(roots[1]), "%s", argv0);
    char * slash = NULL;
    for (char * ch = roots[1]; *ch != '\0'; ch++) {
        if (*ch == '/' || *ch == '\\') { slash = ch; }
    }
    if (slash != NULL) {
        slash[1] = '\0';
    } else {
        roots[1][0] = '\0';
    }
    size_t used = strlen(roots[1]);
    snprintf(roots[1] + used, sizeof(roots[1]) - used, "%s", "../../");
    for (uint32_t r = 0; r < 2; r++) {
        snprintf(shaders_path, sizeof(shaders_path),
            "%stests/shadertest/shadertest.metal", roots[r]);
        if (!file_exists(shaders_path)) { continue; }
        snprintf(rejects_path, sizeof(rejects_path),
            "%stests/shadertest/rejects.metal", roots[r]);
        snprintf(reference_path, sizeof(reference_path),
            "%stests/shadertest/reference.bin", roots[r]);
        snprintf(failures_dir, sizeof(failures_dir),
            "%stests/shadertest/failures", roots[r]);
        return 1;
    }
    return 0;
}

#define SIDE 32 // of the pictures
#define SLOW_FACTOR 4.0f
// a perf case must take at least this much longer than an empty frame
// before it can be SLOW: below, the times are mostly noise
#define SLOW_MIN_US 200

static char * fatal_error = NULL;

static void fail_fatal(const char * where) {
    printf("FATAL (%s): %s\n", where, fatal_error ? fatal_error : "?");
    exit(2);
}

/*
THE SHADERS: shadertest.metal's sections plus the math section, made from
the tables below
*/
#define SECTIONS_CAP 64
typedef struct {
    char name[64];
    char * text;
    uint8_t compiled; // on its own, after the common section
    char error[512];
} Section;
typedef struct {
    Section items[SECTIONS_CAP];
    uint32_t size;
} Sections;
static Sections cases; // shadertest.metal's
static Sections rejects; // rejects.metal's, see REJECTS

static Section * section_named(const char * name) {
    for (uint32_t s = 0; s < cases.size; s++) {
        if (strcmp(cases.items[s].name, name) == 0) { return &cases.items[s]; }
    }
    return NULL;
}

static char * read_text_file(const char * path) {
    FILE * f = fopen(path, "rb");
    if (f == NULL) { return NULL; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char * text = malloc((size_t)size + 1);
    size_t read = fread(text, 1, (size_t)size, f);
    fclose(f);
    text[read] = '\0';
    // CRLF checkouts: drop the CRs
    char * to = text;
    for (char * from = text; *from != '\0'; from++) {
        if (*from != '\r') { *to++ = *from; }
    }
    *to = '\0';
    return text;
}

static void add_section(
    Sections * list, const char * name, const char * start, size_t size)
{
    if (list->size >= SECTIONS_CAP) {
        printf("too many sections, raise SECTIONS_CAP\n");
        exit(2);
    }
    Section * s = &list->items[list->size++];
    memset(s, 0, sizeof(Section));
    snprintf(s->name, sizeof(s->name), "%s", name);
    s->text = malloc(size + 1);
    memcpy(s->text, start, size);
    s->text[size] = '\0';
}

// The next "//== " at the start of a line, from at on
static const char * next_marker(const char * text, const char * at) {
    const char * marker = "//== ";
    const char * found = strstr(at, marker);
    while (found != NULL && found != text && found[-1] != '\n') {
        found = strstr(found + 1, marker);
    }
    return found;
}

// Splits text at its "//== name" lines
static void split_sections(Sections * list, const char * text) {
    const char * at = next_marker(text, text);
    while (at != NULL) {
        const char * name = at + 5;
        const char * name_end = strchr(name, '\n');
        if (name_end == NULL) { break; }
        const char * next = next_marker(text, name_end);
        const char * end = next != NULL ? next : name_end + strlen(name_end);
        char section_name[64];
        size_t name_size = (size_t)(name_end - name);
        if (name_size >= sizeof(section_name)) {
            name_size = sizeof(section_name) - 1;
        }
        memcpy(section_name, name, name_size);
        section_name[name_size] = '\0';
        add_section(
            list, section_name, name_end + 1, (size_t)(end - name_end - 1));
        at = next;
    }
}

/*
MATH: every thread gets one T1TInput (x, y, i, j) and evaluates each
expression of the tables on it
*/
typedef struct {
    float x;
    float y;
    int32_t i;
    int32_t j;
} T1TInput; // as in shadertest.metal

#define MATH_INPUTS 32
static const float math_x[MATH_INPUTS] = {
    0.0f, 0.5f, -0.5f, 1.5f, -1.5f, 2.5f, -2.5f, 0.25f,
    -0.75f, 1.0f, -1.0f, 3.14159f, -2.71828f, 10.3f, -10.7f, 100.5f,
    -0.001f, 0.001f, 7.0f, -7.0f, 0.999f, 10000.5f, -123.456f, 0.3333f,
    2.0f, 4.75f, -4.25f, 0.1f, 64.0f, -64.5f, 0.6f, 0.00001f,
};
static const float math_y[MATH_INPUTS] = {
    1.0f, 0.75f, -0.75f, 2.0f, -2.0f, 3.0f, 0.5f, -0.5f,
    1.25f, 4.0f, -4.0f, 0.3f, 10.0f, -10.0f, 7.5f, 0.1f,
    -0.1f, 1.5f, -3.5f, 2.5f, 0.125f, -1.0f, 5.0f, 0.6f,
    -0.6f, 8.0f, -8.0f, 1.0f, 0.25f, -0.25f, 3.3f, 9.0f,
};
// j is never 0 or -1 (i / j would be undefined for some i)
static const int32_t math_i[MATH_INPUTS] = {
    0, 1, -1, 7, -7, 13, -13, 100,
    -100, 2147483647, (-2147483647 - 1), 255, 256, -256, 65535, 3,
    -3, 1000000, -1000000, 42, -42, 5, 6, -6,
    31, 32, 33, 12345, -54321, 2, -2, 8,
};
static const int32_t math_j[MATH_INPUTS] = {
    1, 2, -2, 3, -3, 5, -5, 7,
    16, -16, 31, 4, 1000, -7, 9, 11,
    -11, 13, 64, -64, 6, -6, 255, 100,
    17, -17, 33, 2, 3, 12, -12, 10,
};

/*
tolerance: 0 = exactly Metal's number, else the largest difference allowed,
relative for numbers past 1 (|got - metal| <= tolerance * max(1, |metal|)).
D3D's spec allows sin() and cos() an absolute error of 0.0008, hence theirs
*/
typedef struct {
    const char * name;
    const char * expr; // of x, y, i, j
    uint32_t width; // a float or a float2, 3, 4
    float tolerance;
} FloatTest;

static const FloatTest float_tests[] = {
    {"abs", "abs(x)", 1, 0.0f},
    {"sign", "sign(x)", 1, 0.0f},
    {"floor", "floor(x)", 1, 0.0f},
    {"ceil", "ceil(x)", 1, 0.0f},
    {"trunc", "trunc(x)", 1, 0.0f},
    {"round", "round(x)", 1, 0.0f},
    {"fract", "fract(x)", 1, 1e-6f},
    {"fmod", "fmod(x, y)", 1, 1e-5f},
    {"min", "min(x, y)", 1, 0.0f},
    {"max", "max(x, y)", 1, 0.0f},
    {"fmin", "fmin(x, y)", 1, 0.0f},
    {"fmax", "fmax(x, y)", 1, 0.0f},
    {"clamp", "clamp(x, -1.0f, 2.0f)", 1, 0.0f},
    {"saturate", "saturate(x)", 1, 0.0f},
    {"step", "step(y, x)", 1, 0.0f},
    {"smoothstep", "smoothstep(-1.0f, 2.0f, x)", 1, 1e-6f},
    {"mix", "mix(x, y, 0.3f)", 1, 1e-6f},
    {"arithmetic", "x * y + x / y - 0.5f", 1, 1e-6f},
    {"sqrt", "sqrt(abs(x))", 1, 1e-6f},
    {"rsqrt", "rsqrt(abs(x) + 0.01f)", 1, 1e-5f},
    {"sin", "sin(x)", 1, 2e-3f},
    {"cos", "cos(x)", 1, 2e-3f},
    {"sin_small", "sin(clamp(x, -3.14f, 3.14f))", 1, 1e-4f},
    {"cos_small", "cos(clamp(x, -3.14f, 3.14f))", 1, 1e-4f},
    {"tan", "tan(clamp(x, -1.2f, 1.2f))", 1, 1e-4f},
    {"asin", "asin(clamp(x, -1.0f, 1.0f))", 1, 1e-4f},
    {"acos", "acos(clamp(x, -1.0f, 1.0f))", 1, 1e-4f},
    {"atan", "atan(x)", 1, 1e-4f},
    {"atan2", "atan2(y, x)", 1, 1e-4f},
    {"exp", "exp(clamp(x, -20.0f, 20.0f))", 1, 1e-5f},
    {"exp2", "exp2(clamp(x, -20.0f, 20.0f))", 1, 1e-5f},
    {"log", "log(abs(x) + 0.01f)", 1, 1e-5f},
    {"log2", "log2(abs(x) + 0.01f)", 1, 1e-5f},
    {"pow", "pow(abs(x) * 0.1f + 0.01f, clamp(y, -3.0f, 3.0f))", 1, 1e-4f},
    {"dot", "dot(float3(x, y, 1.0f), float3(y, -x, 2.0f))", 1, 1e-6f},
    {"length", "length(float3(x, y, 2.0f))", 1, 1e-6f},
    {"distance", "distance(float2(x, y), float2(1.0f, -2.0f))", 1, 1e-6f},
    {"normalize", "normalize(float3(x, y, 2.0f))", 3, 1e-6f},
    {"cross", "cross(float3(x, y, 1.0f), float3(2.0f, -1.0f, y))", 3, 1e-6f},
    {
        "reflect",
        "reflect(float3(x, y, 1.0f), normalize(float3(1.0f, 2.0f, 2.0f)))",
        3, 1e-6f,
    },
    {
        "mul_2x2",
        "mul(float2(x, y), float2x2(float2(1.0f, 2.0f), float2(3.0f, 4.0f)))",
        2, 1e-6f,
    },
    {
        "mul_3x3",
        "mul(float3(x, y, 1.0f), float3x3(float3(1.0f, 2.0f, 3.0f), "
        "float3(4.0f, 5.0f, 6.0f), float3(7.0f, 8.0f, x)))",
        3, 1e-6f,
    },
    {
        "mul_4x4",
        "mul(float4(x, y, 1.0f, -1.0f), float4x4("
        "float4(1.0f, 0.0f, 2.0f, 0.0f), float4(0.0f, 1.0f, 0.0f, 3.0f), "
        "float4(y, 0.0f, 1.0f, 0.0f), float4(0.0f, x, 0.0f, 1.0f)))",
        4, 1e-6f,
    },
    {
        "mul_matrices",
        "mul(float2x2(float2(x, 1.0f), float2(2.0f, y)), "
        "float2x2(float2(1.0f, 2.0f), float2(3.0f, 4.0f)))[1]",
        2, 1e-6f,
    },
    {
        "transpose",
        "transpose(float2x2(float2(x, y), float2(1.0f, 2.0f)))[0]",
        2, 0.0f,
    },
    {"int_to_float", "(float)i", 1, 0.0f},
    {"uint_to_float", "(float)(uint)i", 1, 0.0f},
    {
        "bits_to_float",
        "as_type<float>(((uint)i & (uint)8388607) | (uint)1065353216)",
        1, 0.0f,
    },
    {"select", "x > y ? float2(x, 1.0f) : float2(2.0f, y)", 2, 0.0f},
    {"swizzle", "float4(x, y, 1.0f, 2.0f).wzyx.xzy", 3, 0.0f},
};
#define FLOAT_TESTS_SIZE (sizeof(float_tests) / sizeof(float_tests[0]))

// Ints are always exactly Metal's. Signed overflow is undefined in MSL,
// so wrapping arithmetic goes through uint
typedef struct {
    const char * name;
    const char * expr; // an int or uint of x, y, i, j
} IntTest;

static const IntTest int_tests[] = {
    {"add", "(int)((uint)i + (uint)j)"},
    {"sub", "(int)((uint)i - (uint)j)"},
    {"mul", "(int)((uint)i * (uint)j)"},
    {"div", "i / j"},
    {"mod", "i % j"},
    {"udiv", "(int)((uint)i / (uint)((j & 255) | 1))"},
    {"umod", "(int)((uint)i % (uint)((j & 255) | 1))"},
    {"and", "i & j"},
    {"or", "i | j"},
    {"xor", "i ^ j"},
    {"not", "~i"},
    {"shift_left", "(int)((uint)i << (uint)(j & 31))"},
    {"shift_right", "i >> (j & 31)"},
    {"ushift_right", "(int)((uint)i >> (uint)(j & 31))"},
    {"min", "min(i, j)"},
    {"max", "max(i, j)"},
    {"clamp", "clamp(i, -5, 5)"},
    {"abs", "abs(i % 1000)"},
    {"float_to_int", "(int)x"},
    {"float_to_uint", "(int)(uint)abs(x)"},
    {"floor_to_int", "(int)floor(x)"},
    {"round_to_int", "(int)round(x)"},
    {"float_bits", "(int)as_type<uint>(x)"},
    {"less", "x < y ? 1 : 0"},
    {"any", "any(float2(x, y) > float2(1.0f, 1.0f)) ? 1 : 0"},
    {"all", "all(float2(x, y) > float2(0.0f, 0.0f)) ? 1 : 0"},
    {"logic", "(i > 0 && j > 0) || i == j ? 1 : 0"},
    // vector comparisons: each component, any() and all() of them
    {"vector_less", "any(float2(x, y) < float2(y, 0.5f)) ? 1 : 0"},
    {"vector_equal", "all(float2(x, y) == float2(x, 1.0f)) ? 1 : 0"},
    {"vector_not_equal", "any(int3(i, j, 0) != int3(0, 1, 0)) ? 1 : 0"},
    {"vector_scalar", "any(float3(x, y, 1.0f) >= 2.0f) ? 1 : 0"},
    {"scalar_vector", "all(0.0f < float2(x, y)) ? 1 : 0"},
    {
        "uint_vectors",
        "all(uint2((uint)i, (uint)j) <= uint2(100, 100)) ? 1 : 0",
    },
    {"swizzle_compare", "any(float4(x, y, 1.0f, 2.0f).zw > float2(y, x)) ? 1 : 0"},
    {"not_vector", "all(!(float2(x, y) < float2(0.0f, 0.0f))) ? 1 : 0"},
    {
        "compare_compared",
        "all((float2(x, y) > float2(0.0f, 0.0f)) == "
        "(float2(y, x) > float2(0.0f, 0.0f))) ? 1 : 0",
    },
    {
        "int_vectors",
        "(int2(i, j) / int2(3, -3)).x + (int2(i, j) % int2(5, 5)).y * 1000",
    },
};
#define INT_TESTS_SIZE (sizeof(int_tests) / sizeof(int_tests[0]))

static uint32_t float_slots(void) {
    uint32_t n = 0;
    for (uint32_t t = 0; t < FLOAT_TESTS_SIZE; t++) {
        n += float_tests[t].width;
    }
    return n;
}

static void cat(char ** text, size_t * cap, const char * add) {
    size_t size = strlen(*text);
    size_t add_size = strlen(add);
    if (size + add_size + 1 > *cap) {
        *cap = (size + add_size + 1) * 2;
        *text = realloc(*text, *cap);
    }
    memcpy(*text + size, add, add_size + 1);
}

static void add_math_section(void) {
    size_t cap = 4096;
    char * text = malloc(cap);
    text[0] = '\0';
    char line[1024];
    cat(&text, &cap,
        "kernel void t1t_math_kernel(\n"
        "    uint t [[ thread_position_in_grid ]],\n"
        "    const device T1TInput * inputs [[ buffer(T1_BUFS_I + 0) ]],\n"
        "    device float * fo [[ buffer(T1_BUFS_I + 1) ]],\n"
        "    device int * io [[ buffer(T1_BUFS_I + 2) ]])\n"
        "{\n"
        "    const T1TInput inp = inputs[t];\n"
        "    const float x = inp.x;\n"
        "    const float y = inp.y;\n"
        "    const int i = inp.i;\n"
        "    const int j = inp.j;\n");
    snprintf(line, sizeof(line),
        "    const uint fb = t * %u;\n    const uint ib = t * %u;\n",
        float_slots(), (uint32_t)INT_TESTS_SIZE);
    cat(&text, &cap, line);
    static const char * const lanes[4] = {"x", "y", "z", "w"};
    uint32_t slot = 0;
    for (uint32_t t = 0; t < FLOAT_TESTS_SIZE; t++) {
        const FloatTest * ft = &float_tests[t];
        if (ft->width == 1) {
            snprintf(line, sizeof(line),
                "    fo[fb + %u] = %s; // %s\n", slot, ft->expr, ft->name);
            cat(&text, &cap, line);
        } else {
            snprintf(line, sizeof(line),
                "    { // %s\n        const float%u r = %s;\n",
                ft->name, ft->width, ft->expr);
            cat(&text, &cap, line);
            for (uint32_t l = 0; l < ft->width; l++) {
                snprintf(line, sizeof(line),
                    "        fo[fb + %u] = r.%s;\n", slot + l, lanes[l]);
                cat(&text, &cap, line);
            }
            cat(&text, &cap, "    }\n");
        }
        slot += ft->width;
    }
    for (uint32_t t = 0; t < INT_TESTS_SIZE; t++) {
        snprintf(line, sizeof(line),
            "    io[ib + %u] = (int)(%s); // %s\n",
            t, int_tests[t].expr, int_tests[t].name);
        cat(&text, &cap, line);
    }
    cat(&text, &cap, "}\n");
    add_section(&cases, "math", text, strlen(text));
    free(text);
}

static void cat_source(char ** text, size_t * cap, const Section * s) {
    char line[128];
    snprintf(line, sizeof(line), "// ---- %s\n", s->name);
    cat(text, cap, line);
    cat(text, cap, s->text);
    cat(text, cap, "\n");
}

/*
Compiles every case's section after the common one, alone, then all that
compiled together (what the cases' pipelines use)
*/
static void compile_shaders(void) {
    const Section * common = section_named("common");
    if (common == NULL) {
        printf("%s has no \"//== common\" section\n", shaders_path);
        exit(2);
    }
    size_t all_cap = 4096;
    char * all = malloc(all_cap);
    all[0] = '\0';
    cat_source(&all, &all_cap, common);
    for (uint32_t s = 0; s < cases.size; s++) {
        Section * section = &cases.items[s];
        if (section == common) { continue; }
        size_t cap = 4096;
        char * alone = malloc(cap);
        alone[0] = '\0';
        cat_source(&alone, &cap, common);
        cat_source(&alone, &cap, section);
        uint8_t good = 0;
        T1_gpu_try_shader_source(
            alone, &good, section->error, sizeof(section->error),
            &fatal_error);
        if (fatal_error != NULL) { fail_fatal("compiling a case"); }
        section->compiled = good;
        if (good) { cat_source(&all, &all_cap, section); }
        free(alone);
    }
    uint8_t good = 0;
    char error[512];
    T1_gpu_try_shader_source(all, &good, error, sizeof(error), &fatal_error);
    if (fatal_error != NULL) { fail_fatal("compiling every case"); }
    if (!good) {
        // each compiled alone, so they clash: e.g. 2 functions of one name
        printf("the cases compiled alone but not together:\n%s\n", error);
        exit(2);
    }
    free(all);
}

/*
THE REFERENCE: records of (name, bytes), as Metal gave them
*/
#define RECORDS_CAP 256
typedef struct {
    char name[64];
    uint8_t * data;
    uint32_t size;
} Record;
static Record reference[RECORDS_CAP];
static uint32_t reference_size = 0;
static Record recorded[RECORDS_CAP]; // record mode: what this run got
static uint32_t recorded_size = 0;

static void read_u32(FILE * f, uint32_t * v) {
    if (fread(v, 4, 1, f) != 1) { *v = 0; }
}

static void load_reference(void) {
    FILE * f = fopen(reference_path, "rb");
    if (f == NULL) { return; }
    char magic[4];
    uint32_t count = 0;
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, "T1ST", 4) != 0) {
        printf("%s isn't a shadertest reference\n", reference_path);
        fclose(f);
        return;
    }
    read_u32(f, &count);
    for (uint32_t r = 0; r < count && r < RECORDS_CAP; r++) {
        Record * rec = &reference[reference_size];
        uint32_t name_size = 0;
        read_u32(f, &name_size);
        if (name_size == 0 || name_size >= sizeof(rec->name)) { break; }
        if (fread(rec->name, 1, name_size, f) != name_size) { break; }
        rec->name[name_size] = '\0';
        read_u32(f, &rec->size);
        rec->data = malloc(rec->size + 1);
        if (fread(rec->data, 1, rec->size, f) != rec->size) { break; }
        reference_size++;
    }
    fclose(f);
}

static const Record * reference_named(const char * name) {
    for (uint32_t r = 0; r < reference_size; r++) {
        if (strcmp(reference[r].name, name) == 0) { return &reference[r]; }
    }
    return NULL;
}

static void record(const char * name, const void * data, uint32_t size) {
    if (recorded_size >= RECORDS_CAP) { return; }
    Record * rec = &recorded[recorded_size++];
    snprintf(rec->name, sizeof(rec->name), "%s", name);
    rec->data = malloc(size);
    memcpy(rec->data, data, size);
    rec->size = size;
}

static void save_reference(void) {
    FILE * f = fopen(reference_path, "wb");
    if (f == NULL) {
        printf("can't write %s\n", reference_path);
        exit(2);
    }
    fwrite("T1ST", 1, 4, f);
    fwrite(&recorded_size, 4, 1, f);
    for (uint32_t r = 0; r < recorded_size; r++) {
        uint32_t name_size = (uint32_t)strlen(recorded[r].name);
        fwrite(&name_size, 4, 1, f);
        fwrite(recorded[r].name, 1, name_size, f);
        fwrite(&recorded[r].size, 4, 1, f);
        fwrite(recorded[r].data, 1, recorded[r].size, f);
    }
    fclose(f);
    printf("wrote %u records to %s\n", recorded_size, reference_path);
}

/*
REJECTS: shaders outside the subset, in rejects.metal, which T1 must refuse
with the message in their "// why:" line, on every platform. Many are fine
MSL, so this catches the subset growing by accident on the platform that
runs them as they are: what isn't refused must be translated too
*/
static uint32_t failures = 0;

static void run_rejects(void) {
    uint32_t refused = 0;
    uint32_t total = 0;
    for (uint32_t r = 0; r < rejects.size; r++) {
        const Section * s = &rejects.items[r];
        if (strcmp(s->name, "about") == 0) { continue; }
        total++;
        char why[256] = {0};
        const char * marker = "// why: ";
        const char * at = strstr(s->text, marker);
        if (at != NULL) {
            at += strlen(marker);
            size_t n = 0;
            while (at[n] != '\0' && at[n] != '\n' && n + 1 < sizeof(why)) {
                why[n] = at[n];
                n++;
            }
        }
        if (why[0] == '\0') {
            printf("FAIL  reject %-22s has no \"// why:\" line\n", s->name);
            failures++;
            continue;
        }
        uint8_t good = 0;
        char error[512];
        error[0] = '\0';
        T1_gpu_try_shader_source(
            s->text, &good, error, sizeof(error), &fatal_error);
        if (fatal_error != NULL) { fail_fatal("a reject"); }
        if (good) {
            printf(
                "FAIL  reject %-22s accepted: outside the subset, yet this "
                "platform took it (translate it everywhere, or refuse it)\n",
                s->name);
            failures++;
        } else if (
            strstr(error, "Outside T1's shader subset") == NULL ||
            strstr(error, why) == NULL)
        {
            printf(
                "FAIL  reject %-22s refused, but not with \"%s\":\n%s\n",
                s->name, why, error);
            failures++;
        } else {
            refused++;
        }
    }
    printf("%s  %-29s %u of %u refused\n",
        refused == total ? "pass" : "FAIL", "rejects", refused, total);
}

/*
THE CHECKS: each case's setup makes its passes and says what to read back
after a frame, and how to compare it
*/
typedef uint8_t ExpectKind;
enum {
    EXPECT_PIXELS, // rgba8, tolerance per channel (of 255)
    EXPECT_MATH_FLOATS, // float_tests, their own tolerances
    EXPECT_MATH_INTS, // int_tests
    EXPECT_WORDS, // 4-byte words, exactly
};

#define EXPECTS_CAP 64
typedef struct {
    char name[64];
    ExpectKind kind;
    uint32_t tex; // or T1_TEX_NONE
    T1Buf buf;
    uint32_t size; // bytes
    uint32_t tolerance;
    uint32_t read;
    uint8_t * got;
    uint8_t done;
} Expect;
static Expect expects[EXPECTS_CAP];
static uint32_t expects_size = 0;

static uint32_t passes[256]; // of the phase running, deleted after it
static uint32_t passes_size = 0;

static uint8_t recording = 0;

static Expect * new_expect(const char * name, ExpectKind kind) {
    if (expects_size >= EXPECTS_CAP) {
        printf("too many expects, raise EXPECTS_CAP\n");
        exit(2);
    }
    Expect * e = &expects[expects_size++];
    memset(e, 0, sizeof(Expect));
    snprintf(e->name, sizeof(e->name), "%s", name);
    e->kind = kind;
    e->tex = T1_TEX_NONE;
    e->read = T1_BUF_READ_NONE;
    return e;
}

static void expect_pixels(const char * name, T1Tex tex, uint32_t tolerance) {
    Expect * e = new_expect(name, EXPECT_PIXELS);
    e->tex = tex;
    e->size = SIDE * SIDE * 4;
    e->tolerance = tolerance;
}

static void expect_buf(
    const char * name, ExpectKind kind, T1Buf buf, uint32_t size)
{
    Expect * e = new_expect(name, kind);
    e->buf = buf;
    e->size = size;
}

static uint32_t keep_pass(uint32_t pass) {
    passes[passes_size++] = pass;
    return pass;
}

// A section that didn't compile: its case fails, its passes aren't made
static uint8_t compiled(const char * section) {
    const Section * s = section_named(section);
    if (s == NULL) {
        printf("FAIL  %-22s no section \"//== %s\"\n", section, section);
        failures++;
        return 0;
    }
    if (!s->compiled) {
        printf("FAIL  %-22s doesn't compile:\n%s\n", section, s->error);
        failures++;
        return 0;
    }
    return 1;
}

static T1Tex new_tex(uint32_t w, uint32_t h, const uint8_t * rgba) {
    T1Tex tex = T1_tex_new(rgba, w, h, &fatal_error);
    if (fatal_error != NULL) { fail_fatal("T1_tex_new"); }
    return tex;
}

static T1Buf new_buf(uint64_t size) {
    T1Buf buf = T1_buf_new(size, &fatal_error);
    if (fatal_error != NULL) { fail_fatal("T1_buf_new"); }
    return buf;
}

static uint32_t new_draw_pipeline(
    const char * vertex_fn, const char * fragment_fn, uint8_t blend,
    uint8_t cull)
{
    uint8_t good = 0;
    uint32_t p = T1_pipeline_new_draw(vertex_fn, fragment_fn, blend, cull, &good);
    if (!good) {
        printf("no pipeline %s + %s\n", vertex_fn, fragment_fn);
        exit(2);
    }
    return p;
}

static uint32_t new_compute_pipeline(const char * kernel_fn) {
    uint8_t good = 0;
    uint32_t p = T1_pipeline_new_compute(kernel_fn, &good);
    if (!good) {
        printf("no pipeline %s\n", kernel_fn);
        exit(2);
    }
    return p;
}

static uint32_t clear(T1Tex tex, float r, float g, float b, float a) {
    return keep_pass(T1_pass_new_clear(tex, r, g, b, a));
}

static uint32_t draw(uint32_t pipeline, T1Tex target, uint32_t vertex_count) {
    uint32_t pass = keep_pass(T1_pass_new_draw(pipeline));
    T1_pass_set_u32(pass, "color_tex[0]", target);
    T1_pass_set_u32(pass, "vertex_count", vertex_count);
    return pass;
}

static uint32_t compute_threads(uint32_t pipeline, uint32_t x, uint32_t y) {
    uint32_t pass = keep_pass(T1_pass_new_compute(pipeline));
    T1_pass_set_u32(pass, "threads[0]", x);
    T1_pass_set_u32(pass, "threads[1]", y);
    return pass;
}

static void setup_math(void) {
    if (!compiled("math")) { return; }
    T1TInput inputs[MATH_INPUTS];
    for (uint32_t t = 0; t < MATH_INPUTS; t++) {
        inputs[t].x = math_x[t];
        inputs[t].y = math_y[t];
        inputs[t].i = math_i[t];
        inputs[t].j = math_j[t];
    }
    T1Buf in = new_buf(sizeof(inputs));
    T1_buf_write(in, 0, inputs, sizeof(inputs));
    uint32_t floats_size = MATH_INPUTS * float_slots() * 4;
    uint32_t ints_size = MATH_INPUTS * (uint32_t)INT_TESTS_SIZE * 4;
    T1Buf fo = new_buf(floats_size);
    T1Buf io = new_buf(ints_size);
    uint32_t pass = compute_threads(
        new_compute_pipeline("t1t_math_kernel"), MATH_INPUTS, 1);
    T1_pass_set_u32(pass, "buf[0]", in);
    T1_pass_set_u32(pass, "buf[1]", fo);
    T1_pass_set_u32(pass, "buf[2]", io);
    expect_buf("math_floats", EXPECT_MATH_FLOATS, fo, floats_size);
    expect_buf("math_ints", EXPECT_MATH_INTS, io, ints_size);
}

static void setup_control(void) {
    if (!compiled("control")) { return; }
    T1Buf out = new_buf(16 * 20 * 4);
    uint32_t pass = compute_threads(
        new_compute_pipeline("t1t_control_kernel"), 16, 1);
    T1_pass_set_u32(pass, "buf[0]", out);
    expect_buf("control", EXPECT_WORDS, out, 16 * 20 * 4);
}

static void setup_switch_fallthrough(void) {
    if (!compiled("switch_fallthrough")) { return; }
    T1Buf out = new_buf(8 * 4);
    uint32_t pass = compute_threads(
        new_compute_pipeline("t1t_switch_fallthrough_kernel"), 8, 1);
    T1_pass_set_u32(pass, "buf[0]", out);
    expect_buf("switch_fallthrough", EXPECT_WORDS, out, 8 * 4);
}

#define LAYOUT_BYTES 1024 // more than 2 T1TLayouts (208 bytes each in MSL)
static void setup_layout(void) {
    if (!compiled("layout")) { return; }
    // every word a different float, so a misplaced member shows
    float words[LAYOUT_BYTES / 4];
    for (uint32_t w = 0; w < LAYOUT_BYTES / 4; w++) {
        words[w] = (float)(w + 1) * 0.5f;
    }
    T1Buf in = new_buf(LAYOUT_BYTES);
    T1_buf_write(in, 0, words, LAYOUT_BYTES);
    T1Buf flat = new_buf(2 * 64 * 4);
    /*
    out starts as a copy of in: a struct's padding is only written by a
    copy of the whole struct, and only on some backends, so it's the same
    either way
    */
    T1Buf out = new_buf(LAYOUT_BYTES);
    T1_buf_write(out, 0, words, LAYOUT_BYTES);
    uint32_t pass = compute_threads(
        new_compute_pipeline("t1t_layout_kernel"), 2, 1);
    T1_pass_set_u32(pass, "buf[0]", in);
    T1_pass_set_u32(pass, "buf[1]", flat);
    T1_pass_set_u32(pass, "buf[2]", out);
    expect_buf("layout_read", EXPECT_WORDS, flat, 2 * 64 * 4);
    expect_buf("layout_written", EXPECT_WORDS, out, LAYOUT_BYTES);
}

static void setup_orientation(void) {
    if (!compiled("orientation")) { return; }
    T1Tex target = new_tex(SIDE, SIDE, NULL);
    clear(target, 0.0f, 0.0f, 0.0f, 0.0f);
    draw(new_draw_pipeline(
            "t1t_full_vertex", "t1t_orientation_fragment",
            T1_BLEND_NONE, T1_CULL_NONE),
        target, 3);
    draw(new_draw_pipeline(
            "t1t_corner_vertex", "t1t_color_fragment",
            T1_BLEND_NONE, T1_CULL_NONE),
        target, 3);
    expect_pixels("orientation", target, 1);
}

static void setup_cull(void) {
    if (!compiled("cull")) { return; }
    T1Tex target = new_tex(SIDE, SIDE, NULL);
    clear(target, 0.0f, 0.0f, 0.0f, 1.0f);
    uint32_t back = draw(new_draw_pipeline(
            "t1t_cull_vertex", "t1t_color_fragment",
            T1_BLEND_NONE, T1_CULL_BACK),
        target, 6);
    T1_pass_set_u32(back, "params[0]", 0);
    uint32_t front = draw(new_draw_pipeline(
            "t1t_cull_vertex", "t1t_color_fragment",
            T1_BLEND_NONE, T1_CULL_FRONT),
        target, 6);
    T1_pass_set_u32(front, "params[0]", 1);
    expect_pixels("cull", target, 0);
}

static void setup_perspective(void) {
    if (!compiled("perspective")) { return; }
    T1Tex target = new_tex(SIDE, SIDE, NULL);
    clear(target, 0.0f, 0.0f, 0.0f, 0.0f);
    draw(new_draw_pipeline(
            "t1t_perspective_vertex", "t1t_perspective_fragment",
            T1_BLEND_NONE, T1_CULL_NONE),
        target, 3);
    expect_pixels("perspective", target, 2);
}

static void setup_depth(void) {
    if (!compiled("depth")) { return; }
    T1Tex target = new_tex(SIDE, SIDE, NULL);
    clear(target, 0.0f, 0.0f, 0.0f, 1.0f);
    uint32_t pipeline = new_draw_pipeline(
        "t1t_depth_vertex", "t1t_color_fragment",
        T1_BLEND_NONE, T1_CULL_NONE);
    // the green quad, then the others, sharing one depth buffer
    uint32_t first = draw(pipeline, target, 6);
    T1_pass_set_u32(first, "depth_tex", target);
    uint32_t rest = draw(pipeline, target, 24);
    T1_pass_set_u32(rest, "vertex_start", 6);
    T1_pass_set_u32(rest, "depth_tex", target);
    expect_pixels("depth", target, 0);
}

static void setup_mrt(void) {
    if (!compiled("mrt")) { return; }
    T1Tex a = new_tex(SIDE, SIDE, NULL);
    T1Tex b = new_tex(SIDE, SIDE, NULL);
    clear(a, 0.0f, 0.0f, 0.0f, 0.0f);
    clear(b, 0.0f, 0.0f, 0.0f, 0.0f);
    uint32_t pass = draw(new_draw_pipeline(
            "t1t_full_vertex", "t1t_mrt_fragment",
            T1_BLEND_NONE, T1_CULL_NONE),
        a, 3);
    T1_pass_set_u32(pass, "color_tex[1]", b);
    expect_pixels("mrt_0", a, 1);
    expect_pixels("mrt_1", b, 1);
}

static void setup_sampling(void) {
    if (!compiled("sampling")) { return; }
    // 4x4 pixels, each its own color
    uint8_t rgba[4 * 4 * 4];
    for (uint32_t p = 0; p < 16; p++) {
        rgba[p * 4 + 0] = (uint8_t)(p * 16);
        rgba[p * 4 + 1] = (uint8_t)(255 - p * 13);
        rgba[p * 4 + 2] = (uint8_t)((p % 3) * 120);
        rgba[p * 4 + 3] = (uint8_t)(p % 2 == 0 ? 255 : 128);
    }
    T1Tex source = new_tex(4, 4, rgba);
    T1Tex target = new_tex(SIDE, SIDE, NULL);
    clear(target, 0.0f, 0.0f, 0.0f, 0.0f);
    uint32_t pass = draw(new_draw_pipeline(
            "t1t_full_vertex", "t1t_sampling_fragment",
            T1_BLEND_NONE, T1_CULL_NONE),
        target, 3);
    T1_pass_set_u32(pass, "params[0]", source);
    expect_pixels("sampling", target, 3);
}

static void setup_blend(void) {
    if (!compiled("blend")) { return; }
    T1Tex target = new_tex(SIDE, SIDE, NULL);
    clear(target, 0.2f, 0.4f, 0.6f, 0.8f);
    const uint8_t blends[4] = {
        T1_BLEND_NONE,
        T1_BLEND_ADDITIVE,
        T1_BLEND_PREMULTIPLIED_ALPHA,
        T1_BLEND_STRAIGHT_ALPHA,
    };
    for (uint32_t q = 0; q < 4; q++) {
        uint32_t pass = draw(new_draw_pipeline(
                "t1t_quadrant_vertex", "t1t_blend_fragment",
                blends[q], T1_CULL_NONE),
            target, 6);
        T1_pass_set_u32(pass, "params[0]", q);
    }
    expect_pixels("blend", target, 1);
}

static void setup_discard(void) {
    if (!compiled("discard")) { return; }
    T1Tex target = new_tex(SIDE, SIDE, NULL);
    clear(target, 0.0f, 0.0f, 1.0f, 1.0f);
    draw(new_draw_pipeline(
            "t1t_full_vertex", "t1t_discard_fragment",
            T1_BLEND_NONE, T1_CULL_NONE),
        target, 3);
    expect_pixels("discard", target, 0);
}

static void setup_derivatives(void) {
    if (!compiled("derivatives")) { return; }
    T1Tex target = new_tex(SIDE, SIDE, NULL);
    clear(target, 0.0f, 0.0f, 0.0f, 0.0f);
    draw(new_draw_pipeline(
            "t1t_full_vertex", "t1t_derivatives_fragment",
            T1_BLEND_NONE, T1_CULL_NONE),
        target, 3);
    expect_pixels("derivatives", target, 2);
}

static void setup_compute_write(void) {
    if (!compiled("compute_write")) { return; }
    T1Tex target = new_tex(SIDE, SIDE, NULL);
    uint32_t pass = keep_pass(T1_pass_new_compute(
        new_compute_pipeline("t1t_compute_write_kernel")));
    T1_pass_set_u32(pass, "write_tex", target);
    // halfway values may round either way: 1
    expect_pixels("compute_write", target, 1);
}

static void setup_compute_read_write(void) {
    if (!compiled("compute_read_write")) { return; }
    T1Tex target = new_tex(SIDE, SIDE, NULL);
    clear(target, 0.25f, 0.5f, 0.75f, 1.0f);
    uint32_t pass = keep_pass(T1_pass_new_compute(
        new_compute_pipeline("t1t_compute_read_write_kernel")));
    T1_pass_set_u32(pass, "write_tex", target);
    expect_pixels("compute_read_write", target, 1);
}

#define ATOMIC_WORDS 60
static void setup_atomics(void) {
    if (!compiled("atomics")) { return; }
    T1Buf out = new_buf(ATOMIC_WORDS * 4);
    uint32_t start[ATOMIC_WORDS];
    memset(start, 0, sizeof(start));
    start[1] = 0xFFFFFFFF; // the min's
    start[4] = 0xFFFFFFFF; // the and's
    start[59] = 3; // read by every thread
    T1_buf_write(out, 0, start, sizeof(start));
    uint32_t pass = compute_threads(
        new_compute_pipeline("t1t_atomics_kernel"), 1024, 0);
    T1_pass_set_u32(pass, "buf[0]", out);
    pass = compute_threads(
        new_compute_pipeline("t1t_atomics_settle_kernel"), 1, 0);
    T1_pass_set_u32(pass, "buf[0]", out);
    expect_buf("atomics", EXPECT_WORDS, out, ATOMIC_WORDS * 4);
}

static void setup_threadgroup_1d(void) {
    if (!compiled("threadgroup_1d")) { return; }
    T1Buf out = new_buf(1024 * 4);
    uint32_t pass = compute_threads(
        new_compute_pipeline("t1t_threadgroup_1d_kernel"), 1024, 0);
    T1_pass_set_u32(pass, "buf[0]", out);
    expect_buf("threadgroup_1d", EXPECT_WORDS, out, 1024 * 4);
}

static void setup_threadgroup_2d(void) {
    if (!compiled("threadgroup_2d")) { return; }
    T1Buf out = new_buf(1024 * 4);
    uint32_t pass = compute_threads(
        new_compute_pipeline("t1t_threadgroup_2d_kernel"), 32, 32);
    T1_pass_set_u32(pass, "buf[0]", out);
    expect_buf("threadgroup_2d", EXPECT_WORDS, out, 1024 * 4);
}

static void setup_indirect(void) {
    if (!compiled("indirect")) { return; }
    T1Buf args = new_buf(8 * 4);
    T1Buf out_1d = new_buf(1024 * 4);
    T1Buf out_2d = new_buf(1024 * 4);
    uint32_t pass = compute_threads(
        new_compute_pipeline("t1t_indirect_args_kernel"), 1, 0);
    T1_pass_set_u32(pass, "buf[0]", args);
    pass = keep_pass(T1_pass_new_compute(
        new_compute_pipeline("t1t_indirect_1d_kernel")));
    T1_pass_set_u32(pass, "threads_buf", args);
    T1_pass_set_u32(pass, "threads_buf_offset", 4);
    T1_pass_set_u32(pass, "threads_dims", 1);
    T1_pass_set_u32(pass, "buf[0]", out_1d);
    pass = keep_pass(T1_pass_new_compute(
        new_compute_pipeline("t1t_indirect_2d_kernel")));
    T1_pass_set_u32(pass, "threads_buf", args);
    T1_pass_set_u32(pass, "threads_buf_offset", 20);
    T1_pass_set_u32(pass, "threads_dims", 2);
    T1_pass_set_u32(pass, "buf[0]", out_2d);
    expect_buf("indirect_1d", EXPECT_WORDS, out_1d, 1024 * 4);
    expect_buf("indirect_2d", EXPECT_WORDS, out_2d, 1024 * 4);
}

static void (* const check_setups[])(void) = {
    setup_math,
    setup_control,
    setup_switch_fallthrough,
    setup_layout,
    setup_orientation,
    setup_cull,
    setup_perspective,
    setup_depth,
    setup_mrt,
    setup_sampling,
    setup_blend,
    setup_discard,
    setup_derivatives,
    setup_compute_write,
    setup_compute_read_write,
    setup_atomics,
    setup_threadgroup_1d,
    setup_threadgroup_2d,
    setup_indirect,
};

static uint32_t differing(float got, float metal, float tolerance) {
    if (isnan(metal) || isnan(got)) { return isnan(metal) != isnan(got); }
    if (tolerance == 0.0f) { return got != metal; }
    if (isinf(metal) || isinf(got)) { return got != metal; }
    float scale = fabsf(metal) > 1.0f ? fabsf(metal) : 1.0f;
    return fabsf(got - metal) > tolerance * scale;
}

// prints at most this many lines a case
#define DETAILS_CAP 8

static void compare_math_floats(const Expect * e, const uint8_t * metal) {
    const float * got = (const float *)e->got;
    const float * ref = (const float *)metal;
    uint32_t slots = float_slots();
    uint32_t bad_tests = 0;
    uint32_t slot = 0;
    for (uint32_t t = 0; t < FLOAT_TESTS_SIZE; t++) {
        const FloatTest * ft = &float_tests[t];
        uint32_t bad = 0;
        uint32_t first_input = 0;
        uint32_t first_lane = 0;
        for (uint32_t in = 0; in < MATH_INPUTS; in++) {
            for (uint32_t l = 0; l < ft->width; l++) {
                uint32_t k = in * slots + slot + l;
                if (differing(got[k], ref[k], ft->tolerance)) {
                    if (bad == 0) { first_input = in; first_lane = l; }
                    bad++;
                }
            }
        }
        if (bad > 0) {
            if (bad_tests < DETAILS_CAP) {
                uint32_t k = first_input * slots + slot + first_lane;
                printf(
                    "      %-14s %s: %u wrong, e.g. x %g y %g i %d j %d "
                    "(lane %u): got %.9g, Metal %.9g\n",
                    ft->name, ft->expr, bad,
                    (double)math_x[first_input], (double)math_y[first_input],
                    math_i[first_input], math_j[first_input],
                    first_lane, (double)got[k], (double)ref[k]);
            }
            bad_tests++;
        }
        slot += ft->width;
    }
    if (bad_tests == 0) {
        printf("pass  %-22s %u functions\n", e->name, (uint32_t)FLOAT_TESTS_SIZE);
    } else {
        printf("FAIL  %-22s %u of %u functions differ (above)\n",
            e->name, bad_tests, (uint32_t)FLOAT_TESTS_SIZE);
        failures++;
    }
}

static void compare_math_ints(const Expect * e, const uint8_t * metal) {
    const int32_t * got = (const int32_t *)e->got;
    const int32_t * ref = (const int32_t *)metal;
    uint32_t bad_tests = 0;
    for (uint32_t t = 0; t < INT_TESTS_SIZE; t++) {
        uint32_t bad = 0;
        uint32_t first = 0;
        for (uint32_t in = 0; in < MATH_INPUTS; in++) {
            uint32_t k = in * (uint32_t)INT_TESTS_SIZE + t;
            if (got[k] != ref[k]) {
                if (bad == 0) { first = in; }
                bad++;
            }
        }
        if (bad > 0) {
            if (bad_tests < DETAILS_CAP) {
                uint32_t k = first * (uint32_t)INT_TESTS_SIZE + t;
                printf(
                    "      %-14s %s: %u wrong, e.g. x %g y %g i %d j %d: "
                    "got %d, Metal %d\n",
                    int_tests[t].name, int_tests[t].expr, bad,
                    (double)math_x[first], (double)math_y[first],
                    math_i[first], math_j[first], got[k], ref[k]);
            }
            bad_tests++;
        }
    }
    if (bad_tests == 0) {
        printf("pass  %-22s %u operations\n", e->name, (uint32_t)INT_TESTS_SIZE);
    } else {
        printf("FAIL  %-22s %u of %u operations differ (above)\n",
            e->name, bad_tests, (uint32_t)INT_TESTS_SIZE);
        failures++;
    }
}

static void compare_words(const Expect * e, const uint8_t * metal) {
    const uint32_t * got = (const uint32_t *)e->got;
    const uint32_t * ref = (const uint32_t *)metal;
    uint32_t bad = 0;
    for (uint32_t w = 0; w < e->size / 4; w++) {
        if (got[w] != ref[w]) {
            if (bad < DETAILS_CAP) {
                float got_f;
                float ref_f;
                memcpy(&got_f, &got[w], 4);
                memcpy(&ref_f, &ref[w], 4);
                printf(
                    "      word %u (byte %u): got %d / %g, Metal %d / %g\n",
                    w, w * 4, (int32_t)got[w], (double)got_f,
                    (int32_t)ref[w], (double)ref_f);
            }
            bad++;
        }
    }
    if (bad == 0) {
        printf("pass  %-22s %u words\n", e->name, e->size / 4);
    } else {
        printf("FAIL  %-22s %u of %u words differ (above, as int / float)\n",
            e->name, bad, e->size / 4);
        failures++;
    }
}

static void write_ppm(const char * name, const char * which, const uint8_t * rgba) {
    t1t_mkdir(failures_dir);
    char path[256];
    snprintf(path, sizeof(path), "%s/%s_%s.ppm", failures_dir, name, which);
    FILE * f = fopen(path, "wb");
    if (f == NULL) { return; }
    // scaled up 8 times, rgb (alpha isn't shown)
    const uint32_t scale = 8;
    fprintf(f, "P6\n%u %u\n255\n", SIDE * scale, SIDE * scale);
    for (uint32_t y = 0; y < SIDE * scale; y++) {
        for (uint32_t x = 0; x < SIDE * scale; x++) {
            fwrite(rgba + ((y / scale) * SIDE + x / scale) * 4, 1, 3, f);
        }
    }
    fclose(f);
}

static void compare_pixels(const Expect * e, const uint8_t * metal) {
    uint32_t bad = 0;
    uint32_t worst = 0;
    uint32_t worst_pixel = 0;
    uint32_t worst_channel = 0;
    for (uint32_t p = 0; p < SIDE * SIDE; p++) {
        uint8_t pixel_bad = 0;
        for (uint32_t c = 0; c < 4; c++) {
            int32_t d = (int32_t)e->got[p * 4 + c] - (int32_t)metal[p * 4 + c];
            uint32_t diff = (uint32_t)(d < 0 ? -d : d);
            if (diff > worst) {
                worst = diff;
                worst_pixel = p;
                worst_channel = c;
            }
            if (diff > e->tolerance) { pixel_bad = 1; }
        }
        bad += pixel_bad;
    }
    if (bad == 0) {
        printf("pass  %-22s largest difference %u (tolerance %u)\n",
            e->name, worst, e->tolerance);
        return;
    }
    const uint8_t * g = e->got + worst_pixel * 4;
    const uint8_t * m = metal + worst_pixel * 4;
    printf(
        "FAIL  %-22s %u of %u pixels off by more than %u, the worst by %u "
        "(channel %c) at x %u y %u: got %u %u %u %u, Metal %u %u %u %u\n"
        "      pictures: %s/%s_got.ppm and _metal.ppm\n",
        e->name, bad, SIDE * SIDE, e->tolerance, worst,
        "rgba"[worst_channel], worst_pixel % SIDE, worst_pixel / SIDE,
        g[0], g[1], g[2], g[3], m[0], m[1], m[2], m[3],
        failures_dir, e->name);
    write_ppm(e->name, "got", e->got);
    write_ppm(e->name, "metal", metal);
    failures++;
}

static void compare_expect(const Expect * e) {
    if (recording) {
        record(e->name, e->got, e->size);
        printf("recorded %s\n", e->name);
        return;
    }
    const Record * metal = reference_named(e->name);
    if (metal == NULL || metal->size != e->size) {
        printf("FAIL  %-22s %s: record it on a Mac\n", e->name,
            metal == NULL ? "not in the reference" : "a different size");
        failures++;
        return;
    }
    switch (e->kind) {
        case EXPECT_PIXELS: compare_pixels(e, metal->data); break;
        case EXPECT_MATH_FLOATS: compare_math_floats(e, metal->data); break;
        case EXPECT_MATH_INTS: compare_math_ints(e, metal->data); break;
        case EXPECT_WORDS: compare_words(e, metal->data); break;
        default: break;
    }
}

static void start_reads(void) {
    for (uint32_t x = 0; x < expects_size; x++) {
        Expect * e = &expects[x];
        e->got = malloc(e->size);
        e->read = e->tex != T1_TEX_NONE ?
            T1_tex_read_start(e->tex, 0, 0, SIDE, SIDE) :
            T1_buf_read_start(e->buf, 0, e->size);
        if (e->read == T1_BUF_READ_NONE) {
            printf("can't read back %s (T1_BUF_READS_CAP?)\n", e->name);
            exit(2);
        }
    }
}

// 1 when every read is done
static uint8_t finish_reads(void) {
    uint8_t all = 1;
    for (uint32_t x = 0; x < expects_size; x++) {
        Expect * e = &expects[x];
        if (e->done) { continue; }
        if (T1_buf_read_finish(e->read, e->got)) {
            e->done = 1;
        } else {
            all = 0;
        }
    }
    return all;
}

static void delete_passes(void) {
    for (uint32_t p = 0; p < passes_size; p++) {
        T1_pass_delete(passes[p]);
    }
    passes_size = 0;
}

/*
PERFORMANCE: each case runs alone for PERF_FRAMES frames, the first
PERF_WARMUP of them not counted (T1_gpu_frame_us() is a frame or two
behind), and keeps the fastest frame
*/
#define PERF_SIDE 1024
#define PERF_WARMUP 6
#define PERF_FRAMES 26

static T1Buf perf_out = 0;
static T1Buf perf_layouts = 0;
static T1Tex perf_target = T1_TEX_NONE;
static T1Tex perf_source = T1_TEX_NONE;

static void perf_resources(void) {
    perf_out = new_buf(PERF_SIDE * PERF_SIDE * 4);
    // 1024 T1TLayouts, of whatever floats
    uint32_t layouts_size = 1024 * 256;
    float * words = malloc(layouts_size);
    for (uint32_t w = 0; w < layouts_size / 4; w++) {
        words[w] = (float)(w % 97) * 0.25f;
    }
    perf_layouts = new_buf(layouts_size);
    T1_buf_write(perf_layouts, 0, words, layouts_size);
    free(words);
    perf_target = new_tex(PERF_SIDE, PERF_SIDE, NULL);
    uint8_t * rgba = malloc(256 * 256 * 4);
    for (uint32_t p = 0; p < 256 * 256 * 4; p++) {
        rgba[p] = (uint8_t)((p * 2654435761u) >> 24);
    }
    perf_source = new_tex(256, 256, rgba);
    free(rgba);
}

static void perf_kernel(const char * kernel_fn) {
    uint32_t pass = compute_threads(
        new_compute_pipeline(kernel_fn), PERF_SIDE, PERF_SIDE);
    T1_pass_set_u32(pass, "buf[0]", perf_out);
}

static void perf_empty(void) {}
static void perf_alu(void) { perf_kernel("t1t_perf_alu"); }
static void perf_transcendental(void) {
    perf_kernel("t1t_perf_transcendental");
}
static void perf_branches(void) { perf_kernel("t1t_perf_branches"); }
static void perf_global_table(void) { perf_kernel("t1t_perf_global_table"); }
static void perf_local_table(void) { perf_kernel("t1t_perf_local_table"); }

static void perf_struct_loads(void) {
    uint32_t pass = compute_threads(
        new_compute_pipeline("t1t_perf_struct_loads"), PERF_SIDE, PERF_SIDE);
    T1_pass_set_u32(pass, "buf[0]", perf_layouts);
    T1_pass_set_u32(pass, "buf[1]", perf_out);
}

static void perf_struct_stores(void) {
    uint32_t pass = compute_threads(
        new_compute_pipeline("t1t_perf_struct_stores"), PERF_SIDE, PERF_SIDE);
    T1_pass_set_u32(pass, "buf[0]", perf_layouts);
}

static void perf_samples(void) {
    uint32_t pass = draw(new_draw_pipeline(
            "t1t_full_vertex", "t1t_perf_samples_fragment",
            T1_BLEND_NONE, T1_CULL_NONE),
        perf_target, 3);
    T1_pass_set_u32(pass, "params[0]", perf_source);
}

static void perf_write_tex(void) {
    uint32_t pass = keep_pass(T1_pass_new_compute(
        new_compute_pipeline("t1t_perf_write_tex")));
    T1_pass_set_u32(pass, "write_tex", perf_target);
}

static void perf_blend(void) {
    uint32_t pipeline = new_draw_pipeline(
        "t1t_full_vertex", "t1t_perf_blend_fragment",
        T1_BLEND_STRAIGHT_ALPHA, T1_CULL_NONE);
    for (uint32_t k = 0; k < 8; k++) { draw(pipeline, perf_target, 3); }
}

typedef struct {
    const char * name;
    void (* setup)(void);
    uint64_t us; // the fastest frame
} PerfCase;

// the first is the frame's own cost, the second the unit
static PerfCase perf_cases[] = {
    {"empty", perf_empty, 0},
    {"alu", perf_alu, 0},
    {"transcendental", perf_transcendental, 0},
    {"branches", perf_branches, 0},
    {"global_table", perf_global_table, 0},
    {"local_table", perf_local_table, 0},
    {"struct_loads", perf_struct_loads, 0},
    {"struct_stores", perf_struct_stores, 0},
    {"samples", perf_samples, 0},
    {"write_tex", perf_write_tex, 0},
    {"blend", perf_blend, 0},
};
#define PERF_CASES_SIZE (sizeof(perf_cases) / sizeof(perf_cases[0]))

static void compare_perf(void) {
    if (!compiled("perf")) { return; }
    const uint64_t empty = perf_cases[0].us;
    const uint64_t alu = perf_cases[1].us;
    printf("perf  empty frame %llu us, alu %llu us (the unit)\n",
        (unsigned long long)empty, (unsigned long long)alu);
    if (alu <= empty) {
        printf("FAIL  perf: alu isn't slower than an empty frame, no unit\n");
        failures++;
        return;
    }
    const float unit = (float)(alu - empty);
    for (uint32_t c = 2; c < PERF_CASES_SIZE; c++) {
        const PerfCase * pc = &perf_cases[c];
        const uint64_t net = pc->us > empty ? pc->us - empty : 0;
        const float ratio = (float)net / unit;
        char name[64];
        snprintf(name, sizeof(name), "perf_%s", pc->name);
        if (recording) {
            record(name, &ratio, sizeof(ratio));
            printf("recorded %s: %.2f alu (%llu us)\n", name, (double)ratio,
                (unsigned long long)pc->us);
            continue;
        }
        const Record * metal = reference_named(name);
        if (metal == NULL || metal->size != sizeof(float)) {
            printf("FAIL  %-22s not in the reference: record it on a Mac\n",
                name);
            failures++;
            continue;
        }
        float metal_ratio;
        memcpy(&metal_ratio, metal->data, sizeof(float));
        const uint8_t slow =
            ratio > metal_ratio * SLOW_FACTOR && net >= SLOW_MIN_US;
        printf("%s  %-22s %6.2f alu, Metal %6.2f (%llu us)\n",
            slow ? "SLOW" : "pass", name, (double)ratio, (double)metal_ratio,
            (unsigned long long)pc->us);
        failures += slow;
    }
}

typedef uint8_t Phase;
enum {
    PHASE_CHECK,
    PHASE_PERF,
    PHASE_DONE,
};

int main(int argc, char ** argv) {
    uint8_t perf = 1;
    for (int a = 1; a < argc; a++) {
        if (strcmp(argv[a], "record") == 0) {
            recording = 1;
        } else if (strcmp(argv[a], "noperf") == 0) {
            perf = 0;
        } else {
            printf("usage: shadertest [record] [noperf]\n");
            return 2;
        }
    }

    if (!find_files(argv[0])) {
        printf(
            "can't find tests/shadertest/shadertest.metal, neither from here "
            "nor 2 folders up from the program\n");
        return 2;
    }
    char * text = read_text_file(shaders_path);
    if (text == NULL) {
        printf("can't read %s\n", shaders_path);
        return 2;
    }
    split_sections(&cases, text);
    free(text);
    add_math_section();
    text = read_text_file(rejects_path);
    if (text == NULL) {
        printf("can't read %s\n", rejects_path);
        return 2;
    }
    split_sections(&rejects, text);
    free(text);
    if (recording && !perf) {
        printf("record makes the whole reference, perf cases too\n");
        return 2;
    }
    if (!recording) {
        load_reference();
        if (reference_size == 0) {
            printf("no reference (%s): run \"shadertest record\" on a Mac\n",
                reference_path);
            return 2;
        }
    }

    T1_init(malloc, malloc, free, &fatal_error);
    T1_gpu_init(&fatal_error);
    if (fatal_error != NULL) { fail_fatal("init"); }
    if (!recording) { run_rejects(); }
    compile_shaders();

    T1Tex window_tex = new_tex(64, 64, NULL);
    T1_window_open(window_tex, &fatal_error);
    if (fatal_error != NULL) { fail_fatal("T1_window_open"); }
    T1_window_set_pause_when_hidden(0);
    // stays for every phase: the window shows black
    T1_pass_new_clear(window_tex, 0.0f, 0.0f, 0.0f, 1.0f);

    for (uint32_t s = 0; s < sizeof(check_setups) / sizeof(check_setups[0]); s++) {
        check_setups[s]();
    }

    Phase phase = PHASE_CHECK;
    uint32_t frame = 0;
    uint32_t perf_i = 0;
    uint32_t perf_frame = 0;
    while (phase != PHASE_DONE && T1_frame_update()) {
        frame++;
        if (phase == PHASE_CHECK) {
            if (frame == 1) {
                start_reads(); // of what the next frame's passes leave
            } else if (finish_reads() || frame > 600) {
                for (uint32_t x = 0; x < expects_size; x++) {
                    if (!expects[x].done) {
                        printf("FAIL  %-22s never read back\n", expects[x].name);
                        failures++;
                        continue;
                    }
                    compare_expect(&expects[x]);
                }
                delete_passes();
                if (perf && section_named("perf") && section_named("perf")->compiled) {
                    perf_resources();
                    perf_cases[0].setup();
                    phase = PHASE_PERF;
                } else {
                    if (perf) { compare_perf(); } // says it didn't compile
                    phase = PHASE_DONE;
                }
            }
        } else if (phase == PHASE_PERF) {
            perf_frame++;
            PerfCase * pc = &perf_cases[perf_i];
            if (perf_frame > PERF_WARMUP) {
                uint64_t us = T1_gpu_frame_us();
                if (us > 0 && (pc->us == 0 || us < pc->us)) { pc->us = us; }
            }
            if (perf_frame >= PERF_FRAMES) {
                delete_passes();
                perf_frame = 0;
                perf_i++;
                if (perf_i < PERF_CASES_SIZE) {
                    perf_cases[perf_i].setup();
                } else {
                    compare_perf();
                    phase = PHASE_DONE;
                }
            }
        }
    }

    if (recording) {
        save_reference();
    } else if (failures == 0) {
        printf("\nall passed\n");
    } else {
        printf("\n%u FAILED\n", failures);
    }
    T1_shutdown();
    return recording ? 0 : (failures == 0 ? 0 : 1);
}
