#include "T1.h"

#include "T1_stdint.h"
#include "T1_conditional_includes.h"

#include "T1_assert.h"
#include "T1_tex_array.h"
#include "T1_io.h"
#include "T1_os.h"

u32 T1_tex_read_start(T1Tex tex, u32 x, u32 y, u32 w, u32 h) {
    T1_assert(tex != T1_TEX_NONE && tex != T1_TEX_WINDOW);
    T1_assert(T1_TEX_ARRAY_I(tex) < T1_TEXARRAYS_CAP);
    T1_assert(w > 0 && h > 0);
    T1_assert(x + w <= T1_tex_get_width(tex));
    T1_assert(y + h <= T1_tex_get_height(tex));
    if (
        tex == T1_TEX_NONE ||
        tex == T1_TEX_WINDOW ||
        T1_TEX_ARRAY_I(tex) >= T1_TEXARRAYS_CAP ||
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
void T1_os_res_path(const char * filename, char * recip, u32 recip_cap) {
    T1_os_res_filename_to_pathfile(filename, recip, recip_cap);
}
void T1_os_writable_path(const char * filename, char * recip, u32 recip_cap) {
    T1_os_writable_filename_to_pathfile(filename, recip, recip_cap);
}
u64 T1_os_file_size(const char * path) {
    return T1_os_get_filesize(path);
}
void T1_os_file_read(
    const char * path,
    char * recip,
    u64 recip_cap,
    char ** const fatal_error)
{
    u32 size = 0;
    T1_os_read_file(path, recip, &size, recip_cap, fatal_error);
}
void T1_os_file_write(
    const char * path,
    const char * data,
    u32 size,
    u8 * good)
{
    T1_os_write_file(path, data, size, good);
}
void T1_os_file_delete(const char * path) {
    T1_os_del_file(path);
}
void T1_os_file_copy(const char * src_path, const char * dst_path) {
    T1_os_copy_file(src_path, dst_path);
}
void T1_os_dir_filenames(
    const char * path,
    char * filenames,
    u32 filenames_cap)
{
    T1_os_get_filenames_in(path, filenames, filenames_cap);
}
void T1_os_dir_show(const char * path) {
    T1_os_open_dir_in_file_explorer_window_if_possible(path);
}
