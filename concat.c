/*
This "build" program concatenates all of the T1 source code into a single
package.

Q: How to make the build program?
gcc concat.c -o conat

Q: It worked! how do I build T1?
./concat

Q: It worked, where is the library?
The library is in the /build folder
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

static const char * encapsulation_h_file = "T1.h";

static const char * search_paths[] = {
"./",
"engine_src/shared/",
"engine_src/shared_apple/",
"engine_src/macos/",
"engine_src/shared/debigulator/src/",
"resources/",
};

typedef struct {
    char * name;
    uint8_t hexify;
    uint8_t inlineify;
} Embeddable;

static const Embeddable to_embed_files[] = {
    {"shaders.metal",    0, 1},
    {"fontmetrics.dat",  1, 0},
    {"font.png",         1, 0},
    {"perlin_noise.png", 1, 0},
};

static const char * c_files[] = {
"T1.c",
"inflate.c",
"decode_png.c",
"decode_bmp.c",
"T1_embedded_data.c",
"T1_sticky_error.c",
"T1_objc.c",
"T1_settings.c",
"T1_texquad.c",
"T1_linalg3d.c",
"T1_easing.c",
"T1_meta.c",
"T1_img.c",
"T1_wav.c",
"T1_token.c",
"T1_objparser.c",
"T1_mtlparser.c",
"T1_std.c",
"T1_log.c",
"T1_mesh_summary.c",
"T1_collision.c",
"T1_id.c",
"T1_audio.c",
"T1_global.c",
"T1_triangle.c",
"T1_material.c",
"T1_zlight.c",
"T1_types_cpu_to_gpu.c",
"T1_os_apple_audio.c",
"T1_os_common.c",
"T1_os_macos.c",
"T1_os_apple.c",
"T1_mem.c",
"T1_profiler.c",
"T1_objmodel.c",
"T1_io.c",
"T1_rand.c",
"T1_gpu.c",
"T1_tex.c",
"T1_tex_array.c",
"T1_tex_files.c",
"T1_zsprite.c",
"T1_particle.c",
"T1_anim.c",
"T1_text.c",
"T1_render_view.c",
"T1_ui_widget.c",
"T1_term.c",
"T1_render.c",
"T1_gameloop.c",
"T1_appinit.c",
};

#define PERMA_CAP    50000
#define ARENA_CAP 40000000
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
    
    FILE * file = fopen(full_path, "r");
    
    if (file != NULL) {
        fseek(file, 0, SEEK_END);
        *recip_size = ftell(file)+1;
        assert(*recip_size > 0);
        assert(*recip_size <= recip_cap);
        fseek(file, 0, SEEK_SET);
        
        memset(recip, 0, (*recip_size)+1);
        fread(recip, 1, (*recip_size), file);
        fclose(file);
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
    
    while (*tempcopy != '\0') {
        while (*tempcopy == ' ' || *tempcopy == '\t')
        {
            tempcopy++;
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
    uint32_t to_embed_cap =  900000;
    assert(ARENA_CAP > to_embed_cap);
    asm_state->arena_i += to_embed_cap;
    uint32_t to_embed_size = 0;
    uint32_t initial_output_size = (uint32_t)strlen(asm_state->output);
     
    if (to_embed_files[to_embed_i].inlineify) {
        parse_file_recursively(to_embed_files[to_embed_i].name);
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

int main(void) {
    printf("Concatenating all source into T1.h/T1.c...\n");
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
        strcat(asm_state->output, "#include \"T1_embedded_data.h\"\n\n");
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
    
    deduplicate_includes_clean_comments(asm_state->output);
    write_output_to("T1.h", "build/");
    
    memset(asm_state->output, 0, OUTPUT_CAP);
    
    int32_t i;
    for (i = 0; i < asm_state->c_files_size; i++) {
        const char * filename = c_files[i];
        
        parse_file_recursively(
            /* filename: */ filename);
    }
    
    deduplicate_includes_clean_comments(asm_state->output);
    write_output_to("T1.c", "build/");
    
    printf("Success.\n");
    
    return 0;
}
