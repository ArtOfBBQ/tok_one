#ifndef T1_PASS_H
#define T1_PASS_H

#include "T1_stdint.h"
#include "T1_types_gpucpu.h"
#include "T1_types_public.h"
#include <stddef.h>

/*
A frame is the list of passes, run in the order they were made. A pass has
an id of its own (not a T1_id).
*/

#define T1_PASS_CLEAR   0
#define T1_PASS_DRAW    1
#define T1_PASS_COMPUTE 2

#define T1_PASSES_CAP 64
#define T1_PASS_COLOR_TEXS_CAP 4

typedef struct {
    u32 color_tex[T1_PASS_COLOR_TEXS_CAP]; // [[color(i)]], T1Texs
    u32 params[T1_PARAMS_CAP]; // see T1_PARAMS_I
    u32 write_tex; // a compute pass's T1Tex
    u32 buf[T1_PASS_BUFS_CAP]; // T1Bufs, see T1_BUFS_I
    u32 depth_tex; // the T1Tex whose depth buffer a draw pass uses
    u32 vertex_start; // a draw pass's first vertex_id
    u32 vertex_count; // a draw pass's
    u32 threads[3]; // a compute pass's grid, 0 for write_tex's size
} T1PassU32s;

typedef struct {
    T1PassU32s u32s;
    f32 clear_rgba[4];
    u32 id;
    u32 pipeline; // its id, T1_ID_NONE for a clear pass
    u8  kind; // T1_PASS_*
} T1Pass;

extern T1Pass * T1_passes;
extern u32 T1_passes_size;

void T1_pass_init(void);

u32 T1_pass_new_clear(T1Tex tex, f32 r, f32 g, f32 b, f32 a);
u32 T1_pass_new_draw(u32 pipeline_id);
u32 T1_pass_new_compute(u32 pipeline_id);

// the index of the pass with this id, or -1
s32  T1_pass_id_to_i(u32 id);
// the passes after it move up, so the order stays the same
void T1_pass_delete(u32 id);
b8   T1_pass_exists(u32 id);
void T1_pass_set_u32(u32 pass, const char * property, u32 value);

#endif // T1_PASS_H
