#include "T1_flat_texquad.h"

typedef struct {
    T1CPUTexQuad cpu[MAX_FLATQUADS_PER_BUFFER];
    T1GPUTexQuad gpu[MAX_FLATQUADS_PER_BUFFER];
    uint32_t size;
} T1FlatTexQuadCollection;

static T1FlatTexQuadCollection *
    T1_flat_texquads = NULL;
    
static void T1_flat_texquads_construct_at_i(
    const int32_t i)
{
    T1_std_memset(
        &T1_flat_texquads->cpu[i],
        0,
        sizeof(T1CPUTexQuad));
    T1_std_memset(
        &T1_flat_texquads->gpu[i],
        0,
        sizeof(T1GPUTexQuad));
    
    T1_flat_texquads->cpu[i].zsprite_id = -1;
    T1_flat_texquads->cpu[i].touch_id = -1;
    T1_flat_texquads->cpu[i].visible = 1;
    
    T1_flat_texquads->gpu[i].size_xy[0] = 0.25f;
    T1_flat_texquads->gpu[i].size_xy[1] = 0.25f;
    T1_flat_texquads->gpu[i].tex_array_i = -1;
    T1_flat_texquads->gpu[i].tex_slice_i = -1;
}

void T1_flat_texquad_init(void) {
    T1_flat_texquads =
        T1_mem_malloc_from_unmanaged(
            sizeof(T1FlatTexQuadCollection));
    
    T1_std_memset(
        T1_flat_texquads,
        0,
        sizeof(T1FlatTexQuadCollection));
}

void T1_flat_texquad_delete_all(void)
{
    T1_flat_texquads->size = 0;
}

void T1_flat_texquad_fetch_next(
    T1FlatTexQuadRequest * request)
{
    int32_t ret_i = -1;
    
    for (
        int32_t i = 0;
        i < (int32_t)T1_flat_texquads->size;
        i++)
    {
        if (
            T1_flat_texquads->cpu[i].deleted)
        {
            ret_i = i;
        }
    }
    
    if (ret_i < 0) {
        ret_i = (int32_t)T1_flat_texquads->size;
        T1_flat_texquads->size += 1;
        
        log_assert(
            T1_flat_texquads->size <
                MAX_FLATQUADS_PER_BUFFER);
    }
    
    T1_flat_texquads_construct_at_i(ret_i);
    
    request->cpu = T1_flat_texquads->cpu + ret_i;
    request->gpu = T1_flat_texquads->gpu + ret_i;
}

void T1_flat_texquad_commit(
    T1FlatTexQuadRequest * request)
{
    log_assert(!request->cpu->deleted);
    log_assert(request->gpu->size_xy[0] > 0.0f);
    log_assert(request->gpu->size_xy[1] > 0.0f);
    log_assert(request->gpu->tex_array_i > -2);
    log_assert(request->gpu->tex_slice_i > -2);
    log_assert(
        request->gpu->tex_array_i < TEXTUREARRAYS_SIZE);
    
    request->cpu->committed = 1;
}

void T1_flat_texquad_copy_to_frame_data(
    T1GPUTexQuad * recip_frame_data,
    uint32_t * recip_frame_data_size)
{
    *recip_frame_data_size = 0;
    for (
        uint32_t i = 0;
        i < T1_flat_texquads->size;
        i++)
    {
        if (
            T1_flat_texquads->cpu[i].visible &&
            !T1_flat_texquads->cpu[i].deleted &&
            T1_flat_texquads->cpu[i].committed)
        {
            recip_frame_data[*recip_frame_data_size] =
                T1_flat_texquads->gpu[i];
            *recip_frame_data_size += 1;
        }
    }
}
