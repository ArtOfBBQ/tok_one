#include "T1_pass.h"

#include "T1_types_public.h"
#include "T1_std.h"
#include "T1_assert.h"
#include "T1_mem.h"
#include "T1_pipeline.h"

T1Pass * T1_passes = NULL;
u32 T1_passes_size = 0;
// ids are never reused
static u32 T1_pass_next_id = 1;

void T1_pass_init(void) {
    T1_passes = T1_mem_malloc_unmanaged(sizeof(T1Pass) * T1_PASSES_CAP);
    T1_passes_size = 0;
}

static T1Pass * T1_pass_new(u8 kind, u32 pipeline_id) {
    T1_assert(T1_passes_size < T1_PASSES_CAP);
    if (T1_passes_size >= T1_PASSES_CAP) { return NULL; }

    if (kind != T1_PASS_CLEAR) {
        s32 pl_i = T1_pipeline_id_to_i(pipeline_id);
        T1_assert(pl_i >= 0);
        if (pl_i < 0) { return NULL; }
        T1_assert(
            T1_pipelines[pl_i].is_compute == (kind == T1_PASS_COMPUTE));
    }

    T1Pass * p = T1_passes + T1_passes_size;
    T1_passes_size += 1;

    T1_std_memset(p, 0, sizeof(T1Pass));
    for (u32 i = 0; i < T1_PASS_COLOR_TEXS_CAP; i++) {
        p->u32s.color_tex[i] = T1_TEX_NONE;
    }
    p->u32s.write_tex = T1_TEX_NONE;
    p->u32s.depth_tex = T1_TEX_NONE;
    for (u32 i = 0; i < T1_PASS_BUFS_CAP; i++) {
        p->u32s.buf[i] = T1_BUF_NONE;
    }
    p->kind = kind;
    p->pipeline = pipeline_id;
    p->id = T1_pass_next_id++;

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

    return p->id;
}

u32 T1_pass_new_draw(u32 pipeline_id) {
    T1Pass * p = T1_pass_new(T1_PASS_DRAW, pipeline_id);
    if (p == NULL) { return T1_ID_NONE; }

    return p->id;
}

u32 T1_pass_new_compute(u32 pipeline_id) {
    T1Pass * p = T1_pass_new(T1_PASS_COMPUTE, pipeline_id);
    if (p == NULL) { return T1_ID_NONE; }

    return p->id;
}

s32 T1_pass_id_to_i(u32 id) {
    if (id == T1_ID_NONE) { return -1; }

    for (u32 i = 0; i < T1_passes_size; i++) {
        if (T1_passes[i].id == id) { return (s32)i; }
    }

    return -1;
}

b8 T1_pass_exists(u32 id) {
    return T1_pass_id_to_i(id) >= 0;
}

void T1_pass_delete(u32 id) {
    s32 i = T1_pass_id_to_i(id);
    if (i < 0) { return; }

    for (u32 j = (u32)i; j + 1 < T1_passes_size; j++) {
        T1_passes[j] = T1_passes[j + 1];
    }
    T1_passes_size -= 1;
}

// A pass property's name and where its u32 is in T1PassU32s
typedef struct {
    const char * name;
    u32 offset;
} T1PassProperty;

#define T1_PASS_PROP(name, field) { name, offsetof(T1PassU32s, field) }
static const T1PassProperty T1_pass_properties[] = {
    T1_PASS_PROP("color_tex[0]", color_tex[0]),
    T1_PASS_PROP("color_tex[1]", color_tex[1]),
    T1_PASS_PROP("color_tex[2]", color_tex[2]),
    T1_PASS_PROP("color_tex[3]", color_tex[3]),
    T1_PASS_PROP("params[0]", params[0]),
    T1_PASS_PROP("params[1]", params[1]),
    T1_PASS_PROP("params[2]", params[2]),
    T1_PASS_PROP("params[3]", params[3]),
    T1_PASS_PROP("params[4]", params[4]),
    T1_PASS_PROP("params[5]", params[5]),
    T1_PASS_PROP("params[6]", params[6]),
    T1_PASS_PROP("params[7]", params[7]),
    T1_PASS_PROP("write_tex", write_tex),
    T1_PASS_PROP("buf[0]", buf[0]),
    T1_PASS_PROP("buf[1]", buf[1]),
    T1_PASS_PROP("buf[2]", buf[2]),
    T1_PASS_PROP("buf[3]", buf[3]),
    T1_PASS_PROP("buf[4]", buf[4]),
    T1_PASS_PROP("buf[5]", buf[5]),
    T1_PASS_PROP("buf[6]", buf[6]),
    T1_PASS_PROP("buf[7]", buf[7]),
    T1_PASS_PROP("buf[8]", buf[8]),
    T1_PASS_PROP("buf[9]", buf[9]),
    T1_PASS_PROP("buf[10]", buf[10]),
    T1_PASS_PROP("buf[11]", buf[11]),
    T1_PASS_PROP("depth_tex", depth_tex),
    T1_PASS_PROP("vertex_start", vertex_start),
    T1_PASS_PROP("vertex_count", vertex_count),
    T1_PASS_PROP("threads[0]", threads[0]),
    T1_PASS_PROP("threads[1]", threads[1]),
    T1_PASS_PROP("threads[2]", threads[2]),
};
#define T1_PASS_PROPERTIES_SIZE \
    (sizeof(T1_pass_properties) / sizeof(T1_pass_properties[0]))

void T1_pass_set_u32(u32 pass, const char * property, u32 value) {
    s32 i = T1_pass_id_to_i(pass);
    T1_assert(i >= 0);
    if (i < 0) { return; }
    
    for (u32 j = 0; j < T1_PASS_PROPERTIES_SIZE; j++) {
        if (T1_std_are_equal_strings(T1_pass_properties[j].name, property)) {
            *(u32 *)((u8 *)&T1_passes[i].u32s +
                T1_pass_properties[j].offset) = value;
            return;
        }
    }
    
    char msg[128];
    T1_std_strcpy_cap(msg, 128, "Unknown T1 pass property: ");
    T1_std_strcat_cap(msg, 128, property);
    T1_crash(msg);
}
