#include "T1_shader.h"

#include "T1_assert.h"
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

typedef enum : u8 {
    SK_SPACE, // never stored
    SK_PREPROC,
    SK_IDENT,
    SK_NUMBER,
    SK_ATTR, // [[ ... ]]
    SK_PUNCT,
} ShaderTokenKind;

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
    "fmin", "fmod", "fract", "length", "log", "log2", "max", "min", "mix",
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

typedef enum : u8 {
    STAGE_NONE, // a helper
    STAGE_VERTEX,
    STAGE_FRAGMENT,
    STAGE_KERNEL,
} Stage;

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

static b8 needs_rename(const u32 i) {
    if (kind(i) != SK_IDENT) { return 0; }
    if (in_list(i, glsl_reserved,
        sizeof(glsl_reserved) / sizeof(glsl_reserved[0])))
    {
        return 1;
    }
    const c8 * t = T1_token_text(tz, i);
    u32 n = T1_token_size(tz, i);
    if (n >= 3 && t[0] == 'g' && t[1] == 'l' && t[2] == '_') { return 1; }
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
            if (is(i + 1, "const")) {
                g->g_skip[i] = 1;
            } else {
                g->g_repl[i] = "const";
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
            const c8 * type = glsl_type_name(i + 2);
            if (type == NULL || !is_punct(i + 3, ">")) {
                fail(g, i, "unsupported as_type<>, use a 32-bit type");
                continue;
            }
            u32 s = str_begin(g);
            buf_cat(&g->arena, "t1_as_");
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
            // (void)x; only says x is unused, GLSL has no void casts
            u32 end = unary_end(c, i + 3);
            for (u32 t = i; t < end; t++) { g->g_skip[t] = 1; }
        } else if (is_cast(c, i)) {
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

    // write the unit
    u32 prev_end = 0; // offset in the source after the previous token
    for (u32 i = 0; i < count; i++) {
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
                    // __METAL_VERSION__ -> T1_GLSL
                    const c8 * mv = "__METAL_VERSION__";
                    u32 j = 0;
                    while (j < size) {
                        if (
                            j + 17 <= size &&
                            T1_std_are_equal_strings_of_length(t + j, mv, 17))
                        {
                            buf_cat(g->out, "T1_GLSL");
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
