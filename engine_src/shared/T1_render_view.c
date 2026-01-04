#include "T1_render_view.h"

/*
render_views[0] is the global camera

In a 2D game, move the x to the left to move all of
your zsprites to the right

render_views[1] and up are alternative camera texture
outputs that you can render to, for example to make
a shadow map (set write_type to
T1RENDERVIEW_WRITE_DEPTH) or to make a reflection or
security camera (set write_type
T1RENDERVIEW_WRITE_RGBA)
*/
T1RenderViewCollection * T1_render_views = NULL;
T1CPURenderView * T1_camera = NULL;

void T1_render_view_init(void) {
    log_assert(T1_RENDER_VIEW_CAP > 0);
    T1_render_views = T1_mem_malloc_from_unmanaged(
        sizeof(T1RenderViewCollection));
    
    T1_camera = T1_render_views->cpu; // convenience
}

static void T1_render_view_construct(
    T1CPURenderView * cpu,
    T1GPURenderView * gpu)
{
    T1_std_memset(cpu, 0, sizeof(T1CPURenderView));
    T1_std_memset(gpu, 0, sizeof(T1GPURenderView));
    
    cpu->write_array_i = -1;
    cpu->write_slice_i = -1;
}

int32_t T1_render_view_fetch_next(void) {
    int32_t ret = -1;
    
    for (
        int32_t rv_i = 0;
        rv_i < (int32_t)T1_render_views->size;
        rv_i++)
    {
        if (T1_render_views->cpu[rv_i].deleted) {
            ret = rv_i;
        }
    }
    
    if (
        ret < 0 &&
        T1_render_views->size < T1_RENDER_VIEW_CAP)
    {
        ret = (int32_t)T1_render_views->size;
        T1_render_views->size += 1;
    } else {
        return ret;
    }
    
    log_assert(ret >= 0);
    log_assert(ret < T1_RENDER_VIEW_CAP);
    
    T1_render_view_construct(
        T1_render_views->cpu + ret,
        T1_render_views->gpu + ret);
    
    return ret;
}

void T1_render_view_delete(const int32_t rv_i) {
    log_assert(rv_i >= 0);
    log_assert(rv_i < T1_RENDER_VIEW_CAP);
    
    T1_render_view_construct(
        T1_render_views->cpu,
        T1_render_views->gpu);
    T1_render_views->cpu[rv_i].deleted = true;
    
    while (
        T1_render_views->size > 0 &&
        T1_render_views->cpu[T1_render_views->size-1].
            deleted)
    {
        T1_render_views->size -= 1;
    }
}

void T1_render_view_validate(void) {
    log_assert(T1_render_views != NULL);
    log_assert(
        T1_render_views->cpu[0].write_type ==
            T1RENDERVIEW_WRITE_RENDER_TARGET);
    log_assert(
        T1_render_views->size > 0);
    log_assert(
        T1_render_views->size <= T1_RENDER_VIEW_CAP);
    
    for (
        uint32_t i = 1;
        i < T1_render_views->size;
        i++)
    {
        if (T1_render_views->cpu[i].deleted) {
            continue;
        }
        
        log_assert(
            T1_render_views->cpu[i].write_type >
                T1RENDERVIEW_WRITE_BELOWBOUNDS);
        log_assert(
            T1_render_views->cpu[i].write_type <
                T1RENDERVIEW_WRITE_ABOVEBOUNDS);
        log_assert(
            T1_render_views->cpu[i].write_type !=
                T1RENDERVIEW_WRITE_RENDER_TARGET);
    }
    
    log_assert(!T1_render_views->cpu[T1_render_views->size-1].deleted);
}
