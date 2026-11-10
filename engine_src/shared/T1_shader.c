#include "T1_shader.h"

#include "T1_assert.h"
#include "T1_macro_settings.h"
#include "T1_mem.h"
#include "T1_std.h"
#include "T1_token.h"
#include "T1_types_public_gpucpu.h"
#include "T1_tex.h"

/*
The subset, beyond plain C expressions and statements:
- preprocessor lines (#define, #if..., #error, #pragma pack), no #include
- typedef struct {...} Name; and struct Name {...};, no other typedefs
- 32-bit numbers only: float(N), int(N), uint(N), bool, u32, s32, f32,
  matrices floatNxN, no half, double or 8/16-bit types
- constants at program scope: constant float table[4] = {...};
- pointers only as buffer parameters, "const device Type * name" or
  "constant Type * name", used as name[i] or passed on whole to a helper,
  whose parameter has the same name and type as an entry point's buffer
- no &addresses, ->, methods, templates (but as_type<T>(x)), samplers or
  texture types: T1Textures and T1WriteTex through the T1_ helpers
- matrices multiply with mul(v, m), not *
- calls only to your own functions, macros, constructors and the builtins
  below, which T1 knows how to translate
- threadgroup memory only as "threadgroup Type name[N];" at the top of a
  kernel's body, T1_threadgroup_barrier() only at the top of one too
- what one language refuses or runs differently and T1 can't translate, see
  THE PORTABILITY CHECK
*/

typedef u8 ShaderTokenKind;
enum {
    SK_SPACE, // never stored
    SK_PREPROC,
    SK_IDENT,
    SK_NUMBER,
    SK_ATTR, // [[ ... ]]
    SK_PUNCT,
};

static const c8 * const puncts[] = {
    "->", "::", "<<=", ">>=", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||",
    "++", "--", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
    "(", ")", "{", "}", "[", "]", ";", ",", ".", "?", ":", "~", "!", "%",
    "^", "&", "*", "-", "+", "=", "<", ">", "|", "/",
};

static const c8 * const builtin_types[] = {
    "void", "bool", "bool2", "bool3", "bool4",
    "float", "float2", "float3", "float4",
    "float2x2", "float3x3", "float4x4", "int", "int2", "int3", "int4",
    "uint", "uint2", "uint3", "uint4", "u32", "s32", "f32",
    "uint32_t", "int32_t", "T1Textures", "T1WriteTex",
};

static const c8 * const builtin_functions[] = {
    "abs", "acos", "all", "any", "asin", "atan", "atan2", "ceil", "clamp",
    "cos", "cross", "distance", "dot", "exp", "exp2", "floor", "fmax",
    "fmin", "fmod", "fract", "fwidth", "length", "log", "log2", "max", "min", "mix",
    "normalize", "pow", "reflect", "round", "rsqrt", "saturate", "sign",
    "sin", "smoothstep", "sqrt", "step", "tan", "transpose", "trunc",
    "discard_fragment", "mul", "as_type",
    "T1_sample_nearest", "T1_sample_linear", "T1_sample_linear_repeat",
    "T1_read", "T1_write_tex_read", "T1_write_tex_write",
    "T1_write_tex_size",
    "T1_atomic_load", "T1_atomic_store", "T1_atomic_add", "T1_atomic_min",
    "T1_atomic_max", "T1_atomic_and", "T1_atomic_or", "T1_atomic_xor",
    "T1_atomic_exchange", "T1_atomic_compare_exchange",
    "T1_threadgroup_barrier",
};

// keywords that may stand before '(' without being a call
static const c8 * const paren_keywords[] = {
    "if", "for", "while", "switch", "return",
};

static const c8 * const attributes[] = {
    "vertex_id", "instance_id", "position", "stage_in", "buffer",
    "texture", "color", "thread_position_in_grid",
    "thread_position_in_threadgroup", "threadgroup_position_in_grid",
    "flat", "maybe_unused",
};

typedef struct {
    const c8 * name;
    const c8 * why;
} Banned;

static const Banned banned[] = {
    {"half",   "floats only, no half"},
    {"half2",  "floats only, no half"},
    {"half3",  "floats only, no half"},
    {"half4",  "floats only, no half"},
    {"double", "no double"},
    {"short",  "32-bit numbers only"},
    {"ushort", "32-bit numbers only"},
    {"char",   "32-bit numbers only"},
    {"uchar",  "32-bit numbers only"},
    {"s16",    "32-bit numbers only"},
    {"u16",    "32-bit numbers only"},
    {"s8",     "32-bit numbers only"},
    {"u8",     "32-bit numbers only"},
    {"int16_t", "32-bit numbers only"},
    {"uint16_t", "32-bit numbers only"},
    {"int8_t", "32-bit numbers only"},
    {"uint8_t", "32-bit numbers only"},
    {"sampler", "no samplers of your own, use T1_sample_*()"},
    {"array",  "no array<>, T1Textures holds every texture"},
    {"texture2d", "use T1Textures or T1WriteTex"},
    {"texture2d_array", "use T1Textures or T1WriteTex"},
    {"texture3d", "use T1Textures or T1WriteTex"},
    {"texturecube", "use T1Textures or T1WriteTex"},
    {"depth2d", "use T1Textures or T1WriteTex"},
    {"thread", "no address spaces except buffer parameters"},
    {"template", "no templates"},
    {"typename", "no templates"},
    {"namespace", "no namespaces"},
    {"using",  "no using, T1 puts what's needed in front"},
    {"metal",  "no namespaces"},
    {"class",  "C, not C++"},
    {"operator", "C, not C++"},
    {"auto",   "C, not C++"},
    {"this",   "C, not C++"},
    {"new",    "C, not C++"},
    {"delete", "C, not C++"},
    {"virtual", "C, not C++"},
    {"public", "C, not C++"},
    {"private", "C, not C++"},
    {"static_cast", "C, not C++, cast with (type)"},
    {"reinterpret_cast", "C, not C++, cast with as_type<type>()"},
    {"enum",   "no enums, use #define"},
    {"union",  "no unions"},
    {"goto",   "no goto"},
    {"register", "not a name Metal takes"},
    {"sizeof", "no sizeof"},
    {"extern", "no extern"},
    {"volatile", "no volatile"},
};

#define FUNCS_CAP 256
#define PTRS_CAP 32
#define NAMES_CAP 1024
#define ERRORS_CAP 4

typedef struct {
    const c8 * text;
    u32 size;
} Name;

typedef struct {
    u32 type_i; // token indices
    u32 name_i;
} PtrParam;

#define PARAMS_CAP 32

typedef u8 Stage;
enum {
    STAGE_NONE, // a helper
    STAGE_VERTEX,
    STAGE_FRAGMENT,
    STAGE_KERNEL,
};

typedef struct {
    PtrParam ptrs[PTRS_CAP];
    u32 params_start[PARAMS_CAP]; // token ranges, end exclusive
    u32 params_end[PARAMS_CAP];
    u8 param_global[PARAMS_CAP]; // a buffer, T1Textures or T1WriteTex
    u32 params_size;
    u32 decl_start; // its first word
    u32 name_i;
    u32 params_open; // '('
    u32 params_close;
    u32 body_open;   // '{'
    u32 body_close;
    u32 ptrs_size;
    b8 is_entry;
    Stage stage;
} Func;

typedef struct {
    u32 name_i;
    u32 body_open; // '{'
    u32 body_close;
} StructDef;

typedef struct {
    Func funcs[FUNCS_CAP];
    Name structs[NAMES_CAP];
    Name macros[NAMES_CAP];
    Name matrices[NAMES_CAP];
    StructDef struct_defs[NAMES_CAP];
    u32 struct_defs_size;
    u32 funcs_size;
    u32 structs_size;
    u32 macros_size;
    u32 matrices_size;
    u32 errors_size;
    c8 * error_recip;
    u32 error_recip_cap;
    u32 count;
    /*
    1 for tokens in an #elif or #else branch: they're checked like all
    others, but brackets are matched in the first branches only (each
    branch must open and close the same ones)
    */
    u8 * secondary;
    const c8 * source; // the text that was tokenized
} Check;

static u32 tz = 0;
// what the translation is for: GLSL (0) or HLSL (1), see cat_name()
static b8 hlsl_target = 0;

static void init_tokenizer(void) {
    tz = T1_token_new();
    T1_token_add_run(tz, " \t\r\n", " \t\r\n", SK_SPACE, 1);
    T1_token_add_delimited(tz, "//", "\n", 0, 0, SK_SPACE, 1);
    T1_token_add_delimited(tz, "/*", "*/", 0, 1, SK_SPACE, 1);
    // a preprocessor line, a '\' at its end continues it
    T1_token_add_delimited(tz, "#", "\n", '\\', 0, SK_PREPROC, 0);
    T1_token_add_delimited(tz, "[[", "]]", 0, 1, SK_ATTR, 0);
    T1_token_add_run(tz, "A-Za-z_", "A-Za-z0-9_", SK_IDENT, 0);
    T1_token_add_run(tz, "0-9", "0-9A-Za-z_.", SK_NUMBER, 0);
    for (u32 i = 0; i < sizeof(puncts) / sizeof(puncts[0]); i++) {
        T1_token_add_literal(tz, puncts[i], SK_PUNCT, 0);
    }
    T1_token_build(tz);
}

static u32 kind(const u32 i) {
    return T1_token_kind(tz, i);
}

static b8 is(const u32 i, const c8 * text) {
    return T1_token_equals(tz, i, text);
}

static b8 is_punct(const u32 i, const c8 * text) {
    return kind(i) == SK_PUNCT && is(i, text);
}

static b8 equal_names(const Name a, const u32 i) {
    if (a.size != T1_token_size(tz, i)) { return 0; }
    return T1_std_are_equal_strings_of_length(
        a.text, T1_token_text(tz, i), a.size);
}

static b8 same_text(const u32 a, const u32 b) {
    Name n = {T1_token_text(tz, a), T1_token_size(tz, a)};
    return equal_names(n, b);
}

static b8 in_list(const u32 i, const c8 * const * list, const u32 size) {
    for (u32 j = 0; j < size; j++) {
        if (is(i, list[j])) { return 1; }
    }
    return 0;
}

static b8 in_names(const u32 i, const Name * names, const u32 size) {
    for (u32 j = 0; j < size; j++) {
        if (equal_names(names[j], i)) { return 1; }
    }
    return 0;
}

static void add_name(Name * names, u32 * size, const c8 * text, u32 n) {
    if (*size >= NAMES_CAP) { return; }
    names[*size].text = text;
    names[*size].size = n;
    *size += 1;
}

static b8 is_type(const Check * c, const u32 i) {
    return
        kind(i) == SK_IDENT &&
        (in_list(i, builtin_types,
            sizeof(builtin_types) / sizeof(builtin_types[0])) ||
        in_names(i, c->structs, c->structs_size));
}

// floatNxN
static b8 is_matrix_type(const u32 i) {
    const c8 * t = T1_token_text(tz, i);
    return
        kind(i) == SK_IDENT &&
        T1_token_size(tz, i) == 8 &&
        T1_std_are_equal_strings_of_length(t, "float", 5) &&
        t[6] == 'x';
}

// "line N: why: token"
static void report(Check * c, const u32 i, const c8 * why) {
    c->errors_size += 1;
    if (c->errors_size > ERRORS_CAP || c->error_recip == NULL) { return; }

    c8 * e = c->error_recip;
    u32 cap = c->error_recip_cap;
    if (e[0] == '\0') {
        T1_std_strcat_cap(e, cap,
            "Outside T1's shader subset (see RENDERING in T1.h):");
    }
    T1_std_strcat_cap(e, cap, "\n");
    T1_std_strcat_cap(e, cap, "line ");
    c8 line[16];
    line[0] = '\0';
    T1_std_strcat_u32_cap(line, 16, T1_token_line(tz, i));
    T1_std_strcat_cap(e, cap, line);
    T1_std_strcat_cap(e, cap, ": ");
    T1_std_strcat_cap(e, cap, why);
    T1_std_strcat_cap(e, cap, ": ");

    // the token, up to 40 bytes of its first line
    c8 text[41];
    const c8 * t = T1_token_text(tz, i);
    u32 n = 0;
    while (n < T1_token_size(tz, i) && n < 40 && t[n] != '\n') {
        text[n] = t[n];
        n++;
    }
    text[n] = '\0';
    T1_std_strcat_cap(e, cap, text);
}

static b8 preproc_is(const u32 i, const c8 * directive) {
    if (kind(i) != SK_PREPROC) { return 0; }
    const c8 * t = T1_token_text(tz, i) + 1;
    while (*t == ' ' || *t == '\t') { t++; }
    u32 n = (u32)T1_std_strlen(directive);
    return T1_std_are_equal_strings_of_length(t, directive, n);
}

static void mark_secondary(Check * c) {
    // per #if level, whether we're past its first branch
    u8 past_first[64];
    u32 levels = 0;
    u32 secondary_levels = 0;
    for (u32 i = 0; i < c->count; i++) {
        if (preproc_is(i, "if")) { // also #ifdef, #ifndef
            if (levels < 64) { past_first[levels] = 0; }
            levels++;
        } else if (preproc_is(i, "elif") || preproc_is(i, "else")) {
            if (levels > 0 && levels <= 64 && !past_first[levels - 1]) {
                past_first[levels - 1] = 1;
                secondary_levels++;
            }
        } else if (preproc_is(i, "endif") && levels > 0) {
            if (levels <= 64 && past_first[levels - 1]) {
                secondary_levels--;
            }
            levels--;
        }
        c->secondary[i] = secondary_levels > 0;
    }
}

// A bracket that counts for matching
static b8 is_bracket(const Check * c, const u32 i, const c8 * text) {
    return i < c->count && !c->secondary[i] && is_punct(i, text);
}

// The token after the one matching open (a '(', '{' or '['), or count
static u32 after_match(
    const Check * c,
    const u32 open,
    const c8 * open_s,
    const c8 * close_s)
{
    u32 depth = 0;
    for (u32 i = open; i < c->count; i++) {
        if (is_bracket(c, i, open_s)) { depth++; }
        if (is_bracket(c, i, close_s)) {
            depth--;
            if (depth == 0) { return i + 1; }
        }
    }
    return c->count;
}

// Functions, structs, macros and the names of matrices
static void collect(Check * c) {
    u32 depth = 0; // of {}
    for (u32 i = 0; i < c->count; i++) {
        u32 k = kind(i);

        if (k == SK_PREPROC) {
            const c8 * t = T1_token_text(tz, i) + 1;
            while (*t == ' ' || *t == '\t') { t++; }
            if (T1_std_are_equal_strings_of_length(t, "define", 6)) {
                t += 6;
                while (*t == ' ' || *t == '\t') { t++; }
                u32 n = 0;
                while (
                    (t[n] >= 'A' && t[n] <= 'Z') ||
                    (t[n] >= 'a' && t[n] <= 'z') ||
                    (t[n] >= '0' && t[n] <= '9') ||
                    t[n] == '_')
                {
                    n++;
                }
                add_name(c->macros, &c->macros_size, t, n);
            }
            continue;
        }

        if (is_matrix_type(i) && kind(i + 1) == SK_IDENT) {
            add_name(c->matrices, &c->matrices_size,
                T1_token_text(tz, i + 1), T1_token_size(tz, i + 1));
        }

        if (is(i, "struct") && kind(i + 1) == SK_IDENT) {
            // struct Name {
            add_name(c->structs, &c->structs_size,
                T1_token_text(tz, i + 1), T1_token_size(tz, i + 1));
            if (is_punct(i + 2, "{") && c->struct_defs_size < NAMES_CAP) {
                StructDef * d = &c->struct_defs[c->struct_defs_size++];
                d->name_i = i + 1;
                d->body_open = i + 2;
                d->body_close = after_match(c, i + 2, "{", "}") - 1;
            }
        }
        if (is(i, "typedef") && is(i + 1, "struct") && is_punct(i + 2, "{")) {
            // typedef struct {...} Name;
            u32 name = after_match(c, i + 2, "{", "}");
            if (kind(name) == SK_IDENT) {
                add_name(c->structs, &c->structs_size,
                    T1_token_text(tz, name), T1_token_size(tz, name));
                if (c->struct_defs_size < NAMES_CAP) {
                    StructDef * d = &c->struct_defs[c->struct_defs_size++];
                    d->name_i = name;
                    d->body_open = i + 2;
                    d->body_close = name - 1;
                }
            }
        }

        if (is_bracket(c, i, "{")) { depth++; }
        if (is_bracket(c, i, "}") && depth > 0) { depth--; }

        // a function definition: name ( ... ) {
        if (
            depth == 0 &&
            k == SK_IDENT &&
            is_bracket(c, i + 1, "(") &&
            c->funcs_size < FUNCS_CAP)
        {
            u32 close = after_match(c, i + 1, "(", ")") - 1;
            if (!is_bracket(c, close + 1, "{")) { continue; }

            Func * f = &c->funcs[c->funcs_size++];
            f->name_i = i;
            f->params_open = i + 1;
            f->params_close = close;
            f->body_open = close + 1;
            f->body_close = after_match(c, close + 1, "{", "}") - 1;
            f->ptrs_size = 0;

            // vertex/fragment/kernel among the words before the name
            f->is_entry = 0;
            f->stage = STAGE_NONE;
            f->decl_start = i;
            for (u32 j = i; j > 0; j--) {
                u32 b = j - 1;
                if (kind(b) != SK_IDENT) { break; }
                f->decl_start = b;
                if (is(b, "vertex")) { f->stage = STAGE_VERTEX; }
                if (is(b, "fragment")) { f->stage = STAGE_FRAGMENT; }
                if (is(b, "kernel")) { f->stage = STAGE_KERNEL; }
            }
            f->is_entry = f->stage != STAGE_NONE;

            // the parameters, split at commas outside brackets
            f->params_size = 0;
            u32 start = f->params_open + 1;
            u32 nesting = 0;
            for (u32 j = start; j <= f->params_close && start < close; j++) {
                if (j == f->params_close || (nesting == 0 && is_punct(j, ","))) {
                    if (f->params_size < PARAMS_CAP) {
                        u32 p = f->params_size++;
                        f->params_start[p] = start;
                        f->params_end[p] = j;
                        f->param_global[p] = 0;
                        for (u32 t = start; t < j; t++) {
                            if (
                                is_punct(t, "*") ||
                                is(t, "T1Textures") ||
                                is(t, "T1WriteTex"))
                            {
                                f->param_global[p] = 1;
                            }
                        }
                    }
                    start = j + 1;
                    continue;
                }
                if (is_punct(j, "(") || is_punct(j, "[")) { nesting++; }
                if (is_punct(j, ")") || is_punct(j, "]")) { nesting--; }
            }

            // pointer parameters: ... Type * name [[...]]
            for (u32 j = f->params_open + 1; j < f->params_close; j++) {
                if (
                    is_punct(j, "*") &&
                    kind(j - 1) == SK_IDENT &&
                    kind(j + 1) == SK_IDENT &&
                    f->ptrs_size < PTRS_CAP)
                {
                    f->ptrs[f->ptrs_size].type_i = j - 1;
                    f->ptrs[f->ptrs_size].name_i = j + 1;
                    f->ptrs_size++;
                }
            }

            i = f->body_open - 1; // its body is depth 1
        }
    }
}

static const Func * func_at(const Check * c, const u32 i) {
    for (u32 f = 0; f < c->funcs_size; f++) {
        if (i >= c->funcs[f].params_open && i <= c->funcs[f].body_close) {
            return &c->funcs[f];
        }
    }
    return NULL;
}

static b8 is_defined_func(const Check * c, const u32 i) {
    for (u32 f = 0; f < c->funcs_size; f++) {
        if (same_text(c->funcs[f].name_i, i)) { return 1; }
    }
    return 0;
}

// The end of an operand: a name, number, ')' or ']'
static b8 ends_operand(const u32 i) {
    return
        (kind(i) == SK_IDENT && !is(i, "return")) ||
        kind(i) == SK_NUMBER ||
        is_punct(i, ")") ||
        is_punct(i, "]");
}

/*
The name an operand starting at i ends with: things[0].model ends with
model. Not a name (UINT32_MAX) if it ends with an index or a call.
*/
static u32 operand_last_name(const Check * c, u32 i) {
    if (kind(i) != SK_IDENT) { return UINT32_MAX; }
    u32 last = i;
    i++;
    while (i < c->count) {
        if (is_punct(i, ".") && kind(i + 1) == SK_IDENT) {
            last = i + 1;
            i += 2;
        } else if (is_punct(i, "[")) {
            i = after_match(c, i, "[", "]");
            last = UINT32_MAX;
        } else {
            break;
        }
    }
    return is_punct(i, "(") ? UINT32_MAX : last;
}

static void check_preproc(Check * c, const u32 i) {
    const c8 * t = T1_token_text(tz, i) + 1;
    while (*t == ' ' || *t == '\t') { t++; }
    static const c8 * const ok[] = {
        "define", "undef", "ifdef", "ifndef", "if", "elif", "else",
        "endif", "error", "pragma pack",
    };
    if (T1_std_are_equal_strings_of_length(t, "include", 7)) {
        report(c, i, "no #include, T1 puts what's needed in front");
        return;
    }
    for (u32 j = 0; j < sizeof(ok) / sizeof(ok[0]); j++) {
        u32 n = (u32)T1_std_strlen(ok[j]);
        if (T1_std_are_equal_strings_of_length(t, ok[j], n)) { return; }
    }
    report(c, i, "unsupported preprocessor line");
}

static void check_attribute(Check * c, const u32 i) {
    // [[ a, b(1) ]]: every name before a '(' or ',' must be known
    const c8 * t = T1_token_value(tz, i);
    while (*t != '\0') {
        while (*t == ' ' || *t == ',' || *t == '\t') { t++; }
        if (*t == '\0') { break; }
        const c8 * name = t;
        while (*t != '\0' && *t != ' ' && *t != ',' && *t != '(') { t++; }
        u32 n = (u32)(t - name);
        b8 known = 0;
        for (u32 j = 0; j < sizeof(attributes) / sizeof(attributes[0]); j++) {
            if (
                T1_std_strlen(attributes[j]) == n &&
                T1_std_are_equal_strings_of_length(name, attributes[j], n))
            {
                known = 1;
            }
        }
        if (!known) {
            report(c, i, "unsupported attribute");
            return;
        }
        if (*t == '(') {
            while (*t != '\0' && *t != ')') { t++; }
            if (*t == ')') { t++; }
        }
    }
}

/*
Outside every function and struct: where "constant float table[4] = {...};"
makes a constant (MSL needs program scope variables in the constant address
space, other languages just call them const)
*/
static b8 at_program_scope(const Check * c, const u32 i) {
    if (func_at(c, i) != NULL) { return 0; }
    for (u32 d = 0; d < c->struct_defs_size; d++) {
        const StructDef * def = &c->struct_defs[d];
        if (i > def->body_open && i < def->body_close) { return 0; }
    }
    return 1;
}

// how many { of f's body i is inside (1: directly in the body)
static u32 body_depth(const Check * c, const Func * f, const u32 i) {
    u32 depth = 0;
    for (u32 t = f->body_open; t < i; t++) {
        if (is_bracket(c, t, "{")) { depth++; }
        if (is_bracket(c, t, "}")) { depth--; }
    }
    return depth;
}

static b8 is_atomic_call(const u32 i) {
    return
        T1_token_size(tz, i) > 10 &&
        T1_std_are_equal_strings_of_length(
            T1_token_text(tz, i), "T1_atomic_", 10);
}

/*
T1_atomic_*(buffer, i, ...): the buffer must be a writable one of uints, in
a kernel or a helper (helpers get the buffers of the entry point that calls
them, and only a kernel's can be written)
*/
static void check_atomic_call(Check * c, const Func * f, const u32 i) {
    if (f == NULL || (f->is_entry && f->stage != STAGE_KERNEL)) {
        report(c, i, "T1_atomic_*() only in kernels and their helpers");
        return;
    }
    const PtrParam * ptr = NULL;
    if (is_punct(i + 1, "(")) {
        for (u32 p = 0; p < f->ptrs_size; p++) {
            if (same_text(f->ptrs[p].name_i, i + 2)) { ptr = &f->ptrs[p]; }
        }
    }
    b8 uints = ptr != NULL && (
        is(ptr->type_i, "uint") || is(ptr->type_i, "u32") ||
        is(ptr->type_i, "uint32_t"));
    b8 writable = uints && is(ptr->type_i - 1, "device");
    for (u32 t = f->params_open; writable && t < ptr->type_i; t++) {
        // (the const of this parameter: after the last comma before it)
        if (is_punct(t, ",") || is_punct(t, "(")) { writable = 1; }
        if (is(t, "const") && t + 2 == ptr->type_i) { writable = 0; }
    }
    if (!writable) {
        report(c, i, "T1_atomic_*() takes a writable buffer of uints");
    }
}

// threadgroup Type name[N]; at the top of a kernel's body
static void check_threadgroup_decl(
    Check * c,
    const Func * f,
    const u32 i)
{
    if (
        f == NULL ||
        f->stage != STAGE_KERNEL ||
        i <= f->body_open ||
        body_depth(c, f, i) != 1 ||
        !is_type(c, i + 1) ||
        kind(i + 2) != SK_IDENT ||
        !is_punct(i + 3, "[") ||
        !is_punct(after_match(c, i + 3, "[", "]"), ";"))
    {
        report(c, i,
            "threadgroup memory only as threadgroup Type name[N]; at the "
            "top of a kernel's body");
    }
}

static void check_tokens(Check * c) {
    const u32 banned_size = sizeof(banned) / sizeof(banned[0]);

    for (u32 i = 0; i < c->count; i++) {
        u32 k = kind(i);
        const Func * f = func_at(c, i);
        b8 in_params = f != NULL && i > f->params_open && i < f->params_close;

        if (k == T1_TOKEN_UNMATCHED) {
            report(c, i, "not part of the subset");
            continue;
        }
        if (k == SK_PREPROC) { check_preproc(c, i); continue; }
        if (k == SK_ATTR) { check_attribute(c, i); continue; }

        if (k == SK_IDENT) {
            for (u32 b = 0; b < banned_size; b++) {
                if (is(i, banned[b].name)) { report(c, i, banned[b].why); }
            }

            if (is(i, "typedef") && !is(i + 1, "struct")) {
                report(c, i, "no typedefs except typedef struct");
            }
            if (is(i, "threadgroup")) { check_threadgroup_decl(c, f, i); }
            if (is_atomic_call(i)) { check_atomic_call(c, f, i); }
            if (
                is(i, "T1_threadgroup_barrier") &&
                (f == NULL || f->stage != STAGE_KERNEL ||
                    body_depth(c, f, i) != 1))
            {
                report(c, i,
                    "T1_threadgroup_barrier() only at the top of a "
                    "kernel's body, outside every if and loop");
            }
            if (
                (is(i, "device") || is(i, "constant")) &&
                !in_params &&
                !(is(i, "constant") && at_program_scope(c, i)))
            {
                report(c, i,
                    "address spaces only for buffer parameters and "
                    "constants at program scope");
            }

            // a call
            if (
                is_punct(i + 1, "(") &&
                !is_punct(i - 1, ".") &&
                !is_defined_func(c, i) &&
                !is_type(c, i) &&
                !in_names(i, c->macros, c->macros_size) &&
                !in_list(i, builtin_functions,
                    sizeof(builtin_functions) / sizeof(builtin_functions[0])) &&
                !in_list(i, paren_keywords,
                    sizeof(paren_keywords) / sizeof(paren_keywords[0])))
            {
                report(c, i, "unknown function, not one T1 can translate");
            }

            // methods: .sample(), .read(), ...
            if (is_punct(i - 1, ".") && is_punct(i + 1, "(")) {
                report(c, i, "no methods, use the T1_ helpers");
            }

            // templates, except as_type<T>()
            if (is_punct(i + 1, "<") && is_type(c, i)) {
                report(c, i, "no templates");
            }

            // a buffer, used as name[i] or passed on whole
            if (f != NULL && i > f->body_open && !is_punct(i - 1, ".")) {
                for (u32 p = 0; p < f->ptrs_size; p++) {
                    if (!same_text(f->ptrs[p].name_i, i)) { continue; }
                    b8 indexed = is_punct(i + 1, "[");
                    b8 passed =
                        (is_punct(i - 1, "(") || is_punct(i - 1, ",")) &&
                        (is_punct(i + 1, ",") || is_punct(i + 1, ")"));
                    if (!indexed && !passed) {
                        report(c, i,
                            "use a buffer as name[i] or pass it on whole");
                    }
                }
            }
            continue;
        }

        if (k != SK_PUNCT) { continue; }

        if (is(i, "->")) {
            report(c, i, "no pointers, use buffer[i].field");
        } else if (is(i, "::")) {
            report(c, i, "no namespaces");
        } else if (is(i, "*")) {
            if (is_type(c, i - 1)) {
                // a pointer: only buffer parameters
                b8 buffer = 0;
                if (in_params) {
                    for (u32 j = i; j > f->params_open; j--) {
                        if (is_punct(j, ",")) { break; }
                        if (is(j, "device") || is(j, "constant")) {
                            buffer = 1;
                        }
                    }
                }
                if (!buffer) {
                    report(c, i,
                        "pointers only as device or constant buffer "
                        "parameters");
                }
            } else if (
                in_names(i - 1, c->matrices, c->matrices_size) ||
                is_matrix_type(i + 1) ||
                (operand_last_name(c, i + 1) != UINT32_MAX &&
                in_names(operand_last_name(c, i + 1),
                    c->matrices, c->matrices_size)))
            {
                report(c, i, "multiply matrices with mul(v, m), not *");
            }
        } else if (is(i, "&")) {
            if (is_type(c, i - 1) || !ends_operand(i - 1)) {
                report(c, i, "no & (addresses or references)");
            }
        } else if (is(i, "<")) {
            if (is_type(c, i - 1)) { report(c, i, "no templates"); }
        }
    }

    // a helper's buffer must have an entry point buffer's name and type
    for (u32 h = 0; h < c->funcs_size; h++) {
        const Func * helper = &c->funcs[h];
        if (helper->is_entry) { continue; }
        for (u32 p = 0; p < helper->ptrs_size; p++) {
            b8 found = 0;
            for (u32 e = 0; e < c->funcs_size && !found; e++) {
                const Func * entry = &c->funcs[e];
                if (!entry->is_entry) { continue; }
                for (u32 q = 0; q < entry->ptrs_size; q++) {
                    if (
                        same_text(entry->ptrs[q].name_i,
                            helper->ptrs[p].name_i) &&
                        same_text(entry->ptrs[q].type_i,
                            helper->ptrs[p].type_i))
                    {
                        found = 1;
                    }
                }
            }
            if (!found) {
                report(c, helper->ptrs[p].name_i,
                    "a helper's buffer needs the name and type of an entry "
                    "point's buffer");
            }
        }
    }
}

// The subset's rules, without translating
static void check_portable(Check * c);

static void check_subset(
    const c8 * shader_source,
    u8 * good,
    c8 * error_recip,
    const u32 error_recip_cap)
{
    *good = 0;
    T1_assert(shader_source != NULL);
    T1_assert(error_recip == NULL || error_recip_cap >= 512);

    if (tz == 0) { init_tokenizer(); }
    T1_token_run(tz, shader_source, T1_std_strlen(shader_source));

    Check * c = T1_mem_malloc_managed(sizeof(Check));
    T1_std_memset(c, 0, sizeof(Check));
    c->count = T1_token_count(tz);
    c->source = shader_source;
    c->error_recip = error_recip;
    c->error_recip_cap = error_recip_cap;
    if (error_recip != NULL) { error_recip[0] = '\0'; }
    c->secondary = T1_mem_malloc_managed(c->count + 1);
    mark_secondary(c);

    collect(c);
    check_tokens(c);
    if (c->errors_size == 0) { check_portable(c); }

    if (c->errors_size > ERRORS_CAP && error_recip != NULL) {
        T1_std_strcat_cap(error_recip, error_recip_cap, "\n...");
    }
    *good = c->errors_size == 0;
    T1_mem_free_managed(c->secondary);
    T1_mem_free_managed(c);
}

/*
TRANSLATION TO GLSL 4.50 FOR VULKAN

Every entry point (vertex, fragment or kernel function) becomes its own
GLSL unit: a prelude, then T1's settings and GPU headers and the shaders,
without the other entry points and the helpers this one doesn't call. The
text between tokens (spaces, comments) is copied as it is, tokens are kept,
replaced or skipped.

Comparisons of vectors become GLSL's greaterThan() etc., see VECTOR
COMPARISONS IN GLSL.

The layout a Vulkan backend must match:
- set 0, binding n: buffer(n), a std430 storage buffer
- set 1, binding 0: T1Textures, texture2DArray[T1_TEXARRAYS_CAP]
- set 1, bindings 1 to 3: the samplers T1_nearest, T1_linear and
  T1_linear_repeat
- set 1, binding 4: a compute pass's write_tex, an rgba8 image2D
- kernels: local size by their position's type (see kernel_group_size()),
  and a push constant uvec3 with the grid's size, threads past it return
  right away (not in a kernel with a T1_threadgroup_barrier(): its grid is
  a multiple of its group)
- vertex outputs are flipped in y (Vulkan's clip space points down), so a
  backend flips its front face too
- stage structs' members go in locations in their order (matrices take
  one location per column), [[position]] is gl_Position / gl_FragCoord
*/

#include "T1_embedded_data.h"

static const c8 * const glsl_prelude =
    "#version 450\n"
    "#extension GL_EXT_nonuniform_qualifier : require\n"
    "#define T1_GLSL 1\n"
    "#define f32 float\n"
    "#define u32 uint\n"
    "#define s32 int\n"
    "#define uint32_t uint\n"
    "#define int32_t int\n"
    "#define float2 vec2\n"
    "#define float3 vec3\n"
    "#define float4 vec4\n"
    "#define float2x2 mat2\n"
    "#define float3x3 mat3\n"
    "#define float4x4 mat4\n"
    "#define int2 ivec2\n"
    "#define int3 ivec3\n"
    "#define int4 ivec4\n"
    "#define uint2 uvec2\n"
    "#define uint3 uvec3\n"
    "#define uint4 uvec4\n"
    "#define bool2 bvec2\n"
    "#define bool3 bvec3\n"
    "#define bool4 bvec4\n"
    "#define fmax max\n"
    "#define fmin min\n"
    "#define rsqrt inversesqrt\n"
    "#define atan2(y, x) atan(y, x)\n"
    "#define saturate(x) clamp(x, 0.0, 1.0)\n"
    "#define mul(v, m) ((v) * (m))\n"
    // MSL's fmod truncates, GLSL's mod floors
    "float fmod(float a, float b) { return a - b * trunc(a / b); }\n"
    "vec2 fmod(vec2 a, vec2 b) { return a - b * trunc(a / b); }\n"
    "vec3 fmod(vec3 a, vec3 b) { return a - b * trunc(a / b); }\n"
    "vec4 fmod(vec4 a, vec4 b) { return a - b * trunc(a / b); }\n"
    "vec2 fmod(vec2 a, float b) { return a - b * trunc(a / b); }\n"
    "vec3 fmod(vec3 a, float b) { return a - b * trunc(a / b); }\n"
    "vec4 fmod(vec4 a, float b) { return a - b * trunc(a / b); }\n"
    // as_type<T>(x), by x's type
    "uint t1_as_uint(float x) { return floatBitsToUint(x); }\n"
    "uint t1_as_uint(int x) { return uint(x); }\n"
    "uint t1_as_uint(uint x) { return x; }\n"
    "int t1_as_int(float x) { return floatBitsToInt(x); }\n"
    "int t1_as_int(uint x) { return int(x); }\n"
    "int t1_as_int(int x) { return x; }\n"
    "float t1_as_float(uint x) { return uintBitsToFloat(x); }\n"
    "float t1_as_float(int x) { return intBitsToFloat(x); }\n"
    "float t1_as_float(float x) { return x; }\n"
    "uvec4 t1_as_uvec4(vec4 x) { return floatBitsToUint(x); }\n"
    "vec4 t1_as_vec4(uvec4 x) { return uintBitsToFloat(x); }\n"
    // the T1 helpers, their first argument is the texture parameter
    "#define t1_tex_array(tex) "
        "t1_textures[nonuniformEXT(T1_TEX_ARRAY_I(tex))]\n"
    "#define t1_sample(smp, tex, uv) texture(sampler2DArray("
        "t1_tex_array(tex), smp), vec3((uv), float(T1_TEX_SLICE_I(tex))))\n"
    "#define T1_sample_nearest(t, tex, uv) "
        "t1_sample(t1_nearest_smp, tex, uv)\n"
    "#define T1_sample_linear(t, tex, uv) "
        "t1_sample(t1_linear_smp, tex, uv)\n"
    "#define T1_sample_linear_repeat(t, tex, uv) "
        "t1_sample(t1_linear_repeat_smp, tex, uv)\n"
    "#define T1_read(t, tex, pos) texelFetch(sampler2DArray("
        "t1_tex_array(tex), t1_nearest_smp), "
        "ivec3(ivec2(pos), int(T1_TEX_SLICE_I(tex))), 0)\n"
    "#define T1_write_tex_read(t, pos) imageLoad(t1_write_tex, ivec2(pos))\n"
    "#define T1_write_tex_write(t, pos, color) "
        "imageStore(t1_write_tex, ivec2(pos), (color))\n"
    "#define T1_write_tex_size(t) uvec2(imageSize(t1_write_tex))\n"
    "#define T1_atomic_load(b, i) atomicAdd(b[i], 0u)\n"
    "#define T1_atomic_store(b, i, v) atomicExchange(b[i], (v))\n"
    "#define T1_atomic_add(b, i, v) atomicAdd(b[i], (v))\n"
    "#define T1_atomic_min(b, i, v) atomicMin(b[i], (v))\n"
    "#define T1_atomic_max(b, i, v) atomicMax(b[i], (v))\n"
    "#define T1_atomic_and(b, i, v) atomicAnd(b[i], (v))\n"
    "#define T1_atomic_or(b, i, v) atomicOr(b[i], (v))\n"
    "#define T1_atomic_xor(b, i, v) atomicXor(b[i], (v))\n"
    "#define T1_atomic_exchange(b, i, v) atomicExchange(b[i], (v))\n"
    "#define T1_atomic_compare_exchange(b, i, expected, v) "
        "atomicCompSwap(b[i], (expected), (v))\n"
    "#define T1_threadgroup_barrier() barrier()\n";

static const c8 * const glsl_textures =
    "layout(set = 1, binding = 0) uniform texture2DArray "
        "t1_textures[" T1_TEX_STR(T1_TEXARRAYS_CAP) "];\n"
    "layout(set = 1, binding = 1) uniform sampler t1_nearest_smp;\n"
    "layout(set = 1, binding = 2) uniform sampler t1_linear_smp;\n"
    "layout(set = 1, binding = 3) uniform sampler t1_linear_repeat_smp;\n";

// GLSL words and builtins that can't be names, they get a '_'
static const c8 * const glsl_reserved[] = {
    "in", "out", "inout", "input", "output", "uniform", "buffer", "shared",
    "coherent", "readonly", "writeonly", "restrict", "layout", "centroid",
    "flat", "smooth", "noperspective", "patch", "sample", "varying",
    "attribute", "precise", "invariant", "discard", "lowp", "mediump",
    "highp", "precision", "common", "partition", "active", "filter",
    "resource", "external", "interface", "noinline", "superp", "fixed",
    "cast", "subroutine", "texture", "texelFetch", "imageLoad",
    "imageStore", "imageSize", "main", "vec2", "vec3", "vec4", "ivec2",
    "ivec3", "ivec4", "uvec2", "uvec3", "uvec4", "bvec2", "bvec3",
    "bvec4", "mat2", "mat3", "mat4", "dvec2", "dvec3", "dvec4", "sampler",
    "image2D", "texture2DArray", "sampler2DArray", "mod", "inversesqrt",
};

static const c8 * hlsl_as_type_name(const u32 i);

// the GLSL name of an as_type<> target
static const c8 * glsl_type_name(const u32 i) {
    static const c8 * const map[][2] = {
        {"u32", "uint"}, {"uint", "uint"}, {"uint32_t", "uint"},
        {"s32", "int"}, {"int", "int"}, {"int32_t", "int"},
        {"f32", "float"}, {"float", "float"},
        {"uint4", "uvec4"}, {"float4", "vec4"},
    };
    for (u32 m = 0; m < sizeof(map) / sizeof(map[0]); m++) {
        if (is(i, map[m][0])) { return map[m][1]; }
    }
    return NULL;
}

typedef struct {
    c8 * data;
    u32 size;
    u32 cap;
    b8 overflow;
} Buf;

static void buf_cat_n(Buf * b, const c8 * s, const u32 n) {
    if (b->size + n + 1 > b->cap) { b->overflow = 1; return; }
    T1_std_memcpy(b->data + b->size, s, n);
    b->size += n;
    b->data[b->size] = '\0';
}

static void buf_cat(Buf * b, const c8 * s) {
    buf_cat_n(b, s, (u32)T1_std_strlen(s));
}

typedef struct {
    Check * c;
    Buf arena; // strings for the edits below
    Buf * out;
    // for all units
    u8 * g_skip;
    const c8 ** g_repl;
    const c8 ** g_pre;
    const c8 ** g_post;
    // for the unit being made
    u8 * u_skip;
    u8 * u_drop; // skipped with the text before it
    const c8 ** u_repl;
    const c8 ** u_pre;
    const c8 ** u_post;
    u8 * reachable; // per function
    u32 user_first_token; // the client's first, after T1's headers
    b8 failed;
} Gen;

static b8 is_hlsl_reserved(const u32 i);

static b8 needs_rename(const u32 i) {
    if (kind(i) != SK_IDENT) { return 0; }
    if (hlsl_target) {
        if (is_hlsl_reserved(i)) { return 1; }
    } else if (in_list(i, glsl_reserved,
        sizeof(glsl_reserved) / sizeof(glsl_reserved[0])))
    {
        return 1;
    }
    const c8 * t = T1_token_text(tz, i);
    u32 n = T1_token_size(tz, i);
    if (
        !hlsl_target &&
        n >= 3 && t[0] == 'g' && t[1] == 'l' && t[2] == '_')
    {
        return 1;
    }
    for (u32 j = 0; j + 1 < n; j++) {
        if (t[j] == '_' && t[j + 1] == '_') { return 1; }
    }
    return 0;
}

// a name as it is in GLSL
static void cat_name(Buf * b, const u32 i) {
    buf_cat_n(b, T1_token_text(tz, i), T1_token_size(tz, i));
    if (needs_rename(i)) { buf_cat(b, "_"); }
}

static u32 str_begin(Gen * g) {
    return g->arena.size;
}

static const c8 * str_end(Gen * g, const u32 start) {
    buf_cat_n(&g->arena, "", 0);
    if (g->arena.overflow) { return ""; }
    g->arena.size += 1; // keep the 0
    return g->arena.data + start;
}

// *slot, then text after it
static void add_text(Gen * g, const c8 ** slot, const c8 * text) {
    if (*slot == NULL) { *slot = text; return; }
    u32 s = str_begin(g);
    buf_cat(&g->arena, *slot);
    buf_cat(&g->arena, text);
    *slot = str_end(g, s);
}

static void fail(Gen * g, const u32 i, const c8 * why) {
    report(g->c, i, why);
    g->failed = 1;
}

static b8 is_cast(const Check * c, const u32 i) {
    /*
    After an operand, ( type ) is a call's argument list, except after
    another cast: (float)(uint)x
    */
    const b8 after_cast =
        i >= 3 &&
        is_punct(i - 1, ")") &&
        is_type(c, i - 2) &&
        is_punct(i - 3, "(") &&
        !(i >= 4 && ends_operand(i - 4));
    return
        is_punct(i, "(") &&
        is_type(c, i + 1) &&
        is_punct(i + 2, ")") &&
        !(i > 0 && ends_operand(i - 1) && !after_cast) &&
        (ends_operand(i + 3) || is_punct(i + 3, "(") ||
        is_punct(i + 3, "-") || is_punct(i + 3, "!") ||
        is_punct(i + 3, "~"));
}

// The end (exclusive) of the unary expression starting at i
static u32 unary_end(const Check * c, const u32 i) {
    if (
        is_punct(i, "-") || is_punct(i, "!") || is_punct(i, "~") ||
        is_punct(i, "+"))
    {
        return unary_end(c, i + 1);
    }
    if (is_cast(c, i)) { return unary_end(c, i + 3); }

    u32 j = i + 1;
    if (is_punct(i, "(")) { j = after_match(c, i, "(", ")"); }
    if (
        is(i, "as_type") &&
        is_punct(i + 1, "<") &&
        is_punct(i + 3, ">") &&
        is_punct(i + 4, "("))
    {
        j = after_match(c, i + 4, "(", ")"); // as_type<T>(x)
    }
    while (j < c->count) {
        if (is_punct(j, "(") && kind(j - 1) == SK_IDENT) {
            j = after_match(c, j, "(", ")");
        } else if (is_punct(j, "[")) {
            j = after_match(c, j, "[", "]");
        } else if (is_punct(j, ".") && kind(j + 1) == SK_IDENT) {
            j += 2;
        } else {
            break;
        }
    }
    return j;
}

// Skips the parameters or arguments marked in drop, and their commas
static void skip_list_items(
    Gen * g,
    const u32 * starts,
    const u32 * ends,
    const u8 * drop,
    const u32 size)
{
    for (u32 p = 0; p < size; p++) {
        if (drop[p]) {
            for (u32 t = starts[p]; t < ends[p]; t++) { g->g_skip[t] = 1; }
        }
        // the comma after p stays if p stays and another one follows
        if (p + 1 < size) {
            b8 later_kept = 0;
            for (u32 q = p + 1; q < size; q++) {
                if (!drop[q]) { later_kept = 1; }
            }
            if (drop[p] || !later_kept) { g->g_skip[ends[p]] = 1; }
        }
    }
}

static const Func * find_func(const Check * c, const u32 name_i) {
    for (u32 f = 0; f < c->funcs_size; f++) {
        if (same_text(c->funcs[f].name_i, name_i)) {
            return &c->funcs[f];
        }
    }
    return NULL;
}

// The edits every unit shares
static void global_edits(Gen * g) {
    Check * c = g->c;

    for (u32 i = 0; i < c->count; i++) {
        u32 k = kind(i);

        if (k == SK_PREPROC && preproc_is(i, "pragma")) {
            g->g_skip[i] = 1; // #pragma pack, buffers are std430 anyway
        } else if (k == SK_ATTR) {
            g->g_skip[i] = 1; // entry points and stage structs use them
        } else if (is(i, "static") || is(i, "inline")) {
            g->g_skip[i] = 1;
        } else if (is(i, "constexpr")) {
            // HLSL's globals are uniforms unless they're static
            if (is(i + 1, "const")) {
                if (hlsl_target) {
                    g->g_repl[i] = "static";
                } else {
                    g->g_skip[i] = 1;
                }
            } else {
                g->g_repl[i] = hlsl_target ? "static const" : "const";
            }
        } else if (is(i, "constant") && at_program_scope(c, i)) {
            // a constant at program scope, HLSL's globals are uniforms
            // unless they're static
            if (is(i + 1, "const")) {
                if (hlsl_target) {
                    g->g_repl[i] = "static";
                } else {
                    g->g_skip[i] = 1;
                }
            } else if (is(i - 1, "constexpr")) {
                g->g_skip[i] = 1; // constexpr already became const
            } else {
                g->g_repl[i] = hlsl_target ? "static const" : "const";
            }
        } else if (
            is(i, "discard_fragment") &&
            is_punct(i + 1, "(") &&
            is_punct(i + 2, ")"))
        {
            g->g_repl[i] = "discard";
            g->g_skip[i + 1] = 1;
            g->g_skip[i + 2] = 1;
        } else if (is(i, "as_type") && is_punct(i + 1, "<")) {
            const c8 * type = hlsl_target ?
                hlsl_as_type_name(i + 2) :
                glsl_type_name(i + 2);
            if (type == NULL || !is_punct(i + 3, ">")) {
                fail(g, i, "unsupported as_type<>, use a 32-bit type");
                continue;
            }
            u32 s = str_begin(g);
            buf_cat(&g->arena, hlsl_target ? "as" : "t1_as_");
            buf_cat(&g->arena, type);
            g->g_repl[i] = str_end(g, s);
            g->g_skip[i + 1] = 1;
            g->g_skip[i + 2] = 1;
            g->g_skip[i + 3] = 1;
        } else if (is(i, "typedef")) {
            if (is(i + 1, "struct") && is_punct(i + 2, "{")) {
                // typedef struct {...} Name; -> struct Name {...};
                u32 name = after_match(c, i + 2, "{", "}");
                u32 s = str_begin(g);
                buf_cat(&g->arena, "struct ");
                cat_name(&g->arena, name);
                g->g_repl[i] = str_end(g, s);
                g->g_skip[i + 1] = 1;
                g->g_skip[name] = 1;
            } else {
                /*
                typedef long s64; in T1's own header: the prelude defines
                the 32-bit ones, the others aren't in the subset
                */
                u32 j = i;
                while (j < c->count && !is_punct(j, ";")) {
                    g->g_skip[j++] = 1;
                }
                g->g_skip[j] = 1;
            }
        } else if (is_cast(c, i) && is(i + 1, "void")) {
            // (void)x; only says x is unused, GLSL and HLSL can't cast to void
            u32 end = unary_end(c, i + 3);
            for (u32 t = i; t < end; t++) { g->g_skip[t] = 1; }
        } else if (is_cast(c, i) && !hlsl_target) {
            // (type)x -> type(x)
            g->g_skip[i] = 1;
            g->g_repl[i + 2] = "(";
            add_text(g, &g->g_post[unary_end(c, i + 3) - 1], ")");
        }
    }

    // helpers lose their buffer and texture parameters, calls their args
    for (u32 f = 0; f < c->funcs_size; f++) {
        const Func * fn = &c->funcs[f];
        if (fn->is_entry) { continue; }
        skip_list_items(
            g,
            fn->params_start,
            fn->params_end,
            fn->param_global,
            fn->params_size);
    }
    for (u32 i = 0; i + 1 < c->count; i++) {
        if (kind(i) != SK_IDENT || !is_punct(i + 1, "(")) { continue; }
        const Func * fn = find_func(c, i);
        if (fn == NULL || fn->is_entry || fn->name_i == i) { continue; }

        u32 starts[PARAMS_CAP];
        u32 ends[PARAMS_CAP];
        u32 size = 0;
        u32 close = after_match(c, i + 1, "(", ")") - 1;
        u32 start = i + 2;
        u32 nesting = 0;
        for (u32 j = start; j <= close && start < close; j++) {
            if (j == close || (nesting == 0 && is_punct(j, ","))) {
                if (size < PARAMS_CAP) {
                    starts[size] = start;
                    ends[size] = j;
                    size++;
                }
                start = j + 1;
                continue;
            }
            if (is_punct(j, "(") || is_punct(j, "[")) { nesting++; }
            if (is_punct(j, ")") || is_punct(j, "]")) { nesting--; }
        }
        if (size != fn->params_size) {
            fail(g, i, "a call with the wrong number of arguments");
            continue;
        }
        skip_list_items(g, starts, ends, fn->param_global, size);
    }
}

static void mark_reachable(Gen * g, const u32 f) {
    Check * c = g->c;
    if (g->reachable[f]) { return; }
    g->reachable[f] = 1;
    const Func * fn = &c->funcs[f];
    for (u32 i = fn->body_open; i < fn->body_close; i++) {
        if (kind(i) != SK_IDENT || !is_punct(i + 1, "(")) { continue; }
        for (u32 h = 0; h < c->funcs_size; h++) {
            if (!c->funcs[h].is_entry && same_text(c->funcs[h].name_i, i)) {
                mark_reachable(g, h);
            }
        }
    }
}

static const StructDef * find_struct(const Check * c, const u32 name_i) {
    for (u32 s = 0; s < c->struct_defs_size; s++) {
        if (same_text(c->struct_defs[s].name_i, name_i)) {
            return &c->struct_defs[s];
        }
    }
    return NULL;
}

typedef struct {
    u32 type_i;
    u32 name_i;
    u32 attr_i; // UINT32_MAX if none
} Member;

#define MEMBERS_CAP 32

// A stage struct's members: Type name [[attr]];
static u32 struct_members(
    Gen * g,
    const StructDef * d,
    Member * recip)
{
    u32 size = 0;
    u32 i = d->body_open + 1;
    while (i < d->body_close) {
        if (kind(i) == SK_PREPROC) {
            fail(g, i, "no preprocessor lines in stage structs");
            return 0;
        }
        if (is(i, "const")) { i++; }
        if (
            kind(i) != SK_IDENT ||
            kind(i + 1) != SK_IDENT ||
            size >= MEMBERS_CAP)
        {
            fail(g, i, "stage struct members are: Type name [[...]];");
            return 0;
        }
        recip[size].type_i = i;
        recip[size].name_i = i + 1;
        recip[size].attr_i = UINT32_MAX;
        i += 2;
        if (kind(i) == SK_ATTR) { recip[size].attr_i = i++; }
        if (!is_punct(i, ";")) {
            fail(g, i, "no arrays in stage structs");
            return 0;
        }
        size++;
        i++;
    }
    return size;
}

static b8 is_name_char(const c8 ch) {
    return
        (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
        (ch >= '0' && ch <= '9') || ch == '_';
}

// The attribute has name (as a whole word) in it
static b8 attr_has(const u32 attr_i, const c8 * name) {
    if (attr_i == UINT32_MAX) { return 0; }
    const c8 * t = T1_token_value(tz, attr_i);
    u32 n = (u32)T1_std_strlen(name);
    for (u32 j = 0; t[j] != '\0'; j++) {
        if (
            T1_std_are_equal_strings_of_length(t + j, name, n) &&
            (j == 0 || !is_name_char(t[j - 1])) &&
            !is_name_char(t[j + n]))
        {
            return 1;
        }
    }
    return 0;
}

// What's between the parentheses after name in an attribute: color(1)
static void cat_attr_arg(Buf * b, const u32 attr_i, const c8 * name) {
    const c8 * t = T1_token_value(tz, attr_i);
    u32 n = (u32)T1_std_strlen(name);
    for (u32 j = 0; t[j] != '\0'; j++) {
        if (
            T1_std_are_equal_strings_of_length(t + j, name, n) &&
            t[j + n] == '(')
        {
            u32 k = j + n + 1;
            u32 start = k;
            while (t[k] != '\0' && t[k] != ')') { k++; }
            buf_cat_n(b, t + start, k - start);
            return;
        }
    }
}

static b8 is_integer_type(const u32 i) {
    static const c8 * const ints[] = {
        "u32", "s32", "uint", "int", "uint32_t", "int32_t", "uint2",
        "uint3", "uint4", "int2", "int3", "int4", "bool",
    };
    return in_list(i, ints, sizeof(ints) / sizeof(ints[0]));
}

static u32 locations_of(const u32 type_i) {
    if (is(type_i, "float4x4")) { return 4; }
    if (is(type_i, "float3x3")) { return 3; }
    if (is(type_i, "float2x2")) { return 2; }
    return 1;
}

// layout(location = n) [flat] in/out Type t1_v_name; for a stage struct
static void cat_varyings(
    Gen * g,
    Buf * b,
    const Member * members,
    const u32 size,
    const c8 * direction)
{
    (void)g;
    u32 location = 0;
    for (u32 m = 0; m < size; m++) {
        if (attr_has(members[m].attr_i, "position")) { continue; }
        buf_cat(b, "layout(location = ");
        c8 n[16];
        n[0] = '\0';
        T1_std_strcat_u32_cap(n, 16, location);
        buf_cat(b, n);
        buf_cat(b, ") ");
        if (
            attr_has(members[m].attr_i, "flat") ||
            is_integer_type(members[m].type_i))
        {
            buf_cat(b, "flat ");
        }
        buf_cat(b, direction);
        buf_cat(b, " ");
        cat_name(b, members[m].type_i);
        buf_cat(b, " t1_v_");
        cat_name(b, members[m].name_i);
        buf_cat(b, ";\n");
        location += locations_of(members[m].type_i);
    }
}

// The tokens from..to (exclusive) as GLSL, separated by spaces
static void cat_tokens(Buf * b, const u32 from, const u32 to) {
    for (u32 t = from; t < to; t++) {
        if (kind(t) == SK_ATTR) { continue; }
        if (t > from) { buf_cat(b, " "); }
        cat_name(b, t);
    }
}

// The source with the global and the unit's edits, after what's in g->out
static void write_unit(Gen * g) {
    u32 prev_end = 0; // offset in the source after the previous token
    for (u32 i = 0; i < g->c->count; i++) {
        const c8 * t = T1_token_text(tz, i);
        u32 size = T1_token_size(tz, i);
        u32 offset = (u32)(t - g->c->source);
        if (!g->u_drop[i]) {
            buf_cat_n(g->out, g->c->source + prev_end, offset - prev_end);
            if (g->g_pre[i]) { buf_cat(g->out, g->g_pre[i]); }
            if (g->u_pre[i]) { buf_cat(g->out, g->u_pre[i]); }
            if (!g->g_skip[i] && !g->u_skip[i]) {
                if (g->u_repl[i]) {
                    buf_cat(g->out, g->u_repl[i]);
                } else if (g->g_repl[i]) {
                    buf_cat(g->out, g->g_repl[i]);
                } else if (kind(i) == SK_PREPROC) {
                    // __METAL_VERSION__ -> T1_GLSL or T1_HLSL
                    const c8 * mv = "__METAL_VERSION__";
                    u32 j = 0;
                    while (j < size) {
                        if (
                            j + 17 <= size &&
                            T1_std_are_equal_strings_of_length(t + j, mv, 17))
                        {
                            buf_cat(g->out, hlsl_target ? "T1_HLSL" : "T1_GLSL");
                            j += 17;
                        } else {
                            buf_cat_n(g->out, t + j, 1);
                            j++;
                        }
                    }
                } else {
                    cat_name(g->out, i);
                }
            }
            if (g->g_post[i]) { buf_cat(g->out, g->g_post[i]); }
            if (g->u_post[i]) { buf_cat(g->out, g->u_post[i]); }
        }
        prev_end = offset + size;
    }
    buf_cat(g->out, g->c->source + prev_end);
    buf_cat(g->out, "\n");
}

/*
A unit's start, in both translations: no edits of its own yet, and the
functions entry_f doesn't reach dropped. Returns where the first function
it keeps starts
*/
static u32 unit_begin(Gen * g, const u32 entry_f) {
    Check * c = g->c;
    u32 count = c->count;

    T1_std_memset(g->u_skip, 0, count + 1);
    T1_std_memset(g->u_drop, 0, count + 1);
    for (u32 i = 0; i <= count; i++) {
        g->u_repl[i] = NULL;
        g->u_pre[i] = NULL;
        g->u_post[i] = NULL;
    }
    T1_std_memset(g->reachable, 0, c->funcs_size);
    mark_reachable(g, entry_f);

    // what's left out
    u32 first_kept_func = UINT32_MAX;
    for (u32 f = 0; f < c->funcs_size; f++) {
        const Func * fn = &c->funcs[f];
        if (!g->reachable[f]) {
            for (u32 t = fn->decl_start; t <= fn->body_close; t++) {
                g->u_drop[t] = 1;
            }
        } else if (fn->decl_start < first_kept_func) {
            first_kept_func = fn->decl_start;
        }
    }
    return first_kept_func;
}

static void cat_u32(Buf * b, const u32 v);

/*
A kernel's group, by the type of its position in the grid (or in its group):
uint: 256 x 1 x 1, uint2: 16 x 16 x 1, uint3: 8 x 8 x 4
*/
static void kernel_group_size(const Func * e, u32 threads[3]) {
    threads[0] = 16; threads[1] = 16; threads[2] = 1;
    for (u32 p = 0; p < e->params_size; p++) {
        for (u32 t = e->params_start[p]; t < e->params_end[p]; t++) {
            if (
                kind(t) != SK_ATTR ||
                (!attr_has(t, "thread_position_in_grid") &&
                !attr_has(t, "thread_position_in_threadgroup") &&
                !attr_has(t, "threadgroup_position_in_grid")))
            {
                continue;
            }
            const u32 type_i = t - 2;
            if (is(type_i, "uint3")) {
                threads[0] = 8; threads[1] = 8; threads[2] = 4;
            } else if (!is(type_i, "uint2")) {
                threads[0] = 256; threads[1] = 1; threads[2] = 1;
            }
            return;
        }
    }
}

static b8 kernel_has_barrier(const Func * e) {
    for (u32 t = e->body_open; t < e->body_close; t++) {
        if (is(t, "T1_threadgroup_barrier")) { return 1; }
    }
    return 0;
}

/*
The kernel's threadgroup memory goes out of its body, to program scope as
the keyword says ("shared " in GLSL, "groupshared " in HLSL), right in front
of the kernel, after the structs and macros it may use (so after the unit's
other edits there): every entry point is a unit of its own, so nothing else
sees it
*/
static void threadgroup_decls(
    Gen * g,
    const Func * e,
    const c8 * keyword)
{
    Buf * b = &g->arena;
    for (u32 t = e->body_open; t < e->body_close; t++) {
        if (!is(t, "threadgroup")) { continue; }
        const u32 semi = after_match(g->c, t + 3, "[", "]");
        const u32 start = str_begin(g);
        buf_cat(b, keyword);
        cat_name(b, t + 1);
        buf_cat(b, " ");
        cat_tokens(b, t + 2, semi);
        buf_cat(b, ";\n");
        add_text(g, &g->u_pre[e->decl_start], str_end(g, start));
        for (u32 s = t; s <= semi; s++) { g->u_skip[s] = 1; }
    }
}

// a writable buffer's atomics (T1_atomic_*() in HLSL's prelude call them)
static void cat_hlsl_atomics(Buf * b, const u32 name) {
    static const c8 * const ops[][2] = {
        {"add", "Add"}, {"min", "Min"}, {"max", "Max"}, {"and", "And"},
        {"or", "Or"}, {"xor", "Xor"}, {"exchange", "Exchange"},
    };
    for (u32 o = 0; o < sizeof(ops) / sizeof(ops[0]); o++) {
        buf_cat(b, "uint t1_atomic_");
        buf_cat(b, ops[o][0]);
        buf_cat(b, "_");
        cat_name(b, name);
        buf_cat(b, "(uint t1_i, uint t1_v) { uint t1_o; ");
        cat_name(b, name);
        buf_cat(b, ".Interlocked");
        buf_cat(b, ops[o][1]);
        buf_cat(b, "(t1_i * 4, t1_v, t1_o); return t1_o; }\n");
    }
    buf_cat(b, "uint t1_atomic_compare_exchange_");
    cat_name(b, name);
    buf_cat(b, "(uint t1_i, uint t1_e, uint t1_v) { uint t1_o; ");
    cat_name(b, name);
    buf_cat(b,
        ".InterlockedCompareExchange(t1_i * 4, t1_e, t1_v, t1_o); "
        "return t1_o; }\n");
}

static void make_unit(Gen * g, const u32 entry_f) {
    Check * c = g->c;
    const Func * e = &c->funcs[entry_f];
    u32 first_kept_func = unit_begin(g, entry_f);

    // the unit's marker line
    static const c8 * const stage_names[] = {"", "vert", "frag", "comp"};
    buf_cat(g->out, "//T1_UNIT ");
    buf_cat(g->out, stage_names[e->stage]);
    buf_cat(g->out, " ");
    buf_cat_n(g->out, T1_token_text(tz, e->name_i),
        T1_token_size(tz, e->name_i));
    buf_cat(g->out, "\n");
    buf_cat(g->out, glsl_prelude);

    // the globals, before the first function
    Buf globals_buf;
    globals_buf.cap = 64 * 1024;
    globals_buf.data = T1_mem_malloc_managed(globals_buf.cap);
    globals_buf.data[0] = '\0';
    globals_buf.size = 0;
    globals_buf.overflow = 0;
    Buf * gl = &globals_buf;
    buf_cat(gl, "\n");
    buf_cat(gl, glsl_textures);

    Buf prologue_buf;
    c8 prologue_data[8192];
    prologue_buf.data = prologue_data;
    prologue_buf.size = 0;
    prologue_buf.cap = 8192;
    prologue_buf.overflow = 0;
    prologue_data[0] = '\0';
    Buf * pro = &prologue_buf;
    buf_cat(pro, "\n");

    if (e->stage == STAGE_KERNEL) {
        u32 threads[3];
        kernel_group_size(e, threads);
        buf_cat(gl, "layout(local_size_x = ");
        cat_u32(gl, threads[0]);
        buf_cat(gl, ", local_size_y = ");
        cat_u32(gl, threads[1]);
        buf_cat(gl, ", local_size_z = ");
        cat_u32(gl, threads[2]);
        buf_cat(gl, ") in;\n"
            "layout(push_constant) uniform T1Grid { uvec3 t1_grid_size; };\n");
        if (!kernel_has_barrier(e)) {
            buf_cat(pro,
                "    if (any(greaterThanEqual(gl_GlobalInvocationID, "
                "t1_grid_size))) { return; }\n");
        }
    }

    // the parameters
    for (u32 p = 0; p < e->params_size; p++) {
        u32 start = e->params_start[p];
        u32 end = e->params_end[p];
        u32 attr = UINT32_MAX;
        u32 star = UINT32_MAX;
        b8 readonly = 0;
        for (u32 t = start; t < end; t++) {
            if (kind(t) == SK_ATTR) { attr = t; }
            if (is_punct(t, "*")) { star = t; }
            if (is(t, "const") || is(t, "constant")) { readonly = 1; }
        }
        u32 name = (attr != UINT32_MAX ? attr : end) - 1;
        if (attr == UINT32_MAX || kind(name) != SK_IDENT) {
            fail(g, start, "entry point parameters need an attribute");
            continue;
        }

        if (attr_has(attr, "buffer")) {
            if (star == UINT32_MAX) {
                fail(g, start, "a buffer parameter must be a pointer");
                continue;
            }
            buf_cat(gl, "layout(std430, set = 0, binding = ");
            cat_attr_arg(gl, attr, "buffer");
            buf_cat(gl, readonly ? ") readonly buffer T1Buf_" : ") buffer T1Buf_");
            cat_name(gl, name);
            buf_cat(gl, " { ");
            cat_name(gl, star - 1);
            buf_cat(gl, " ");
            cat_name(gl, name);
            buf_cat(gl, "[]; };\n");
        } else if (attr_has(attr, "texture")) {
            if (is(name - 1, "T1WriteTex")) {
                buf_cat(gl,
                    "layout(set = 1, binding = 4, rgba8) uniform image2D "
                    "t1_write_tex;\n");
            } else if (!is(name - 1, "T1Textures")) {
                fail(g, start, "textures are T1Textures or T1WriteTex");
            }
        } else if (
            attr_has(attr, "vertex_id") ||
            attr_has(attr, "instance_id") ||
            attr_has(attr, "thread_position_in_grid") ||
            attr_has(attr, "thread_position_in_threadgroup") ||
            attr_has(attr, "threadgroup_position_in_grid") ||
            (attr_has(attr, "position") && !attr_has(attr, "stage_in")))
        {
            const c8 * swizzle =
                is(name - 1, "uint3") ? "" :
                is(name - 1, "uint2") ? ".xy" : ".x";
            u32 sv = str_begin(g);
            buf_cat(&g->arena,
                attr_has(attr, "vertex_id") ? "gl_VertexIndex" :
                attr_has(attr, "instance_id") ? "gl_InstanceIndex" :
                attr_has(attr, "position") ? "gl_FragCoord" :
                attr_has(attr, "thread_position_in_threadgroup") ?
                    "gl_LocalInvocationID" :
                attr_has(attr, "threadgroup_position_in_grid") ?
                    "gl_WorkGroupID" :
                "gl_GlobalInvocationID");
            if (
                attr_has(attr, "thread_position_in_grid") ||
                attr_has(attr, "thread_position_in_threadgroup") ||
                attr_has(attr, "threadgroup_position_in_grid"))
            {
                buf_cat(&g->arena, swizzle);
            }
            const c8 * value = str_end(g, sv);
            buf_cat(pro, "    ");
            cat_tokens(pro, start, name + 1);
            buf_cat(pro, " = ");
            cat_name(pro, name - 1);
            buf_cat(pro, "(");
            buf_cat(pro, value);
            buf_cat(pro, ");\n");
        } else if (attr_has(attr, "stage_in")) {
            const StructDef * d = find_struct(c, name - 1);
            Member members[MEMBERS_CAP];
            u32 size = d ? struct_members(g, d, members) : 0;
            if (d == NULL) {
                fail(g, name - 1, "stage_in needs a struct");
                continue;
            }
            cat_varyings(g, gl, members, size, "in");
            buf_cat(pro, "    ");
            cat_name(pro, name - 1);
            buf_cat(pro, " ");
            cat_name(pro, name);
            buf_cat(pro, ";\n");
            for (u32 m = 0; m < size; m++) {
                buf_cat(pro, "    ");
                cat_name(pro, name);
                buf_cat(pro, ".");
                cat_name(pro, members[m].name_i);
                if (attr_has(members[m].attr_i, "position")) {
                    buf_cat(pro, " = gl_FragCoord;\n");
                } else {
                    buf_cat(pro, " = t1_v_");
                    cat_name(pro, members[m].name_i);
                    buf_cat(pro, ";\n");
                }
            }
        } else {
            fail(g, attr, "unsupported entry point parameter");
        }
    }

    // the outputs, returns write them
    u32 ret_type = e->name_i - 1;
    Member outs[MEMBERS_CAP];
    u32 outs_size = 0;
    b8 ret_struct = 0;
    if (e->stage != STAGE_KERNEL) {
        const StructDef * d = find_struct(c, ret_type);
        if (d != NULL) {
            ret_struct = 1;
            outs_size = struct_members(g, d, outs);
        }
        if (e->stage == STAGE_VERTEX) {
            if (!ret_struct) {
                fail(g, ret_type, "a vertex function returns a struct");
            }
            cat_varyings(g, gl, outs, outs_size, "out");
        } else if (ret_struct) {
            for (u32 m = 0; m < outs_size; m++) {
                if (!attr_has(outs[m].attr_i, "color")) {
                    fail(g, outs[m].name_i,
                        "fragment outputs need [[color(n)]]");
                    continue;
                }
                buf_cat(gl, "layout(location = ");
                cat_attr_arg(gl, outs[m].attr_i, "color");
                buf_cat(gl, ") out ");
                cat_name(gl, outs[m].type_i);
                buf_cat(gl, " t1_o_");
                cat_name(gl, outs[m].name_i);
                buf_cat(gl, ";\n");
            }
        } else {
            buf_cat(gl, "layout(location = 0) out ");
            cat_name(gl, ret_type);
            buf_cat(gl, " t1_o_color;\n");
        }
    }

    for (u32 i = e->body_open; i < e->body_close && e->stage != STAGE_KERNEL; i++) {
        if (!is(i, "return")) { continue; }
        u32 semi = i;
        u32 nesting = 0;
        while (semi < e->body_close) {
            if (is_punct(semi, "(")) { nesting++; }
            if (is_punct(semi, ")")) { nesting--; }
            if (nesting == 0 && is_punct(semi, ";")) { break; }
            semi++;
        }
        u32 s = str_begin(g);
        Buf * a = &g->arena;
        if (ret_struct) {
            buf_cat(a, "{ ");
            cat_name(a, ret_type);
            buf_cat(a, " t1_ret = ");
            g->u_repl[i] = str_end(g, s);
            s = str_begin(g);
            buf_cat(a, ";");
            for (u32 m = 0; m < outs_size; m++) {
                if (attr_has(outs[m].attr_i, "position")) {
                    buf_cat(a, " gl_Position = t1_ret.");
                    cat_name(a, outs[m].name_i);
                    buf_cat(a, "; gl_Position.y = -gl_Position.y;");
                } else {
                    buf_cat(a, e->stage == STAGE_VERTEX ?
                        " t1_v_" : " t1_o_");
                    cat_name(a, outs[m].name_i);
                    buf_cat(a, " = t1_ret.");
                    cat_name(a, outs[m].name_i);
                    buf_cat(a, ";");
                }
            }
            buf_cat(a, " return; }");
            g->u_repl[semi] = str_end(g, s);
        } else {
            buf_cat(a, "{ t1_o_color = ");
            g->u_repl[i] = str_end(g, s);
            g->u_repl[semi] = "; return; }";
        }
    }

    // void main() instead of the entry point's head
    g->u_repl[e->decl_start] = "void main()";
    for (u32 t = e->decl_start + 1; t <= e->params_close; t++) {
        g->u_skip[t] = 1;
    }

    if (pro->overflow) { fail(g, e->name_i, "too many parameters"); }
    {
        u32 s = str_begin(g);
        buf_cat(&g->arena, pro->data);
        g->u_post[e->body_open] = str_end(g, s);
    }
    buf_cat(gl, "\n");
    if (gl->overflow) { fail(g, e->name_i, "too many globals"); }
    {
        u32 s = str_begin(g);
        buf_cat(&g->arena, gl->data);
        g->u_pre[first_kept_func] = str_end(g, s);
    }
    T1_mem_free_managed(gl->data);

    if (e->stage == STAGE_KERNEL) { threadgroup_decls(g, e, "shared "); }

    write_unit(g);
}

/*
What both translations start with: the client's shaders checked, T1's
headers before them, tokenized, collected, and g set up to write into
recip. 0 if the check failed (its errors are in error_recip): then there's
nothing to gen_end(). hlsl says which translation runs, see hlsl_target
*/
static b8 gen_begin(
    Gen * g,
    Buf * out,
    const c8 * shader_source,
    c8 * recip,
    const u32 recip_cap,
    c8 * error_recip,
    const u32 error_recip_cap,
    const b8 hlsl)
{
    T1_assert(recip != NULL && recip_cap > 0);
    recip[0] = '\0';

    // the shaders must be in the subset, T1's own headers aren't checked
    u8 checked = 0;
    check_subset(shader_source, &checked, error_recip, error_recip_cap);
    if (!checked) { return 0; }

    u32 headers_size = (u32)(
        T1_std_strlen(T1_embedded_data_shader_settings) +
        T1_std_strlen(T1_embedded_data_shader_includes));
    u32 full_cap = headers_size + (u32)T1_std_strlen(shader_source) + 1;
    c8 * full = T1_mem_malloc_managed(full_cap);
    full[0] = '\0';
    T1_std_strcat_cap(full, full_cap, T1_embedded_data_shader_settings);
    T1_std_strcat_cap(full, full_cap, T1_embedded_data_shader_includes);
    T1_std_strcat_cap(full, full_cap, shader_source);

    T1_token_run(tz, full, T1_std_strlen(full));
    hlsl_target = hlsl;

    Check * c = T1_mem_malloc_managed(sizeof(Check));
    T1_std_memset(c, 0, sizeof(Check));
    c->count = T1_token_count(tz);
    c->source = full;
    c->error_recip = error_recip;
    c->error_recip_cap = error_recip_cap;
    if (error_recip != NULL) { error_recip[0] = '\0'; }
    c->secondary = T1_mem_malloc_managed(c->count + 1);
    mark_secondary(c);
    collect(c);

    T1_std_memset(g, 0, sizeof(Gen));
    g->c = c;
    g->arena.cap = 4 * 1024 * 1024;
    g->arena.data = T1_mem_malloc_managed(g->arena.cap);
    g->arena.data[0] = '\0';
    out->data = recip;
    out->size = 0;
    out->cap = recip_cap;
    out->overflow = 0;
    g->out = out;
    u32 n = c->count + 1;
    g->g_skip = T1_mem_malloc_managed(n);
    g->u_skip = T1_mem_malloc_managed(n);
    g->u_drop = T1_mem_malloc_managed(n);
    g->g_repl = T1_mem_malloc_managed(sizeof(c8 *) * n);
    g->g_pre = T1_mem_malloc_managed(sizeof(c8 *) * n);
    g->g_post = T1_mem_malloc_managed(sizeof(c8 *) * n);
    g->u_repl = T1_mem_malloc_managed(sizeof(c8 *) * n);
    g->u_pre = T1_mem_malloc_managed(sizeof(c8 *) * n);
    g->u_post = T1_mem_malloc_managed(sizeof(c8 *) * n);
    g->reachable = T1_mem_malloc_managed(c->funcs_size + 1);
    T1_std_memset(g->g_skip, 0, n);
    for (u32 i = 0; i < n; i++) {
        g->g_repl[i] = NULL;
        g->g_pre[i] = NULL;
        g->g_post[i] = NULL;
    }

    g->user_first_token = c->count;
    for (u32 i = 0; i < c->count; i++) {
        if ((u32)(T1_token_text(tz, i) - full) >= headers_size) {
            g->user_first_token = i;
            break;
        }
    }
    return 1;
}

// Frees what gen_begin() made, *good says whether the translation worked
static void gen_end(Gen * g, u8 * good, const c8 * ran_out) {
    if (g->arena.overflow || g->out->overflow) { fail(g, 0, ran_out); }
    *good = !g->failed;

    hlsl_target = 0;
    T1_mem_free_managed(g->arena.data);
    T1_mem_free_managed(g->g_skip);
    T1_mem_free_managed(g->u_skip);
    T1_mem_free_managed(g->u_drop);
    T1_mem_free_managed((void *)g->g_repl);
    T1_mem_free_managed((void *)g->g_pre);
    T1_mem_free_managed((void *)g->g_post);
    T1_mem_free_managed((void *)g->u_repl);
    T1_mem_free_managed((void *)g->u_pre);
    T1_mem_free_managed((void *)g->u_post);
    T1_mem_free_managed(g->reachable);
    T1_mem_free_managed(g->c->secondary);
    T1_mem_free_managed((void *)g->c->source);
    T1_mem_free_managed(g->c);
}

/*
VECTOR COMPARISONS IN GLSL

MSL's a > b on vectors compares each component (a bool vector, for any(),
all() or a boolN), GLSL only compares scalars that way: vectors need
greaterThan(a, b), and its a == b is a single bool (and !v is not(v)).
So comparisons whose
operands are vectors become calls, a scalar beside a vector becomes a
vector (MSL compares each component with it):
    pos >= size  ->  greaterThanEqual(pos, size)
    uv > 0.5f    ->  greaterThan(uv, vec2(0.5f))
That takes the operands' types, so this infers the types of expressions:
from declarations (parameters, locals, program scope constants, struct
members), literals, constructors, casts, swizzles, indexing and calls. An
expression it can't follow is left as it is.
*/

typedef u8 VBase;
enum {
    VB_NONE, // not known
    VB_FLOAT,
    VB_INT,
    VB_UINT,
    VB_BOOL,
    VB_MATRIX, // width is the number of columns
    VB_STRUCT,
};

typedef struct {
    VBase base;
    u8 width; // 1 for scalars
    b8 array; // indexing gives the type itself (an array or a buffer)
    u32 type_i; // a token naming the struct, for VB_STRUCT
} VType;

static VType vtype_none(void) {
    VType t;
    t.base = VB_NONE;
    t.width = 0;
    t.array = 0;
    t.type_i = 0;
    return t;
}

static VType vtype(const VBase base, const u8 width) {
    VType t = vtype_none();
    t.base = base;
    t.width = width;
    return t;
}

static b8 is_vector(const VType t) {
    return
        t.width > 1 && !t.array &&
        (t.base == VB_FLOAT || t.base == VB_INT ||
        t.base == VB_UINT || t.base == VB_BOOL);
}

static b8 is_scalar(const VType t) {
    return
        t.width == 1 && !t.array &&
        (t.base == VB_FLOAT || t.base == VB_INT ||
        t.base == VB_UINT || t.base == VB_BOOL);
}

// The type a type name means
static VType vtype_of_name(const Check * c, const u32 i) {
    if (kind(i) != SK_IDENT) { return vtype_none(); }
    if (is_matrix_type(i)) {
        return vtype(VB_MATRIX, (u8)(T1_token_text(tz, i)[5] - '0'));
    }
    typedef struct {
        const c8 * prefix;
        VBase base;
    } Prim;
    static const Prim prims[] = {
        {"float", VB_FLOAT}, {"int", VB_INT}, {"uint", VB_UINT},
        {"bool", VB_BOOL},
    };
    const c8 * t = T1_token_text(tz, i);
    const u32 n = T1_token_size(tz, i);
    for (u32 p = 0; p < sizeof(prims) / sizeof(prims[0]); p++) {
        const u32 pn = (u32)T1_std_strlen(prims[p].prefix);
        if (!T1_std_are_equal_strings_of_length(t, prims[p].prefix, pn)) {
            continue;
        }
        if (n == pn) { return vtype(prims[p].base, 1); }
        if (n == pn + 1 && t[pn] >= '2' && t[pn] <= '4') {
            return vtype(prims[p].base, (u8)(t[pn] - '0'));
        }
    }
    if (is(i, "f32")) { return vtype(VB_FLOAT, 1); }
    if (is(i, "u32") || is(i, "uint32_t")) { return vtype(VB_UINT, 1); }
    if (is(i, "s32") || is(i, "int32_t")) { return vtype(VB_INT, 1); }
    if (in_names(i, c->structs, c->structs_size)) {
        VType st = vtype(VB_STRUCT, 1);
        st.type_i = i;
        return st;
    }
    return vtype_none();
}

/*
Whether the name at j is declared there: Type name, or Type * name (a
buffer), followed by what can follow a declared name
*/
static b8 declares(const Check * c, const u32 j) {
    if (kind(j) != SK_IDENT || j < 2) { return 0; }
    const b8 typed =
        is_type(c, j - 1) ||
        (is_punct(j - 1, "*") && is_type(c, j - 2));
    return
        typed &&
        (is_punct(j + 1, "=") || is_punct(j + 1, ";") ||
        is_punct(j + 1, ",") || is_punct(j + 1, "[") ||
        is_punct(j + 1, ")") || kind(j + 1) == SK_ATTR);
}

static VType vtype_declared_at(const Check * c, const u32 j) {
    const b8 pointer = is_punct(j - 1, "*");
    VType t = vtype_of_name(c, pointer ? j - 2 : j - 1);
    t.array = pointer || is_punct(j + 1, "[");
    return t;
}

// The type of the variable named at i: the latest declaration before it in
// its function, else one at program scope
static VType vtype_of_variable(const Check * c, const u32 i) {
    const Func * f = func_at(c, i);
    if (f != NULL) {
        for (u32 j = i; j > f->params_open; j--) {
            if (same_text(j - 1, i) && declares(c, j - 1)) {
                return vtype_declared_at(c, j - 1);
            }
        }
    }
    for (u32 j = 0; j < c->count; j++) {
        if (same_text(j, i) && declares(c, j) && at_program_scope(c, j)) {
            return vtype_declared_at(c, j);
        }
    }
    return vtype_none();
}

static VType vtype_of_member(
    const Check * c, const VType owner, const u32 member_i)
{
    const StructDef * d = find_struct(c, owner.type_i);
    if (d == NULL) { return vtype_none(); }
    for (u32 j = d->body_open + 1; j < d->body_close; j++) {
        if (same_text(j, member_i) && is_type(c, j - 1)) {
            VType t = vtype_of_name(c, j - 1);
            t.array = is_punct(j + 1, "[");
            return t;
        }
    }
    return vtype_none();
}

// A swizzle's length, 0 if member_i isn't one
static u8 swizzle_size(const u32 member_i) {
    const c8 * t = T1_token_text(tz, member_i);
    const u32 n = T1_token_size(tz, member_i);
    if (n < 1 || n > 4) { return 0; }
    for (u32 k = 0; k < n; k++) {
        const c8 ch = t[k];
        const b8 xyzw = ch == 'x' || ch == 'y' || ch == 'z' || ch == 'w';
        const b8 rgba = ch == 'r' || ch == 'g' || ch == 'b' || ch == 'a';
        if (!xyzw && !rgba) { return 0; }
    }
    return (u8)n;
}

static VType vtype_of_number(const u32 i) {
    const c8 * t = T1_token_text(tz, i);
    const u32 n = T1_token_size(tz, i);
    const b8 hex = n > 1 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X');
    if (hex) {
        return vtype(t[n - 1] == 'u' || t[n - 1] == 'U' ? VB_UINT : VB_INT, 1);
    }
    for (u32 k = 0; k < n; k++) {
        if (t[k] == '.' || t[k] == 'e' || t[k] == 'f' || t[k] == 'F') {
            return vtype(VB_FLOAT, 1);
        }
    }
    return vtype(
        t[n - 1] == 'u' || t[n - 1] == 'U' ? VB_UINT : VB_INT, 1);
}

// The wider of two types (a vector beside a scalar)
static VType vtype_wider(const VType a, const VType b) {
    if (a.base == VB_NONE) { return b; }
    if (b.base == VB_NONE) { return a; }
    return b.width > a.width ? b : a;
}

#define VARGS_CAP 8

static VType vtype_of_call(
    const Check * c, const u32 name_i, const VType * args, const u32 size)
{
    VType widest = vtype_none();
    for (u32 a = 0; a < size; a++) { widest = vtype_wider(widest, args[a]); }
    if (is_type(c, name_i)) { return vtype_of_name(c, name_i); }
    if (
        is(name_i, "dot") || is(name_i, "length") || is(name_i, "distance"))
    {
        return vtype(VB_FLOAT, 1);
    }
    if (is(name_i, "any") || is(name_i, "all")) { return vtype(VB_BOOL, 1); }
    if (is(name_i, "cross")) { return vtype(VB_FLOAT, 3); }
    if (
        is(name_i, "T1_sample_nearest") || is(name_i, "T1_sample_linear") ||
        is(name_i, "T1_sample_linear_repeat") || is(name_i, "T1_read") ||
        is(name_i, "T1_write_tex_read"))
    {
        return vtype(VB_FLOAT, 4);
    }
    if (is(name_i, "T1_write_tex_size")) { return vtype(VB_UINT, 2); }
    if (is(name_i, "mul") && size == 2) {
        if (args[0].base == VB_MATRIX) { return args[0]; }
        return vtype(VB_FLOAT, args[1].width);
    }
    if (is(name_i, "transpose") || is(name_i, "reflect")) {
        return size > 0 ? args[0] : vtype_none();
    }
    const Func * f = find_func(c, name_i);
    if (f != NULL) { return vtype_of_name(c, f->name_i - 1); }
    if (
        in_list(name_i, builtin_functions,
            sizeof(builtin_functions) / sizeof(builtin_functions[0])))
    {
        return widest; // abs(), clamp(), mix(), step(), ...: per component
    }
    return vtype_none(); // a macro
}

#define VEDITS_CAP 64

typedef struct {
    u32 left_first; // the operand's first token for a !
    u32 op;
    u32 right_last;
    VType left;
    VType right;
} VEdit;

/*
A small parser of expressions: types and the comparisons to rewrite. end
is exclusive, bad set means it found something it doesn't follow (its
edits are then dropped)
*/
typedef struct {
    const Check * c;
    u32 i;
    u32 end;
    b8 bad;
    VEdit edits[VEDITS_CAP];
    u32 edits_size;
    u32 compared[VEDITS_CAP]; // every comparison operator and ! parsed
    u32 compared_size;
    u32 float_mod; // a % on floats
    u32 scalar_swizzle; // a swizzle on one number
    u32 not_number; // a ! on a number
    u32 bool_math; // + - * / on a bool
} VParse;

static VType vparse_expr(VParse * p, const u32 min_prec);

static b8 vparse_at(const VParse * p, const c8 * punct) {
    return p->i < p->end && is_punct(p->i, punct);
}

// C's precedences, 0 for anything that isn't a binary operator
static u32 binary_prec(const u32 i) {
    if (kind(i) != SK_PUNCT) { return 0; }
    static const c8 * const assign[] = {
        "=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=",
    };
    if (in_list(i, assign, sizeof(assign) / sizeof(assign[0]))) { return 2; }
    if (is(i, "||")) { return 4; }
    if (is(i, "&&")) { return 5; }
    if (is(i, "|")) { return 6; }
    if (is(i, "^")) { return 7; }
    if (is(i, "&")) { return 8; }
    if (is(i, "==") || is(i, "!=")) { return 9; }
    if (is(i, "<") || is(i, ">") || is(i, "<=") || is(i, ">=")) {
        return 10;
    }
    if (is(i, "<<") || is(i, ">>")) { return 11; }
    if (is(i, "+") || is(i, "-")) { return 12; }
    if (is(i, "*") || is(i, "/") || is(i, "%")) { return 13; }
    return 0;
}

// ( ... ) at p->i: the expression inside, p->i after the ')'
static VType vparse_parens(VParse * p) {
    const u32 close = after_match(p->c, p->i, "(", ")") - 1;
    if (close >= p->end) { p->bad = 1; return vtype_none(); }
    const u32 outer_end = p->end;
    p->end = close;
    p->i++;
    VType t = vparse_expr(p, 1);
    if (p->i != close) { p->bad = 1; }
    p->end = outer_end;
    p->i = close + 1;
    return t;
}

// a call's arguments at p->i (its '('), p->i after the ')'
static u32 vparse_args(VParse * p, VType * args) {
    const u32 close = after_match(p->c, p->i, "(", ")") - 1;
    if (close >= p->end) { p->bad = 1; return 0; }
    const u32 outer_end = p->end;
    p->end = close;
    p->i++;
    u32 size = 0;
    while (p->i < close && !p->bad) {
        VType a = vparse_expr(p, 2); // above the comma
        if (size < VARGS_CAP) { args[size++] = a; }
        if (p->i == close) { break; }
        if (!vparse_at(p, ",")) { p->bad = 1; break; }
        p->i++;
    }
    p->end = outer_end;
    p->i = close + 1;
    return size;
}

static VType vparse_primary(VParse * p) {
    const Check * c = p->c;
    const u32 t = p->i;
    if (t >= p->end) { p->bad = 1; return vtype_none(); }
    if (is_punct(t, "(")) { return vparse_parens(p); }
    if (kind(t) == SK_NUMBER) {
        p->i++;
        return vtype_of_number(t);
    }
    if (kind(t) != SK_IDENT) { p->bad = 1; return vtype_none(); }
    if (is(t, "true") || is(t, "false")) {
        p->i++;
        return vtype(VB_BOOL, 1);
    }
    if (
        is(t, "as_type") && is_punct(t + 1, "<") && is_punct(t + 3, ">") &&
        is_punct(t + 4, "("))
    {
        p->i = t + 4;
        VType args[VARGS_CAP];
        vparse_args(p, args);
        return vtype_of_name(c, t + 2);
    }
    if (t + 1 < p->end && is_punct(t + 1, "(")) {
        p->i = t + 1;
        VType args[VARGS_CAP];
        u32 size = vparse_args(p, args);
        return vtype_of_call(c, t, args, size);
    }
    p->i++;
    return vtype_of_variable(c, t);
}

static VType vparse_postfix(VParse * p) {
    VType t = vparse_primary(p);
    while (!p->bad && p->i < p->end) {
        if (vparse_at(p, ".") && kind(p->i + 1) == SK_IDENT) {
            const u32 member = p->i + 1;
            const u8 swizzle = swizzle_size(member);
            if (t.base == VB_STRUCT && !t.array) {
                t = vtype_of_member(p->c, t, member);
            } else if (is_vector(t) || is_scalar(t)) {
                if (is_scalar(t) && swizzle > 0) { p->scalar_swizzle = member; }
                t = swizzle > 0 ? vtype(t.base, swizzle) : vtype_none();
            } else {
                t = vtype_none();
            }
            p->i += 2;
        } else if (vparse_at(p, "[")) {
            const u32 close = after_match(p->c, p->i, "[", "]") - 1;
            if (close >= p->end) { p->bad = 1; break; }
            const u32 outer_end = p->end;
            p->end = close;
            p->i++;
            vparse_expr(p, 1);
            if (p->i != close) { p->bad = 1; }
            p->end = outer_end;
            p->i = close + 1;
            if (t.array) {
                t.array = 0;
            } else if (t.base == VB_MATRIX) {
                t = vtype(VB_FLOAT, t.width);
            } else if (is_vector(t)) {
                t = vtype(t.base, 1);
            } else {
                t = vtype_none();
            }
        } else if (vparse_at(p, "++") || vparse_at(p, "--")) {
            p->i++;
        } else {
            break;
        }
    }
    return t;
}

static VType vparse_unary(VParse * p) {
    const u32 t = p->i;
    if (t >= p->end) { p->bad = 1; return vtype_none(); }
    if (is_punct(t, "!")) {
        if (p->compared_size < VEDITS_CAP) {
            p->compared[p->compared_size++] = t;
        }
        p->i++;
        const u32 operand = p->i;
        VType v = vparse_unary(p);
        if (
            !v.array && v.width == 1 &&
            (v.base == VB_FLOAT || v.base == VB_INT || v.base == VB_UINT))
        {
            p->not_number = t;
        }
        // !v on a bool vector: GLSL's not(v)
        if (!p->bad && is_vector(v) && p->edits_size < VEDITS_CAP) {
            VEdit * e = &p->edits[p->edits_size++];
            e->left_first = operand;
            e->op = t;
            e->right_last = p->i - 1;
            e->left = v;
            e->right = v;
        }
        return vtype(VB_BOOL, v.width);
    }
    if (
        is_punct(t, "-") || is_punct(t, "+") || is_punct(t, "~") ||
        is_punct(t, "++") || is_punct(t, "--"))
    {
        p->i++;
        return vparse_unary(p);
    }
    if (is_cast(p->c, t)) {
        p->i += 3;
        vparse_unary(p);
        return vtype_of_name(p->c, t + 1);
    }
    return vparse_postfix(p);
}

static VType vparse_expr(VParse * p, const u32 min_prec) {
    const u32 first = p->i;
    VType left = vparse_unary(p);
    while (!p->bad && p->i < p->end) {
        const u32 op = p->i;
        if (is_punct(op, "?")) {
            if (min_prec > 3) { break; }
            p->i++;
            VType a = vparse_expr(p, 1);
            if (!vparse_at(p, ":")) { p->bad = 1; break; }
            p->i++;
            VType b = vparse_expr(p, 3);
            left = a.base != VB_NONE ? a : b;
            continue;
        }
        const u32 prec = binary_prec(op);
        if (prec == 0 || prec < min_prec) { break; }
        p->i++;
        VType right = vparse_expr(p, prec == 2 ? 2 : prec + 1);
        if (p->bad) { break; }
        if (is_punct(op, "%") && (left.base == VB_FLOAT || right.base == VB_FLOAT)) {
            p->float_mod = op;
        }
        if (prec >= 12 && (left.base == VB_BOOL || right.base == VB_BOOL)) {
            p->bool_math = op;
        }
        if (prec == 9 || prec == 10) {
            if (p->compared_size < VEDITS_CAP) {
                p->compared[p->compared_size++] = op;
            }
            if (
                (is_vector(left) || is_vector(right)) &&
                p->edits_size < VEDITS_CAP)
            {
                VEdit * e = &p->edits[p->edits_size++];
                e->left_first = first;
                e->op = op;
                e->right_last = p->i - 1;
                e->left = left;
                e->right = right;
            }
            VType wider = vtype_wider(left, right);
            left = vtype(VB_BOOL, wider.width > 0 ? wider.width : 1);
        } else if (prec == 4 || prec == 5) {
            left = vtype(VB_BOOL, 1);
        } else if (prec != 2) {
            left = vtype_wider(left, right);
        }
    }
    return left;
}

// Ends an expression a comparison is in: lower precedence or a bracket
static b8 vparse_boundary(const u32 i) {
    if (kind(i) == SK_PREPROC || kind(i) == SK_ATTR) { return 1; }
    if (kind(i) == SK_IDENT) {
        return is(i, "return") || is(i, "case") || is(i, "else");
    }
    if (kind(i) != SK_PUNCT) { return 0; }
    const u32 prec = binary_prec(i);
    if (prec != 0 && prec <= 8) { return 1; } // & ^ | && || =
    return
        is(i, ";") || is(i, ",") || is(i, "?") || is(i, ":") ||
        is(i, "{") || is(i, "}");
}

// The '(' or '[' matching the close at i, going back
static u32 before_match(const Check * c, const u32 close) {
    const c8 * open_s = is_punct(close, ")") ? "(" : "[";
    const c8 * close_s = is_punct(close, ")") ? ")" : "]";
    u32 depth = 0;
    for (u32 j = close + 1; j > 0; j--) {
        const u32 k = j - 1;
        if (is_bracket(c, k, close_s)) { depth++; }
        if (is_bracket(c, k, open_s)) {
            depth--;
            if (depth == 0) { return k; }
        }
    }
    return 0;
}

static const c8 * glsl_compare_fn(const u32 op) {
    if (is(op, "<")) { return "lessThan("; }
    if (is(op, "<=")) { return "lessThanEqual("; }
    if (is(op, ">")) { return "greaterThan("; }
    if (is(op, ">=")) { return "greaterThanEqual("; }
    if (is(op, "==")) { return "equal("; }
    return "notEqual(";
}

// "vec3(" for a float3 etc.: a scalar as wide as v
static void cat_glsl_vector_open(Buf * b, const VType v) {
    if (v.base == VB_INT) { buf_cat(b, "i"); }
    if (v.base == VB_UINT) { buf_cat(b, "u"); }
    if (v.base == VB_BOOL) { buf_cat(b, "b"); }
    buf_cat(b, "vec");
    const c8 width[2] = {(c8)('0' + v.width), '\0'};
    buf_cat(b, width);
    buf_cat(b, "(");
}

static void vedit_apply(Gen * g, const VEdit * e) {
    if (is_punct(e->op, "!")) {
        g->g_repl[e->op] = "not(";
        add_text(g, &g->g_post[e->right_last], ")");
        return;
    }
    const b8 widen_left = is_scalar(e->left) && is_vector(e->right);
    const b8 widen_right = is_scalar(e->right) && is_vector(e->left);

    // before the left operand: the call, and the left's widening, in front
    // of an inner comparison's that starts there too
    u32 s = str_begin(g);
    buf_cat(&g->arena, glsl_compare_fn(e->op));
    if (widen_left) { cat_glsl_vector_open(&g->arena, e->right); }
    if (g->g_pre[e->left_first]) {
        buf_cat(&g->arena, g->g_pre[e->left_first]);
    }
    g->g_pre[e->left_first] = str_end(g, s);

    s = str_begin(g);
    if (widen_left) { buf_cat(&g->arena, ")"); }
    buf_cat(&g->arena, ", ");
    if (widen_right) { cat_glsl_vector_open(&g->arena, e->left); }
    g->g_repl[e->op] = str_end(g, s);

    add_text(g, &g->g_post[e->right_last], widen_right ? "))" : ")");
}

static void glsl_vector_comparisons(Gen * g) {
    const Check * c = g->c;
    u8 * seen = T1_mem_malloc_managed(c->count + 1);
    T1_std_memset(seen, 0, c->count + 1);
    VParse * p = T1_mem_malloc_managed(sizeof(VParse));
    for (u32 i = g->user_first_token; i < c->count; i++) {
        // comparisons and !, the operators whose vector forms differ
        const u32 prec = binary_prec(i);
        const b8 compares = prec == 9 || prec == 10 || is_punct(i, "!");
        if (!compares || seen[i] || c->secondary[i]) { continue; }
        const Func * f = func_at(c, i);
        if (f == NULL || i <= f->body_open) { continue; }
        // as_type<T>: not a comparison
        if (is(i - 1, "as_type") || (i >= 3 && is(i - 3, "as_type"))) {
            continue;
        }

        // the expression around it, up to lower precedence or a bracket
        u32 first = i;
        while (first > f->body_open + 1) {
            const u32 b = first - 1;
            if (is_punct(b, ")") || is_punct(b, "]")) {
                first = before_match(c, b);
                continue;
            }
            if (is_punct(b, "(") || is_punct(b, "[") || vparse_boundary(b)) {
                break;
            }
            first = b;
        }
        u32 end = i + 1;
        while (end < f->body_close) {
            if (is_punct(end, "(")) {
                end = after_match(c, end, "(", ")");
                continue;
            }
            if (is_punct(end, "[")) {
                end = after_match(c, end, "[", "]");
                continue;
            }
            if (
                is_punct(end, ")") || is_punct(end, "]") ||
                vparse_boundary(end))
            {
                break;
            }
            end++;
        }

        T1_std_memset(p, 0, sizeof(VParse));
        p->c = c;
        p->i = first;
        p->end = end;
        vparse_expr(p, 1);
        if (p->bad || p->i != end) { continue; }
        // an inner one may be done already, parsed alone (inside brackets)
        for (u32 e = 0; e < p->edits_size; e++) {
            if (!seen[p->edits[e].op]) { vedit_apply(g, &p->edits[e]); }
        }
        for (u32 k = 0; k < p->compared_size; k++) {
            seen[p->compared[k]] = 1;
        }
    }
    T1_mem_free_managed(p);
    T1_mem_free_managed(seen);
}

void T1_shader_to_glsl(
    const c8 * shader_source,
    c8 * recip,
    const u32 recip_cap,
    u8 * good,
    c8 * error_recip,
    const u32 error_recip_cap)
{
    *good = 0;
    Gen g;
    Buf out;
    if (
        !gen_begin(
            &g, &out, shader_source, recip, recip_cap,
            error_recip, error_recip_cap, /* hlsl: */ 0))
    {
        return;
    }

    global_edits(&g);
    glsl_vector_comparisons(&g);
    for (u32 f = 0; f < g.c->funcs_size && !g.failed; f++) {
        if (g.c->funcs[f].is_entry) { make_unit(&g, f); }
    }

    gen_end(&g, good, "T1_shader_to_glsl() ran out of room");
}

/*
TRANSLATION TO HLSL (SHADER MODEL 5.0) FOR DIRECT3D 11

Like the GLSL translation, every entry point becomes a unit of its own that
starts with a line "//T1_UNIT vert|frag|comp name". Entry points keep their
names, so a unit is compiled with its entry point's name.

The layout a Direct3D 11 backend must match:
- buffer(n): register t(T1_HLSL_BUFS_T + n), a ByteAddressBuffer (raw
  bytes, like a Metal buffer), except a kernel's writable buffers:
  RWByteAddressBuffers from u1 on, a line "//T1_UAV slot n" each
- T1Textures: t0 to t(T1_TEXARRAYS_CAP - 1), a Texture2DArray each
- samplers s0 to s2: T1_nearest, T1_linear and T1_linear_repeat
- a compute pass's write_tex: u0, an R32_UINT view of its slice, each uint
  an rgba8 pixel that T1's helpers pack and unpack
- kernels: a line "//T1_THREADS x y z" has their group's size, b0 is a
  constant buffer with the grid's size (a uint3, threads past it return
  right away) and, 16 bytes in, the first thread of this Dispatch() (a
  uint3: a grid of more than 65535 groups a dimension takes several)
- an HLSL matrix holds the columns of the MSL one as its rows: in memory
  they're the same (#pragma pack_matrix(row_major)), constructors and m[i]
  stay as they are, and mul() swaps its arguments
- buffers are read byte by byte with MSL's layout (a float3 takes 16 bytes
  and starts at a multiple of 16), through t1_load_ and t1_store_ functions
- clip space, texture coordinates and the winding of front faces are
  Metal's already
*/

// HLSL words and intrinsics that can't be names, they get a '_'
static const c8 * const hlsl_reserved[] = {
    "in", "out", "inout", "uniform", "register", "packoffset", "sampler",
    "sampler1D", "sampler2D", "sampler3D", "samplerCUBE", "SamplerState",
    "SamplerComparisonState", "texture", "Texture1D", "Texture2D",
    "Texture3D", "TextureCube", "Texture2DArray", "RWTexture2DArray",
    "Buffer", "ByteAddressBuffer", "RWByteAddressBuffer", "StructuredBuffer",
    "RWStructuredBuffer", "cbuffer", "tbuffer", "technique", "pass",
    "compile", "line", "lineadj", "point", "triangle", "triangleadj",
    "linear", "centroid", "nointerpolation", "noperspective", "sample",
    "precise", "shared", "groupshared", "row_major", "column_major", "snorm",
    "unorm", "string", "vector", "matrix", "half", "dword", "export", "asm",
    "long", "short", "signed", "unsigned", "explicit", "friend", "mutable",
    "protected", "throw", "try", "catch", "globallycoherent", "lerp",
    "frac", "rcp", "mad", "clip", "ddx", "ddy", "asuint", "asint", "asfloat",
    "countbits", "firstbithigh", "firstbitlow", "reversebits", "isnan",
    "isinf", "isfinite", "noise", "lit", "dst", "log10", "sincos", "modf",
    "ldexp", "frexp", "determinant", "numthreads",
};

static b8 is_hlsl_reserved(const u32 i) {
    return in_list(
        i, hlsl_reserved, sizeof(hlsl_reserved) / sizeof(hlsl_reserved[0]));
}

// the as*() of an as_type<> target: asuint(), asint() or asfloat()
static const c8 * hlsl_as_type_name(const u32 i) {
    static const c8 * const map[][2] = {
        {"u32", "uint"}, {"uint", "uint"}, {"uint32_t", "uint"},
        {"s32", "int"}, {"int", "int"}, {"int32_t", "int"},
        {"f32", "float"}, {"float", "float"},
        {"uint4", "uint"}, {"float4", "float"},
    };
    for (u32 m = 0; m < sizeof(map) / sizeof(map[0]); m++) {
        if (is(i, map[m][0])) { return map[m][1]; }
    }
    return NULL;
}

/*
The types buffers can hold besides structs, with MSL's sizes and alignments
and how to load and store them at the byte a of the raw buffer b
*/
typedef struct {
    const c8 * type;
    const c8 * size;
    const c8 * align;
    const c8 * load;
    const c8 * store;
} HlslPrim;

static const HlslPrim hlsl_prims[] = {
    {"float", "4", "4", "asfloat(b.Load(a))", "b.Store(a, asuint(v))"},
    {"float2", "8", "8", "asfloat(b.Load2(a))", "b.Store2(a, asuint(v))"},
    {"float3", "16", "16", "asfloat(b.Load3(a))", "b.Store3(a, asuint(v))"},
    {"float4", "16", "16", "asfloat(b.Load4(a))", "b.Store4(a, asuint(v))"},
    {"uint", "4", "4", "b.Load(a)", "b.Store(a, v)"},
    {"uint2", "8", "8", "b.Load2(a)", "b.Store2(a, v)"},
    {"uint3", "16", "16", "b.Load3(a)", "b.Store3(a, v)"},
    {"uint4", "16", "16", "b.Load4(a)", "b.Store4(a, v)"},
    {"int", "4", "4", "asint(b.Load(a))", "b.Store(a, asuint(v))"},
    {"int2", "8", "8", "asint(b.Load2(a))", "b.Store2(a, asuint(v))"},
    {"int3", "16", "16", "asint(b.Load3(a))", "b.Store3(a, asuint(v))"},
    {"int4", "16", "16", "asint(b.Load4(a))", "b.Store4(a, asuint(v))"},
    // a matrix is its columns, each aligned like a vector
    {"float2x2", "16", "8",
        "float2x2(asfloat(b.Load2(a)), asfloat(b.Load2(a + 8)))",
        "b.Store2(a, asuint(v[0])); b.Store2(a + 8, asuint(v[1]))"},
    {"float3x3", "48", "16",
        "float3x3(asfloat(b.Load3(a)), asfloat(b.Load3(a + 16)), "
            "asfloat(b.Load3(a + 32)))",
        "b.Store3(a, asuint(v[0])); b.Store3(a + 16, asuint(v[1])); "
            "b.Store3(a + 32, asuint(v[2]))"},
    {"float4x4", "64", "16",
        "float4x4(asfloat(b.Load4(a)), asfloat(b.Load4(a + 16)), "
            "asfloat(b.Load4(a + 32)), asfloat(b.Load4(a + 48)))",
        "b.Store4(a, asuint(v[0])); b.Store4(a + 16, asuint(v[1])); "
            "b.Store4(a + 32, asuint(v[2])); b.Store4(a + 48, asuint(v[3]))"},
};

// The prim's name in HLSL (u32 is uint), NULL if it's not a prim
static const c8 * hlsl_prim(const u32 i) {
    static const c8 * const aliases[][2] = {
        {"f32", "float"}, {"u32", "uint"}, {"uint32_t", "uint"},
        {"s32", "int"}, {"int32_t", "int"},
    };
    for (u32 a = 0; a < sizeof(aliases) / sizeof(aliases[0]); a++) {
        if (is(i, aliases[a][0])) { return aliases[a][1]; }
    }
    for (u32 p = 0; p < sizeof(hlsl_prims) / sizeof(hlsl_prims[0]); p++) {
        if (is(i, hlsl_prims[p].type)) { return hlsl_prims[p].type; }
    }
    return NULL;
}

// a type as it is in HLSL
static void cat_hlsl_type(Buf * b, const u32 i) {
    const c8 * prim = hlsl_prim(i);
    if (prim != NULL) {
        buf_cat(b, prim);
    } else {
        cat_name(b, i);
    }
}

static void cat_u32(Buf * b, const u32 value) {
    c8 n[16];
    n[0] = '\0';
    T1_std_strcat_u32_cap(n, 16, value);
    buf_cat(b, n);
}

static const c8 * const hlsl_prelude =
    // (see cat_hlsl_atomics(): each writable buffer of uints has its own)
    "#define T1_atomic_load(b, i) t1_atomic_add_##b((i), 0u)\n"
    "#define T1_atomic_store(b, i, v) t1_atomic_exchange_##b((i), (v))\n"
    "#define T1_atomic_add(b, i, v) t1_atomic_add_##b((i), (v))\n"
    "#define T1_atomic_min(b, i, v) t1_atomic_min_##b((i), (v))\n"
    "#define T1_atomic_max(b, i, v) t1_atomic_max_##b((i), (v))\n"
    "#define T1_atomic_and(b, i, v) t1_atomic_and_##b((i), (v))\n"
    "#define T1_atomic_or(b, i, v) t1_atomic_or_##b((i), (v))\n"
    "#define T1_atomic_xor(b, i, v) t1_atomic_xor_##b((i), (v))\n"
    "#define T1_atomic_exchange(b, i, v) t1_atomic_exchange_##b((i), (v))\n"
    "#define T1_atomic_compare_exchange(b, i, e, v) "
        "t1_atomic_compare_exchange_##b((i), (e), (v))\n"
    // (FXC's preprocessor refuses "name()": no parameters, no parentheses)
    "#define T1_threadgroup_barrier GroupMemoryBarrierWithGroupSync\n"
    // (see hlsl_vector_writes())
    "#define T1_VEC_SET_2(T) T##2 t1_vec_set_##T##2(T##2 v, uint i, T e) { "
        "v.x = i == 0u ? e : v.x; v.y = i == 1u ? e : v.y; return v; }\n"
    "#define T1_VEC_SET_3(T) T##3 t1_vec_set_##T##3(T##3 v, uint i, T e) { "
        "v.x = i == 0u ? e : v.x; v.y = i == 1u ? e : v.y; "
        "v.z = i == 2u ? e : v.z; return v; }\n"
    "#define T1_VEC_SET_4(T) T##4 t1_vec_set_##T##4(T##4 v, uint i, T e) { "
        "v.x = i == 0u ? e : v.x; v.y = i == 1u ? e : v.y; "
        "v.z = i == 2u ? e : v.z; v.w = i == 3u ? e : v.w; return v; }\n"
    "#define T1_VEC_SET(T) T1_VEC_SET_2(T) T1_VEC_SET_3(T) T1_VEC_SET_4(T)\n"
    "T1_VEC_SET(float) T1_VEC_SET(int) T1_VEC_SET(uint) T1_VEC_SET(bool)\n"
    "float2x2 t1_mat_set_float2x2(float2x2 m, uint i, float2 e) { "
        "m[0] = i == 0u ? e : m[0]; m[1] = i == 1u ? e : m[1]; return m; }\n"
    "float3x3 t1_mat_set_float3x3(float3x3 m, uint i, float3 e) { "
        "m[0] = i == 0u ? e : m[0]; m[1] = i == 1u ? e : m[1]; "
        "m[2] = i == 2u ? e : m[2]; return m; }\n"
    "float4x4 t1_mat_set_float4x4(float4x4 m, uint i, float4 e) { "
        "m[0] = i == 0u ? e : m[0]; m[1] = i == 1u ? e : m[1]; "
        "m[2] = i == 2u ? e : m[2]; m[3] = i == 3u ? e : m[3]; return m; }\n"
    // (see hlsl_constructor_edits())
    "float2x2 t1_mat_fill_float2x2(float x) { "
        "return float2x2(x, 0.0f, 0.0f, x); }\n"
    "float2x2 t1_mat_fill_float2x2(float2x2 m) { return m; }\n"
    "float3x3 t1_mat_fill_float3x3(float x) { "
        "return float3x3(x, 0.0f, 0.0f, 0.0f, x, 0.0f, 0.0f, 0.0f, x); }\n"
    "float3x3 t1_mat_fill_float3x3(float3x3 m) { return m; }\n"
    "float4x4 t1_mat_fill_float4x4(float x) { return float4x4("
        "x, 0.0f, 0.0f, 0.0f, 0.0f, x, 0.0f, 0.0f, "
        "0.0f, 0.0f, x, 0.0f, 0.0f, 0.0f, 0.0f, x); }\n"
    "float4x4 t1_mat_fill_float4x4(float4x4 m) { return m; }\n"
    "#define T1_HLSL 1\n"
    "#pragma pack_matrix(row_major)\n"
    "#define f32 float\n"
    "#define u32 uint\n"
    "#define s32 int\n"
    "#define uint32_t uint\n"
    "#define int32_t int\n"
    "#define fmax max\n"
    "#define fmin min\n"
    "#define fract frac\n"
    "#define mix lerp\n"
    /*
    See the matrices above: mul() swaps its arguments. Through functions
    defined before the macro: FXC's preprocessor doesn't swap them with
    #define mul(a, b) mul(b, a) (every projection came out transposed)
    */
    "float4 t1_mul(float4 a, float4x4 b) { return mul(b, a); }\n"
    "float3 t1_mul(float3 a, float3x3 b) { return mul(b, a); }\n"
    "float2 t1_mul(float2 a, float2x2 b) { return mul(b, a); }\n"
    "float4x4 t1_mul(float4x4 a, float4x4 b) { return mul(b, a); }\n"
    "float3x3 t1_mul(float3x3 a, float3x3 b) { return mul(b, a); }\n"
    "float2x2 t1_mul(float2x2 a, float2x2 b) { return mul(b, a); }\n"
    "#define mul(a, b) t1_mul(a, b)\n"
    /*
    HLSL's fmod goes through a reciprocal, fmod(7, -3.5) came out 3.5. MSL's
    truncates: a - b * trunc(a / b), and a / b just off a whole number
    (rounding) is put right, so the result is below |b|, with a's sign
    */
#define T1_HLSL_FMOD(A, B) \
    A " t1_fmod(" A " a, " B " b) {\n" \
    "    " A " r = a - b * trunc(a / b);\n" \
    "    " A " ab = abs(b);\n" \
    "    r = abs(r) >= ab ? r - ab * sign(r) : r;\n" \
    "    r = r * sign(a) < 0.0f ? r + ab * sign(a) : r;\n" \
    "    return r;\n" \
    "}\n"
    T1_HLSL_FMOD("float", "float")
    T1_HLSL_FMOD("float2", "float2")
    T1_HLSL_FMOD("float3", "float3")
    T1_HLSL_FMOD("float4", "float4")
    T1_HLSL_FMOD("float2", "float")
    T1_HLSL_FMOD("float3", "float")
    T1_HLSL_FMOD("float4", "float")
#undef T1_HLSL_FMOD
    "#define fmod(a, b) t1_fmod(a, b)\n"
    // MSL rounds halfway away from 0, HLSL to even
    "#define round(x) trunc((x) + 0.5f * sign(x))\n"
    "#define T1_MAX(a, b) ((a) > (b) ? (a) : (b))\n"
    "#define T1_MIN(a, b) ((a) < (b) ? (a) : (b))\n"
    "#define T1_ROUND_UP(x, a) (((x) + (a) - 1) / (a) * (a))\n";

/*
An integer constant expression of numbers, macros, + - * / % and
parentheses, e.g. the n of buffer(T1_BUFS_I + 1): HLSL needs it as a plain
number in register()
*/
typedef struct {
    const Check * c;
    const c8 * s;
    u32 depth; // of macros
    b8 bad;
} Eval;

static s64 eval_sum(Eval * e);

static void eval_space(Eval * e) {
    while (*e->s == ' ' || *e->s == '\t') { e->s++; }
}

// what a #define gives name, NULL if it's not one
static const c8 * macro_value(const Check * c, const c8 * name, const u32 n) {
    for (u32 i = 0; i < c->count; i++) {
        if (!preproc_is(i, "define")) { continue; }
        const c8 * t = T1_token_text(tz, i) + 1;
        while (*t == ' ' || *t == '\t') { t++; }
        t += 6;
        while (*t == ' ' || *t == '\t') { t++; }
        if (
            T1_std_are_equal_strings_of_length(t, name, n) &&
            !is_name_char(t[n]) &&
            t[n] != '(')
        {
            return t + n;
        }
    }
    return NULL;
}

static s64 eval_atom(Eval * e) {
    eval_space(e);
    c8 ch = *e->s;
    if (ch == '(') {
        e->s++;
        s64 value = eval_sum(e);
        eval_space(e);
        if (*e->s == ')') { e->s++; } else { e->bad = 1; }
        return value;
    }
    if (ch == '-') { e->s++; return -eval_atom(e); }
    if (ch >= '0' && ch <= '9') {
        s64 value = 0;
        if (ch == '0' && (e->s[1] == 'x' || e->s[1] == 'X')) {
            e->s += 2;
            while (1) {
                c8 d = *e->s;
                s64 digit =
                    d >= '0' && d <= '9' ? d - '0' :
                    d >= 'a' && d <= 'f' ? d - 'a' + 10 :
                    d >= 'A' && d <= 'F' ? d - 'A' + 10 : -1;
                if (digit < 0) { break; }
                value = value * 16 + digit;
                e->s++;
            }
        } else {
            while (*e->s >= '0' && *e->s <= '9') {
                value = value * 10 + (*e->s - '0');
                e->s++;
            }
        }
        while (*e->s == 'u' || *e->s == 'U') { e->s++; }
        return value;
    }
    if (is_name_char(ch)) {
        const c8 * name = e->s;
        while (is_name_char(*e->s)) { e->s++; }
        const c8 * value_text =
            macro_value(e->c, name, (u32)(e->s - name));
        if (value_text == NULL || e->depth >= 16) {
            e->bad = 1;
            return 0;
        }
        Eval sub = {e->c, value_text, e->depth + 1, 0};
        s64 value = eval_sum(&sub);
        eval_space(&sub);
        if (
            sub.bad ||
            !(*sub.s == '\0' || *sub.s == '\r' || *sub.s == '\n' ||
            (sub.s[0] == '/' && (sub.s[1] == '/' || sub.s[1] == '*'))))
        {
            e->bad = 1;
        }
        return value;
    }
    e->bad = 1;
    return 0;
}

static s64 eval_product(Eval * e) {
    s64 value = eval_atom(e);
    while (!e->bad) {
        eval_space(e);
        c8 op = *e->s;
        if ((op != '*' && op != '/' && op != '%') || e->s[1] == '/') { break; }
        e->s++;
        s64 rhs = eval_atom(e);
        if (op != '*' && rhs == 0) { e->bad = 1; break; }
        value = op == '*' ? value * rhs : op == '/' ? value / rhs : value % rhs;
    }
    return value;
}

static s64 eval_sum(Eval * e) {
    s64 value = eval_product(e);
    while (!e->bad) {
        eval_space(e);
        c8 op = *e->s;
        if (op != '+' && op != '-') { break; }
        e->s++;
        s64 rhs = eval_product(e);
        value = op == '+' ? value + rhs : value - rhs;
    }
    return value;
}

// buffer(n) in an attribute as a number, -1 if it's not a constant
static s64 eval_attr_arg(const Check * c, const u32 attr_i, const c8 * name) {
    c8 arg[256];
    Buf b = {arg, 0, 256, 0};
    arg[0] = '\0';
    cat_attr_arg(&b, attr_i, name);
    if (b.overflow || arg[0] == '\0') { return -1; }
    Eval e = {c, arg, 0, 0};
    s64 value = eval_sum(&e);
    eval_space(&e);
    return e.bad || *e.s != '\0' ? -1 : value;
}

/*
The #pragma pack(n) in effect at token i, 0 for none: it caps the alignment
of struct members
*/
static u32 pack_at(const Check * c, const u32 at) {
    u32 stack[16];
    u32 size = 0;
    u32 pack = 0;
    for (u32 i = 0; i < at && i < c->count; i++) {
        if (!preproc_is(i, "pragma")) { continue; }
        const c8 * t = T1_token_value(tz, i);
        const c8 * p = t;
        while (*p != '\0' && !T1_std_are_equal_strings_of_length(p, "pack", 4)) {
            p++;
        }
        if (*p == '\0') { continue; }
        b8 push = 0;
        b8 pop = 0;
        u32 value = 0;
        b8 has_value = 0;
        for (; *p != '\0' && *p != '\n'; p++) {
            if (T1_std_are_equal_strings_of_length(p, "push", 4)) { push = 1; }
            if (T1_std_are_equal_strings_of_length(p, "pop", 3)) { pop = 1; }
            if (*p >= '0' && *p <= '9') {
                value = value * 10 + (u32)(*p - '0');
                has_value = 1;
            }
        }
        if (push && size < 16) { stack[size++] = pack; }
        if (pop) {
            pack = size > 0 ? stack[--size] : 0;
        } else {
            pack = has_value ? value : (push ? pack : 0);
        }
    }
    return pack;
}

typedef struct {
    u32 type_i;
    u32 name_i;
    u32 count_open; // '[' of an array, UINT32_MAX if it's not one
    u32 count_close;
} DataMember;

#define DATA_MEMBERS_CAP 64

// whether an #if's line from text on ends here
static b8 eval_line_end(const c8 * s) {
    return
        *s == '\0' || *s == '\r' || *s == '\n' ||
        (s[0] == '/' && (s[1] == '/' || s[1] == '*'));
}

/*
An #if's condition: sums compared with == != < > <= >=, joined by && and
||, and defined(NAME). -1 if it has anything else
*/
static s64 eval_condition(Eval * e) {
    s64 result = 0;
    b8 in_or = 0;
    while (!e->bad) {
        s64 all = 1; // of the && chain
        while (!e->bad) {
            eval_space(e);
            s64 value;
            if (T1_std_are_equal_strings_of_length(e->s, "defined", 7)) {
                e->s += 7;
                eval_space(e);
                b8 paren = *e->s == '(';
                if (paren) { e->s++; eval_space(e); }
                const c8 * name = e->s;
                while (is_name_char(*e->s)) { e->s++; }
                u32 n = (u32)(e->s - name);
                value = n > 0 && macro_value(e->c, name, n) != NULL;
                if (n == 0) { e->bad = 1; }
                eval_space(e);
                if (paren) {
                    if (*e->s == ')') { e->s++; } else { e->bad = 1; }
                }
            } else {
                value = eval_sum(e);
                eval_space(e);
                static const c8 * const ops[] = {
                    "==", "!=", "<=", ">=", "<", ">",
                };
                for (u32 o = 0; o < 6; o++) {
                    u32 n = o < 4 ? 2 : 1;
                    if (!T1_std_are_equal_strings_of_length(e->s, ops[o], n)) {
                        continue;
                    }
                    e->s += n;
                    s64 rhs = eval_sum(e);
                    value =
                        o == 0 ? value == rhs : o == 1 ? value != rhs :
                        o == 2 ? value <= rhs : o == 3 ? value >= rhs :
                        o == 4 ? value < rhs : value > rhs;
                    break;
                }
            }
            all = all && value != 0;
            eval_space(e);
            if (!T1_std_are_equal_strings_of_length(e->s, "&&", 2)) { break; }
            e->s += 2;
        }
        result = in_or ? (result || all) : all;
        in_or = 1;
        eval_space(e);
        if (!T1_std_are_equal_strings_of_length(e->s, "||", 2)) { break; }
        e->s += 2;
    }
    eval_space(e);
    return e->bad || !eval_line_end(e->s) ? -1 : result;
}

/*
A struct's members for its load and store functions: [const] Type name;
also "Type a, b[N];", and the members of the #if branches its macros take
(so the layout is the one the client's headers compile to). 0 if it has
something else (attributes, conditions it can't evaluate)
*/
static u32 data_members(
    const Check * c,
    const StructDef * d,
    DataMember * recip)
{
    // per #if level: whether it's taken, whether a branch of it was
    b8 taken[16];
    b8 any_taken[16];
    u32 depth = 0;
    b8 active = 1;

    u32 size = 0;
    u32 i = d->body_open + 1;
    while (i < d->body_close) {
        if (kind(i) == SK_PREPROC) {
            const c8 * t = T1_token_text(tz, i) + 1;
            while (*t == ' ' || *t == '\t') { t++; }
            b8 parent = depth == 0 || taken[depth - 1];
            if (preproc_is(i, "ifdef") || preproc_is(i, "ifndef")) {
                b8 negate = preproc_is(i, "ifndef");
                t += negate ? 6 : 5;
                while (*t == ' ' || *t == '\t') { t++; }
                const c8 * name = t;
                while (is_name_char(*t)) { t++; }
                if (depth >= 16 || t == name) { return 0; }
                b8 is_defined =
                    macro_value(c, name, (u32)(t - name)) != NULL;
                taken[depth] = parent && (is_defined != negate);
                any_taken[depth] = taken[depth];
                depth++;
            } else if (preproc_is(i, "if") || preproc_is(i, "elif")) {
                b8 elif = preproc_is(i, "elif");
                if (elif && depth == 0) { return 0; }
                if (!elif && depth >= 16) { return 0; }
                Eval e = {c, t + (elif ? 4 : 2), 0, 0};
                s64 value = eval_condition(&e);
                if (value < 0) { return 0; }
                if (elif) {
                    parent = depth == 1 || taken[depth - 2];
                    taken[depth - 1] =
                        parent && !any_taken[depth - 1] && value != 0;
                    any_taken[depth - 1] =
                        any_taken[depth - 1] || taken[depth - 1];
                } else {
                    taken[depth] = parent && value != 0;
                    any_taken[depth] = taken[depth];
                    depth++;
                }
            } else if (preproc_is(i, "else")) {
                if (depth == 0) { return 0; }
                parent = depth == 1 || taken[depth - 2];
                taken[depth - 1] = parent && !any_taken[depth - 1];
                any_taken[depth - 1] = 1;
            } else if (preproc_is(i, "endif")) {
                if (depth == 0) { return 0; }
                depth--;
            } else if (active) {
                // an #error the macros reach, or a #define of its own
                return 0;
            }
            active = depth == 0 || taken[depth - 1];
            i++;
            continue;
        }
        if (!active) { i++; continue; }
        if (is(i, "const")) { i++; }
        if (kind(i) != SK_IDENT) { return 0; }
        u32 type = i++;
        while (1) {
            if (kind(i) != SK_IDENT || size >= DATA_MEMBERS_CAP) { return 0; }
            DataMember * m = &recip[size++];
            m->type_i = type;
            m->name_i = i++;
            m->count_open = UINT32_MAX;
            m->count_close = UINT32_MAX;
            if (is_punct(i, "[")) {
                m->count_open = i;
                while (i < d->body_close && !is_punct(i, "]")) {
                    if (kind(i) == SK_ATTR || kind(i) == SK_PREPROC) {
                        return 0;
                    }
                    i++;
                }
                m->count_close = i++;
            }
            if (is_punct(i, ",")) { i++; continue; }
            if (is_punct(i, ";")) { i++; break; }
            return 0;
        }
    }
    return depth == 0 ? size : 0;
}

// "(N)" of an array member, "1" if it's not one
static void cat_count(Buf * b, const DataMember * m) {
    if (m->count_open == UINT32_MAX) { buf_cat(b, "1"); return; }
    buf_cat(b, "(");
    cat_tokens(b, m->count_open + 1, m->count_close);
    buf_cat(b, ")");
}

// "t1_size_" or "t1_align_" etc. and the type
static void cat_layout_name(Buf * b, const c8 * what, const u32 type_i) {
    buf_cat(b, what);
    cat_hlsl_type(b, type_i);
}

static void cat_member_access(
    Buf * b,
    const DataMember * m,
    const c8 * index)
{
    cat_name(b, m->name_i);
    if (m->count_open != UINT32_MAX) {
        buf_cat(b, "[");
        buf_cat(b, index);
        buf_cat(b, "]");
    }
}

/*
The layout (MSL's) of a struct and its load and store functions, after its
definition. Structs with something they can't load (bool, attributes) get
none, using them in a buffer is then an error of HLSL's
*/
static void hlsl_struct_functions(Gen * g, b8 * loadable) {
    Check * c = g->c;

    for (u32 s = 0; s < c->struct_defs_size; s++) {
        const StructDef * d = &c->struct_defs[s];
        loadable[s] = 0;

        DataMember members[DATA_MEMBERS_CAP];
        u32 size = data_members(c, d, members);
        if (size == 0) { continue; }

        // every member's type must be loadable and defined before
        b8 ok = 1;
        for (u32 m = 0; m < size && ok; m++) {
            if (hlsl_prim(members[m].type_i) != NULL) { continue; }
            b8 found = 0;
            for (u32 earlier = 0; earlier < s; earlier++) {
                if (
                    loadable[earlier] &&
                    same_text(c->struct_defs[earlier].name_i,
                        members[m].type_i))
                {
                    found = 1;
                }
            }
            ok = found;
        }
        if (!ok) { continue; }

        // where its definition ends
        u32 semi = d->name_i > d->body_close ? d->name_i + 1 : d->body_close + 1;
        if (!is_punct(semi, ";")) { continue; }
        loadable[s] = 1;

        u32 pack = pack_at(c, d->body_open);
        u32 start = str_begin(g);
        Buf * a = &g->arena;

        // static const uint t1_align_T_0 = T1_MAX(4u, t1_align_U0); ...
        buf_cat(a, "\n");
        for (u32 m = 0; m < size; m++) {
            buf_cat(a, "static const uint t1_align_");
            cat_name(a, d->name_i);
            buf_cat(a, "_");
            cat_u32(a, m);
            buf_cat(a, " = T1_MAX(");
            if (m == 0) {
                buf_cat(a, "4u");
            } else {
                buf_cat(a, "t1_align_");
                cat_name(a, d->name_i);
                buf_cat(a, "_");
                cat_u32(a, m - 1);
            }
            buf_cat(a, ", ");
            if (pack > 0) { buf_cat(a, "T1_MIN("); }
            cat_layout_name(a, "t1_align_", members[m].type_i);
            if (pack > 0) {
                buf_cat(a, ", ");
                cat_u32(a, pack);
                buf_cat(a, "u)");
            }
            buf_cat(a, ");\n");

            // the member starts at the first multiple of its alignment
            buf_cat(a, "static const uint t1_off_");
            cat_name(a, d->name_i);
            buf_cat(a, "_");
            cat_u32(a, m);
            buf_cat(a, " = ");
            if (m == 0) {
                buf_cat(a, "0;\n");
            } else {
                buf_cat(a, "T1_ROUND_UP(t1_off_");
                cat_name(a, d->name_i);
                buf_cat(a, "_");
                cat_u32(a, m - 1);
                buf_cat(a, " + ");
                cat_layout_name(a, "t1_size_", members[m - 1].type_i);
                buf_cat(a, " * ");
                cat_count(a, &members[m - 1]);
                buf_cat(a, ", ");
                if (pack > 0) { buf_cat(a, "T1_MIN("); }
                cat_layout_name(a, "t1_align_", members[m].type_i);
                if (pack > 0) {
                    buf_cat(a, ", ");
                    cat_u32(a, pack);
                    buf_cat(a, "u)");
                }
                buf_cat(a, ");\n");
            }
        }
        buf_cat(a, "static const uint t1_align_");
        cat_name(a, d->name_i);
        buf_cat(a, " = t1_align_");
        cat_name(a, d->name_i);
        buf_cat(a, "_");
        cat_u32(a, size - 1);
        buf_cat(a, ";\n");
        // its size is a multiple of its alignment, like in an array
        buf_cat(a, "static const uint t1_size_");
        cat_name(a, d->name_i);
        buf_cat(a, " = T1_ROUND_UP(t1_off_");
        cat_name(a, d->name_i);
        buf_cat(a, "_");
        cat_u32(a, size - 1);
        buf_cat(a, " + ");
        cat_layout_name(a, "t1_size_", members[size - 1].type_i);
        buf_cat(a, " * ");
        cat_count(a, &members[size - 1]);
        buf_cat(a, ", t1_align_");
        cat_name(a, d->name_i);
        buf_cat(a, ");\n");

        // a load function for read-only and writable buffers, a store one
        static const c8 * const buffer_types[] = {
            "ByteAddressBuffer", "RWByteAddressBuffer",
        };
        for (u32 bt = 0; bt < 3; bt++) {
            b8 store = bt == 2;
            if (store) {
                buf_cat(a, "void t1_store_");
                cat_name(a, d->name_i);
                buf_cat(a, "(RWByteAddressBuffer b, uint a, ");
                cat_name(a, d->name_i);
                buf_cat(a, " v) {\n");
            } else {
                cat_name(a, d->name_i);
                buf_cat(a, " t1_load_");
                cat_name(a, d->name_i);
                buf_cat(a, "(");
                buf_cat(a, buffer_types[bt]);
                buf_cat(a, " b, uint a) {\n    ");
                cat_name(a, d->name_i);
                buf_cat(a, " v;\n");
            }
            for (u32 m = 0; m < size; m++) {
                b8 array = members[m].count_open != UINT32_MAX;
                const c8 * index = array ? "t1_i" : "";
                buf_cat(a, "    ");
                /*
                In braces: a for's variable lives on after the loop in
                HLSL. Unrolled: the arrays are short, and it spares the
                compiler a local array indexed with a variable
                */
                if (array) {
                    buf_cat(a, "{ [unroll] for (uint t1_i = 0; t1_i < ");
                    cat_count(a, &members[m]);
                    buf_cat(a, "; t1_i++) { ");
                }
                if (store) {
                    cat_layout_name(a, "t1_store_", members[m].type_i);
                    buf_cat(a, "(b, ");
                } else {
                    buf_cat(a, "v.");
                    cat_member_access(a, &members[m], index);
                    buf_cat(a, " = ");
                    cat_layout_name(a, "t1_load_", members[m].type_i);
                    buf_cat(a, "(b, ");
                }
                buf_cat(a, "a + t1_off_");
                cat_name(a, d->name_i);
                buf_cat(a, "_");
                cat_u32(a, m);
                if (array) {
                    buf_cat(a, " + t1_i * ");
                    cat_layout_name(a, "t1_size_", members[m].type_i);
                }
                if (store) {
                    buf_cat(a, ", v.");
                    cat_member_access(a, &members[m], index);
                }
                buf_cat(a, ");");
                if (array) { buf_cat(a, " } }"); }
                buf_cat(a, "\n");
            }
            buf_cat(a, store ? "}\n" : "    return v;\n}\n");
        }
        add_text(g, &g->g_post[semi], str_end(g, start));
    }
}

static b8 is_assign_op(const u32 i) {
    static const c8 * const ops[] = {
        "=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=",
        "++", "--",
    };
    return
        kind(i) == SK_PUNCT &&
        in_list(i, ops, sizeof(ops) / sizeof(ops[0]));
}

/*
FXC can't write a vector's component or a matrix's column through an index it
doesn't know, MSL and GLSL can (X3500, or "forced to unroll loop, but
unrolling failed" around it). So a statement that does, on a variable or its
members, sets the whole vector or matrix instead (a buffer's element needs
none of this, its address goes down to what's written, hlsl_member_offsets()):
    v[i] += e;  ->  { uint t1_c = (i); v = t1_vec_set_float3(v, t1_c,
                      v[t1_c] + (e)); }
    m[i].x = e; ->  { uint t1_c = (i); float3 t1_v = m[t1_c]; t1_v.x = e;
                      m = t1_mat_set_float3x3(m, t1_c, t1_v); }
    m[i][j] = e; does both. Reading through an index is fine, and so are
literal indices, which stay as they are.
*/

// The first index in the chain (up to op) FXC can't write through, or op
static u32 chain_first_runtime_index(
    const Check * c,
    VType * t,
    const u32 chain,
    const u32 op)
{
    u32 at = chain;
    while (at < op) {
        if (is_punct(at, ".")) {
            if (t->base != VB_STRUCT || t->array) { return op; } // a swizzle
            *t = vtype_of_member(c, *t, at + 1);
            at += 2;
        } else if (is_punct(at, "[")) {
            const u32 close = after_match(c, at, "[", "]") - 1;
            if (t->array) {
                t->array = 0;
            } else if (t->base == VB_MATRIX || is_vector(*t)) {
                if (kind(at + 1) != SK_NUMBER || at + 2 != close) {
                    return at;
                }
                *t = t->base == VB_MATRIX ?
                    vtype(VB_FLOAT, t->width) : vtype(t->base, 1);
            } else {
                return op;
            }
            at = close + 1;
        } else {
            return op;
        }
    }
    return op;
}

/*
first..open is written out again (and again) for the rewrite, so it can't
hold what must happen once, or what has edits of its own: buffers' reads
*/
static b8 chain_is_plain(const Func * fn, const u32 first, const u32 open) {
    for (u32 t = first; t < open; t++) {
        if (is_assign_op(t) || (is_punct(t, "(") && kind(t - 1) == SK_IDENT)) {
            return 0;
        }
        for (u32 p = 0; p < fn->ptrs_size; p++) {
            if (same_text(fn->ptrs[p].name_i, t)) { return 0; }
        }
    }
    return 1;
}

static void cat_vec_set_name(Buf * a, const VType t) {
    buf_cat(a, "t1_vec_set_");
    buf_cat(a,
        t.base == VB_FLOAT ? "float" :
        t.base == VB_INT ? "int" :
        t.base == VB_UINT ? "uint" : "bool");
    cat_u32(a, t.width);
}

// What op says on top of the value set: "v[t1_c] + (", or just "("
static void cat_set_value_open(
    Buf * a,
    const c8 * vec,
    const c8 * index,
    const u32 op)
{
    const b8 step = is_punct(op, "++") || is_punct(op, "--");
    if (!is_punct(op, "=")) {
        buf_cat(a, vec);
        buf_cat(a, "[");
        buf_cat(a, index);
        buf_cat(a, "] ");
        // the operator without its '=' (++ and -- add or take 1)
        buf_cat_n(a,
            T1_token_text(tz, op),
            step ? 1 : T1_token_size(tz, op) - 1);
        buf_cat(a, " ");
    }
    buf_cat(a, step ? "(1" : "(");
}

/*
The statement from first to semi writes through the index at open, of a
vector or matrix of type t that first..open names.
*/
static void hlsl_chain_write(
    Gen * g,
    const VType t,
    const u32 first,
    const u32 open,
    const u32 op,
    const u32 semi)
{
    Check * c = g->c;
    Buf * a = &g->arena;
    const u32 close = after_match(c, open, "[", "]") - 1;

    u32 s = str_begin(g);
    for (u32 n = first; n < open; n++) { cat_name(a, n); }
    const c8 * base = str_end(g, s);

    g->g_repl[first] = "{ uint t1_c = (";
    for (u32 n = first + 1; n <= open; n++) { g->g_skip[n] = 1; }

    if (t.base != VB_MATRIX) {
        s = str_begin(g);
        buf_cat(a, "); ");
        buf_cat(a, base);
        buf_cat(a, " = ");
        cat_vec_set_name(a, t);
        buf_cat(a, "(");
        buf_cat(a, base);
        buf_cat(a, ", t1_c, ");
        cat_set_value_open(a, base, "t1_c", op);
        g->g_repl[close] = str_end(g, s);
        g->g_skip[op] = 1;

        s = str_begin(g);
        buf_cat(a, ")); }");
        g->g_repl[semi] = str_end(g, s);
        return;
    }

    const VType column = vtype(VB_FLOAT, t.width);
    s = str_begin(g);
    buf_cat(a, "); float");
    cat_u32(a, t.width);
    buf_cat(a, " t1_v = ");
    buf_cat(a, base);
    buf_cat(a, "[t1_c]; ");
    const b8 element = is_punct(close + 1, "[");
    buf_cat(a, element ? "{ uint t1_d = (" : "t1_v");
    g->g_repl[close] = str_end(g, s);

    u32 store = str_begin(g);
    if (element) {
        const u32 close2 = after_match(c, close + 1, "[", "]") - 1;
        g->g_skip[close + 1] = 1;
        s = str_begin(g);
        buf_cat(a, "); t1_v = ");
        cat_vec_set_name(a, column);
        buf_cat(a, "(t1_v, t1_d, ");
        cat_set_value_open(a, "t1_v", "t1_d", op);
        g->g_repl[close2] = str_end(g, s);
        g->g_skip[op] = 1;
        store = str_begin(g);
        buf_cat(a, ")); } ");
    } else {
        buf_cat(a, "; ");
    }
    buf_cat(a, base);
    buf_cat(a, " = t1_mat_set_float");
    cat_u32(a, t.width);
    buf_cat(a, "x");
    cat_u32(a, t.width);
    buf_cat(a, "(");
    buf_cat(a, base);
    buf_cat(a, ", t1_c, t1_v); }");
    g->g_repl[semi] = str_end(g, store);
}

// Goes past the .member and [index] parts of a chain, from t
static u32 chain_after(const Check * c, const Func * fn, u32 t) {
    while (t < fn->body_close) {
        if (is_punct(t, ".") && kind(t + 1) == SK_IDENT) {
            t += 2;
        } else if (is_punct(t, "[")) {
            t = after_match(c, t, "[", "]");
        } else {
            break;
        }
    }
    return t;
}

// The statement that op starts ends at the ';' this returns, or fails
static u32 assign_statement_end(const Func * fn, const u32 op) {
    u32 semi = op;
    u32 nesting = 0;
    while (semi < fn->body_close) {
        if (is_punct(semi, "(")) { nesting++; }
        if (is_punct(semi, ")")) {
            if (nesting == 0) { break; }
            nesting--;
        }
        if (nesting == 0 && is_punct(semi, ";")) { break; }
        semi++;
    }
    return semi;
}

static b8 starts_statement(const u32 i) {
    return
        is_punct(i - 1, ";") || is_punct(i - 1, "{") ||
        is_punct(i - 1, "}") || is_punct(i - 1, ")") ||
        is_punct(i - 1, ":") || is(i - 1, "else");
}

static void hlsl_vector_writes(Gen * g) {
    Check * c = g->c;

    for (u32 f = 0; f < c->funcs_size; f++) {
        const Func * fn = &c->funcs[f];
        for (u32 i = fn->body_open + 1; i < fn->body_close; i++) {
            if (kind(i) != SK_IDENT || is_punct(i - 1, ".")) { continue; }
            if (!is_punct(i + 1, "[") && !is_punct(i + 1, ".")) { continue; }

            // (a buffer's element: hlsl_buffer_edits())
            b8 buffer = 0;
            for (u32 p = 0; p < fn->ptrs_size; p++) {
                if (same_text(fn->ptrs[p].name_i, i)) { buffer = 1; }
            }
            if (buffer) { continue; }

            // the chain up to what comes after it
            const u32 op = chain_after(c, fn, i + 1);
            VType t = vtype_of_variable(c, i);
            const u32 open = chain_first_runtime_index(c, &t, i + 1, op);
            if (open == op) { continue; }

            if (!is_assign_op(op)) {
                if (is_punct(i - 1, "++") || is_punct(i - 1, "--")) {
                    fail(g, i, "write a vector's component as v[i] op= value;");
                }
                continue;
            }
            const u32 semi = assign_statement_end(fn, op);
            if (!is_punct(semi, ";") || !starts_statement(i)) {
                fail(g, i, "write a vector's component as v[i] op= value;");
                continue;
            }
            if (!chain_is_plain(fn, i, open)) {
                fail(g, i,
                    "writing a vector's component through a variable index, "
                    "keep the indices before it plain (no calls or buffers)");
                continue;
            }
            // (setting the whole vector would undo what other threads wrote
            // to its other components)
            for (u32 j = i; j > fn->params_open; j--) {
                if (same_text(j - 1, i) && declares(c, j - 1)) {
                    if (is(j - 3, "threadgroup")) {
                        fail(g, i,
                            "threadgroup memory's vectors take literal "
                            "indices only");
                    }
                    break;
                }
            }
            hlsl_chain_write(g, t, i, open, op, semi);
        }
    }
}

/*
The type's token of the declaration that j names: float a = 1.0f, b; gives
float for a and b. UINT32_MAX if j isn't one
*/
static u32 local_declaration_type(
    const Check * c,
    const Func * fn,
    const u32 j)
{
    u32 nesting = 0;
    for (u32 t = j - 1; t > fn->body_open; t--) {
        if (
            is_bracket(c, t, ")") || is_bracket(c, t, "]") ||
            is_bracket(c, t, "}"))
        {
            nesting++;
            continue;
        }
        if (
            is_bracket(c, t, "(") || is_bracket(c, t, "[") ||
            is_bracket(c, t, "{"))
        {
            if (nesting == 0) { return UINT32_MAX; }
            nesting--;
            continue;
        }
        if (nesting > 0) { continue; }
        if (is_punct(t, ";")) { return UINT32_MAX; }
        if (is_type(c, t) && declares(c, t + 1)) {
            return t + 1 == j || is_punct(j - 1, ",") ? t : UINT32_MAX;
        }
    }
    return UINT32_MAX;
}

/*
Whether FXC would refuse this array's reads unless it starts with values: its
first use isn't a write, or it's read at a literal index (FXC follows those).
Otherwise it stays as it is: zeros are work for every thread, which a stack
or a table filled before it's read shouldn't pay for
*/
static b8 local_array_needs_zeros(
    const Check * c,
    const Func * fn,
    const u32 name)
{
    b8 first = 1;
    for (u32 t = name + 1; t < fn->body_close; t++) {
        if (
            !same_text(t, name) ||
            is_punct(t - 1, ".") ||
            !is_punct(t + 1, "["))
        {
            continue;
        }
        const u32 close = after_match(c, t + 1, "[", "]") - 1;
        const u32 op = chain_after(c, fn, close + 1);
        const b8 write = is_assign_op(op);
        if (first && !is_punct(op, "=")) { return 1; }
        if (!write && kind(t + 2) == SK_NUMBER && t + 3 == close) { return 1; }
        first = 0;
    }
    return 0;
}

/*
A local array of constants (const float t[4] = { 0.1f, ... };) would be
filled again by every thread that runs it, in memory of its own: on a GPU
that means memory, per thread. FXC keeps a static one in a table the GPU
only reads. It makes no difference to what the shader does: nothing can
write to it, and only numbers, constructors and macros are in it.
*/
static void hlsl_const_tables(Gen * g) {
    Check * c = g->c;
    for (u32 f = 0; f < c->funcs_size; f++) {
        const Func * fn = &c->funcs[f];
        for (u32 i = fn->body_open + 1; i + 4 < fn->body_close; i++) {
            if (!is(i, "const") || c->secondary[i]) {
                continue;
            }
            // (constexpr is static already, see global_edits())
            if (is(i - 1, "constexpr")) { continue; }
            const u32 type = i + 1;
            if (
                !is_type(c, type) ||
                kind(type + 1) != SK_IDENT ||
                !is_punct(type + 2, "["))
            {
                continue;
            }
            const u32 eq = after_match(c, type + 2, "[", "]");
            if (!is_punct(eq, "=") || !is_punct(eq + 1, "{")) { continue; }
            const u32 end = after_match(c, eq + 1, "{", "}");
            b8 constants = 1;
            for (u32 t = eq + 2; t < end && constants; t++) {
                constants =
                    kind(t) != SK_IDENT ||
                    is_type(c, t) ||
                    in_names(t, c->macros, c->macros_size);
            }
            if (constants) { g->g_pre[i] = "static "; }
        }
    }
}

/*
A local with nothing in it is undefined in MSL, and a read of it an error in
FXC (X4000, X4575) where it can tell: they start at 0, arrays if they must.
*/
static void hlsl_local_zeros(Gen * g) {
    Check * c = g->c;
    Buf * a = &g->arena;
    for (u32 f = 0; f < c->funcs_size; f++) {
        const Func * fn = &c->funcs[f];
        for (u32 j = fn->body_open + 1; j < fn->body_close; j++) {
            if (kind(j) != SK_IDENT) { continue; }
            const b8 array = is_punct(j + 1, "[");
            const u32 end = array ? after_match(c, j + 1, "[", "]") : j + 1;
            if (!is_punct(end, ";") && !is_punct(end, ",")) { continue; }
            const u32 type = local_declaration_type(c, fn, j);
            if (type == UINT32_MAX) { continue; }
            if (is(type, "T1Textures") || is(type, "T1WriteTex")) { continue; }
            if (is(type - 1, "threadgroup")) { continue; }
            if (array && !local_array_needs_zeros(c, fn, j)) { continue; }
            u32 s = str_begin(g);
            buf_cat(a, " = (");
            cat_name(a, type);
            for (u32 n = j + 1; n < end; n++) { cat_name(a, n); }
            buf_cat(a, ")0");
            add_text(g, &g->g_post[end - 1], str_end(g, s));
        }
    }
}

/*
Turns the members after a buffer element (the chain from t to chain_end,
like .pos[1]) into offsets added to its address: .pos[1] becomes
" + t1_off_T_m + (1) * t1_size_float", as far as there are load functions,
then on into a matrix's column and a vector's component: .m[j].x becomes
" + t1_off_T_m + (j) * t1_size_float3 + 0u". Only what's used is loaded and
stored, which spares FXC the rest and leaves the neighbours to other threads
(and a store of a whole matrix right after loading one, in a thread that has
just stored it, loses the change on some AMD drivers).
type_i starts as the element's type, *type_name ends as the type at the
address, *last is the last token of the address (it starts as the element's
']')
*/
static void hlsl_member_offsets(
    Gen * g,
    const b8 * loadable,
    u32 t,
    const u32 chain_end,
    u32 type_i,
    const c8 ** type_name,
    u32 * last)
{
    Check * c = g->c;
    Buf * a = &g->arena;
    while (t < chain_end && is_punct(t, ".")) {
        const StructDef * d = find_struct(c, type_i);
        if (d == NULL || !loadable[d - c->struct_defs]) { break; }
        DataMember members[DATA_MEMBERS_CAP];
        u32 size = data_members(c, d, members);
        u32 m = 0;
        while (m < size && !same_text(members[m].name_i, t + 1)) {
            m++;
        }
        if (m == size) { break; }
        b8 array = members[m].count_open != UINT32_MAX;
        // a whole array member has no load function
        if (array && !is_punct(t + 2, "[")) { break; }

        u32 s = str_begin(g);
        buf_cat(a, " + t1_off_");
        cat_name(a, d->name_i);
        buf_cat(a, "_");
        cat_u32(a, m);
        g->g_repl[t] = str_end(g, s);
        g->g_skip[t + 1] = 1;
        type_i = members[m].type_i;
        *last = t + 1;
        t += 2;
        if (array) {
            u32 index_close = after_match(c, t, "[", "]") - 1;
            g->g_repl[t] = " + (";
            s = str_begin(g);
            buf_cat(a, ") * t1_size_");
            cat_hlsl_type(a, type_i);
            g->g_repl[index_close] = str_end(g, s);
            *last = index_close;
            t = index_close + 1;
        }
    }

    u32 s = str_begin(g);
    cat_hlsl_type(a, type_i);
    *type_name = str_end(g, s);

    VType v = vtype_of_name(c, type_i);
    if (v.base == VB_MATRIX && t < chain_end && is_punct(t, "[")) {
        const u32 close = after_match(c, t, "[", "]") - 1;
        g->g_repl[t] = " + (";
        s = str_begin(g);
        buf_cat(a, ") * t1_size_float");
        cat_u32(a, v.width);
        g->g_repl[close] = str_end(g, s);
        s = str_begin(g);
        buf_cat(a, "float");
        cat_u32(a, v.width);
        *type_name = str_end(g, s);
        *last = close;
        t = close + 1;
        v = vtype(VB_FLOAT, v.width);
    }
    if (!is_vector(v) || v.base == VB_BOOL || t >= chain_end) { return; }

    const c8 * component =
        v.base == VB_FLOAT ? "float" : v.base == VB_INT ? "int" : "uint";
    if (is_punct(t, "[")) {
        const u32 close = after_match(c, t, "[", "]") - 1;
        g->g_repl[t] = " + (";
        g->g_repl[close] = ") * 4u";
        *type_name = component;
        *last = close;
    } else if (
        is_punct(t, ".") &&
        swizzle_size(t + 1) == 1 &&
        T1_token_size(tz, t + 1) == 1)
    {
        const c8 letter = T1_token_text(tz, t + 1)[0];
        s = str_begin(g);
        buf_cat(a, " + ");
        cat_u32(a,
            (letter == 'x' || letter == 'r') ? 0u :
            (letter == 'y' || letter == 'g') ? 4u :
            (letter == 'z' || letter == 'b') ? 8u : 12u);
        buf_cat(a, "u");
        g->g_repl[t] = str_end(g, s);
        g->g_skip[t + 1] = 1;
        *type_name = component;
        *last = t + 1;
    }
}

/*
name[i] of a buffer becomes t1_load_T(name, (i) * t1_size_T), and a
statement that writes to it, like name[i].pos += v;, becomes
{ uint t1_a = (i) * t1_size_T; T t1_e = t1_load_T(name, t1_a);
t1_e.pos += v; t1_store_T(name, t1_a, t1_e); }
Only the member read or written is loaded (and stored): name[i].pos[1]
reads t1_load_float(name, (i) * t1_size_T + t1_off_T_m + (1) * ...).
Loading whole elements to use one member made FXC take a minute per
shader, throwing the rest away
*/
static void hlsl_buffer_edits(Gen * g, const b8 * loadable) {
    Check * c = g->c;

    for (u32 f = 0; f < c->funcs_size; f++) {
        const Func * fn = &c->funcs[f];
        for (u32 i = fn->body_open + 1; i < fn->body_close; i++) {
            if (
                kind(i) != SK_IDENT ||
                is_punct(i - 1, ".") ||
                !is_punct(i + 1, "["))
            {
                continue;
            }
            const PtrParam * ptr = NULL;
            for (u32 p = 0; p < fn->ptrs_size; p++) {
                if (same_text(fn->ptrs[p].name_i, i)) { ptr = &fn->ptrs[p]; }
            }
            if (ptr == NULL) { continue; }

            u32 close = after_match(c, i + 1, "[", "]") - 1;
            const u32 chain_end = chain_after(c, fn, close + 1);

            if (
                is_punct(i - 1, "++") || is_punct(i - 1, "--") ||
                is_punct(chain_end, "++") || is_punct(chain_end, "--"))
            {
                fail(g, i, "write buffer elements as name[i] op= value;");
                continue;
            }

            u32 s = str_begin(g);
            Buf * a = &g->arena;
            if (!is_assign_op(chain_end)) {
                const c8 * type_name = NULL;
                u32 last = close;
                hlsl_member_offsets(
                    g, loadable, close + 1, chain_end, ptr->type_i,
                    &type_name, &last);

                s = str_begin(g);
                buf_cat(a, "t1_load_");
                buf_cat(a, type_name);
                buf_cat(a, "(");
                cat_name(a, i);
                buf_cat(a, ", (");
                g->g_repl[i] = str_end(g, s);
                g->g_skip[i + 1] = 1;
                s = str_begin(g);
                buf_cat(a, ") * t1_size_");
                cat_hlsl_type(a, ptr->type_i);
                g->g_repl[close] = str_end(g, s);
                add_text(g, &g->g_post[last], ")");
                continue;
            }

            // a write: the statement must start here and end with a ';'
            const u32 semi = assign_statement_end(fn, chain_end);
            if (!is_punct(semi, ";") || !starts_statement(i)) {
                fail(g, i, "write buffer elements as name[i] op= value;");
                continue;
            }
            buf_cat(a, "{ uint t1_a = (");
            g->g_repl[i] = str_end(g, s);
            g->g_skip[i + 1] = 1;
            s = str_begin(g);
            buf_cat(a, ") * t1_size_");
            cat_hlsl_type(a, ptr->type_i);
            g->g_repl[close] = str_end(g, s);

            /*
            Only the member written to is loaded and stored, then the rest
            of the chain (e.g. a swizzle) works on the member
            */
            const c8 * type_name = NULL;
            u32 last = close; // the last token of the address
            hlsl_member_offsets(
                g, loadable, close + 1, chain_end, ptr->type_i,
                &type_name, &last);

            s = str_begin(g);
            buf_cat(a, "; ");
            buf_cat(a, type_name);
            if (last + 1 == chain_end && is_punct(chain_end, "=")) {
                // (all of it replaced: nothing to load)
                buf_cat(a, " t1_e");
            } else {
                buf_cat(a, " t1_e = t1_load_");
                buf_cat(a, type_name);
                buf_cat(a, "(");
                cat_name(a, i);
                buf_cat(a, ", t1_a); t1_e");
            }
            add_text(g, &g->g_post[last], str_end(g, s));
            s = str_begin(g);
            buf_cat(a, "; t1_store_");
            buf_cat(a, type_name);
            buf_cat(a, "(");
            cat_name(a, i);
            buf_cat(a, ", t1_a, t1_e); }");
            g->g_repl[semi] = str_end(g, s);
        }
    }
}

/*
float4(1.0f) fills every component in MSL, HLSL's constructors need them
all: ((float4)(1.0f)) does the same
*/
// text, then *slot after it (for edits that must come before older ones)
static void add_text_before(Gen * g, const c8 ** slot, const c8 * text) {
    if (*slot == NULL) { *slot = text; return; }
    u32 s = str_begin(g);
    buf_cat(&g->arena, text);
    buf_cat(&g->arena, *slot);
    *slot = str_end(g, s);
}

/*
FXC refuses a case that runs on into the next one, MSL allows it. A switch
with one keeps its selector in a local, and each run of cases that fall
into each other becomes one case with all their labels, where each part
but the last runs only if the selector isn't one of the later parts'
labels (where the old switch would have come in):
    switch (x) { case 0: a; case 1: b; break; default: c; }
->  { int t1_swN = (int)(x); switch (t1_swN) {
        case 0: case 1: if (!(t1_swN == (int)(1))) { a; } b; break;
        default: c; break; } }
The last case of every switch gets a break too, FXC wants one there.
Switches are done innermost first, so their edits nest: a pre is put
before the ones already there, a post after them.
*/
#define SWITCH_SEGS_CAP 64
typedef struct {
    u32 first_label; // 'case' or 'default'
    u32 body_start;  // after the last label's ':'
    u32 body_end;    // the next label or the switch's '}'
    b8 has_default;
    b8 falls;        // into the next one
} SwitchSeg;

// the body's last statement can't run on (break, return, ...)
static b8 switch_seg_ends(const Check * c, const SwitchSeg * s) {
    if (s->body_end <= s->body_start) { return 0; }
    if (!is_punct(s->body_end - 1, ";")) { return 0; }
    u32 depth = 0;
    u32 last_start = s->body_start;
    for (u32 t = s->body_start; t + 1 < s->body_end; t++) {
        if (
            is_bracket(c, t, "(") ||
            is_bracket(c, t, "[") ||
            is_bracket(c, t, "{"))
        {
            depth++;
        } else if (
            is_bracket(c, t, ")") ||
            is_bracket(c, t, "]") ||
            is_bracket(c, t, "}"))
        {
            depth--;
            if (depth == 0 && is_punct(t, "}")) { last_start = t + 1; }
        } else if (depth == 0 && is_punct(t, ";")) {
            last_start = t + 1;
        }
    }
    return
        is(last_start, "break") ||
        is(last_start, "return") ||
        is(last_start, "continue") ||
        is(last_start, "discard_fragment");
}

// sel op (int)(v), with join between them, for seg's case labels
static void cat_switch_labels(
    Buf * b,
    const SwitchSeg * seg,
    const c8 * sel,
    const c8 * op,
    const c8 * join,
    b8 * first)
{
    for (u32 t = seg->first_label; t < seg->body_start; t++) {
        if (!is(t, "case")) { continue; }
        u32 colon = t + 1;
        while (colon < seg->body_start && !is_punct(colon, ":")) {
            colon++;
        }
        if (!*first) { buf_cat(b, join); }
        *first = 0;
        buf_cat(b, sel);
        buf_cat(b, op);
        buf_cat(b, "(int)(");
        cat_tokens(b, t + 1, colon);
        buf_cat(b, ")");
        t = colon;
    }
}

static void hlsl_switch_edits(Gen * g) {
    Check * c = g->c;
    for (u32 sw = c->count; sw-- > 0;) {
        if (!is(sw, "switch") || !is_punct(sw + 1, "(")) { continue; }
        u32 sel_close = after_match(c, sw + 1, "(", ")") - 1;
        u32 open = sel_close + 1;
        if (!is_bracket(c, open, "{")) { continue; }
        u32 close = after_match(c, open, "{", "}") - 1;

        SwitchSeg segs[SWITCH_SEGS_CAP];
        u32 segs_size = 0;
        b8 plain = 1; // no #if inside, nothing before the first label
        u32 depth = 0;
        b8 in_labels = 0;
        for (u32 t = open + 1; t < close; t++) {
            if (c->secondary[t] || kind(t) == SK_PREPROC) { plain = 0; }
            b8 label =
                depth == 0 &&
                (is(t, "case") ||
                 (is(t, "default") && is_punct(t + 1, ":")));
            if (label) {
                if (!in_labels) {
                    if (segs_size == SWITCH_SEGS_CAP) { plain = 0; break; }
                    if (segs_size > 0) { segs[segs_size - 1].body_end = t; }
                    segs[segs_size].first_label = t;
                    segs[segs_size].has_default = 0;
                    segs_size++;
                }
                in_labels = 1;
                SwitchSeg * s = &segs[segs_size - 1];
                if (is(t, "default")) { s->has_default = 1; }
                while (t < close && !is_punct(t, ":")) { t++; }
                s->body_start = t + 1;
                s->body_end = close;
                continue;
            }
            if (segs_size == 0) { plain = 0; }
            in_labels = 0;
            if (
                is_bracket(c, t, "(") ||
                is_bracket(c, t, "[") ||
                is_bracket(c, t, "{"))
            {
                depth++;
            }
            if (
                is_bracket(c, t, ")") ||
                is_bracket(c, t, "]") ||
                is_bracket(c, t, "}"))
            {
                depth--;
            }
        }
        if (!plain || segs_size == 0) { continue; }

        b8 any_falls = 0;
        for (u32 k = 0; k < segs_size; k++) {
            segs[k].falls =
                k + 1 < segs_size && !switch_seg_ends(c, &segs[k]);
            if (segs[k].falls) { any_falls = 1; }
        }

        // the last case's break
        const SwitchSeg * last = &segs[segs_size - 1];
        if (last->body_end > last->body_start && !switch_seg_ends(c, last)) {
            add_text(g, &g->g_post[last->body_end - 1], " break;");
        }
        if (!any_falls) { continue; }

        u32 s = str_begin(g);
        buf_cat(&g->arena, "t1_sw");
        cat_u32(&g->arena, sw);
        const c8 * sel = str_end(g, s);

        s = str_begin(g);
        buf_cat(&g->arena, "{ int ");
        buf_cat(&g->arena, sel);
        buf_cat(&g->arena, " = (int)");
        add_text_before(g, &g->g_pre[sw], str_end(g, s));
        g->g_skip[sw] = 1;
        s = str_begin(g);
        buf_cat(&g->arena, "; switch (");
        buf_cat(&g->arena, sel);
        buf_cat(&g->arena, ")");
        add_text(g, &g->g_post[sel_close], str_end(g, s));
        add_text(g, &g->g_post[close], " }");

        for (u32 k = 0; k < segs_size; k++) {
            if (!segs[k].falls) { continue; }
            // a run of cases: k to end - 1 fall into the next one
            u32 end = k;
            while (segs[end].falls) { end++; }
            for (u32 p = k; p < end; p++) {
                // part p runs unless the switch came in later
                s = str_begin(g);
                buf_cat(&g->arena, "if (!(");
                b8 first = 1;
                for (u32 q = p + 1; q <= end; q++) {
                    cat_switch_labels(
                        &g->arena, &segs[q], sel, " == ", " || ", &first);
                    if (!segs[q].has_default) { continue; }
                    // default: none of the switch's labels
                    if (!first) { buf_cat(&g->arena, " || "); }
                    first = 0;
                    buf_cat(&g->arena, "(");
                    b8 first_d = 1;
                    for (u32 r = 0; r < segs_size; r++) {
                        cat_switch_labels(
                            &g->arena, &segs[r], sel, " != ", " && ",
                            &first_d);
                    }
                    if (first_d) { buf_cat(&g->arena, "true"); }
                    buf_cat(&g->arena, ")");
                }
                if (first) { buf_cat(&g->arena, "false"); }
                buf_cat(&g->arena, ")) { ");
                add_text_before(
                    g, &g->g_pre[segs[p].body_start], str_end(g, s));
                add_text(g, &g->g_post[segs[p].body_end - 1], " }");
            }
            // the later parts' labels join the first part's
            s = str_begin(g);
            for (u32 q = k + 1; q <= end; q++) {
                buf_cat(&g->arena, " ");
                cat_tokens(
                    &g->arena, segs[q].first_label, segs[q].body_start);
                for (u32 t = segs[q].first_label; t < segs[q].body_start; t++) {
                    g->g_skip[t] = 1;
                }
            }
            add_text(g, &g->g_post[segs[k].body_start - 1], str_end(g, s));
            k = end;
        }
    }
}

static void hlsl_constructor_edits(Gen * g) {
    static const c8 * const vectors[] = {
        "float2", "float3", "float4", "int2", "int3", "int4", "uint2",
        "uint3", "uint4",
    };
    Check * c = g->c;
    for (u32 i = 0; i + 1 < c->count; i++) {
        if (
            kind(i) != SK_IDENT ||
            !is_punct(i + 1, "(") ||
            is_punct(i - 1, ".") ||
            (!is_matrix_type(i) &&
                !in_list(i, vectors, sizeof(vectors) / sizeof(vectors[0]))))
        {
            continue;
        }
        u32 close = after_match(c, i + 1, "(", ")") - 1;
        if (close <= i + 2) { continue; }
        u32 nesting = 0;
        b8 one_arg = 1;
        for (u32 t = i + 2; t < close; t++) {
            if (is_punct(t, "(") || is_punct(t, "[")) { nesting++; }
            if (is_punct(t, ")") || is_punct(t, "]")) { nesting--; }
            if (nesting == 0 && is_punct(t, ",")) { one_arg = 0; }
        }
        if (!one_arg) { continue; }

        // float3x3(x) has x on its diagonal in MSL and GLSL
        if (is_matrix_type(i)) {
            u32 m = str_begin(g);
            buf_cat(&g->arena, "t1_mat_fill_");
            cat_name(&g->arena, i);
            g->g_repl[i] = str_end(g, m);
            continue;
        }

        u32 s = str_begin(g);
        buf_cat(&g->arena, "((");
        cat_name(&g->arena, i);
        buf_cat(&g->arena, ")");
        g->g_repl[i] = str_end(g, s);
        add_text(g, &g->g_post[close], ")");
    }
}

/*
The members of stage structs (what vertex functions return and fragment
functions get or return) get semantics: [[position]] is SV_Position,
[[color(n)]] SV_Targetn, the others TEXCOORDn in their order. Flat or
integer ones aren't interpolated.
*/
static void hlsl_stage_edits(Gen * g) {
    Check * c = g->c;
    u8 * done = T1_mem_malloc_managed(c->struct_defs_size + 1);
    T1_std_memset(done, 0, c->struct_defs_size + 1);

    for (u32 f = 0; f < c->funcs_size; f++) {
        const Func * e = &c->funcs[f];
        if (e->stage != STAGE_VERTEX && e->stage != STAGE_FRAGMENT) {
            continue;
        }

        u32 types[PARAMS_CAP + 1];
        u32 types_size = 0;
        types[types_size++] = e->name_i - 1;
        for (u32 p = 0; p < e->params_size; p++) {
            for (u32 t = e->params_start[p]; t < e->params_end[p]; t++) {
                if (kind(t) == SK_ATTR && attr_has(t, "stage_in")) {
                    types[types_size++] = t - 2;
                }
            }
        }

        for (u32 ti = 0; ti < types_size; ti++) {
            const StructDef * d = find_struct(c, types[ti]);
            if (d == NULL) { continue; }
            u32 s = (u32)(d - c->struct_defs);
            if (done[s]) { continue; }
            done[s] = 1;

            Member members[MEMBERS_CAP];
            u32 size = struct_members(g, d, members);
            u32 location = 0;
            for (u32 m = 0; m < size; m++) {
                u32 str = str_begin(g);
                Buf * a = &g->arena;
                if (attr_has(members[m].attr_i, "position")) {
                    buf_cat(a, " : SV_Position");
                } else if (attr_has(members[m].attr_i, "color")) {
                    buf_cat(a, " : SV_Target");
                    cat_attr_arg(a, members[m].attr_i, "color");
                } else {
                    buf_cat(a, " : TEXCOORD");
                    cat_u32(a, location);
                    location += locations_of(members[m].type_i);
                }
                const c8 * semantic = str_end(g, str);
                if (members[m].attr_i != UINT32_MAX) {
                    // global_edits() skipped every attribute
                    g->g_skip[members[m].attr_i] = 0;
                    g->g_repl[members[m].attr_i] = semantic;
                } else {
                    add_text(g, &g->g_post[members[m].name_i], semantic);
                }

                if (
                    !attr_has(members[m].attr_i, "position") &&
                    !attr_has(members[m].attr_i, "color") &&
                    (attr_has(members[m].attr_i, "flat") ||
                    is_integer_type(members[m].type_i)))
                {
                    str = str_begin(g);
                    buf_cat(a, "nointerpolation ");
                    cat_hlsl_type(a, members[m].type_i);
                    g->g_repl[members[m].type_i] = str_end(g, str);
                }
            }
        }
    }

    T1_mem_free_managed(done);
}

/*
Picking a texture array by a variable: shader model 5.0 can't index an
array of textures with one, so the array's index goes down a tree of
branches, half the arrays left at each (log2 of T1_TEXARRAYS_CAP compares
to any of them). A switch with a case per array did the same, but AMD's
drivers ran it far slower (a shadow map's pass: 4.5 ms of its 11)
*/
static void cat_hlsl_tex_tree(
    Buf * b,
    const u32 lo,
    const u32 hi, // past the last array
    const c8 * call, // what follows "r = t1_textures[i"
    const u32 depth)
{
    c8 indent[32];
    u32 n = 0;
    for (u32 d = 0; d < depth + 1 && n + 4 < sizeof(indent); d++) {
        for (u32 s = 0; s < 4; s++) { indent[n++] = ' '; }
    }
    indent[n] = '\0';
    if (hi - lo == 1) {
        buf_cat(b, indent);
        buf_cat(b, "r = t1_textures[");
        cat_u32(b, lo);
        buf_cat(b, call);
        return;
    }
    const u32 mid = (lo + hi) / 2;
    buf_cat(b, indent);
    buf_cat(b, "[branch] if (a < ");
    cat_u32(b, mid);
    buf_cat(b, "u) {\n");
    cat_hlsl_tex_tree(b, lo, mid, call, depth + 1);
    buf_cat(b, indent);
    buf_cat(b, "} else {\n");
    cat_hlsl_tex_tree(b, mid, hi, call, depth + 1);
    buf_cat(b, indent);
    buf_cat(b, "}\n");
}

/*
What a unit's stage needs before the shaders: sampling any T1Tex (see
cat_hlsl_tex_tree()), the write_tex helpers and buffers
*/
static void cat_hlsl_textures(Buf * b, const Stage stage, const b8 used) {
    if (!used) {
        // saves compiling the trees below
        buf_cat(b, "Texture2DArray t1_textures[1] : register(t0);\n");
        return;
    }
    buf_cat(b, "Texture2DArray t1_textures[");
    cat_u32(b, T1_TEXARRAYS_CAP);
    buf_cat(b, "] : register(t0);\n"
        "SamplerState t1_nearest_smp : register(s0);\n"
        "SamplerState t1_linear_smp : register(s1);\n"
        "SamplerState t1_linear_repeat_smp : register(s2);\n");

    // implicit mip levels only where they exist: fragments with mipmaps
    #if T1_MIPMAPS_ACTIVE == T1_ACTIVE
    b8 implicit_lod = stage == STAGE_FRAGMENT;
    #elif T1_MIPMAPS_ACTIVE == T1_INACTIVE
    b8 implicit_lod = 0;
    (void)stage;
    #else
    #error
    #endif
    /*
    The uv's derivatives are taken before the branches: tex may differ from
    pixel to pixel, and inside a branch that does the neighbouring pixels
    of a quad may not take the same one, so implicit derivatives there
    (Sample()) would pick wrong mips or make the compiler flatten it. An
    array past the last one gives 0s
    */
    buf_cat(b,
        "float4 t1_sample(SamplerState smp, uint tex, float2 uv) {\n"
        "    float3 c = float3(uv, (float)T1_TEX_SLICE_I(tex));\n");
    if (implicit_lod) {
        buf_cat(b,
            "    float2 dx = ddx(uv);\n"
            "    float2 dy = ddy(uv);\n");
    }
    buf_cat(b,
        "    float4 r = float4(0.0f, 0.0f, 0.0f, 0.0f);\n"
        "    uint a = T1_TEX_ARRAY_I(tex);\n"
        "    [branch] if (a < ");
    cat_u32(b, T1_TEXARRAYS_CAP);
    buf_cat(b, "u) {\n");
    cat_hlsl_tex_tree(b, 0, T1_TEXARRAYS_CAP, implicit_lod ?
        "].SampleGrad(smp, c, dx, dy);\n" :
        "].SampleLevel(smp, c, 0.0f);\n", 1);
    buf_cat(b,
        "    }\n"
        "    return r;\n"
        "}\n"
        "float4 t1_read(uint tex, uint2 pos) {\n"
        "    int4 c = int4(int2(pos), (int)T1_TEX_SLICE_I(tex), 0);\n"
        "    float4 r = float4(0.0f, 0.0f, 0.0f, 0.0f);\n"
        "    uint a = T1_TEX_ARRAY_I(tex);\n"
        "    [branch] if (a < ");
    cat_u32(b, T1_TEXARRAYS_CAP);
    buf_cat(b, "u) {\n");
    cat_hlsl_tex_tree(b, 0, T1_TEXARRAYS_CAP, "].Load(c);\n", 1);
    buf_cat(b,
        "    }\n"
        "    return r;\n"
        "}\n"
        // the T1 helpers, their first argument is the texture parameter
        "#define T1_sample_nearest(t, tex, uv) "
            "t1_sample(t1_nearest_smp, (tex), (uv))\n"
        "#define T1_sample_linear(t, tex, uv) "
            "t1_sample(t1_linear_smp, (tex), (uv))\n"
        "#define T1_sample_linear_repeat(t, tex, uv) "
            "t1_sample(t1_linear_repeat_smp, (tex), (uv))\n"
        "#define T1_read(t, tex, pos) t1_read((tex), (pos))\n");
}

static const c8 * const hlsl_write_tex =
    "RWTexture2DArray<uint> t1_write_tex : register(u0);\n"
    "float4 t1_unpack_rgba8(uint v) {\n"
    "    return float4(v & 0xFF, (v >> 8) & 0xFF, (v >> 16) & 0xFF, v >> 24)"
        " / 255.0f;\n"
    "}\n"
    "uint t1_pack_rgba8(float4 color) {\n"
    "    uint4 u = (uint4)(saturate(color) * 255.0f + 0.5f);\n"
    "    return u.x | (u.y << 8) | (u.z << 16) | (u.w << 24);\n"
    "}\n"
    "uint2 t1_write_tex_size() {\n"
    "    uint w; uint h; uint slices;\n"
    "    t1_write_tex.GetDimensions(w, h, slices);\n"
    "    return uint2(w, h);\n"
    "}\n"
    "#define T1_write_tex_read(t, pos) "
        "t1_unpack_rgba8(t1_write_tex[uint3((pos), 0)])\n"
    "#define T1_write_tex_write(t, pos, color) "
        "(t1_write_tex[uint3((pos), 0)] = t1_pack_rgba8(color))\n"
    "#define T1_write_tex_size(t) t1_write_tex_size()\n";

// The load and store functions of the types that aren't structs
static void cat_hlsl_prims(Buf * b) {
    for (u32 p = 0; p < sizeof(hlsl_prims) / sizeof(hlsl_prims[0]); p++) {
        const HlslPrim * prim = &hlsl_prims[p];
        buf_cat(b, "#define t1_size_");
        buf_cat(b, prim->type);
        buf_cat(b, " ");
        buf_cat(b, prim->size);
        buf_cat(b, "u\n#define t1_align_");
        buf_cat(b, prim->type);
        buf_cat(b, " ");
        buf_cat(b, prim->align);
        buf_cat(b, "u\n");
        for (u32 rw = 0; rw < 2; rw++) {
            buf_cat(b, prim->type);
            buf_cat(b, " t1_load_");
            buf_cat(b, prim->type);
            buf_cat(b, rw ?
                "(RWByteAddressBuffer b, uint a) { return " :
                "(ByteAddressBuffer b, uint a) { return ");
            buf_cat(b, prim->load);
            buf_cat(b, "; }\n");
        }
        buf_cat(b, "void t1_store_");
        buf_cat(b, prim->type);
        buf_cat(b, "(RWByteAddressBuffer b, uint a, ");
        buf_cat(b, prim->type);
        buf_cat(b, " v) { ");
        buf_cat(b, prim->store);
        buf_cat(b, "; }\n");
    }
}

static void make_unit_hlsl(Gen * g, const u32 entry_f) {
    Check * c = g->c;
    const Func * e = &c->funcs[entry_f];
    u32 count = c->count;
    unit_begin(g, entry_f);

    // The globals and helpers go before the shaders, after T1's headers
    Buf globals_buf;
    globals_buf.cap = 64 * 1024;
    globals_buf.data = T1_mem_malloc_managed(globals_buf.cap);
    globals_buf.data[0] = '\0';
    globals_buf.size = 0;
    globals_buf.overflow = 0;
    Buf * gl = &globals_buf;
    buf_cat(gl, "\n");

    // whether the functions this unit keeps read textures
    b8 reads_textures = 0;
    for (u32 f = 0; f < c->funcs_size; f++) {
        if (!g->reachable[f]) { continue; }
        const Func * fn = &c->funcs[f];
        for (u32 t = fn->body_open; t < fn->body_close; t++) {
            if (
                kind(t) == SK_IDENT &&
                T1_token_size(tz, t) >= 7 &&
                (T1_std_are_equal_strings_of_length(
                    T1_token_text(tz, t), "T1_sample", 9) ||
                is(t, "T1_read")))
            {
                reads_textures = 1;
            }
        }
    }
    // or a macro of theirs could
    for (u32 t = 0; t < count && !reads_textures; t++) {
        if (kind(t) != SK_PREPROC) { continue; }
        const c8 * text = T1_token_text(tz, t);
        u32 size = T1_token_size(tz, t);
        for (u32 j = 0; j + 7 <= size; j++) {
            if (
                (j + 9 <= size &&
                T1_std_are_equal_strings_of_length(text + j, "T1_sample", 9)) ||
                T1_std_are_equal_strings_of_length(text + j, "T1_read", 7))
            {
                reads_textures = 1;
            }
        }
    }
    cat_hlsl_textures(gl, e->stage, reads_textures);

    // lines for the backend, after the unit's marker line
    c8 header_data[1024];
    Buf header_buf = {header_data, 0, 1024, 0};
    header_data[0] = '\0';
    Buf * hd = &header_buf;

    c8 prologue_data[8192];
    Buf prologue_buf = {prologue_data, 0, 8192, 0};
    prologue_data[0] = '\0';
    Buf * pro = &prologue_buf;
    buf_cat(pro, "\n");

    // the head's parameters, which replace the entry point's
    c8 params_data[1024];
    Buf params_buf = {params_data, 0, 1024, 0};
    params_data[0] = '\0';
    Buf * hp = &params_buf;

    u32 threads[3] = {16, 16, 1};
    if (e->stage == STAGE_KERNEL) { kernel_group_size(e, threads); }
    if (e->stage == STAGE_KERNEL) {
        /*
        A grid of more than 65535 groups a dimension is dispatched in
        parts, t1_grid_start is the part's first thread (see
        T1_gpu_d3d11.c)
        */
        buf_cat(gl,
            "cbuffer T1Grid : register(b0) {\n"
            "    uint3 t1_grid_size;\n"
            "    uint t1_grid_pad;\n"
            "    uint3 t1_grid_start;\n"
            "};\n");
        buf_cat(hp,
            "uint3 t1_part_dtid : SV_DispatchThreadID, "
            "uint3 t1_gtid : SV_GroupThreadID, "
            "uint3 t1_part_gid : SV_GroupID");
        buf_cat(pro, "    uint3 t1_dtid = t1_part_dtid + t1_grid_start;\n");
        buf_cat(pro, "    uint3 t1_gid = t1_part_gid + t1_grid_start / uint3(");
        for (u32 d = 0; d < 3; d++) {
            cat_u32(pro, threads[d]);
            buf_cat(pro, d < 2 ? ", " : ");\n");
        }
        if (!kernel_has_barrier(e)) {
            buf_cat(pro, "    if (any(t1_dtid >= t1_grid_size)) { return; }\n");
        }
    } else if (e->stage == STAGE_VERTEX) {
        /*
        Metal's [[vertex_id]] counts from the draw's first vertex, D3D's
        SV_VertexID from 0 whatever the StartVertexLocation, so the draw
        passes its first vertex (see T1_gpu_d3d11.c)
        */
        buf_cat(gl,
            "cbuffer T1Draw : register(b0) {\n"
            "    uint t1_vertex_start;\n"
            "    uint3 t1_draw_pad;\n"
            "};\n");
        buf_cat(hp,
            "uint t1_part_vertex_id : SV_VertexID, "
            "uint t1_instance_id : SV_InstanceID");
        buf_cat(pro,
            "    uint t1_vertex_id = t1_part_vertex_id + t1_vertex_start;\n");
    }

    // a fragment's stage_in, and its member at [[position]] if any
    u32 stage_in_name = UINT32_MAX;
    u32 stage_in_position = UINT32_MAX;
    for (u32 p = 0; p < e->params_size; p++) {
        for (u32 t = e->params_start[p]; t < e->params_end[p]; t++) {
            if (kind(t) != SK_ATTR || !attr_has(t, "stage_in")) { continue; }
            stage_in_name = t - 1;
            const StructDef * d = find_struct(c, t - 2);
            if (d == NULL) {
                fail(g, t - 2, "stage_in needs a struct");
                continue;
            }
            Member members[MEMBERS_CAP];
            u32 size = struct_members(g, d, members);
            for (u32 m = 0; m < size; m++) {
                if (attr_has(members[m].attr_i, "position")) {
                    stage_in_position = members[m].name_i;
                }
            }
            if (hp->size > 0) { buf_cat(hp, ", "); }
            cat_tokens(hp, t - 2, t);
        }
    }

    u32 uav_slot = 1; // u0 is the write_tex
    b8 frag_coord = 0;

    for (u32 p = 0; p < e->params_size; p++) {
        u32 start = e->params_start[p];
        u32 end = e->params_end[p];
        u32 attr = UINT32_MAX;
        u32 star = UINT32_MAX;
        b8 readonly = 0;
        for (u32 t = start; t < end; t++) {
            if (kind(t) == SK_ATTR) { attr = t; }
            if (is_punct(t, "*")) { star = t; }
            if (is(t, "const") || is(t, "constant")) { readonly = 1; }
        }
        u32 name = (attr != UINT32_MAX ? attr : end) - 1;
        if (attr == UINT32_MAX || kind(name) != SK_IDENT) {
            fail(g, start, "entry point parameters need an attribute");
            continue;
        }

        if (attr_has(attr, "buffer")) {
            if (star == UINT32_MAX) {
                fail(g, start, "a buffer parameter must be a pointer");
                continue;
            }
            /*
            Only what a pass binds: T1_FRAME_CONSTS_I, T1_PARAMS_I and
            its buffers, writable ones only its buffers. Anything else
            would read zeros (or drop writes) on Direct3D without a word
            */
            s64 n = eval_attr_arg(c, attr, "buffer");
            b8 pass_buf = n >= T1_BUFS_I && n < T1_BUFS_I + T1_PASS_BUFS_CAP;
            if (n != T1_FRAME_CONSTS_I && n != T1_PARAMS_I && !pass_buf) {
                fail(g, attr,
                    "buffer(n) needs T1_FRAME_CONSTS_I, T1_PARAMS_I or a "
                    "constant from T1_BUFS_I to T1_BUFS_I + "
                    "T1_PASS_BUFS_CAP - 1");
                continue;
            }
            if (e->stage == STAGE_KERNEL && !readonly && !pass_buf) {
                fail(g, attr, "a kernel only writes its pass's buffers");
                continue;
            }
            if (e->stage == STAGE_KERNEL && !readonly) {
                if (uav_slot > 7) {
                    fail(g, attr, "at most 7 writable buffers per kernel");
                    continue;
                }
                buf_cat(gl, "RWByteAddressBuffer ");
                cat_name(gl, name);
                buf_cat(gl, " : register(u");
                cat_u32(gl, uav_slot);
                buf_cat(gl, ");\n");
                if (
                    is(star - 1, "uint") || is(star - 1, "u32") ||
                    is(star - 1, "uint32_t"))
                {
                    cat_hlsl_atomics(gl, name);
                }
                buf_cat(hd, "//T1_UAV ");
                cat_u32(hd, uav_slot);
                buf_cat(hd, " ");
                cat_u32(hd, (u32)n);
                buf_cat(hd, "\n");
                uav_slot++;
            } else {
                buf_cat(gl, "ByteAddressBuffer ");
                cat_name(gl, name);
                buf_cat(gl, " : register(t");
                cat_u32(gl, (u32)n + T1_HLSL_BUFS_T);
                buf_cat(gl, ");\n");
            }
        } else if (attr_has(attr, "texture")) {
            if (is(name - 1, "T1WriteTex")) {
                if (e->stage != STAGE_KERNEL) {
                    fail(g, start, "only kernels have a T1WriteTex");
                    continue;
                }
                buf_cat(gl, hlsl_write_tex);
            } else if (!is(name - 1, "T1Textures")) {
                fail(g, start, "textures are T1Textures or T1WriteTex");
            }
        } else if (
            attr_has(attr, "vertex_id") ||
            attr_has(attr, "instance_id") ||
            attr_has(attr, "thread_position_in_grid") ||
            attr_has(attr, "thread_position_in_threadgroup") ||
            attr_has(attr, "threadgroup_position_in_grid") ||
            (attr_has(attr, "position") && !attr_has(attr, "stage_in")))
        {
            u32 s = str_begin(g);
            Buf * a = &g->arena;
            if (attr_has(attr, "vertex_id")) {
                buf_cat(a, "t1_vertex_id");
            } else if (attr_has(attr, "instance_id")) {
                buf_cat(a, "t1_instance_id");
            } else if (attr_has(attr, "position")) {
                if (stage_in_position != UINT32_MAX) {
                    cat_name(a, stage_in_name);
                    buf_cat(a, ".");
                    cat_name(a, stage_in_position);
                } else {
                    buf_cat(a, "t1_frag_coord");
                    frag_coord = 1;
                }
            } else {
                buf_cat(a,
                    attr_has(attr, "thread_position_in_threadgroup") ?
                        "t1_gtid" :
                    attr_has(attr, "threadgroup_position_in_grid") ?
                        "t1_gid" : "t1_dtid");
                buf_cat(a,
                    is(name - 1, "uint3") ? "" :
                    is(name - 1, "uint2") ? ".xy" : ".x");
            }
            const c8 * value = str_end(g, s);
            buf_cat(pro, "    ");
            cat_tokens(pro, start, name + 1);
            buf_cat(pro, " = (");
            cat_hlsl_type(pro, name - 1);
            buf_cat(pro, ")(");
            buf_cat(pro, value);
            buf_cat(pro, ");\n");
        } else if (!attr_has(attr, "stage_in")) {
            fail(g, attr, "unsupported entry point parameter");
        }
    }
    if (frag_coord) {
        if (hp->size > 0) { buf_cat(hp, ", "); }
        buf_cat(hp, "float4 t1_frag_coord : SV_Position");
    }

    // the unit's marker line, its lines for the backend and the prelude
    static const c8 * const stage_names[] = {"", "vert", "frag", "comp"};
    buf_cat(g->out, "//T1_UNIT ");
    buf_cat(g->out, stage_names[e->stage]);
    buf_cat(g->out, " ");
    // as the head names it (e.g. "sample_"), the backend compiles that
    cat_name(g->out, e->name_i);
    buf_cat(g->out, "\n");
    if (e->stage == STAGE_KERNEL) {
        buf_cat(g->out, "//T1_THREADS ");
        for (u32 d = 0; d < 3; d++) {
            cat_u32(g->out, threads[d]);
            buf_cat(g->out, d < 2 ? " " : "\n");
        }
    }
    buf_cat(g->out, hd->data);
    buf_cat(g->out, hlsl_prelude);
    cat_hlsl_prims(g->out);

    // the entry point's head
    {
        u32 s = str_begin(g);
        Buf * a = &g->arena;
        if (e->stage == STAGE_KERNEL) {
            buf_cat(a, "[numthreads(");
            for (u32 d = 0; d < 3; d++) {
                cat_u32(a, threads[d]);
                buf_cat(a, d < 2 ? ", " : ")]\n");
            }
            buf_cat(a, "void ");
        } else {
            cat_hlsl_type(a, e->name_i - 1);
            buf_cat(a, " ");
        }
        cat_name(a, e->name_i);
        buf_cat(a, "(");
        buf_cat(a, hp->data);
        buf_cat(a, ")");
        if (e->stage == STAGE_FRAGMENT && find_struct(c, e->name_i - 1) == NULL) {
            buf_cat(a, " : SV_Target0");
        }
        g->u_repl[e->decl_start] = str_end(g, s);
        for (u32 t = e->decl_start + 1; t <= e->params_close; t++) {
            g->u_skip[t] = 1;
        }
    }

    if (pro->overflow || hp->overflow || hd->overflow) {
        fail(g, e->name_i, "too many parameters");
    }
    {
        u32 s = str_begin(g);
        buf_cat(&g->arena, pro->data);
        g->u_post[e->body_open] = str_end(g, s);
    }
    buf_cat(gl, "\n");
    if (gl->overflow) { fail(g, e->name_i, "too many globals"); }
    {
        // before the first of the client's tokens this unit keeps
        u32 first = g->user_first_token;
        while (first < count && g->u_drop[first]) { first++; }
        u32 s = str_begin(g);
        buf_cat(&g->arena, gl->data);
        if (first < count) {
            g->u_pre[first] = str_end(g, s);
        }
    }
    T1_mem_free_managed(gl->data);

    if (e->stage == STAGE_KERNEL) { threadgroup_decls(g, e, "groupshared "); }

    write_unit(g);
}

void T1_shader_to_hlsl(
    const c8 * shader_source,
    c8 * recip,
    const u32 recip_cap,
    u8 * good,
    c8 * error_recip,
    const u32 error_recip_cap)
{
    *good = 0;
    Gen g;
    Buf out;
    if (
        !gen_begin(
            &g, &out, shader_source, recip, recip_cap,
            error_recip, error_recip_cap, /* hlsl: */ 1))
    {
        return;
    }

    b8 * loadable = T1_mem_malloc_managed(g.c->struct_defs_size + 1);
    global_edits(&g);
    hlsl_switch_edits(&g);
    hlsl_struct_functions(&g, loadable);
    hlsl_vector_writes(&g);
    hlsl_local_zeros(&g);
    hlsl_const_tables(&g);
    hlsl_buffer_edits(&g, loadable);
    hlsl_constructor_edits(&g);
    hlsl_stage_edits(&g);
    for (u32 f = 0; f < g.c->funcs_size && !g.failed; f++) {
        if (g.c->funcs[f].is_entry) { make_unit_hlsl(&g, f); }
    }
    T1_mem_free_managed(loadable);

    gen_end(&g, good, "T1_shader_to_hlsl() ran out of room");
}

/*
THE PORTABILITY CHECK: what one of MSL, HLSL (FXC) or GLSL refuses, or runs
differently, and T1 can't translate, is outside the subset for all of them.
A Mac can't run FXC and Windows can't run Metal, so this is what keeps a
shader written on one working on the other.
*/

static u32 next_real(const Check * c, u32 t) {
    while (t < c->count && (kind(t) == SK_PREPROC || c->secondary[t])) { t++; }
    return t;
}

// Goes past the statement at *at, returns whether every path through it returns
static b8 statement_returns(const Check * c, u32 * at) {
    const u32 t = next_real(c, *at);
    if (t >= c->count) { *at = t; return 0; }
    if (is_bracket(c, t, "{")) {
        const u32 after = after_match(c, t, "{", "}");
        b8 returns = 0;
        u32 s = next_real(c, t + 1);
        while (s + 1 < after) {
            if (statement_returns(c, &s)) { returns = 1; }
            s = next_real(c, s);
        }
        *at = after;
        return returns;
    }
    if (is(t, "if") && is_punct(t + 1, "(")) {
        u32 s = after_match(c, t + 1, "(", ")");
        const b8 then_returns = statement_returns(c, &s);
        const u32 e = next_real(c, s);
        if (is(e, "else")) {
            s = e + 1;
            const b8 else_returns = statement_returns(c, &s);
            *at = s;
            return then_returns && else_returns;
        }
        *at = s;
        return 0;
    }
    /*
    A switch returns if it has a default, nothing breaks out of it (or out of
    a loop in it: that doesn't matter here, and is rare) and it ends in a return
    */
    if (is(t, "switch") && is_punct(t + 1, "(")) {
        const u32 open = next_real(c, after_match(c, t + 1, "(", ")"));
        if (is_bracket(c, open, "{")) {
            const u32 after = after_match(c, open, "{", "}");
            b8 has_default = 0;
            b8 breaks = 0;
            for (u32 s = open + 1; s + 1 < after; s++) {
                if (is(s, "default")) { has_default = 1; }
                if (is(s, "break")) { breaks = 1; }
            }
            // the last statement, after a ';', a brace or its label's ':'
            u32 last = after - 2;
            while (
                last > open + 1 &&
                !is_punct(last - 1, ";") && !is_punct(last - 1, ":") &&
                !is_punct(last - 1, "{") && !is_punct(last - 1, "}"))
            {
                last--;
            }
            *at = after;
            return has_default && !breaks && is(last, "return");
        }
    }
    if (
        (is(t, "for") || is(t, "while") || is(t, "switch")) &&
        is_punct(t + 1, "("))
    {
        u32 s = after_match(c, t + 1, "(", ")");
        statement_returns(c, &s);
        *at = s;
        return 0;
    }
    if (is(t, "do")) {
        u32 s = t + 1;
        statement_returns(c, &s); // then its while (...);
        statement_returns(c, &s);
        *at = s;
        return 0;
    }
    const b8 returns = is(t, "return");
    u32 s = t;
    while (s < c->count && (!is_punct(s, ";") || c->secondary[s])) { s++; }
    *at = s + 1;
    return returns;
}

static const Func * func_named(const Check * c, const u32 i) {
    for (u32 f = 0; f < c->funcs_size; f++) {
        if (same_text(c->funcs[f].name_i, i)) { return &c->funcs[f]; }
    }
    return NULL;
}

// Whether f calls target, through other functions or itself
static b8 calls_reach(
    const Check * c,
    const Func * f,
    const Func * target,
    u8 * visited)
{
    const u32 f_i = (u32)(f - c->funcs);
    if (visited[f_i]) { return 0; }
    visited[f_i] = 1;
    for (u32 t = f->body_open + 1; t < f->body_close; t++) {
        if (kind(t) != SK_IDENT || !is_punct(t + 1, "(")) { continue; }
        if (is_punct(t - 1, ".")) { continue; }
        const Func * callee = func_named(c, t);
        if (callee == NULL) { continue; }
        if (callee == target || calls_reach(c, callee, target, visited)) {
            return 1;
        }
    }
    return 0;
}

static void check_calls(Check * c, u8 * visited) {
    for (u32 f = 0; f < c->funcs_size; f++) {
        const Func * fn = &c->funcs[f];
        T1_std_memset(visited, 0, c->funcs_size);
        if (calls_reach(c, fn, fn, visited)) {
            report(c, fn->name_i, "no recursion");
        }

        // what only a fragment function may use, by itself or its helpers
        for (u32 t = fn->body_open + 1; t < fn->body_close; t++) {
            if (!is(t, "fwidth") && !is(t, "discard_fragment")) { continue; }
            for (u32 e = 0; e < c->funcs_size; e++) {
                const Func * entry = &c->funcs[e];
                if (!entry->is_entry || entry->stage == STAGE_FRAGMENT) {
                    continue;
                }
                T1_std_memset(visited, 0, c->funcs_size);
                if (entry == fn || calls_reach(c, entry, fn, visited)) {
                    report(c, t, "only fragment functions have this");
                }
            }
        }
    }
}

// The '{' around t, or the function's
static u32 enclosing_brace(const Check * c, const Func * f, const u32 t) {
    u32 depth = 0;
    for (u32 j = t; j > f->body_open; j--) {
        if (is_bracket(c, j, "}")) { depth++; }
        if (is_bracket(c, j, "{")) {
            if (depth == 0) { return j; }
            depth--;
        }
    }
    return f->body_open;
}

// The 'for' whose header t is in, or UINT32_MAX
static u32 for_of_header(const Check * c, const Func * f, const u32 t) {
    u32 depth = 0;
    for (u32 j = t; j > f->body_open; j--) {
        if (is_bracket(c, j, ")")) { depth++; }
        if (is_bracket(c, j, "(")) {
            if (depth == 0) {
                if (is(j - 1, "for")) { return j - 1; }
            } else {
                depth--;
            }
        }
        if (is_bracket(c, j, "{") || is_bracket(c, j, "}")) { break; }
    }
    return UINT32_MAX;
}

static u32 for_end(const Check * c, const u32 for_i) {
    u32 s = after_match(c, for_i + 1, "(", ")");
    statement_returns(c, &s);
    return s;
}

/*
FXC's for variables outlive their loop, and one that has another's name takes
over from it: a for's variable can't share a name with one around it
*/
static void check_for_names(Check * c, const Func * f) {
    for (u32 t = f->body_open + 1; t < f->body_close; t++) {
        if (!is(t, "for") || !is_punct(t + 1, "(") || c->secondary[t]) {
            continue;
        }
        const u32 end = for_end(c, t);
        for (u32 n = t + 2; n < f->body_close && !is_punct(n, ";"); n++) {
            if (!declares(c, n)) { continue; }
            for (u32 d = f->params_open + 1; d < f->body_close; d++) {
                if (
                    d == n ||
                    !same_text(d, n) ||
                    !declares(c, d) ||
                    c->secondary[d])
                {
                    continue;
                }
                b8 clash;
                if (d < f->params_close) {
                    clash = 1;
                } else {
                    const u32 other_for = for_of_header(c, f, d);
                    const b8 inside = d > t && d < end;
                    if (other_for != UINT32_MAX) {
                        clash =
                            inside ||
                            (t > other_for && t < for_end(c, other_for));
                    } else {
                        const u32 b = enclosing_brace(c, f, d);
                        clash =
                            !inside &&
                            t > b && t < after_match(c, b, "{", "}");
                    }
                }
                if (clash) {
                    report(c, n,
                        "a for's variable can't have the name of one around "
                        "it (FXC keeps it after the loop)");
                    break;
                }
            }
        }
    }
}

static b8 name_tainted(
    const u32 from,
    const u32 to,
    const Name * names,
    const u32 size)
{
    for (u32 t = from; t < to; t++) {
        if (
            kind(t) == SK_IDENT &&
            !is_punct(t - 1, ".") &&
            in_names(t, names, size))
        {
            return 1;
        }
    }
    return 0;
}

static b8 text_has(const c8 * text, const c8 * part) {
    const u64 n = T1_std_strlen(part);
    for (; *text != '\0'; text++) {
        if (T1_std_are_equal_strings_of_length(text, part, n)) { return 1; }
    }
    return 0;
}

#define THREADGROUP_DECLS_CAP 16

static u32 shared_index(const u32 t, const Name * shared, const u32 size) {
    for (u32 n = 0; n < size && n < THREADGROUP_DECLS_CAP; n++) {
        if (equal_names(shared[n], t)) { return n; }
    }
    return THREADGROUP_DECLS_CAP - 1;
}

/*
A write to threadgroup memory that every thread makes to the same place is a
race in MSL and GLSL, and an error in FXC (X3695) where it can tell: a loop
over every slot gets past FXC, and is the same race. Its index must depend on
the thread's position (found by following the kernel's assignments), or it's
inside an if that does.
*/
static void check_threadgroup_writes(
    Check * c,
    const Func * f,
    Name * shared,
    Name * moving)
{
    u32 shared_size = 0;
    for (u32 t = f->body_open + 1; t < f->body_close; t++) {
        if (is(t, "threadgroup")) {
            add_name(
                shared, &shared_size,
                T1_token_text(tz, t + 2), T1_token_size(tz, t + 2));
        }
    }
    if (shared_size == 0) { return; }
    b8 written[THREADGROUP_DECLS_CAP] = {0};

    u32 moving_size = 0;
    for (u32 t = f->params_open + 1; t < f->params_close; t++) {
        if (
            kind(t) == SK_ATTR &&
            text_has(T1_token_value(tz, t), "thread_position_in"))
        {
            add_name(
                moving, &moving_size,
                T1_token_text(tz, t - 1), T1_token_size(tz, t - 1));
        }
    }
    u32 before;
    do {
        before = moving_size;
        for (u32 t = f->body_open + 1; t < f->body_close; t++) {
            if (!is_assign_op(t) || is_punct(t, "++") || is_punct(t, "--")) {
                continue;
            }
            // the variable it's written to: where the chain starts
            u32 root = t - 1;
            while (root > f->body_open) {
                if (is_punct(root, "]")) {
                    root = before_match(c, root);
                    root -= 1;
                } else if (is_punct(root - 1, ".")) {
                    root -= 2;
                } else {
                    break;
                }
            }
            if (kind(root) != SK_IDENT || in_names(root, moving, moving_size)) {
                continue;
            }
            u32 end = t + 1;
            while (end < f->body_close && !is_punct(end, ";")) { end++; }
            if (name_tainted(t + 1, end, moving, moving_size)) {
                add_name(
                    moving, &moving_size,
                    T1_token_text(tz, root), T1_token_size(tz, root));
            }
        }
    } while (moving_size != before);

    for (u32 t = f->body_open + 1; t < f->body_close; t++) {
        if (
            kind(t) != SK_IDENT ||
            is_punct(t - 1, ".") ||
            !is_punct(t + 1, "[") ||
            !in_names(t, shared, shared_size))
        {
            continue;
        }
        const u32 close = after_match(c, t + 1, "[", "]") - 1;
        const u32 op = chain_after(c, f, close + 1);
        if (!is_assign_op(op)) { continue; }
        written[shared_index(t, shared, shared_size)] = 1;
        b8 moves = name_tainted(t + 2, close, moving, moving_size);
        for (u32 u = f->body_open + 1; u < t && !moves; u++) {
            if (!is(u, "if") || !is_punct(u + 1, "(")) { continue; }
            const u32 cond_end = after_match(c, u + 1, "(", ")");
            u32 s = cond_end;
            statement_returns(c, &s);
            const u32 e = next_real(c, s);
            if (is(e, "else")) {
                s = e + 1;
                statement_returns(c, &s);
            }
            if (t < s && name_tainted(u + 2, cond_end, moving, moving_size)) {
                moves = 1;
            }
        }
        if (!moves) {
            report(c, t,
                "every thread writes the same threadgroup memory: index it "
                "by the thread's position, or write in an if on it");
        }
    }

    // (undefined in MSL and GLSL, an error in FXC: X4575)
    for (u32 t = f->body_open + 1; t < f->body_close; t++) {
        if (
            is(t, "threadgroup") &&
            !written[shared_index(t + 2, shared, shared_size)])
        {
            report(c, t + 2, "nothing writes this threadgroup memory");
        }
    }
}

// Whether what's from from to to changes anything but its own value
static b8 has_side_effects(const Check * c, const u32 from, const u32 to) {
    for (u32 t = from; t < to; t++) {
        if (is_assign_op(t) || is_atomic_call(t) || is(t, "T1_write_tex_write")) {
            return 1;
        }
        if (kind(t) != SK_IDENT || !is_punct(t + 1, "(")) { continue; }
        const Func * callee = func_named(c, t);
        if (callee == NULL) { continue; }
        for (u32 p = 0; p < callee->ptrs_size; p++) {
            const u32 type_i = callee->ptrs[p].type_i;
            if (is(type_i - 1, "device") && !is(type_i - 2, "const")) {
                return 1;
            }
        }
        for (u32 a = callee->params_open; a < callee->params_close; a++) {
            if (is(a, "T1WriteTex")) { return 1; }
        }
    }
    return 0;
}

// The end of the operand starting at from: at a stop of its bracket level
static u32 operand_end(const Check * c, const Func * f, const u32 from) {
    u32 t = from;
    while (t < f->body_close) {
        if (is_punct(t, "(")) { t = after_match(c, t, "(", ")"); continue; }
        if (is_punct(t, "[")) { t = after_match(c, t, "[", "]"); continue; }
        if (
            is_punct(t, ")") || is_punct(t, "]") || is_punct(t, ";") ||
            is_punct(t, ",") || is_punct(t, "}") || is_punct(t, ":") ||
            is_punct(t, "?"))
        {
            break;
        }
        t++;
    }
    return t;
}

// The ':' of the '?' at q
static u32 ternary_colon(const Check * c, const Func * f, const u32 q) {
    u32 t = q + 1;
    u32 inner = 0;
    while (t < f->body_close) {
        if (is_punct(t, "(")) { t = after_match(c, t, "(", ")"); continue; }
        if (is_punct(t, "[")) { t = after_match(c, t, "[", "]"); continue; }
        if (is_punct(t, "?")) { inner++; }
        if (is_punct(t, ":")) {
            if (inner == 0) { return t; }
            inner--;
        }
        if (is_punct(t, ";")) { break; }
        t++;
    }
    return t;
}

// The type of from..to, none if the parser doesn't follow it
static VType vparse_range(
    const Check * c,
    VParse * p,
    const u32 from,
    const u32 to)
{
    T1_std_memset(p, 0, sizeof(VParse));
    p->c = c;
    p->i = from;
    p->end = to;
    const VType t = vparse_expr(p, 1);
    return p->bad || p->i != to ? vtype_none() : t;
}

// The first token of the expression around i (up to lower precedence)
static u32 expression_first(const Check * c, const Func * f, const u32 i) {
    u32 first = i;
    while (first > f->body_open + 1) {
        const u32 b = first - 1;
        if (is_punct(b, ")") || is_punct(b, "]")) {
            first = before_match(c, b);
            continue;
        }
        if (is_punct(b, "(") || is_punct(b, "[") || vparse_boundary(b)) {
            break;
        }
        first = b;
    }
    return first;
}

static u32 expression_last(const Check * c, const Func * f, const u32 i) {
    u32 end = i + 1;
    while (end < f->body_close) {
        if (is_punct(end, "(")) { end = after_match(c, end, "(", ")"); continue; }
        if (is_punct(end, "[")) { end = after_match(c, end, "[", "]"); continue; }
        if (is_punct(end, ")") || is_punct(end, "]") || vparse_boundary(end)) {
            break;
        }
        end++;
    }
    return end;
}

// One number that isn't a bool: GLSL won't take it as a condition
static b8 is_number(const VType t) {
    return
        !t.array && t.width == 1 &&
        (t.base == VB_FLOAT || t.base == VB_INT || t.base == VB_UINT);
}

// .xx: as a place to write to, one component twice
static b8 swizzle_repeats(const u32 i) {
    const c8 * n = T1_token_text(tz, i);
    const u32 size = T1_token_size(tz, i);
    for (u32 a = 0; a < size; a++) {
        for (u32 b = a + 1; b < size; b++) {
            if (n[a] == n[b]) { return 1; }
        }
    }
    return 0;
}

static void check_expressions(Check * c, const Func * f, VParse * p) {
    for (u32 t = f->body_open + 1; t < f->body_close; t++) {
        if (c->secondary[t]) { continue; }

        // HLSL runs both sides of these, MSL and GLSL only what they need
        if (is_punct(t, "&&") || is_punct(t, "||")) {
            if (has_side_effects(c, t + 1, operand_end(c, f, t + 1))) {
                report(c, t,
                    "HLSL runs both sides of && and ||: no ++, = or writing "
                    "calls on the right of one");
            }
        }
        if (is_punct(t, "?")) {
            const u32 colon = ternary_colon(c, f, t);
            const u32 end = operand_end(c, f, colon + 1);
            if (has_side_effects(c, t + 1, end)) {
                report(c, t,
                    "HLSL runs both sides of ?: : no ++, = or writing calls "
                    "in them");
            }
            const VType arm = vparse_range(c, p, t + 1, colon);
            if (arm.base == VB_STRUCT || arm.array) {
                report(c, t, "?: between numbers and vectors only, not structs");
            }
            const u32 first = expression_first(c, f, t);
            const VType cond = vparse_range(c, p, first, t);
            if (is_vector(cond)) {
                report(c, t, "?: takes one bool, use any() or all()");
            } else if (is_number(cond)) {
                report(c, t, "?: takes a bool, compare the number");
            }
        }

        if ((is(t, "if") || is(t, "while")) && is_punct(t + 1, "(")) {
            const u32 close = after_match(c, t + 1, "(", ")") - 1;
            const VType cond = vparse_range(c, p, t + 2, close);
            if (is_vector(cond)) {
                report(c, t + 1, "a condition is one bool, use any() or all()");
            } else if (is_number(cond)) {
                report(c, t + 1, "a condition is a bool, compare the number");
            }
        }

        if (
            is_punct(t, "%") || is_punct(t, "!") || is_punct(t, "+") ||
            is_punct(t, "-") || is_punct(t, "*") || is_punct(t, "/"))
        {
            vparse_range(
                c, p, expression_first(c, f, t), expression_last(c, f, t));
            if (p->float_mod != 0) {
                report(c, t, "% is for integers, use fmod() on floats");
            }
            if (p->not_number != 0) {
                report(c, t, "! takes a bool, compare the number");
            }
            if (p->bool_math != 0) {
                report(c, t, "no math on bools, convert them first");
            }
        }
        if (is_punct(t, ".") && kind(t + 1) == SK_IDENT) {
            vparse_range(
                c, p, expression_first(c, f, t), expression_last(c, f, t));
            if (p->scalar_swizzle != 0) {
                report(c, t, "no swizzles on a single number");
            }
        }

        // what's assigned must be what it's assigned to, as MSL has it (FXC
        // cuts vectors short with a warning, and converts them)
        if (is_punct(t, "=") && !is_punct(t + 1, "{")) {
            const b8 declared = declares(c, t - 1);
            const u32 first = expression_first(c, f, t);
            const VType left =
                declared ?
                    vtype_declared_at(c, t - 1) :
                    vparse_range(c, p, first, t);
            u32 end = t + 1;
            while (end < f->body_close) {
                if (is_punct(end, "(")) {
                    end = after_match(c, end, "(", ")");
                } else if (is_punct(end, "[")) {
                    end = after_match(c, end, "[", "]");
                } else if (
                    is_punct(end, ";") || is_punct(end, ",") ||
                    is_punct(end, ")") || is_punct(end, "]"))
                {
                    break;
                } else {
                    end++;
                }
            }
            const VType right = vparse_range(c, p, t + 1, end);
            if (left.array && !declared) {
                report(c, t, "arrays can't be assigned, copy their elements");
            } else if (
                (is_vector(left) || left.base == VB_MATRIX) &&
                (is_vector(right) || right.base == VB_MATRIX) &&
                (left.base != right.base || left.width != right.width))
            {
                report(c, t,
                    "a vector or matrix takes only its own type, convert it "
                    "with a constructor");
            } else if (is_scalar(left) && is_vector(right)) {
                report(c, t, "a vector doesn't fit in a single number");
            } else if (
                !left.array && left.width == 1 &&
                (left.base == VB_INT || left.base == VB_UINT) &&
                is_number(right) && right.base == VB_FLOAT)
            {
                report(c, t, "a float doesn't become an integer unasked, cast it");
            }
            if (
                is_punct(t - 2, ".") &&
                swizzle_size(t - 1) > 0 &&
                swizzle_repeats(t - 1))
            {
                report(c, t, "a swizzle written to can't repeat a component");
            }
        }
    }
}

// = { ... }: HLSL and GLSL want every element, MSL fills in the rest with 0
static void check_initializers(Check * c) {
    for (u32 t = 0; t + 1 < c->count; t++) {
        if (!is_punct(t, "=") || !is_punct(t + 1, "{") || c->secondary[t]) {
            continue;
        }
        const u32 close = after_match(c, t + 1, "{", "}") - 1;
        u32 elements = close > t + 2 ? 1 : 0;
        u32 nesting = 0;
        for (u32 e = t + 2; e < close; e++) {
            if (
                is_bracket(c, e, "(") || is_bracket(c, e, "[") ||
                is_bracket(c, e, "{"))
            {
                nesting++;
            }
            if (
                is_bracket(c, e, ")") || is_bracket(c, e, "]") ||
                is_bracket(c, e, "}"))
            {
                nesting--;
            }
            if (nesting == 0 && is_punct(e, ",") && e + 1 < close) {
                elements++;
            }
        }
        b8 short_of = elements == 0;
        if (is_punct(t - 1, "]")) {
            const u32 open = before_match(c, t - 1);
            if (open + 1 == t - 1) {
                // float a[] = {...}: the size is what's there
            } else if (kind(open + 1) == SK_NUMBER && open + 2 == t - 1) {
                u32 size = 0;
                const c8 * n = T1_token_text(tz, open + 1);
                for (u32 d = 0; d < T1_token_size(tz, open + 1); d++) {
                    if (n[d] >= '0' && n[d] <= '9') {
                        size = size * 10 + (u32)(n[d] - '0');
                    }
                }
                if (elements < size) { short_of = 1; }
            } else if (elements == 1) {
                short_of = 1; // float a[N] = {0};
            }
        } else if (declares(c, t - 1)) {
            const StructDef * d = find_struct(c, t - 2);
            if (d != NULL) {
                DataMember members[DATA_MEMBERS_CAP];
                const u32 size = data_members(c, d, members);
                b8 arrays = 0;
                for (u32 m = 0; m < size; m++) {
                    if (members[m].count_open != UINT32_MAX) { arrays = 1; }
                }
                if (!arrays && elements < size) { short_of = 1; }
            }
        }
        if (short_of) {
            report(c, t,
                "an initializer lists every element (HLSL and GLSL don't "
                "fill in the rest)");
        }
    }
}

static void check_portable(Check * c) {
    u8 * visited = T1_mem_malloc_managed(c->funcs_size + 1);
    Name * names = T1_mem_malloc_managed(sizeof(Name) * NAMES_CAP * 2);
    VParse * p = T1_mem_malloc_managed(sizeof(VParse));

    check_calls(c, visited);
    check_initializers(c);
    for (u32 f = 0; f < c->funcs_size; f++) {
        const Func * fn = &c->funcs[f];

        for (u32 t = fn->params_open + 1; t < fn->params_close; t++) {
            if (is_punct(t, "[")) {
                report(c, t,
                    "no arrays as parameters (MSL shares them with the "
                    "caller, HLSL and GLSL copy them)");
            }
        }
        for (u32 t = fn->body_open + 1; t < fn->body_close; t++) {
            if (is(t, "struct")) {
                report(c, t, "structs at program scope only");
            }
        }
        if (!is(fn->name_i - 1, "void")) {
            u32 s = fn->body_open;
            if (!statement_returns(c, &s)) {
                report(c, fn->name_i,
                    "every path must return a value, FXC refuses it (X3507)");
            }
        }
        check_for_names(c, fn);
        if (fn->stage == STAGE_KERNEL) {
            check_threadgroup_writes(c, fn, names, names + NAMES_CAP);
        }
        check_expressions(c, fn, p);
    }
    T1_mem_free_managed(p);
    T1_mem_free_managed(names);
    T1_mem_free_managed(visited);
}

/*
THE CHECK: the subset's rules, then every translation, so a shader that
passes on one platform translates on all of them (some rules only show
while translating, e.g. which buffers a kernel may write)
*/
void T1_shader_check(
    const c8 * shader_source,
    u8 * good,
    c8 * error_recip,
    const u32 error_recip_cap)
{
    check_subset(shader_source, good, error_recip, error_recip_cap);
    if (!*good) { return; }

    const u32 cap =
        4 * 1024 * 1024 + (u32)T1_std_strlen(shader_source) * 64;
    c8 * translated = T1_mem_malloc_managed(cap);
    T1_shader_to_hlsl(
        shader_source, translated, cap, good, error_recip, error_recip_cap);
    if (*good) {
        T1_shader_to_glsl(
            shader_source, translated, cap, good, error_recip,
            error_recip_cap);
    }
    T1_mem_free_managed(translated);
}
