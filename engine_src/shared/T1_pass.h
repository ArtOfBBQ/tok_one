#ifndef T1_PASS_H
#define T1_PASS_H

#include "T1_stdint.h"
#include "T1_types_gpucpu.h"
#include "T1_types_public.h"
#include "T1_property.h"

/*
A frame is the list of passes, run in the order they were made. A pass has
a T1_id but can't be animated or merged.
*/

#define T1_PASS_CLEAR   0
#define T1_PASS_DRAW    1
#define T1_PASS_COMPUTE 2

// what a draw pass draws, combine them with |
#define T1_DRAW_OPAQUE     0x01 // zsprites without alpha_on or bloom_on
#define T1_DRAW_BLENDED    0x02 // zsprites with alpha_on
#define T1_DRAW_BLOOMING   0x04 // zsprites with bloom_on
#define T1_DRAW_TEXQUADS   0x08
#define T1_DRAW_BILLBOARDS 0x10
// 6 vertices for a quad that covers the targets, for post-processing
#define T1_DRAW_FULLSCREEN 0x20
// "vertex_count" vertices the client's vertex function makes up itself
#define T1_DRAW_PROCEDURAL 0x40
#define T1_DRAW_ZSPRITES (T1_DRAW_OPAQUE | T1_DRAW_BLENDED | T1_DRAW_BLOOMING)

#define T1_PASSES_CAP 64
#define T1_PASS_COLOR_TEXS_CAP 4

typedef struct {
    u32 cam; // a draw pass's camera's T1_id, T1_ID_NONE for no depth
    u32 color_tex[T1_PASS_COLOR_TEXS_CAP]; // [[color(i)]], T1Texs
    u32 read_tex[T1_READ_TEXS_CAP]; // T1Texs, see T1_READ_TEXS_BUFFER_I
    u32 write_tex; // a compute pass's T1Tex
    u32 buf[T1_PASS_BUFS_CAP]; // T1Bufs, see T1_BUFS_I
    u32 vertex_count; // a T1_DRAW_PROCEDURAL draw pass's
    u32 threads[3]; // a compute pass's grid, 0 for write_tex's size
} T1PassU32s;

typedef struct {
    T1PassU32s u32s;
    f32 clear_rgba[4];
    u32 T1_id;
    u32 pipeline; // its T1_id, T1_ID_NONE for a clear pass
    u32 sources; // T1_DRAW_* bits of a draw pass
    u8  kind; // T1_PASS_*
} T1Pass;

extern T1Pass * T1_passes;
extern u32 T1_passes_size;

void T1_pass_init(void);

u32 T1_pass_new_clear(T1Tex tex, f32 r, f32 g, f32 b, f32 a);
u32 T1_pass_new_draw(u32 pipeline_T1_id, u32 sources);
u32 T1_pass_new_compute(u32 pipeline_T1_id);

// the index of the pass with this T1_id, or -1
s32  T1_pass_T1_id_to_i(u32 T1_id);
// the passes after it move up, so the order stays the same
void T1_pass_delete(u32 T1_id);
u32  T1_pass_set_property(
    const u32 T1_id,
    const T1Property * property,
    const u32 bits);

#endif // T1_PASS_H
