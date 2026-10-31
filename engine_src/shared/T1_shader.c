#include "T1_shader.h"

#include "T1_assert.h"
#include "T1_macro_settings.h"
#include "T1_mem.h"
#include "T1_std.h"
#include "T1_token.h"

/*
The subset, beyond plain C expressions and statements:
- preprocessor lines (#define, #if..., #error, #pragma pack), no #include
- typedef struct {...} Name; and struct Name {...};, no other typedefs
- 32-bit numbers only: float(N), int(N), uint(N), bool, u32, s32, f32,
  matrices floatNxN, no half, double or 8/16-bit types
- pointers only as buffer parameters, "const device Type * name" or
  "constant Type * name", used as name[i] or passed on whole to a helper,
  whose parameter has the same name and type as an entry point's buffer
- no &addresses, ->, methods, templates (but as_type<T>(x)), samplers or
  texture types: T1Textures and T1WriteTex through the T1_ helpers
- matrices multiply with mul(v, m), not *
- calls only to your own functions, macros, constructors and the builtins
  below, which T1 knows how to translate
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
    "void", "bool", "float", "float2", "float3", "float4",
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
};

// keywords that may stand before '(' without being a call
static const c8 * const paren_keywords[] = {
    "if", "for", "while", "switch", "return",
};

static const c8 * const attributes[] = {
    "vertex_id", "instance_id", "position", "stage_in", "buffer",
    "texture", "color", "thread_position_in_grid", "flat", "maybe_unused",
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
    {"threadgroup", "no threadgroup memory"},
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
            if ((is(i, "device") || is(i, "constant")) && !in_params) {
                report(c, i,
                    "address spaces only for buffer parameters");
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

void T1_shader_check(
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

The layout a Vulkan backend must match:
- set 0, binding n: buffer(n), a std430 storage buffer
- set 1, binding 0: T1Textures, texture2DArray[T1_TEXARRAYS_CAP]
- set 1, bindings 1 to 3: the samplers T1_nearest, T1_linear and
  T1_linear_repeat
- set 1, binding 4: a compute pass's write_tex, an rgba8 image2D
- kernels: local size 8 x 8 x 1, and a push constant uvec3 with the grid's
  size, threads past it return right away
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
    "#define T1_write_tex_size(t) uvec2(imageSize(t1_write_tex))\n";

static const c8 * const glsl_textures =
    "layout(set = 1, binding = 0) uniform texture2DArray "
        "t1_textures[T1_TEXARRAYS_CAP];\n"
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
    const c8 ** g_post;
    // for the unit being made
    u8 * u_skip;
    u8 * u_drop; // skipped with the text before it
    const c8 ** u_repl;
    const c8 ** u_pre;
    const c8 ** u_post;
    u8 * reachable; // per function
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
    return
        is_punct(i, "(") &&
        is_type(c, i + 1) &&
        is_punct(i + 2, ")") &&
        !(i > 0 && ends_operand(i - 1)) &&
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

static void make_unit(Gen * g, const u32 entry_f) {
    Check * c = g->c;
    const Func * e = &c->funcs[entry_f];
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
        buf_cat(gl,
            "layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) "
            "in;\n"
            "layout(push_constant) uniform T1Grid { uvec3 t1_grid_size; };\n");
        buf_cat(pro,
            "    if (any(greaterThanEqual(gl_GlobalInvocationID, "
            "t1_grid_size))) { return; }\n");
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
            (attr_has(attr, "position") && !attr_has(attr, "stage_in")))
        {
            const c8 * value =
                attr_has(attr, "vertex_id") ? "gl_VertexIndex" :
                attr_has(attr, "instance_id") ? "gl_InstanceIndex" :
                attr_has(attr, "position") ? "gl_FragCoord" :
                is(name - 1, "uint3") ? "gl_GlobalInvocationID" :
                is(name - 1, "uint2") ? "gl_GlobalInvocationID.xy" :
                "gl_GlobalInvocationID.x";
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

    write_unit(g);
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
    T1_assert(recip != NULL && recip_cap > 0);
    recip[0] = '\0';

    // the shaders must be in the subset, T1's own headers aren't checked
    T1_shader_check(shader_source, good, error_recip, error_recip_cap);
    if (!*good) { return; }
    *good = 0;

    u32 full_cap = (u32)(
        T1_std_strlen(T1_embedded_data_shader_settings) +
        T1_std_strlen(T1_embedded_data_shader_includes) +
        T1_std_strlen(shader_source) + 1);
    c8 * full = T1_mem_malloc_managed(full_cap);
    full[0] = '\0';
    T1_std_strcat_cap(full, full_cap, T1_embedded_data_shader_settings);
    T1_std_strcat_cap(full, full_cap, T1_embedded_data_shader_includes);
    T1_std_strcat_cap(full, full_cap, shader_source);

    T1_token_run(tz, full, T1_std_strlen(full));

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

    Gen g;
    T1_std_memset(&g, 0, sizeof(Gen));
    g.c = c;
    g.arena.cap = 4 * 1024 * 1024;
    g.arena.data = T1_mem_malloc_managed(g.arena.cap);
    g.arena.data[0] = '\0';
    Buf out;
    out.data = recip;
    out.size = 0;
    out.cap = recip_cap;
    out.overflow = 0;
    g.out = &out;
    u32 n = c->count + 1;
    g.g_skip = T1_mem_malloc_managed(n);
    g.u_skip = T1_mem_malloc_managed(n);
    g.u_drop = T1_mem_malloc_managed(n);
    g.g_repl = T1_mem_malloc_managed(sizeof(c8 *) * n);
    g.g_post = T1_mem_malloc_managed(sizeof(c8 *) * n);
    g.u_repl = T1_mem_malloc_managed(sizeof(c8 *) * n);
    g.u_pre = T1_mem_malloc_managed(sizeof(c8 *) * n);
    g.u_post = T1_mem_malloc_managed(sizeof(c8 *) * n);
    g.reachable = T1_mem_malloc_managed(c->funcs_size + 1);
    T1_std_memset(g.g_skip, 0, n);
    for (u32 i = 0; i < n; i++) {
        g.g_repl[i] = NULL;
        g.g_post[i] = NULL;
    }

    global_edits(&g);
    for (u32 f = 0; f < c->funcs_size && !g.failed; f++) {
        if (c->funcs[f].is_entry) { make_unit(&g, f); }
    }

    if (g.arena.overflow || out.overflow) {
        fail(&g, 0, "T1_shader_to_glsl() ran out of room");
    }
    *good = !g.failed;

    T1_mem_free_managed(g.arena.data);
    T1_mem_free_managed(g.g_skip);
    T1_mem_free_managed(g.u_skip);
    T1_mem_free_managed(g.u_drop);
    T1_mem_free_managed((void *)g.g_repl);
    T1_mem_free_managed((void *)g.g_post);
    T1_mem_free_managed((void *)g.u_repl);
    T1_mem_free_managed((void *)g.u_pre);
    T1_mem_free_managed((void *)g.u_post);
    T1_mem_free_managed(g.reachable);
    T1_mem_free_managed(c->secondary);
    T1_mem_free_managed(c);
    T1_mem_free_managed(full);
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
  constant buffer with the grid's size (a uint3), threads past it return
  right away
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
    "ldexp", "frexp", "determinant", "fwidth", "numthreads",
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
    // see the matrices above
    "#define mul(a, b) mul(b, a)\n"
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

/*
A struct's members for its load and store functions: [const] Type name;
also "Type a, b[N];". 0 if it has something else (attributes, #ifs)
*/
static u32 data_members(const StructDef * d, DataMember * recip) {
    u32 size = 0;
    u32 i = d->body_open + 1;
    while (i < d->body_close) {
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
    return size;
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
        u32 size = data_members(d, members);
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
                HLSL. Unrolled: elements of local arrays can't be indexed
                with variables
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
name[i] of a buffer becomes t1_load_T(name, (i) * t1_size_T), and a
statement that writes to it, like name[i].pos += v;, becomes
{ uint t1_a = (i) * t1_size_T; T t1_e = t1_load_T(name, t1_a);
t1_e.pos += v; t1_store_T(name, t1_a, t1_e); }
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
            u32 chain_end = close + 1;
            while (chain_end < fn->body_close) {
                if (is_punct(chain_end, ".") && kind(chain_end + 1) == SK_IDENT) {
                    chain_end += 2;
                } else if (is_punct(chain_end, "[")) {
                    chain_end = after_match(c, chain_end, "[", "]");
                } else {
                    break;
                }
            }

            if (is_punct(i - 1, "++") || is_punct(i - 1, "--")) {
                fail(g, i, "write buffer elements as name[i] op= value;");
                continue;
            }

            u32 s = str_begin(g);
            Buf * a = &g->arena;
            if (!is_assign_op(chain_end)) {
                buf_cat(a, "t1_load_");
                cat_hlsl_type(a, ptr->type_i);
                buf_cat(a, "(");
                cat_name(a, i);
                buf_cat(a, ", (");
                g->g_repl[i] = str_end(g, s);
                g->g_skip[i + 1] = 1;
                s = str_begin(g);
                buf_cat(a, ") * t1_size_");
                cat_hlsl_type(a, ptr->type_i);
                buf_cat(a, ")");
                g->g_repl[close] = str_end(g, s);
                continue;
            }

            // a write: the statement must start here and end with a ';'
            u32 semi = chain_end;
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
            b8 starts_statement =
                is_punct(i - 1, ";") || is_punct(i - 1, "{") ||
                is_punct(i - 1, "}") || is_punct(i - 1, ")") ||
                is(i - 1, "else");
            if (!is_punct(semi, ";") || !starts_statement) {
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
            Only the member written to is loaded and stored: .pos[1] adds
            the offset of pos and 1 element to the address, then the
            rest of the chain (e.g. a swizzle) works on the member
            */
            u32 type_i = ptr->type_i;
            u32 last = close; // the last token of the address
            u32 t = close + 1;
            while (t < chain_end && is_punct(t, ".")) {
                const StructDef * d = find_struct(c, type_i);
                if (d == NULL || !loadable[d - c->struct_defs]) { break; }
                DataMember members[DATA_MEMBERS_CAP];
                u32 size = data_members(d, members);
                u32 m = 0;
                while (m < size && !same_text(members[m].name_i, t + 1)) {
                    m++;
                }
                if (m == size) { break; }
                b8 array = members[m].count_open != UINT32_MAX;
                // a whole array member has no load function
                if (array && !is_punct(t + 2, "[")) { break; }

                s = str_begin(g);
                buf_cat(a, " + t1_off_");
                cat_name(a, d->name_i);
                buf_cat(a, "_");
                cat_u32(a, m);
                g->g_repl[t] = str_end(g, s);
                g->g_skip[t + 1] = 1;
                type_i = members[m].type_i;
                last = t + 1;
                t += 2;
                if (array) {
                    u32 index_close = after_match(c, t, "[", "]") - 1;
                    g->g_repl[t] = " + (";
                    s = str_begin(g);
                    buf_cat(a, ") * t1_size_");
                    cat_hlsl_type(a, type_i);
                    g->g_repl[index_close] = str_end(g, s);
                    last = index_close;
                    t = index_close + 1;
                }
            }

            s = str_begin(g);
            buf_cat(a, "; ");
            cat_hlsl_type(a, type_i);
            buf_cat(a, " t1_e = t1_load_");
            cat_hlsl_type(a, type_i);
            buf_cat(a, "(");
            cat_name(a, i);
            buf_cat(a, ", t1_a); t1_e");
            add_text(g, &g->g_post[last], str_end(g, s));
            s = str_begin(g);
            buf_cat(a, "; t1_store_");
            cat_hlsl_type(a, type_i);
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
            !in_list(i, vectors, sizeof(vectors) / sizeof(vectors[0])))
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
What a unit's stage needs before the shaders: sampling any T1Tex (a switch,
shader model 5.0 can't index an array of textures with a variable), the
write_tex helpers and buffers
*/
static void cat_hlsl_textures(Buf * b, const Stage stage, const b8 used) {
    if (!used) {
        // saves compiling the switches below
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
    buf_cat(b,
        "float4 t1_sample(SamplerState smp, uint tex, float2 uv) {\n"
        "    float3 c = float3(uv, (float)T1_TEX_SLICE_I(tex));\n"
        "    float4 r = float4(0.0f, 0.0f, 0.0f, 0.0f);\n"
        "    switch (T1_TEX_ARRAY_I(tex)) {\n");
    for (u32 t = 0; t < T1_TEXARRAYS_CAP; t++) {
        buf_cat(b, "    case ");
        cat_u32(b, t);
        buf_cat(b, ": r = t1_textures[");
        cat_u32(b, t);
        buf_cat(b, implicit_lod ?
            "].Sample(smp, c); break;\n" :
            "].SampleLevel(smp, c, 0.0f); break;\n");
    }
    buf_cat(b,
        "    }\n"
        "    return r;\n"
        "}\n"
        "float4 t1_read(uint tex, uint2 pos) {\n"
        "    int4 c = int4(int2(pos), (int)T1_TEX_SLICE_I(tex), 0);\n"
        "    float4 r = float4(0.0f, 0.0f, 0.0f, 0.0f);\n"
        "    switch (T1_TEX_ARRAY_I(tex)) {\n");
    for (u32 t = 0; t < T1_TEXARRAYS_CAP; t++) {
        buf_cat(b, "    case ");
        cat_u32(b, t);
        buf_cat(b, ": r = t1_textures[");
        cat_u32(b, t);
        buf_cat(b, "].Load(c); break;\n");
    }
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

static void make_unit_hlsl(
    Gen * g,
    const u32 entry_f,
    const u32 user_first_token)
{
    Check * c = g->c;
    const Func * e = &c->funcs[entry_f];
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
    for (u32 f = 0; f < c->funcs_size; f++) {
        const Func * fn = &c->funcs[f];
        if (!g->reachable[f]) {
            for (u32 t = fn->decl_start; t <= fn->body_close; t++) {
                g->u_drop[t] = 1;
            }
        }
    }

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

    if (e->stage == STAGE_KERNEL) {
        buf_cat(gl, "cbuffer T1Grid : register(b0) { uint3 t1_grid_size; };\n");
        buf_cat(hp, "uint3 t1_dtid : SV_DispatchThreadID");
        buf_cat(pro,
            "    if (any(t1_dtid >= t1_grid_size)) { return; }\n");
    } else if (e->stage == STAGE_VERTEX) {
        buf_cat(hp,
            "uint t1_vertex_id : SV_VertexID, "
            "uint t1_instance_id : SV_InstanceID");
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
    u32 threads[3] = {16, 16, 1}; // for a write_tex without a grid
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
            s64 n = eval_attr_arg(c, attr, "buffer");
            if (n < 0 || n + T1_HLSL_BUFS_T > 127) {
                fail(g, attr, "buffer(n) needs a constant from 0 to 95");
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
            (attr_has(attr, "position") && !attr_has(attr, "stage_in")))
        {
            b8 grid = attr_has(attr, "thread_position_in_grid");
            if (grid) {
                if (is(name - 1, "uint3")) {
                    threads[0] = 8; threads[1] = 8; threads[2] = 4;
                } else if (!is(name - 1, "uint2")) {
                    threads[0] = 256; threads[1] = 1; threads[2] = 1;
                }
            }
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
                    is(name - 1, "uint3") ? "t1_dtid" :
                    is(name - 1, "uint2") ? "t1_dtid.xy" : "t1_dtid.x");
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
    buf_cat_n(g->out, T1_token_text(tz, e->name_i),
        T1_token_size(tz, e->name_i));
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
        u32 first = user_first_token;
        while (first < count && g->u_drop[first]) { first++; }
        u32 s = str_begin(g);
        buf_cat(&g->arena, gl->data);
        if (first < count) {
            g->u_pre[first] = str_end(g, s);
        }
    }
    T1_mem_free_managed(gl->data);

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
    T1_assert(recip != NULL && recip_cap > 0);
    recip[0] = '\0';

    // the shaders must be in the subset, T1's own headers aren't checked
    T1_shader_check(shader_source, good, error_recip, error_recip_cap);
    if (!*good) { return; }
    *good = 0;

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
    hlsl_target = 1;

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

    // the first token of the client's shaders, after T1's headers
    u32 user_first_token = c->count;
    for (u32 i = 0; i < c->count; i++) {
        if ((u32)(T1_token_text(tz, i) - full) >= headers_size) {
            user_first_token = i;
            break;
        }
    }

    Gen g;
    T1_std_memset(&g, 0, sizeof(Gen));
    g.c = c;
    g.arena.cap = 4 * 1024 * 1024;
    g.arena.data = T1_mem_malloc_managed(g.arena.cap);
    g.arena.data[0] = '\0';
    Buf out;
    out.data = recip;
    out.size = 0;
    out.cap = recip_cap;
    out.overflow = 0;
    g.out = &out;
    u32 n = c->count + 1;
    g.g_skip = T1_mem_malloc_managed(n);
    g.u_skip = T1_mem_malloc_managed(n);
    g.u_drop = T1_mem_malloc_managed(n);
    g.g_repl = T1_mem_malloc_managed(sizeof(c8 *) * n);
    g.g_post = T1_mem_malloc_managed(sizeof(c8 *) * n);
    g.u_repl = T1_mem_malloc_managed(sizeof(c8 *) * n);
    g.u_pre = T1_mem_malloc_managed(sizeof(c8 *) * n);
    g.u_post = T1_mem_malloc_managed(sizeof(c8 *) * n);
    g.reachable = T1_mem_malloc_managed(c->funcs_size + 1);
    b8 * loadable = T1_mem_malloc_managed(c->struct_defs_size + 1);
    T1_std_memset(g.g_skip, 0, n);
    for (u32 i = 0; i < n; i++) {
        g.g_repl[i] = NULL;
        g.g_post[i] = NULL;
    }

    global_edits(&g);
    hlsl_struct_functions(&g, loadable);
    hlsl_buffer_edits(&g, loadable);
    hlsl_constructor_edits(&g);
    hlsl_stage_edits(&g);
    for (u32 f = 0; f < c->funcs_size && !g.failed; f++) {
        if (c->funcs[f].is_entry) { make_unit_hlsl(&g, f, user_first_token); }
    }

    if (g.arena.overflow || out.overflow) {
        fail(&g, 0, "T1_shader_to_hlsl() ran out of room");
    }
    *good = !g.failed;

    hlsl_target = 0;
    T1_mem_free_managed(loadable);
    T1_mem_free_managed(g.arena.data);
    T1_mem_free_managed(g.g_skip);
    T1_mem_free_managed(g.u_skip);
    T1_mem_free_managed(g.u_drop);
    T1_mem_free_managed((void *)g.g_repl);
    T1_mem_free_managed((void *)g.g_post);
    T1_mem_free_managed((void *)g.u_repl);
    T1_mem_free_managed((void *)g.u_pre);
    T1_mem_free_managed((void *)g.u_post);
    T1_mem_free_managed(g.reachable);
    T1_mem_free_managed(c->secondary);
    T1_mem_free_managed(c);
    T1_mem_free_managed(full);
}
