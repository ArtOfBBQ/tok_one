#include "T1.h"

#include "T1_stdint.h"

#include "T1_assert.h"
#include "T1_tex_array.h"
#include "T1_io.h"
#include "T1_os.h"

u32 T1_tex_read_start(T1Tex tex, u32 x, u32 y, u32 w, u32 h) {
    T1_assert(tex != T1_TEX_NONE);
    // (a compressed texture can't be read back)
    T1_assert(!T1_tex_is_compressed(tex));
    T1_assert(T1_TEX_ARRAY_I(tex) < T1_TEXARRAYS_CAP);
    T1_assert(w > 0 && h > 0);
    T1_assert(x + w <= T1_tex_get_width(tex));
    T1_assert(y + h <= T1_tex_get_height(tex));
    if (
        tex == T1_TEX_NONE ||
        T1_TEX_ARRAY_I(tex) >= T1_TEXARRAYS_CAP ||
        T1_tex_is_compressed(tex) ||
        w == 0 || h == 0 ||
        x + w > T1_tex_get_width(tex) ||
        y + h > T1_tex_get_height(tex))
    {
        return T1_BUF_READ_NONE;
    }
    
    return T1_os_gpu_tex_read_start(tex, x, y, w, h);
}


/*
3D models
*/

/*
Files
*/
void T1_res_path(const char * filename, char * recip, u32 recip_cap) {
    T1_os_res_filename_to_pathfile(filename, recip, recip_cap);
}
void T1_writable_path(const char * filename, char * recip, u32 recip_cap) {
    T1_os_writable_filename_to_pathfile(filename, recip, recip_cap);
}
u64 T1_file_size(const char * path) {
    return T1_os_get_filesize(path);
}
void T1_file_read(
    const char * path,
    char * recip,
    u64 recip_cap,
    char ** const fatal_error)
{
    u32 size = 0;
    T1_os_read_file(path, recip, &size, recip_cap, fatal_error);
}
void T1_file_write(
    const char * path,
    const char * data,
    u32 size,
    u8 * good)
{
    T1_os_write_file(path, data, size, good);
}
void T1_file_delete(const char * path) {
    T1_os_del_file(path);
}
void T1_file_copy(const char * src_path, const char * dst_path) {
    T1_os_copy_file(src_path, dst_path);
}
void T1_dir_filenames(
    const char * path,
    char * filenames,
    u32 filenames_cap)
{
    T1_os_get_filenames_in(path, filenames, filenames_cap);
}
u8 T1_file_exists(const char * path) {
    return T1_os_file_exists(path);
}

/*
The operating system
*/
u64 T1_time_us(void) {
    return T1_os_get_current_time_us();
}
u32 T1_cpu_core_count(void) {
    return T1_os_get_cpu_logical_core_count();
}
void T1_thread_start(void * (* function_to_run)(void *), void * arg_on_heap) {
    T1_os_start_thread(function_to_run, arg_on_heap);
}
u32 T1_mutex_new(void) {
    return T1_os_init_mutex_and_return_id();
}
u8 T1_mutex_trylock(u32 mutex_id) {
    return T1_os_mutex_trylock(mutex_id);
}
void T1_mutex_lock(u32 mutex_id) {
    T1_os_mutex_lock(mutex_id);
}
void T1_mutex_unlock(u32 mutex_id) {
    T1_os_mutex_unlock(mutex_id);
}
void T1_write_error(const char * text) {
    T1_os_write_error(text);
}

/*
Fonts
*/
void T1_font_glyph(
    u32 codepoint,
    f32 size,
    u8 * alpha,
    u32 alpha_cap,
    u32 * w,
    u32 * h,
    f32 * left,
    f32 * top,
    f32 * advance,
    u8 * good)
{
    *w = 0;
    *h = 0;
    *left = 0.0f;
    *top = 0.0f;
    *advance = 0.0f;
    *good = 0;
    // (UTF-16's surrogates are no codepoints)
    if (
        codepoint > 0x10FFFFu ||
        (codepoint >= 0xD800u && codepoint <= 0xDFFFu) ||
        !(size > 0.0f))
    {
        return;
    }
    T1_os_font_glyph(
        codepoint, size, alpha, alpha_cap, w, h, left, top, advance, good);
}

void T1_font_set_language(const char * language) {
    // a BCP 47 code: letters, digits and '-', short (else the system's)
    char code[T1_FONT_LANGUAGE_CAP];
    code[0] = '\0';
    if (language != NULL) {
        u32 i = 0;
        for (; language[i] != '\0' && i + 1 < T1_FONT_LANGUAGE_CAP; i++) {
            const char c = language[i];
            const u8 ok =
                (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '-';
            if (!ok) { break; }
            code[i] = c;
        }
        code[i] = '\0';
        if (language[i] != '\0') { code[0] = '\0'; }
    }
    T1_os_font_set_language(code);
}
