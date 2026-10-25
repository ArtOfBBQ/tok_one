#include "T1_texquad.h"

#include <stdlib.h> // TODO: stop using qsort

#if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
#include <math.h> // isnan check
#elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
#else
#error
#endif

#include "T1_mem.h"
#include "T1_simd.h"
#include "T1_tex.h"
#include "T1_log.h"
#include "T1_id.h"


#if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
static void assert_sanity_check_texquad_vals(
    T1GPUTexQuadf32 * gpu_f32,
    T1GPUTexQuadu32 * gpu_s32)
{
    (void)gpu_s32;
    
    if (gpu_f32) {
        T1_log_assert(!isnan(gpu_f32->xyz[0]));
        T1_log_assert(!isnan(gpu_f32->xyz[1]));
        T1_log_assert(!isnan(gpu_f32->xyz[2]));
        T1_log_assert(!isnan(gpu_f32->rgba[0]));
        T1_log_assert(!isnan(gpu_f32->rgba[1]));
        T1_log_assert(!isnan(gpu_f32->rgba[2]));
        T1_log_assert(!isnan(gpu_f32->rgba[3]));
        T1_log_assert(!isnan(gpu_f32->wh[0]));
        T1_log_assert(!isnan(gpu_f32->wh[1]));
    }
}

#elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
#define assert_sanity_check_texquad_vals(x, y)
#define assert_sanity_check_texquad_vals_by_id(id)
#else
#error
#endif

typedef struct {
    T1CPUTexQuad cpu[T1_MAX_FLATQUADS_PER_BUFFER];
    T1GPUTexQuad gpu[T1_MAX_FLATQUADS_PER_BUFFER];
    s32 size;
} T1FlatTexQuadCollection;

static T1FlatTexQuadCollection * T1_texquads = NULL;

void T1_texquad_construct(
    T1GPUTexQuadf32 * f32s,
    T1GPUTexQuadu32 * s32s)
{
    T1_std_memset(
        f32s,
        0,
        sizeof(T1GPUTexQuadf32));
    T1_std_memset(
        s32s,
        0,
        sizeof(T1GPUTexQuadu32));
    
    f32s->wh[0] = 0.25f;
    f32s->wh[1] = 0.25f;
    f32s->mul_wh[0] = 1.0f;
    f32s->mul_wh[1] = 1.0f;
    f32s->rgba[0] = 1.0f;
    f32s->rgba[1] = 1.0f;
    f32s->rgba[2] = 1.0f;
    f32s->rgba[3] = 1.0f;
    s32s->reserved_and_tex = T1_TEX_NONE;
    s32s->touch_id = T1_TOUCH_ID_NONE;
    s32s->layers = 1;
}

static void T1_texquad_construct_at_i(
    const s32 i)
{
    T1_log_assert(i >= 0);
    T1_log_assert(i  < T1_MAX_FLATQUADS_PER_BUFFER);
    
    T1_std_memset(
        &T1_texquads->cpu[i],
        0,
        sizeof(T1CPUTexQuad));
    
    T1_texquads->cpu[i].T1_id = T1_ID_NONE;
    
    T1_texquad_construct(
        &T1_texquads->gpu[i].f32s,
        &T1_texquads->gpu[i].u32s);
}

void T1_texquad_init(void) {
    T1_texquads =
        T1_mem_malloc_unmanaged(
            sizeof(T1FlatTexQuadCollection));
    
    T1_std_memset(
        T1_texquads,
        0,
        sizeof(T1FlatTexQuadCollection));
}

void T1_texquad_delete(u32 T1_id)
{
    for (
        s32 i = T1_texquads->size-1;
        i >= 0;
        i--)
    {
        if (T1_texquads->cpu[i].T1_id == T1_id)
        {
            T1_texquads->cpu[i].deleted = 1;
            T1_texquads->cpu[i].T1_id = T1_ID_NONE;
        }
    }
    
    while (
        T1_texquads->size > 0 &&
        T1_texquads->cpu[T1_texquads->size-1].deleted)
    {
        T1_texquads->size -= 1;
    }
}

void T1_texquad_get_avg_xyz(
    f32 * recip_xyz,
    u32 T1_id,
    b8 * found)
{
    *found = 0;
    f32 count = 0.0f;
    recip_xyz[0] = 0.0f;
    recip_xyz[1] = 0.0f;
    recip_xyz[2] = 0.0f;
    
    for (
        s32 tq_i = 0;
        tq_i < (s32)T1_texquads->size;
        tq_i++)
    {
        if (
            T1_texquads->cpu[tq_i].T1_id ==
                T1_id)
        {
            count += 1.0f;
            recip_xyz[0] +=
                T1_texquads->gpu[tq_i].f32s.xyz[0];
            recip_xyz[1] +=
                T1_texquads->gpu[tq_i].f32s.xyz[1];
            recip_xyz[2] +=
                T1_texquads->gpu[tq_i].f32s.xyz[2];
        }
    }
    
    if (count > 0.0f) {
        recip_xyz[0] /= count;
        recip_xyz[1] /= count;
        recip_xyz[2] /= count;
    }
}

void T1_texquad_delete_all(void)
{
    T1_texquads->size = 0;
}

u32 T1_texquad_new(void)
{
    T1TexQuadRequest request;
    T1_texquad_fetch_next(&request);
    request.cpu->T1_id = T1_id_next_nonui_id();
    // xyz[2] is the distance and also the depth, which must be in (0, 1)
    request.gpu->f32s.xyz[2] = 0.5f;
    T1_texquad_commit(&request);
    return request.cpu->T1_id;
}

static void * T1_texquad_store_struct(
    const s32 i,
    const u8 store)
{
    switch (store) {
        case T1_PROP_STORE_TQ_CPU_F32:
            return &T1_texquads->cpu[i];
        case T1_PROP_STORE_TQ_GPU_F32:
            return &T1_texquads->gpu[i].f32s;
        case T1_PROP_STORE_TQ_GPU_U32:
            return &T1_texquads->gpu[i].u32s;
        default:
            T1_log_assert(0);
            return NULL;
    }
}

u32 T1_texquad_set_property(
    u32 T1_id,
    const T1Property * property,
    u32 bits)
{
    u32 count = 0;
    
    for (s32 i = 0; i < T1_texquads->size; i++) {
        if (
            T1_texquads->cpu[i].T1_id != T1_id ||
            T1_texquads->cpu[i].deleted)
        {
            continue;
        }
        
        T1_property_write(
            T1_texquad_store_struct(i, property->store),
            property,
            bits);
        assert_sanity_check_texquad_vals(
            &T1_texquads->gpu[i].f32s,
            &T1_texquads->gpu[i].u32s);
        count += 1;
    }
    
    return count;
}

void T1_texquad_rename_T1_id(u32 from_T1_id, u32 to_T1_id)
{
    for (s32 i = 0; i < T1_texquads->size; i++) {
        if (T1_texquads->cpu[i].T1_id == from_T1_id) {
            T1_texquads->cpu[i].T1_id = to_T1_id;
        }
    }
}

void T1_texquad_fetch_next(
    T1TexQuadRequest * request)
{
    s32 ret_i = -1;
    
    for (
        s32 i = 0;
        i < (s32)T1_texquads->size;
        i++)
    {
        if (
            T1_texquads->cpu[i].deleted)
        {
            ret_i = i;
        }
    }
    
    if (ret_i < 0) {
        ret_i = T1_texquads->size;
        T1_texquads->size += 1;
        
        T1_log_assert(T1_texquads->size > 0);
        T1_log_assert(T1_texquads->size <
            T1_MAX_FLATQUADS_PER_BUFFER);
    }
    
    T1_log_assert(ret_i >= 0);
    
    T1_texquad_construct_at_i(ret_i);
    
    request->cpu = T1_texquads->cpu + ret_i;
    request->gpu = T1_texquads->gpu + ret_i;
}

void T1_texquad_defragment(void) {
    s32 i = 0;
    s32 j = (s32)T1_texquads->size-1;
    
    while (i < j) {
        if (!T1_texquads->cpu[i].deleted) {
            i++;
            continue;
        }
        
        if (T1_texquads->cpu[i].deleted) {
            T1_texquads->cpu[i] = T1_texquads->cpu[j];
            T1_texquads->gpu[i] = T1_texquads->gpu[j];
            T1_texquads->cpu[j].deleted = 1;
            j--;
        }
    }
    
    while (
        T1_texquads->size > 0 &&
        T1_texquads->cpu[T1_texquads->size-1].deleted)
    {
        T1_texquads->size -= 1;
    }
}

void T1_texquad_commit(
    T1TexQuadRequest * request)
{
    T1_log_assert(!request->cpu->deleted);
    T1_log_assert(request->gpu->f32s.wh[0] > 0.0f);
    T1_log_assert(request->gpu->f32s.wh[1] > 0.0f);
    // xyz is in world units, projected by the distance xyz[2]
    T1_log_assert(request->gpu->f32s.xyz[2] > 0.0f);
    T1_log_assert(!request->cpu->deleted);
    
    assert_sanity_check_texquad_vals(
        /* T1GPUTexQuadf32 * gpu_f32: */
            &request->gpu->f32s,
        /* T1GPUTexQuads32 * gpu_s32: */
            &request->gpu->u32s);
    
    request->cpu->committed = 1;
}

void T1_texquad_apply_anim_step(
    const u32 T1_id,
    const u8 store,
    const T1AnimStep * step)
{
    for (s32 i = 0; i < T1_texquads->size; i++) {
        if (
            T1_texquads->cpu[i].deleted ||
            (T1_id != T1_ANIM_HIT_EVERYTHING &&
                T1_texquads->cpu[i].T1_id != T1_id))
        {
            continue;
        }
        
        T1_property_apply_anim_step(T1_texquad_store_struct(i, store), step);
        assert_sanity_check_texquad_vals(
            &T1_texquads->gpu[i].f32s,
            &T1_texquads->gpu[i].u32s);
    }
}

static s32 cmp_highest_z_texquad(
    const void * a,
    const void * b)
{
    f32 fa = ((T1GPUTexQuad *)a)->f32s.xyz[2];
    f32 fb = ((T1GPUTexQuad *)b)->f32s.xyz[2];
    
    if (fb < fa) {
        return -1;
    } else if (fb > fa) {
        return 1;
    } else {
        return 0;
    }
}

void T1_texquad_copy_to_frame_data(
    T1GPUTexQuad * recip_fd,
    u32 * recip_fd_size)
{
    *recip_fd_size = 0;
    for (
        s32 i = 0;
        i < T1_texquads->size;
        i++)
    {
        if (
            T1_texquads->gpu[i].u32s.layers != 0 &&
            !T1_texquads->cpu[i].deleted &&
            T1_texquads->cpu[i].committed)
        {
            recip_fd[*recip_fd_size] =
                T1_texquads->gpu[i];
            
            recip_fd[*recip_fd_size].f32s.xyz[0] +=
                T1_texquads->cpu[i].offset_xyz[0];
            recip_fd[*recip_fd_size].f32s.xyz[1] +=
                T1_texquads->cpu[i].offset_xyz[1];
            recip_fd[*recip_fd_size].f32s.xyz[2] +=
                T1_texquads->cpu[i].offset_xyz[2];
            
            *recip_fd_size += 1;
        }
        
        if (T1_texquads->cpu[i].one_frame_only) {
            T1_texquads->cpu[i].deleted = 1;
        }
    }
    
    #if 1
    qsort(
        /* void *base: */
            recip_fd,
        /* u64 nel: */
            *recip_fd_size,
        /* u64 width: */
            sizeof(T1GPUTexQuad),
        /* s32 (* _Nonnull compar)(const void *, const void *): */
            cmp_highest_z_texquad);
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    for (
        u32 i = 0;
        i + 1 < *recip_fd_size;
        i++)
    {
        T1_log_assert(
            recip_fd[i].f32s.xyz[2] >= recip_fd[i+1].f32s.xyz[2]);
    }
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    #endif
}
