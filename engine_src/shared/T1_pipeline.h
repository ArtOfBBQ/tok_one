#ifndef T1_PIPELINE_H
#define T1_PIPELINE_H

#include "T1_stdint.h"

/*
A pipeline is a shader function (or a vertex and fragment function pair)
from the client's shader source, referenced by name, plus the state that
can't change after creation. Pipelines have a T1_id but no properties, and
can't be animated or merged.
*/

// how a draw pipeline's color output combines with what's already drawn
#define T1_BLEND_NONE     0
// adds the output to what's there, so black adds nothing
#define T1_BLEND_ADDITIVE 1
// the output covers what's there by its alpha, its rgb must be
// premultiplied by that alpha
#define T1_BLEND_PREMULTIPLIED_ALPHA 2
// the output covers what's there by its alpha, its rgb is not premultiplied
#define T1_BLEND_STRAIGHT_ALPHA 3

// which triangles a draw pipeline skips
#define T1_CULL_NONE  0
#define T1_CULL_BACK  1
#define T1_CULL_FRONT 2

#define T1_PIPELINES_CAP 64
#define T1_PIPELINE_FUNC_NAME_CAP 64

typedef struct {
    // the kernel function's name for a compute pipeline
    c8  vertex_fn[T1_PIPELINE_FUNC_NAME_CAP];
    c8  fragment_fn[T1_PIPELINE_FUNC_NAME_CAP];
    u32 T1_id; // T1_ID_NONE for an unused slot
    u8  is_compute;
    u8  blend;
    u8  cull;
} T1Pipeline;

extern T1Pipeline * T1_pipelines;
extern u32 T1_pipelines_size;

void T1_pipeline_init(void);

/*
*good is 0 if a function doesn't exist in the loaded shaders. Before any
shaders are loaded, the names are checked when they are.
*/
u32 T1_pipeline_new_draw(
    const c8 * vertex_fn,
    const c8 * fragment_fn,
    u8 blend,
    u8 cull,
    u8 * good);
u32 T1_pipeline_new_compute(
    const c8 * kernel_fn,
    u8 * good);

// the index of the pipeline with this T1_id, or -1
s32  T1_pipeline_T1_id_to_i(u32 T1_id);
void T1_pipeline_delete(u32 T1_id);

#endif // T1_PIPELINE_H
