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
        log_assert(
            T1_render_views->cpu[i].write_type !=
                T1RENDERVIEW_WRITE_RENDER_TARGET);
    }
}
