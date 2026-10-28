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

typedef struct {
    PtrParam ptrs[PTRS_CAP];
    u32 name_i;
    u32 params_open; // '('
    u32 params_close;
    u32 body_open;   // '{'
    u32 body_close;
    u32 ptrs_size;
    b8 is_entry;
} Func;

typedef struct {
    Func funcs[FUNCS_CAP];
    Name structs[NAMES_CAP];
    Name macros[NAMES_CAP];
    Name matrices[NAMES_CAP];
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
        }
        if (is(i, "typedef") && is(i + 1, "struct") && is_punct(i + 2, "{")) {
            // typedef struct {...} Name;
            u32 name = after_match(c, i + 2, "{", "}");
            if (kind(name) == SK_IDENT) {
                add_name(c->structs, &c->structs_size,
                    T1_token_text(tz, name), T1_token_size(tz, name));
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
            for (u32 j = i; j > 0; j--) {
                u32 b = j - 1;
                if (kind(b) != SK_IDENT) { break; }
                if (is(b, "vertex") || is(b, "fragment") || is(b, "kernel")) {
                    f->is_entry = 1;
                }
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
