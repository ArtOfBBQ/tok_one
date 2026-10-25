/*
This "build" program concatenates all of the T1 source code into a single
package.

Q: How to make the build program?
gcc concat.c -o conat

Q: It worked! how do I build T1?
./concat

Q: It worked, where is the library?
The library is in the /build folder (T1.h and T1.c). The examples are built
against it too, their binaries are in /build/examples
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>


static const char * encapsulation_h_file = "T1.h";

// examples/<name>.c, built against the library into build/examples/<name>
static const char * examples[] = {
    "noise",
    "fps",
};

static const char * search_paths[] = {
"./",
"engine_src/shared/",
"engine_src/shared_apple/",
"engine_src/macos/",
"resources/",
};

typedef struct {
    char * name;
    uint8_t hexify;
    uint8_t inlineify;
    // NULL = embed the file called name, else inline these files (NULL
    // terminated) one after the other, name is then only the variable name
    const char ** inline_files;
    // NULL = embed the file called name, else decode this white png and embed
    // only its alpha channel, run length encoded (see T1_tex_files.c), name is
    // then only the variable name
    const char * alpha_rle_from_png;
} Embeddable;

// The T1 types that shaders need to know about. T1 prepends these to the
// shader source the client passes before compiling it into a metal library
static const char * shader_include_files[] = {
    "T1_stdint.h",
    "T1_types_gpucpu.h",
    "T1_types_public_gpucpu.h",
    NULL,
};

static const Embeddable to_embed_files[] = {
    {"shader_includes",  0, 1, shader_include_files, NULL},
    {"fontmetrics.dat",  1, 0, NULL, NULL},
    {"font_alpha_rle",   1, 0, NULL, "font.png"},
};

static const char * c_files[] = {
"T1.c",
"T1_embedded_data.c",
"T1_fatal_error.c",
"T1_objc.c",
"T1_settings.c",
"T1_texquad.c",
"T1_linalg3d.c",
"T1_easing.c",
"T1_img.c",
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
"T1_objmodel.c",
"T1_io.c",
"T1_gpu.c",
"T1_tex.c",
"T1_tex_array.c",
"T1_tex_files.c",
"T1_zsprite.c",
"T1_particle.c",
"T1_anim.c",
"T1_property.c",
"T1_text.c",
"T1_render_view.c",
"T1_term.c",
"T1_render.c",
"T1_gameloop.c",
"T1_appinit.c",
};

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

/*
A small png decoder, so T1's own images can be embedded without T1 (or this
build tool) depending on an image library. It only reads what T1's images
use: 8 bit RGBA, not interlaced.
*/

// reads the zlib/deflate stream of the png's IDAT chunks
typedef struct {
    const uint8_t * in;
    uint64_t in_size;
    uint64_t in_pos;
    uint32_t bit_buf;
    uint32_t bit_count;
    uint8_t * out;
    uint64_t out_cap;
    uint64_t out_pos;
} PngInflate;

// a canonical huffman code: how many codes of each length, symbols in order
typedef struct {
    int16_t count[16];
    int16_t symbol[288];
} PngHuffman;

static uint32_t png_inflate_bits(PngInflate * s, uint32_t need) {
    uint32_t val = s->bit_buf;
    while (s->bit_count < need) {
        if (s->in_pos >= s->in_size) {
            printf("Error - png data ends too early\n");
            assert(0);
        }
        val |= (uint32_t)s->in[s->in_pos++] << s->bit_count;
        s->bit_count += 8;
    }
    s->bit_buf = (uint32_t)((uint64_t)val >> need);
    s->bit_count -= need;
    return val & ((1u << need) - 1);
}

static void png_huffman_build(
    PngHuffman * h,
    const uint8_t * lengths,
    int lengths_size)
{
    memset(h->count, 0, sizeof(h->count));
    for (int i = 0; i < lengths_size; i++) {
        h->count[lengths[i]]++;
    }
    
    int16_t offsets[16];
    offsets[1] = 0;
    for (int len = 1; len < 15; len++) {
        offsets[len + 1] = (int16_t)(offsets[len] + h->count[len]);
    }
    
    for (int sym = 0; sym < lengths_size; sym++) {
        if (lengths[sym] != 0) {
            h->symbol[offsets[lengths[sym]]++] = (int16_t)sym;
        }
    }
}

static int png_huffman_decode(PngInflate * s, const PngHuffman * h) {
    int code = 0;
    int first = 0;
    int index = 0;
    for (int len = 1; len < 16; len++) {
        code |= (int)png_inflate_bits(s, 1);
        int count = h->count[len];
        if (code - count < first) {
            return h->symbol[index + (code - first)];
        }
        index += count;
        first += count;
        first <<= 1;
        code <<= 1;
    }
    printf("Error - bad huffman code in png\n");
    assert(0);
    return -1;
}

static void png_inflate_codes(
    PngInflate * s,
    const PngHuffman * lengthcode,
    const PngHuffman * distcode)
{
    static const uint16_t length_base[29] = {
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
    static const uint8_t length_extra[29] = {
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
    static const uint16_t dist_base[30] = {
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
        8193, 12289, 16385, 24577};
    static const uint8_t dist_extra[30] = {
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
        7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
    
    while (1) {
        int sym = png_huffman_decode(s, lengthcode);
        
        if (sym < 256) {
            assert(s->out_pos < s->out_cap);
            s->out[s->out_pos++] = (uint8_t)sym;
            continue;
        }
        
        if (sym == 256) { return; } // end of block
        
        sym -= 257;
        assert(sym < 29);
        uint32_t len =
            length_base[sym] + png_inflate_bits(s, length_extra[sym]);
        
        int dist_sym = png_huffman_decode(s, distcode);
        assert(dist_sym < 30);
        uint32_t dist =
            dist_base[dist_sym] + png_inflate_bits(s, dist_extra[dist_sym]);
        
        assert(dist <= s->out_pos);
        assert(s->out_pos + len <= s->out_cap);
        // byte by byte, the copy can overlap what it writes
        for (uint32_t i = 0; i < len; i++) {
            s->out[s->out_pos] = s->out[s->out_pos - dist];
            s->out_pos++;
        }
    }
}

static void png_inflate_stored_block(PngInflate * s) {
    // stored blocks start at a byte boundary
    s->bit_buf = 0;
    s->bit_count = 0;
    
    assert(s->in_pos + 4 <= s->in_size);
    uint32_t len = s->in[s->in_pos] | ((uint32_t)s->in[s->in_pos + 1] << 8);
    uint32_t nlen =
        s->in[s->in_pos + 2] | ((uint32_t)s->in[s->in_pos + 3] << 8);
    assert(len == (~nlen & 0xFFFF));
    s->in_pos += 4;
    
    assert(s->in_pos + len <= s->in_size);
    assert(s->out_pos + len <= s->out_cap);
    memcpy(s->out + s->out_pos, s->in + s->in_pos, len);
    s->in_pos += len;
    s->out_pos += len;
}

static void png_inflate_fixed_block(PngInflate * s) {
    uint8_t lengths[288];
    int sym = 0;
    for (; sym < 144; sym++) { lengths[sym] = 8; }
    for (; sym < 256; sym++) { lengths[sym] = 9; }
    for (; sym < 280; sym++) { lengths[sym] = 7; }
    for (; sym < 288; sym++) { lengths[sym] = 8; }
    PngHuffman lengthcode;
    png_huffman_build(&lengthcode, lengths, 288);
    
    for (sym = 0; sym < 30; sym++) { lengths[sym] = 5; }
    PngHuffman distcode;
    png_huffman_build(&distcode, lengths, 30);
    
    png_inflate_codes(s, &lengthcode, &distcode);
}

static void png_inflate_dynamic_block(PngInflate * s) {
    static const uint8_t order[19] = {
        16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    
    int nlen  = (int)png_inflate_bits(s, 5) + 257;
    int ndist = (int)png_inflate_bits(s, 5) + 1;
    int ncode = (int)png_inflate_bits(s, 4) + 4;
    assert(nlen <= 286 && ndist <= 30);
    
    uint8_t lengths[320];
    memset(lengths, 0, sizeof(lengths));
    for (int i = 0; i < ncode; i++) {
        lengths[order[i]] = (uint8_t)png_inflate_bits(s, 3);
    }
    PngHuffman lencode;
    png_huffman_build(&lencode, lengths, 19);
    
    // the code lengths of both codes, compressed with lencode
    int i = 0;
    while (i < nlen + ndist) {
        int sym = png_huffman_decode(s, &lencode);
        if (sym < 16) {
            lengths[i++] = (uint8_t)sym;
            continue;
        }
        
        uint8_t repeat_len = 0;
        uint32_t repeat = 0;
        if (sym == 16) {
            assert(i > 0);
            repeat_len = lengths[i - 1];
            repeat = 3 + png_inflate_bits(s, 2);
        } else if (sym == 17) {
            repeat = 3 + png_inflate_bits(s, 3);
        } else {
            repeat = 11 + png_inflate_bits(s, 7);
        }
        assert(i + (int)repeat <= nlen + ndist);
        while (repeat--) { lengths[i++] = repeat_len; }
    }
    
    PngHuffman lengthcode;
    png_huffman_build(&lengthcode, lengths, nlen);
    PngHuffman distcode;
    png_huffman_build(&distcode, lengths + nlen, ndist);
    
    png_inflate_codes(s, &lengthcode, &distcode);
}

static uint32_t png_read_u32_be(const uint8_t * at) {
    return
        ((uint32_t)at[0] << 24) |
        ((uint32_t)at[1] << 16) |
        ((uint32_t)at[2] << 8) |
        (uint32_t)at[3];
}

static uint8_t png_paeth(uint8_t a, uint8_t b, uint8_t c) {
    int p = (int)a + (int)b - (int)c;
    int pa = abs(p - (int)a);
    int pb = abs(p - (int)b);
    int pc = abs(p - (int)c);
    if (pa <= pb && pa <= pc) { return a; }
    if (pb <= pc) { return b; }
    return c;
}

// returns malloc()ed rgba, or crashes with a message if it can't
static uint8_t * png_decode_rgba(
    const uint8_t * png,
    uint64_t png_size,
    uint32_t * out_width,
    uint32_t * out_height)
{
    static const uint8_t signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    if (png_size < 8 || memcmp(png, signature, 8) != 0) {
        printf("Error - not a png\n");
        assert(0);
    }
    
    // join the IDAT chunks, they're 1 zlib stream
    uint8_t * zlib = malloc(png_size);
    uint64_t zlib_size = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    
    uint64_t pos = 8;
    while (pos + 12 <= png_size) {
        uint32_t chunk_size = png_read_u32_be(png + pos);
        const uint8_t * type = png + pos + 4;
        const uint8_t * data = png + pos + 8;
        assert(pos + 12 + chunk_size <= png_size);
        
        if (memcmp(type, "IHDR", 4) == 0) {
            width  = png_read_u32_be(data);
            height = png_read_u32_be(data + 4);
            uint8_t bit_depth  = data[8];
            uint8_t color_type = data[9];
            uint8_t interlace  = data[12];
            if (bit_depth != 8 || color_type != 6 || interlace != 0) {
                printf(
                    "Error - only 8 bit RGBA, non-interlaced pngs can be "
                    "embedded\n");
                assert(0);
            }
        } else if (memcmp(type, "IDAT", 4) == 0) {
            memcpy(zlib + zlib_size, data, chunk_size);
            zlib_size += chunk_size;
        } else if (memcmp(type, "IEND", 4) == 0) {
            break;
        }
        
        pos += 12 + chunk_size;
    }
    assert(width > 0 && height > 0 && zlib_size > 2);
    
    // a zlib header (deflate), then deflate blocks, then a checksum we skip
    assert((zlib[0] & 0x0F) == 8);
    
    uint64_t row_size = 1 + ((uint64_t)width * 4); // a filter byte per row
    PngInflate s;
    memset(&s, 0, sizeof(s));
    s.in = zlib + 2;
    s.in_size = zlib_size - 2;
    s.out_cap = row_size * height;
    s.out = malloc(s.out_cap);
    
    uint32_t last_block = 0;
    while (!last_block) {
        last_block = png_inflate_bits(&s, 1);
        uint32_t type = png_inflate_bits(&s, 2);
        if (type == 0) {
            png_inflate_stored_block(&s);
        } else if (type == 1) {
            png_inflate_fixed_block(&s);
        } else if (type == 2) {
            png_inflate_dynamic_block(&s);
        } else {
            printf("Error - bad deflate block type in png\n");
            assert(0);
        }
    }
    assert(s.out_pos == s.out_cap);
    free(zlib);
    
    // undo each row's filter, left/up/upleft are the same channel's bytes
    uint64_t stride = (uint64_t)width * 4;
    uint8_t * rgba = malloc(stride * height);
    for (uint32_t y = 0; y < height; y++) {
        const uint8_t * filtered = s.out + (y * row_size) + 1;
        uint8_t filter = s.out[y * row_size];
        uint8_t * row = rgba + (y * stride);
        const uint8_t * up_row = y > 0 ? row - stride : NULL;
        
        for (uint64_t x = 0; x < stride; x++) {
            uint8_t left    = x >= 4 ? row[x - 4] : 0;
            uint8_t up      = up_row ? up_row[x] : 0;
            uint8_t up_left = (up_row && x >= 4) ? up_row[x - 4] : 0;
            
            uint8_t predicted = 0;
            switch (filter) {
                case 0: predicted = 0; break;
                case 1: predicted = left; break;
                case 2: predicted = up; break;
                case 3: predicted = (uint8_t)(((int)left + (int)up) / 2); break;
                case 4: predicted = png_paeth(left, up, up_left); break;
                default:
                    printf("Error - bad png row filter\n");
                    assert(0);
            }
            row[x] = (uint8_t)(filtered[x] + predicted);
        }
    }
    free(s.out);
    
    *out_width = width;
    *out_height = height;
    return rgba;
}

/*
Decodes the png in recip and overwrites it with:
u32 width, u32 height (little endian), then (count, alpha) byte pairs that
cover the whole image row by row, count is 1 to 255
*/
static void png_to_alpha_rle_in_place(
    char * recip,
    uint32_t * recip_size,
    const uint32_t recip_cap)
{
    // try_get_file_at_path() counts a 0 terminator
    uint64_t png_size = *recip_size - 1;
    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t * rgba = png_decode_rgba(
        (const uint8_t *)recip,
        png_size,
        &width,
        &height);
    
    uint8_t * out = (uint8_t *)recip;
    uint32_t out_i = 0;
    memcpy(out + out_i, &width, 4);
    out_i += 4;
    memcpy(out + out_i, &height, 4);
    out_i += 4;
    
    uint64_t pixels = (uint64_t)width * height;
    uint64_t px = 0;
    while (px < pixels) {
        uint8_t alpha = rgba[(px * 4) + 3];
        // the image has to be white, we only keep its alpha
        assert(alpha == 0 || (
            rgba[(px * 4) + 0] == 255 &&
            rgba[(px * 4) + 1] == 255 &&
            rgba[(px * 4) + 2] == 255));
        uint8_t count = 0;
        while (
            px < pixels &&
            count < 255 &&
            rgba[(px * 4) + 3] == alpha)
        {
            count++;
            px++;
        }
        assert(out_i + 2 < recip_cap);
        out[out_i++] = count;
        out[out_i++] = alpha;
    }
    
    free(rgba);
    *recip_size = out_i;
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
                to_embed_files[to_embed_i].alpha_rle_from_png ?
                    to_embed_files[to_embed_i].alpha_rle_from_png :
                    to_embed_files[to_embed_i].name);
        
        if (to_embed_size < 1) {
            printf(
                "Couldn't find file to embed: %s\n",
                to_embed_files[to_embed_i].name);
            assert(0);
        }
        
        if (to_embed_files[to_embed_i].alpha_rle_from_png) {
            png_to_alpha_rle_in_place(
                asm_state->arena,
                &to_embed_size,
                to_embed_cap);
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
    
    mkdir("build/examples", 0777);
    uint32_t examples_size = sizeof(examples) / sizeof(examples[0]);
    for (uint32_t e = 0; e < examples_size; e++) {
        char cmd[512];
        snprintf(
            cmd,
            sizeof(cmd),
            "gcc -std=c11 -O2 -I build examples/%s.c build/T1.c "
            "-o build/examples/%s",
            examples[e],
            examples[e]);
        printf("building example: build/examples/%s\n", examples[e]);
        if (system(cmd) != 0) {
            printf("ERROR - failed to build example: %s\n", examples[e]);
            return 1;
        }
    }
    
    printf("Success.\n");
    
    return 0;
}
