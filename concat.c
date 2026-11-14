/*
Q: How to make the build program?
gcc concat.c -o concat

Q: It worked! how do I build T1?
./concat

Q: It worked, where is the library?
The library is in the /build folder (T1.h and T1.c), the same two files for
every platform (each platform's layer is in an #if, the compiler picks).
The examples are built against it too, their binaries are in
/build/examples

Q: And the web?
The same T1.c, compiled for WebAssembly, with build/T1_web.html as the page
(how, at its top; see engine_src/web/). If wasi-sdk's clang is where WEB_CC
below says, the examples and shadertest are built for it too, each a folder
in build/web/: T1's own server for trying them (tools/serve.c, built into
build/web/ when it's newer)
    ./build/web/serve build/web
then http://localhost:8000/examples/ in Chrome (add ?log to an example's
address to have its log printed by the server)
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>

/*
concat builds T1 for the platform it runs on: its own copy of gcc picks the
platform layer
*/
#if defined(_WIN32)
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#elif defined(__APPLE__)
#else
#error "no platform layer for this OS: Windows (_WIN32) or macOS (__APPLE__)"
#endif


static const char * encapsulation_h_file = "T1.h";

/*
<dir>/<name>/<name>.c, built against the library into build/<dir>/<name>:
the examples, and the tests (tests/shadertest checks a GPU backend against
Metal, see its header).
An example can change one macro setting in its copy of T1.h: setting_line
(exactly as in T1_macro_settings.h, NULL for none) becomes new_setting_line
*/
typedef struct {
    const char * dir;
    const char * name;
    const char * setting_line;
    const char * new_setting_line;
} Example;

static const Example examples[] = {
    {"examples", "noise", NULL, NULL},
    {"examples", "frametime", NULL, NULL},
    {"examples", "particles", NULL, NULL},
    {"examples", "gamepad", NULL, NULL},
    {"examples", "text", NULL, NULL},
    {"tests", "shadertest", NULL, NULL},
    {
        "examples",
        "soundkeyboard",
        "#define T1_AUDIO_ACTIVE 2",
        "#define T1_AUDIO_ACTIVE 1",
    },
};

/*
Every platform's layer is in T1.c, each in an #if of its own (see
T1_PLATFORM_* in T1_os.h): the compiler keeps the one it builds for, so
T1.h and T1.c are the same files on every platform, whichever one concat
ran on
*/
static const char * search_paths[] = {
"./",
"engine_src/shared/",
"engine_src/shared_apple/",
"engine_src/macos/",
"engine_src/windows/",
"engine_src/web/",
"resources/",
};

typedef struct {
    char * name;
    uint8_t hexify;
    uint8_t inlineify;
    // NULL = embed the file called name, else inline these files (NULL
    // terminated) one after the other, name is then only the variable name
    const char ** inline_files;
} Embeddable;

// The T1 types that shaders need to know about. T1 prepends these to the
// shader source the client passes before compiling it into a metal library
static const char * shader_include_files[] = {
    "T1_stdint.h",
    "T1_types_gpucpu.h",
    "T1_types_public_gpucpu.h",
    NULL,
};

/*
Left out of the shader includes: T1 gives shaders the values its macro
settings had when T1.c was compiled instead (T1_embedded_data_shader_settings),
so they follow the client's edits to their copy of T1.h
*/
static const char * macro_settings_file = "T1_macro_settings.h";

static const Embeddable to_embed_files[] = {
    {"shader_includes",  0, 1, shader_include_files},
};

// the files every platform compiles
static const char * c_files[] = {
"T1.c",
"T1_embedded_data.c",
"T1_fatal_error.c",
"T1_img.c",
"T1_std.c",
"T1_token.c",
"T1_shader.c",
"T1_assert.c",
"T1_pipeline.c",
"T1_pass.c",
"T1_buf.c",
"T1_audio.c",
"T1_global.c",
"T1_os_common.c",
"T1_mem.c",
"T1_io.c",
"T1_tex.c",
"T1_tex_array.c",
"T1_bc.c",
"T1_gameloop.c",
"T1_appinit.c",
};

/*
The platform layers, after the shared files (so the shared headers are in
before them, outside every #if), each layer's files in an
"#if T1_PLATFORM_X == T1_ACTIVE // T1_LAYER" of its own. The headers only a
layer includes are pasted into its #if again for each layer that does
*/
#define LAYER_FILES_CAP 8
typedef struct {
    const char * platform; // its T1_PLATFORM_* macro
    const char * files[LAYER_FILES_CAP]; // NULL after the last
} Layer;

static const Layer layers[] = {
    {"T1_PLATFORM_APPLE", {
        "T1_objc.c",
        "T1_os_apple_audio.c",
        "T1_os_macos.c",
        "T1_os_apple.c",
        "T1_gpu.c",
        NULL,
    }},
    {"T1_PLATFORM_WINDOWS", {
        "T1_os_windows.c",
        "T1_os_windows_audio.c",
        "T1_gpu_d3d11.c",
        NULL,
    }},
    {"T1_PLATFORM_WEB", {
        "T1_os_web.c",
        "T1_gpu_webgpu.c",
        NULL,
    }},
};

/*
THE WEB BUILD: wasi-sdk's clang (LLVM's clang for wasm with the C library
for it, one download: github.com/WebAssembly/wasi-sdk), unpacked into a
folder wasi-sdk in your home folder (C:/wasi-sdk on Windows). The page is
engine_src/web/T1_web.html, copied next to each app.wasm as index.html
*/
#if defined(_WIN32)
#define WEB_CC "C:/wasi-sdk/bin/clang"
#define WEB_NULL "NUL"
#define WEB_SERVE "build/web/serve.exe"
#else
#define WEB_CC "~/wasi-sdk/bin/clang"
#define WEB_NULL "/dev/null"
#define WEB_SERVE "build/web/serve"
#endif
/*
What every T1 app for the web needs (see T1_web.html): wasm with threads,
and a shared memory the page makes, up to 4 GB. Then the examples' own:
-msimd128, wasm's 128-bit vectors (the target's like a CPU's are), -O3
*/
#define WEB_FLAGS \
    " --target=wasm32-wasip1-threads" \
    " -Wl,--import-memory,--max-memory=4294967296" \
    " -std=c11 -O3 -msimd128"

#define PERMA_CAP    50000
#define ARENA_CAP 60000000
#define OBSERVED_INCLUDES_CAP 1000
#define FILESTACK_CAP 200
#define OUTPUT_CAP 10000000
typedef struct {
    char     output[OUTPUT_CAP];
    char     perma[PERMA_CAP];
    char     arena[ARENA_CAP];
    char     * observed_includes[OBSERVED_INCLUDES_CAP];
    uint32_t perma_i;
    uint32_t arena_i;
    uint16_t to_embed_files_size;
    uint16_t c_files_size;
    uint16_t search_paths_size;
    uint16_t observed_includes_size;
} AsmState;

static AsmState * asm_state = NULL;

uint32_t find_observed_include_or_register_new(
    const char * include_filename)
{
    uint32_t i;
    for (i = 0; i < asm_state->observed_includes_size; i++) {
        if (strcmp(include_filename, asm_state->observed_includes[i]) == 0) {
            return i;
        }
    }
    
    assert((asm_state->perma_i + strlen(include_filename) + 1) < PERMA_CAP);
    strcpy(
        asm_state->perma + asm_state->perma_i,
        include_filename);
    asm_state->observed_includes[asm_state->observed_includes_size] =
        asm_state->perma + asm_state->perma_i;
    asm_state->observed_includes_size += 1;
    asm_state->perma_i += (strlen(include_filename) + 1);
    
    return UINT32_MAX;
}

static void try_get_file_at_path(
    char * recip,
    uint32_t * recip_size,
    uint32_t recip_cap,
    const char * path,
    const char * filename)
{
    uint32_t arena_pop_i = asm_state->arena_i;
    
    *recip_size = 0;
    char * full_path = asm_state->arena + asm_state->arena_i; 
    strcpy(full_path, path);
    strcat(full_path, filename);
    asm_state->arena_i += strlen(full_path);
    assert(asm_state->arena_i < ARENA_CAP);
    
    FILE * file = fopen(full_path, "rb");

    if (file != NULL) {
        fseek(file, 0, SEEK_END);
        *recip_size = ftell(file)+1;
        assert(*recip_size > 0);
        assert(*recip_size <= recip_cap);
        fseek(file, 0, SEEK_SET);

        memset(recip, 0, (*recip_size)+1);
        size_t bytes_read = fread(recip, 1, (*recip_size), file);
        fclose(file);

        // Windows line endings (git checks out CRLF there) become \n, so
        // the output is the same on every platform
        uint32_t kept = 0;
        for (size_t i = 0; i < bytes_read; i++) {
            if (recip[i] != '\r') { recip[kept++] = recip[i]; }
        }
        memset(recip + kept, 0, (*recip_size) - kept);
        *recip_size = kept + 1;
    }
    
    asm_state->arena_i = arena_pop_i;
}

static void try_get_file_from_filename(
    char * recip,
    uint32_t * recip_size,
    uint32_t recip_cap,
    const char * filename)
{
    *recip_size = 0;
    uint32_t i;
    for (i = 0; i < asm_state->search_paths_size; i++) {
        try_get_file_at_path(
            recip,
            recip_size,
            recip_cap,
            search_paths[i],
            filename);
        if (*recip_size > 0) { return; }
    }
}

void parse_file_recursively(
    const char * filename)
{
    uint32_t arena_pop_i = asm_state->arena_i;
    
    uint32_t file_size = 0;
    try_get_file_from_filename(
        asm_state->arena + asm_state->arena_i,
        &file_size,
        ARENA_CAP - asm_state->arena_i,
        filename);
    char * file = asm_state->arena + asm_state->arena_i;
    asm_state->arena_i += file_size;
    
    if (!file_size) {
        printf(
            "ERROR - Couldn't find file: %s in any of the include paths\n",
            filename);
        assert(0);
    } else {
        uint32_t i = 0;
        while (file[i] != '\0' && i < file_size) {
            while (file[i] == ' ' || file[i] == '\t') {
                i++;
            }
            
            if (
                file[i+0] == '#' &&
                file[i+1] == 'i' &&
                file[i+2] == 'n' &&
                file[i+3] == 'c' &&
                file[i+4] == 'l' &&
                file[i+5] == 'u' &&
                file[i+6] == 'd' &&
                file[i+7] == 'e' &&
                file[i+8] == ' ')
            {
                char includefile[512];
                memset(includefile, 0, 512);
                uint32_t j = 0;
                char to_match = '?';
                uint8_t comment_out_include = 1;
                
                if (file[i+9] == '<') { to_match = '>'; }
                if (file[i+9] == '"') { to_match = '"'; }
                
                while (file[i+10+j] != to_match)
                {
                    assert(file[i+10+j] != '\0');
                    includefile[j] = file[i+10+j];
                    j++;
                    assert(j < 510); /* gigantic include filename? */
                }
                
                uint32_t observed_i = find_observed_include_or_register_new(
                    includefile);
                
                if (to_match == '"') {
                    if (observed_i == UINT32_MAX) {
                        parse_file_recursively(
                            /* const char * filename: */
                                includefile);
                    }
                }
                
                if (to_match == '>') {
                    comment_out_include = 0;
                }
                
                if (comment_out_include) {
                    file[i+0]     = '/';
                    file[i+1]     = '*';
                    file[i+j+9+0] = '*';
                    file[i+j+9+1] = '/';
                }
                
                i += j;
            }
            while (file[i] != '\n' && file[i] != '\0') {
                i++;
            }
            if (file[i] == '\n') { i++; }
        }
        
        assert((strlen(file) + strlen(asm_state->output) + 1) < OUTPUT_CAP);
        strcat(asm_state->output, file);
    }
    
    asm_state->arena_i = arena_pop_i;
    
}

static void write_output_to(
    const char * filename,
    const char * path_with_separator)
{
    char filepath[512];
    memset(filepath, 0, 512);
    strcpy(filepath, path_with_separator);
    strcat(filepath, filename);
    FILE * file = fopen(filepath, "wb");
    
    uint32_t output_size = (uint32_t)strlen(asm_state->output);
    asm_state->output[output_size+0] = '\0';
    asm_state->output[output_size+1] = '\0';
    if (file != NULL) {
        fseek(file, 0, SEEK_SET);
        
        fwrite(asm_state->output, 1, output_size, file);
        fclose(file);
    }
}

static void deduplicate_includes_clean_comments(char * in_out)
{
    uint32_t arena_pop_i = asm_state->arena_i;
    
    char * tempcopy = asm_state->arena + asm_state->arena_i;
    uint32_t in_size = (uint32_t)strlen(in_out);
    memcpy(tempcopy, in_out, in_size);
    memset(in_out, 0, in_size);
    
    memset(
        asm_state->observed_includes,
        0,
        sizeof(char *) * OBSERVED_INCLUDES_CAP);
    asm_state->observed_includes_size = 0;
    
    char * tempcopy_head = tempcopy;
    // a layer's <> includes count only inside it (see layers[])
    uint16_t layer_mark = 0;
    
    while (*tempcopy != '\0') {
        while (*tempcopy == ' ' || *tempcopy == '\t')
        {
            tempcopy++;
        }
        
        if (strncmp(tempcopy, "#if T1_PLATFORM_", 16) == 0) {
            const char * eol = strchr(tempcopy, '\n');
            const char * mark = strstr(tempcopy, "// T1_LAYER");
            if (mark != NULL && (eol == NULL || mark < eol)) {
                layer_mark = asm_state->observed_includes_size;
            }
        }
        if (strncmp(tempcopy, "#endif // T1_LAYER", 18) == 0) {
            asm_state->observed_includes_size = layer_mark;
        }
        
        if (tempcopy[0] == '/' && tempcopy[1] == '/') {
            while (*tempcopy != '\n' && *tempcopy != '\0') {
                tempcopy++;
            }
            if (*tempcopy == '\n') { tempcopy++; }
            continue;
        }
         
        if (
            tempcopy[0] == '/' &&
            tempcopy[1] == '*' &&
            tempcopy[2] == 'n' &&
            tempcopy[3] == 'c' &&
            tempcopy[4] == 'l' &&
            tempcopy[5] == 'u' &&
            tempcopy[6] == 'd' &&
            tempcopy[7] == 'e')
        {
            if ((uintptr_t)tempcopy > (uintptr_t)tempcopy_head) {
                uint32_t copy_size = (uint32_t)(
                    (uintptr_t)tempcopy - (uintptr_t)tempcopy_head);
                memcpy(in_out, tempcopy_head, copy_size);
                in_out += copy_size;
            }
            
            while (*tempcopy != '\n' && *tempcopy != '\0') {
                tempcopy++;
            }
            if (*tempcopy == '\n') { tempcopy++; }
            
            tempcopy_head = tempcopy;
            continue;
        }
        
        if (tempcopy[0] == '/' && tempcopy[1] == '*')
        {
            while (!(tempcopy[0] == '*' && tempcopy[1] == '/') &&
                *tempcopy != '\0') 
            {
                tempcopy++;
            }
            if (tempcopy[0] == '*' && tempcopy[1] == '/') {
                tempcopy += 2;
            }
            if (*tempcopy == '\n') { tempcopy++; }
            continue;
        }
        
        if (
            tempcopy[0] == '#' &&
            tempcopy[1] == 'i' &&
            tempcopy[2] == 'n' &&
            tempcopy[3] == 'c' &&
            tempcopy[4] == 'l' &&
            tempcopy[5] == 'u' &&
            tempcopy[6] == 'd' &&
            tempcopy[7] == 'e' &&
            tempcopy[8] == ' ' &&
            tempcopy[9] == '<')
        {
            char includefile[512];
            memset(includefile, 0, 512);
            uint32_t i = 0;
            
            while (tempcopy[10+i] != '>')
            {
                assert(tempcopy[10+i] != '\0');
                includefile[i] = tempcopy[10+i];
                i++;
                assert(i < 510); /* gigantic include filename? */
            }
            
            uint32_t observed_i =
                find_observed_include_or_register_new(includefile);
            
            if (observed_i != UINT32_MAX) {
                if ((uintptr_t)tempcopy > (uintptr_t)tempcopy_head) {
                    uint32_t copy_size = (uint32_t)(
                        (uintptr_t)tempcopy - (uintptr_t)tempcopy_head);
                    memcpy(in_out, tempcopy_head, copy_size);
                    in_out += copy_size;
                }
                
                while (*tempcopy != '\n' && *tempcopy != '\0') {
                    tempcopy++;
                }
                if (*tempcopy == '\n') { tempcopy++; }
                
                tempcopy_head = tempcopy;
                continue;
            }
        }
        tempcopy++;
    }
    
    assert((uintptr_t)tempcopy > (uintptr_t)tempcopy_head);
    uint32_t copy_size = (uint32_t)((uintptr_t)tempcopy - (uintptr_t)tempcopy_head);
    memcpy(in_out, tempcopy_head, copy_size);
    
    asm_state->arena_i = arena_pop_i;
}

/*
If line is prefix (e.g. "#ifndef ") and a name with nothing after it,
copies the name and returns 1
*/
static uint8_t line_directive_name(
    const char * line,
    const char * prefix,
    char * name,
    uint32_t name_cap)
{
    uint32_t prefix_len = (uint32_t)strlen(prefix);
    if (strncmp(line, prefix, prefix_len) != 0) { return 0; }

    uint32_t i = 0;
    const char * at = line + prefix_len;
    while (
        at[i] != '\0' && at[i] != '\n' && at[i] != ' ' && at[i] != '\t')
    {
        assert(i + 1 < name_cap);
        name[i] = at[i];
        i++;
    }
    name[i] = '\0';

    // nothing else on the line, e.g. "#define X_H 1" isn't a guard
    while (at[i] == ' ' || at[i] == '\t') { i++; }
    if (at[i] != '\n' && at[i] != '\0') { return 0; }

    return i > 0;
}

/*
Deletes the include guards of the files T1.h/T1.c were made of: a
"#ifndef X_H" line directly followed by "#define X_H", and the #endif that
closes it. Other #ifndefs (e.g. NDEBUG) stay.
*/
#define GUARD_NESTING_CAP 256
static void strip_include_guards(char * in_out) {
    uint32_t arena_pop_i = asm_state->arena_i;

    uint32_t in_size = (uint32_t)strlen(in_out);
    char * src = asm_state->arena + asm_state->arena_i;
    asm_state->arena_i += in_size + 1;
    assert(asm_state->arena_i < ARENA_CAP);
    memcpy(src, in_out, in_size + 1);

    // whether each open #if... is a guard we deleted
    uint8_t is_guard[GUARD_NESTING_CAP];
    uint32_t depth = 0;

    char * out = in_out;
    const char * line = src;
    while (*line != '\0') {
        const char * next = line;
        while (*next != '\n' && *next != '\0') { next++; }
        if (*next == '\n') { next++; }

        const char * t = line;
        while (*t == ' ' || *t == '\t') { t++; }

        uint8_t keep = 1;
        char name[256];
        char define_name[256];

        if (line_directive_name(t, "#ifndef ", name, 256)) {
            uint32_t len = (uint32_t)strlen(name);
            const char * d = next;
            while (*d == ' ' || *d == '\t') { d++; }
            uint8_t guard =
                len > 2 &&
                strcmp(name + len - 2, "_H") == 0 &&
                line_directive_name(d, "#define ", define_name, 256) &&
                strcmp(name, define_name) == 0;

            assert(depth < GUARD_NESTING_CAP);
            is_guard[depth++] = guard;

            if (guard) {
                // skip the #define line too, and the empty line after it
                keep = 0;
                while (*next != '\n' && *next != '\0') { next++; }
                if (*next == '\n') { next++; }
                if (*next == '\n') { next++; }
            }
        } else if (
            strncmp(t, "#if", 3) == 0)
        {
            // #if, #ifdef
            assert(depth < GUARD_NESTING_CAP);
            is_guard[depth++] = 0;
        } else if (strncmp(t, "#endif", 6) == 0) {
            assert(depth > 0);
            depth--;
            if (is_guard[depth]) { keep = 0; }
        }

        if (keep) {
            memcpy(out, line, (size_t)(next - line));
            out += next - line;
        }
        line = next;
    }
    *out = '\0';
    assert(depth == 0);

    asm_state->arena_i = arena_pop_i;
}

/*
Copies build/filename to dir/filename, replacing the first line that is
exactly replace_line (if not NULL) with replacement. Returns 0 on failure.
*/
static uint8_t copy_build_file_to(
    const char * filename,
    const char * dir,
    const char * replace_line,
    const char * replacement)
{
    char src_path[512];
    char dst_path[512];
    snprintf(src_path, sizeof(src_path), "build/%s", filename);
    snprintf(dst_path, sizeof(dst_path), "%s%s", dir, filename);
    
    FILE * src = fopen(src_path, "rb");
    if (src == NULL) { return 0; }
    fseek(src, 0, SEEK_END);
    long size = ftell(src);
    fseek(src, 0, SEEK_SET);
    char * contents = malloc((size_t)size + 1);
    if (contents == NULL) { fclose(src); return 0; }
    size_t read = fread(contents, 1, (size_t)size, src);
    fclose(src);
    contents[read] = '\0';
    
    FILE * dst = fopen(dst_path, "wb");
    if (dst == NULL) { free(contents); return 0; }
    
    char * at = replace_line == NULL ? NULL : strstr(contents, replace_line);
    if (at != NULL) {
        size_t before = (size_t)(at - contents);
        size_t replaced = strlen(replace_line);
        fwrite(contents, 1, before, dst);
        fwrite(replacement, 1, strlen(replacement), dst);
        fwrite(at + replaced, 1, read - before - replaced, dst);
    } else {
        if (replace_line != NULL) {
            printf(
                "Error: expected replace_line to be NULL, got: %s\n",
                replace_line);
            assert(0);
        }
        fwrite(contents, 1, read, dst);
    }
    
    fclose(dst);
    free(contents);
    return 1;
}

static void delete_build_file(const char * fn) {
    char filepath[512];
    memset(filepath, 0, 512);
    strcpy(filepath, "build/");
    strcat(filepath, fn);
    
    remove(filepath);
}

static void convert_to_multi_line_string(
    const char * orig,
    uint32_t size,
    char * recip)
{
    uint32_t pop_arena_i = asm_state->arena_i;
    
    uint32_t i = 0;
    uint32_t j = 0;
    recip[j++] = '"';
    while (i < size) {
        while (
            i < size &&
            orig[i] != '"' &&
            orig[i] != '\n')
        {
            recip[j++] = orig[i++];
        }
        if (orig[i] == '"' && orig[i-1] != '\\') {
            recip[j++] = '\\';
            recip[j++] = '"';
            i++;
        }
        if (orig[i] == '\n') {
            recip[j++] = '\\';
            recip[j++] = 'n';
            recip[j++] = '"';
            i++;
            if (i >= size) { break; }
            recip[j++] = '\n';
            recip[j++] = '"';
        }
    }
    
    asm_state->arena_i = pop_arena_i;
}

static void hexify_byte_stream(
    char * to_hexify,
    uint32_t size)
{
    uint32_t pop_arena_i = asm_state->arena_i;
    
    char * copy = asm_state->arena + asm_state->arena_i;
    assert((asm_state->arena_i + size) < ARENA_CAP);
    asm_state->arena_i += size;
    memcpy(copy, to_hexify, size);
    
    memset(to_hexify, 0, size);
    
    uint32_t i;
    for (i = 0; i < size; i++) {
        to_hexify += sprintf(to_hexify, "0x%02X, ", (unsigned char)copy[i]);
        if (i % 13 == 12) {
            to_hexify += sprintf(to_hexify, "\n");
        }
    }
    
    asm_state->arena_i = pop_arena_i;
}

static void name_to_adjusted_name(
    char * adjusted_name,
    const char * original_name)
{
    adjusted_name[0] = '\0';
    
    strcat(adjusted_name, "T1_embedded_data_");
    
    uint32_t i = 0;
    uint32_t len = strlen(adjusted_name);
    
    while (original_name[i] != '\0') {
        if (original_name[i] == '.') {
            adjusted_name[i+len] = '_';
        } else {
            adjusted_name[i+len] = original_name[i];
        }
        i++;
    }
    adjusted_name[i+len] = '\0';
}


static void append_embedded_file_to_out(
    const uint32_t to_embed_i)
{
    uint32_t pop_arena_i = asm_state->arena_i;
    
    char * to_embed = asm_state->arena + asm_state->arena_i;
    uint32_t to_embed_cap = 50000000;
    assert(ARENA_CAP > to_embed_cap);
    asm_state->arena_i += to_embed_cap;
    uint32_t to_embed_size = 0;
    uint32_t initial_output_size = (uint32_t)strlen(asm_state->output);
     
    if (to_embed_files[to_embed_i].inlineify) {
        const char ** inline_files = to_embed_files[to_embed_i].inline_files;
        if (inline_files == NULL) {
            parse_file_recursively(to_embed_files[to_embed_i].name);
        } else {
            find_observed_include_or_register_new(macro_settings_file);
            for (uint32_t i = 0; inline_files[i] != NULL; i++) {
                // register first so later files don't include it again
                if (
                    find_observed_include_or_register_new(inline_files[i]) !=
                        UINT32_MAX)
                {
                    continue;
                }
                parse_file_recursively(inline_files[i]);
            }
        }
        uint32_t new_output_size =
            (uint32_t)strlen(asm_state->output) - initial_output_size;
        
        deduplicate_includes_clean_comments(
            asm_state->output + initial_output_size);
        
        new_output_size =
            (uint32_t)strlen(asm_state->output) - initial_output_size;
        
        assert(to_embed_cap > new_output_size);
        convert_to_multi_line_string(
            /* char * start: */
                asm_state->output + initial_output_size,
            /* uint32_t size: */
                new_output_size,
            /* char * recip: */
                to_embed);
        
        memset(
            asm_state->output + initial_output_size,
            0,
            OUTPUT_CAP - initial_output_size);
    } else {
        try_get_file_from_filename(
            /* char * recip: */
                asm_state->arena,
            /* uint32_t * recip_size: */
                &to_embed_size,
            /* uint32_t recip_cap: */
                to_embed_cap,
            /* const char * filename: */
                to_embed_files[to_embed_i].name);
        
        if (to_embed_size < 1) {
            printf(
                "Couldn't find file to embed: %s\n",
                to_embed_files[to_embed_i].name);
            assert(0);
        }
    }
    
    if (to_embed_files[to_embed_i].hexify) {
        hexify_byte_stream(
            /* const char * orig: */
                to_embed,
            /* uint32_t size: */
                to_embed_size);
    }
    
    assert((strlen(to_embed) + strlen(asm_state->output) + 20) < OUTPUT_CAP);
    
    if (to_embed_files[to_embed_i].inlineify) {
        strcat(asm_state->output, "const char * ");
    } else {
        strcat(asm_state->output, "const unsigned char ");
    }
    char adjusted_name[256];
    name_to_adjusted_name(adjusted_name, to_embed_files[to_embed_i].name);
    strcat(asm_state->output, adjusted_name);
    if (to_embed_files[to_embed_i].inlineify) {
        strcat(asm_state->output, " =\n");
    } else {
        strcat(asm_state->output, "[] = {\n");
    }
    strcat(asm_state->output, to_embed);
    if (to_embed_files[to_embed_i].inlineify) {
        strcat(asm_state->output, ";\n\n\n");
    } else {
        strcat(asm_state->output, "\n};\n");
        strcat(asm_state->output, "const unsigned int ");
        strcat(asm_state->output, adjusted_name);
        strcat(asm_state->output, "_size = ");
        sprintf(
            asm_state->output + strlen(asm_state->output),
            "%u;\n\n\n",
            to_embed_size);
    }
    
    asm_state->arena_i = pop_arena_i;
}

/*
Appends T1_embedded_data_shader_settings: for every #define in the macro
settings, a line the C preprocessor fills with the value T1.c is compiled
with, e.g. "#define T1_SHADOWS_ACTIVE " "1" "\n"
*/
static void append_shader_settings_to_out(void) {
    char path[512];
    FILE * file = NULL;
    for (uint32_t i = 0; i < asm_state->search_paths_size && !file; i++) {
        snprintf(
            path, sizeof(path),
            "%s%s",
            search_paths[i],
            macro_settings_file);
        file = fopen(path, "rb");
    }
    assert(file != NULL);
    
    strcat(asm_state->output,
        "#define T1_EMBEDDED_STR_(x) #x\n"
        "#define T1_EMBEDDED_STR(x) T1_EMBEDDED_STR_(x)\n"
        "const char * T1_embedded_data_shader_settings =\n");
    
    char line[1024];
    while (fgets(line, sizeof(line), file) != NULL) {
        const char * at = line;
        while (*at == ' ' || *at == '\t') { at++; }
        if (strncmp(at, "#define ", 8) != 0) { continue; }
        at += 8;
        
        char name[256];
        uint32_t n = 0;
        while (
            at[n] != '\0' && at[n] != ' ' && at[n] != '\t' &&
            at[n] != '\r' && at[n] != '\n' && at[n] != '(')
        {
            assert(n + 1 < sizeof(name));
            name[n] = at[n];
            n++;
        }
        name[n] = '\0';
        
        // skip the include guard and function-like macros
        if (n == 0 || at[n] == '(') { continue; }
        if (n > 2 && strcmp(name + n - 2, "_H") == 0) { continue; }
        
        char entry[1024];
        snprintf(
            entry,
            sizeof(entry),
            "#ifdef %s\n"
            "    \"#define %s \" T1_EMBEDDED_STR(%s) \"\\n\"\n"
            "#endif\n",
            name, name, name);
        assert(strlen(asm_state->output) + strlen(entry) + 1 < OUTPUT_CAP);
        strcat(asm_state->output, entry);
    }
    fclose(file);
    
    strcat(asm_state->output, "    \"\";\n\n");
}

/*
T1.c, into out_dir: the shared files, then every platform's layer in its
#if. It includes T1.h rather than repeating it, so the macro settings at
the top of T1.h are the only ones: edit them there
*/
static void write_library_c(
    const char * out_dir,
    char ** t1_h_includes,
    uint16_t t1_h_includes_size)
{
    asm_state->arena_i = 0;
    memset(asm_state->output, 0, OUTPUT_CAP);
    memset(asm_state->arena, 0, ARENA_CAP);
    
    memcpy(
        asm_state->observed_includes,
        t1_h_includes,
        sizeof(char *) * t1_h_includes_size);
    asm_state->observed_includes_size = t1_h_includes_size;
    find_observed_include_or_register_new(encapsulation_h_file);
    strcat(asm_state->output, "#include \"T1.h\"\n\n");
    
    for (int32_t i = 0; i < asm_state->c_files_size; i++) {
        parse_file_recursively(/* filename: */ c_files[i]);
    }
    
    for (uint32_t l = 0; l < sizeof(layers) / sizeof(layers[0]); l++) {
        // what this layer pastes in, forgotten after it for the next one
        const uint16_t observed_mark = asm_state->observed_includes_size;
        char line[256];
        snprintf(line, sizeof(line),
            "\n#if %s == T1_ACTIVE // T1_LAYER\n", layers[l].platform);
        strcat(asm_state->output, line);
        for (uint32_t f = 0; layers[l].files[f] != NULL; f++) {
            parse_file_recursively(/* filename: */ layers[l].files[f]);
        }
        strcat(asm_state->output, "\n#endif // T1_LAYER\n");
        asm_state->observed_includes_size = observed_mark;
    }
    
    deduplicate_includes_clean_comments(asm_state->output);
    strip_include_guards(asm_state->output);
    write_output_to("T1.c", out_dir);
}

static uint8_t copy_file(const char * from, const char * to) {
    FILE * src = fopen(from, "rb");
    if (src == NULL) { return 0; }
    FILE * dst = fopen(to, "wb");
    if (dst == NULL) { fclose(src); return 0; }
    char chunk[65536];
    size_t read;
    while ((read = fread(chunk, 1, sizeof(chunk), src)) > 0) {
        fwrite(chunk, 1, read, dst);
    }
    fclose(src);
    fclose(dst);
    return 1;
}

/*
The examples for the browser, into build/web/examples/<name>/ (the page
and app.wasm), from each example's own T1.h and T1.c (copied there for the
host build before), and shadertest into build/web/tests/shadertest/ with
its files next to its page (see web_test_files[]). Without a wasm toolchain they're skipped, which isn't a
failure
*/
static const char * web_test_files[] = {
    "tests/shadertest/shadertest.metal",
    "tests/shadertest/rejects.metal",
    "tests/shadertest/reference.bin",
};
/*
tools/serve.c, the server to try the web builds with, into build/web/ when
it's missing or older than its source. Built with the host's gcc, like the
examples: it needs no wasm toolchain
*/
static uint8_t build_web_server(void) {
    struct stat source;
    struct stat built;
    if (stat("tools/serve.c", &source) != 0) { return 0; }
    if (stat(WEB_SERVE, &built) == 0 && built.st_mtime >= source.st_mtime) {
        return 1;
    }
    printf("building: " WEB_SERVE "\n");
    #if defined(_WIN32)
    const char * cmd = "gcc -std=c11 -O2 tools/serve.c -o " WEB_SERVE " -lws2_32";
    #else
    const char * cmd = "gcc -std=c11 -O2 tools/serve.c -o " WEB_SERVE;
    #endif
    if (system(cmd) != 0) {
        printf("ERROR - failed to build: " WEB_SERVE "\n");
        return 0;
    }
    return 1;
}

static uint8_t build_web_examples(void) {
    FILE * check = fopen("build/web/check.c", "wb");
    if (check == NULL) { return 0; }
    fprintf(check, "#include <stdio.h>\nint main(void) { puts(\"\"); }\n");
    fclose(check);
    if (
        system(
            WEB_CC WEB_FLAGS " build/web/check.c -o build/web/check.wasm"
            " > " WEB_NULL " 2>&1") != 0)
    {
        printf(
            "web: examples skipped, no wasm compiler at " WEB_CC
            " (see THE WEB BUILD in concat.c)\n");
        return 1;
    }
    
    mkdir("build/web/examples", 0777);
    uint32_t examples_size = sizeof(examples) / sizeof(examples[0]);
    
    // a page that links them, for ./serve build/web/examples
    FILE * index = fopen("build/web/examples/index.html", "wb");
    if (index == NULL) { return 0; }
    fprintf(
        index,
        "<!doctype html>\n<meta charset=\"utf-8\">\n"
        "<title>T1's examples</title>\n"
        "<style>body { font: 18px/1.6 system-ui, sans-serif; "
        "margin: 40px; }</style>\n<h1>T1's examples</h1>\n<ul>\n");
    for (uint32_t e = 0; e < examples_size; e++) {
        if (strcmp(examples[e].dir, "examples") != 0) { continue; }
        fprintf(
            index,
            "<li><a href=\"%s/\">%s</a></li>\n",
            examples[e].name,
            examples[e].name);
    }
    fprintf(index, "</ul>\n");
    fclose(index);
    
    mkdir("build/web/tests", 0777);
    for (uint32_t e = 0; e < examples_size; e++) {
        const char * kind = examples[e].dir;
        const char * name = examples[e].name;
        
        char dir[256];
        snprintf(dir, sizeof(dir), "build/web/%s/%s", kind, name);
        mkdir(dir, 0777);
        
        // a test's files, where it opens them from: tests/<name>/
        if (strcmp(kind, "tests") == 0) {
            char path[512];
            snprintf(path, sizeof(path), "%s/tests", dir);
            mkdir(path, 0777);
            snprintf(path, sizeof(path), "%s/tests/%s", dir, name);
            mkdir(path, 0777);
            uint32_t files_size =
                sizeof(web_test_files) / sizeof(web_test_files[0]);
            for (uint32_t f = 0; f < files_size; f++) {
                char prefix[256];
                snprintf(prefix, sizeof(prefix), "tests/%s/", name);
                if (strncmp(web_test_files[f], prefix, strlen(prefix)) != 0) {
                    continue;
                }
                snprintf(path, sizeof(path), "%s/%s", dir, web_test_files[f]);
                if (!copy_file(web_test_files[f], path)) {
                    printf("ERROR - no %s\n", web_test_files[f]);
                    return 0;
                }
            }
        }
        
        char to[512];
        snprintf(to, sizeof(to), "%s/index.html", dir);
        if (!copy_file("engine_src/web/T1_web.html", to)) { return 0; }
        
        char cmd[1024];
        snprintf(
            cmd,
            sizeof(cmd),
            WEB_CC WEB_FLAGS " %s/%s/%s.c %s/%s/T1.c -o %s/app.wasm",
            kind,
            name,
            name,
            kind,
            name,
            dir);
        printf("building: %s\n", dir);
        if (system(cmd) != 0) {
            printf("ERROR - failed to build: %s\n", dir);
            return 0;
        }
    }
    return 1;
}

int main(void) {
    printf("Concatenating all source into T1.h/T1.c...\n");
    mkdir("build", 0777);
    assert(ARENA_CAP > OUTPUT_CAP);
    
    delete_build_file("T1.h");
    delete_build_file("T1.c");
    delete_build_file("T1_embedded_data.h");
    delete_build_file("T1_embedded_data.c");
    
    asm_state = malloc(sizeof(AsmState));
    if (!asm_state) {
        printf("Error - Failed to malloc %lu bytes?!\n", sizeof(AsmState));
        return 1;
    }
    memset(asm_state, 0, sizeof(AsmState));
    
    asm_state->to_embed_files_size =
        sizeof(to_embed_files) / sizeof(to_embed_files[0]);
    asm_state->c_files_size =
        sizeof(c_files) / sizeof(c_files[0]);
    asm_state->search_paths_size =
        sizeof(search_paths) / sizeof(search_paths[0]);
    assert(asm_state->search_paths_size > 0);
    
    // Create the .c file for embedded data
    {
        strcat(asm_state->output, "#include \"T1_embedded_data.h\"\n");
        strcat(asm_state->output, "#include \"T1_macro_settings.h\"\n\n");
        append_shader_settings_to_out();
        uint32_t i = 0;
        for (i = 0; i < asm_state->to_embed_files_size; i++) {
            uint32_t prev_out_size = (uint32_t)strlen(asm_state->output);
            append_embedded_file_to_out(i);
        }
    }
    write_output_to(
        "T1_embedded_data.c",
        "engine_src/shared/");
    
    // Create the .h for embedded types
    {
        asm_state->arena_i = 0;
        memset(asm_state->output, 0, OUTPUT_CAP);
        memset(asm_state->arena, 0, ARENA_CAP);
        
        strcat(
            asm_state->output,
            "#ifndef T1_EMBEDDED_DATA_H\n#define T1_EMBEDDED_DATA_H\n\n");
        strcat(
            asm_state->output,
            "// the macro settings T1.c was compiled with, for shaders\n"
            "extern const char * T1_embedded_data_shader_settings;\n\n");
        
        uint32_t i = 0;
        for (i = 0; i < asm_state->to_embed_files_size; i++) {
            uint32_t prev_out_size = (uint32_t)strlen(asm_state->output);
            if (to_embed_files[i].inlineify) {
                strcat(asm_state->output, "extern const char * ");
            } else {
                strcat(asm_state->output, "extern const unsigned char ");
            }
            char adjusted_name[256];
            name_to_adjusted_name(adjusted_name, to_embed_files[i].name);
            strcat(asm_state->output, adjusted_name);
            if (to_embed_files[i].inlineify) {
                // pass
            } else {
                strcat(asm_state->output, "[];\n");
                strcat(asm_state->output, "extern const unsigned int ");
                strcat(asm_state->output, adjusted_name);
                strcat(asm_state->output, "_size");
            }
            strcat(asm_state->output, ";\n\n");
        }
        
        strcat(
            asm_state->output,
            "#endif // T1_EMBEDDED_DATA_H");
    }
    
    write_output_to(
        "T1_embedded_data.h",
        "engine_src/shared/");
    
    asm_state->arena_i = 0;
    memset(asm_state->output, 0, OUTPUT_CAP);
    memset(asm_state->arena, 0, ARENA_CAP);
    
    parse_file_recursively(
        /* filename: */ encapsulation_h_file);
    
    // what T1.h is made of, so T1.c can #include it instead of a 2nd copy
    char * t1_h_includes[OBSERVED_INCLUDES_CAP];
    uint16_t t1_h_includes_size = asm_state->observed_includes_size;
    memcpy(
        t1_h_includes,
        asm_state->observed_includes,
        sizeof(char *) * t1_h_includes_size);
    
    deduplicate_includes_clean_comments(asm_state->output);
    strip_include_guards(asm_state->output);

    // one guard around everything instead
    {
        uint32_t arena_pop_i = asm_state->arena_i;
        uint32_t size = (uint32_t)strlen(asm_state->output);
        char * body = asm_state->arena + asm_state->arena_i;
        asm_state->arena_i += size + 1;
        assert(asm_state->arena_i < ARENA_CAP);
        memcpy(body, asm_state->output, size + 1);

        const char * guard_start = "#ifndef T1_H\n#define T1_H\n\n";
        const char * guard_end = "\n#endif // T1_H\n";
        assert(
            strlen(guard_start) + size + strlen(guard_end) + 1 < OUTPUT_CAP);
        strcpy(asm_state->output, guard_start);
        strcat(asm_state->output, body);
        strcat(asm_state->output, guard_end);

        asm_state->arena_i = arena_pop_i;
    }

    write_output_to("T1.h", "build/");
    
    write_library_c("build/", t1_h_includes, t1_h_includes_size);
    // the web's page, the third file a web app needs (see its top)
    if (!copy_file("engine_src/web/T1_web.html", "build/T1_web.html")) {
        printf("ERROR - couldn't copy engine_src/web/T1_web.html\n");
        return 1;
    }
    
    /*
    Every example keeps its own T1.h and T1.c next to its source, like your
    app would, with its own settings (e.g. its window title)
    */
    mkdir("build/examples", 0777);
    mkdir("build/tests", 0777);
    uint32_t examples_size = sizeof(examples) / sizeof(examples[0]);
    for (uint32_t e = 0; e < examples_size; e++) {
        char dir[256];
        const char * kind = examples[e].dir;
        const char * name = examples[e].name;
        snprintf(dir, sizeof(dir), "%s/%s/", kind, name);

        if (
            !copy_build_file_to(
                "T1.h",
                dir,
                examples[e].setting_line,
                examples[e].new_setting_line) ||
            !copy_build_file_to("T1.c", dir, NULL, NULL))
        {
            printf("ERROR - failed to copy T1.h/T1.c to %s\n", dir);
            return 1;
        }

        char cmd[512];
        snprintf(
            cmd,
            sizeof(cmd),
            "gcc -std=c11 -O3 %s/%s/%s.c %s/%s/T1.c "
            "-o build/%s/%s",
            kind,
            name,
            name,
            kind,
            name,
            kind,
            name);
        printf("building: build/%s/%s\n", kind, name);
        if (system(cmd) != 0) {
            printf("ERROR - failed to build: %s/%s\n", kind, name);
            return 1;
        }
    }
    
    mkdir("build/web", 0777);
    if (!build_web_server() || !build_web_examples()) { return 1; }
    
    printf("Success.\n");
    
    return 0;
}
