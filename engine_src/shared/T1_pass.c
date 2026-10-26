#include "T1_pass.h"

#include "T1_types_public.h"
#include "T1_std.h"
#include "T1_assert.h"
#include "T1_mem.h"
#include "T1_id.h"
#include "T1_pipeline.h"

T1Pass * T1_passes = NULL;
u32 T1_passes_size = 0;

void T1_pass_init(void) {
    T1_passes = T1_mem_malloc_unmanaged(sizeof(T1Pass) * T1_PASSES_CAP);
    T1_passes_size = 0;
}

static T1Pass * T1_pass_new(u8 kind, u32 pipeline_T1_id) {
    T1_assert(T1_passes_size < T1_PASSES_CAP);
    if (T1_passes_size >= T1_PASSES_CAP) { return NULL; }

    if (kind != T1_PASS_CLEAR) {
        s32 pl_i = T1_pipeline_T1_id_to_i(pipeline_T1_id);
        T1_assert(pl_i >= 0);
        if (pl_i < 0) { return NULL; }
        T1_assert(
            T1_pipelines[pl_i].is_compute == (kind == T1_PASS_COMPUTE));
    }

    T1Pass * p = T1_passes + T1_passes_size;
    T1_passes_size += 1;

    T1_std_memset(p, 0, sizeof(T1Pass));
    p->u32s.cam = T1_ID_NONE;
    for (u32 i = 0; i < T1_PASS_COLOR_TEXS_CAP; i++) {
        p->u32s.color_tex[i] = T1_TEX_NONE;
    }
    for (u32 i = 0; i < T1_READ_TEXS_CAP; i++) {
        p->u32s.read_tex[i] = T1_TEX_NONE;
    }
    p->u32s.write_tex = T1_TEX_NONE;
    p->u32s.depth_tex = T1_TEX_NONE;
    for (u32 i = 0; i < T1_PASS_BUFS_CAP; i++) {
        p->u32s.buf[i] = T1_BUF_NONE;
    }
    p->kind = kind;
    p->pipeline = pipeline_T1_id;
    p->T1_id = T1_id_next_nonui_id();

    return p;
}

u32 T1_pass_new_clear(T1Tex tex, f32 r, f32 g, f32 b, f32 a) {
    T1_assert(tex != T1_TEX_NONE);

    T1Pass * p = T1_pass_new(T1_PASS_CLEAR, T1_ID_NONE);
    if (p == NULL) { return T1_ID_NONE; }

    p->u32s.color_tex[0] = tex;
    p->clear_rgba[0] = r;
    p->clear_rgba[1] = g;
    p->clear_rgba[2] = b;
    p->clear_rgba[3] = a;

    return p->T1_id;
}

u32 T1_pass_new_draw(u32 pipeline_T1_id, u32 sources) {
    T1_assert(sources != 0);
    // these use different vertex data, so they can't share a pass
    T1_assert(
        (sources & ~T1_DRAW_ZSPRITES) == 0 ||
        sources == T1_DRAW_TEXQUADS ||
        sources == T1_DRAW_BILLBOARDS ||
        sources == T1_DRAW_FULLSCREEN ||
        sources == T1_DRAW_PROCEDURAL);

    T1Pass * p = T1_pass_new(T1_PASS_DRAW, pipeline_T1_id);
    if (p == NULL) { return T1_ID_NONE; }

    p->sources = sources;

    return p->T1_id;
}

u32 T1_pass_new_compute(u32 pipeline_T1_id) {
    T1Pass * p = T1_pass_new(T1_PASS_COMPUTE, pipeline_T1_id);
    if (p == NULL) { return T1_ID_NONE; }

    return p->T1_id;
}

s32 T1_pass_T1_id_to_i(u32 T1_id) {
    if (T1_id == T1_ID_NONE) { return -1; }

    for (u32 i = 0; i < T1_passes_size; i++) {
        if (T1_passes[i].T1_id == T1_id) { return (s32)i; }
    }

    return -1;
}

void T1_pass_delete(u32 T1_id) {
    s32 i = T1_pass_T1_id_to_i(T1_id);
    if (i < 0) { return; }

    for (u32 j = (u32)i; j + 1 < T1_passes_size; j++) {
        T1_passes[j] = T1_passes[j + 1];
    }
    T1_passes_size -= 1;
}

u32 T1_pass_set_property(
    const u32 T1_id,
    const T1Property * property,
    const u32 bits)
{
    s32 i = T1_pass_T1_id_to_i(T1_id);
    if (i < 0) { return 0; }

    T1_assert(property->store == T1_PROP_STORE_PASS_U32);
    T1_property_write(&T1_passes[i].u32s, property, bits);

    return 1;
}
