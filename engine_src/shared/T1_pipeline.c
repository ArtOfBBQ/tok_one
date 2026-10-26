#include "T1_pipeline.h"

#include "T1_types_public.h"
#include "T1_std.h"
#include "T1_assert.h"
#include "T1_mem.h"
#include "T1_id.h"
#include "T1_os.h"

T1Pipeline * T1_pipelines = NULL;
u32 T1_pipelines_size = 0;

static s32 T1_pipeline_fetch_next(void) {
    for (u32 i = 0; i < T1_pipelines_size; i++) {
        if (T1_pipelines[i].T1_id == T1_ID_NONE) { return (s32)i; }
    }

    T1_assert(T1_pipelines_size < T1_PIPELINES_CAP);
    if (T1_pipelines_size >= T1_PIPELINES_CAP) { return -1; }

    T1_pipelines_size += 1;
    return (s32)T1_pipelines_size - 1;
}

static u32 T1_pipeline_new(
    const c8 * vertex_fn,
    const c8 * fragment_fn,
    u8 is_compute,
    u8 blend,
    u8 cull,
    u8 * good)
{
    *good = 0;

    T1_assert(vertex_fn != NULL);
    T1_assert(vertex_fn[0] != '\0');
    T1_assert(
        T1_std_strlen(vertex_fn) < T1_PIPELINE_FUNC_NAME_CAP);
    T1_assert(
        is_compute ||
        T1_std_strlen(fragment_fn) < T1_PIPELINE_FUNC_NAME_CAP);
    T1_assert(blend <= T1_BLEND_STRAIGHT_ALPHA);
    T1_assert(cull <= T1_CULL_FRONT);

    s32 i = T1_pipeline_fetch_next();
    if (i < 0) { return T1_ID_NONE; }

    T1Pipeline * p = T1_pipelines + i;
    T1_std_memset(p, 0, sizeof(T1Pipeline));
    T1_std_strcpy_cap(p->vertex_fn, T1_PIPELINE_FUNC_NAME_CAP, vertex_fn);
    if (!is_compute) {
        T1_assert(fragment_fn != NULL);
        T1_assert(fragment_fn[0] != '\0');
        T1_std_strcpy_cap(
            p->fragment_fn, T1_PIPELINE_FUNC_NAME_CAP, fragment_fn);
    }
    p->is_compute = is_compute;
    p->blend = blend;
    p->cull = cull;
    p->T1_id = T1_id_next_nonui_id();

    T1_os_gpu_pipeline_added(i, good);
    if (!*good) {
        p->T1_id = T1_ID_NONE;
        return T1_ID_NONE;
    }

    return p->T1_id;
}

u32 T1_pipeline_new_draw(
    const c8 * vertex_fn,
    const c8 * fragment_fn,
    u8 blend,
    u8 cull,
    u8 * good)
{
    return T1_pipeline_new(vertex_fn, fragment_fn, 0, blend, cull, good);
}

u32 T1_pipeline_new_compute(
    const c8 * kernel_fn,
    u8 * good)
{
    return T1_pipeline_new(
        kernel_fn, NULL, 1, T1_BLEND_NONE, T1_CULL_NONE, good);
}

s32 T1_pipeline_T1_id_to_i(u32 T1_id) {
    if (T1_id == T1_ID_NONE) { return -1; }

    for (u32 i = 0; i < T1_pipelines_size; i++) {
        if (T1_pipelines[i].T1_id == T1_id) { return (s32)i; }
    }

    return -1;
}

void T1_pipeline_delete(u32 T1_id) {
    s32 i = T1_pipeline_T1_id_to_i(T1_id);
    if (i < 0) { return; }

    T1_os_gpu_pipeline_deleted(i);
    T1_pipelines[i].T1_id = T1_ID_NONE;
}

void T1_pipeline_init(void) {
    T1_pipelines = T1_mem_malloc_unmanaged(
        sizeof(T1Pipeline) * T1_PIPELINES_CAP);
    for (u32 i = 0; i < T1_PIPELINES_CAP; i++) {
        T1_pipelines[i].T1_id = T1_ID_NONE;
    }
    T1_pipelines_size = 0;
}
