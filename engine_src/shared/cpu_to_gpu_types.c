#include "cpu_to_gpu_types.h"

#ifndef LOGGER_IGNORE_ASSERTS
void validate_framedata(
    GPUVertex * vertices,
    uint32_t vertices_size)
{
    for (
        uint32_t i = 0;
        i < vertices_size;
        i++)
    {
        assert(vertices[i].uv[0] > -0.1f);
        assert(vertices[i].uv[1] > -0.1f);
        assert(vertices[i].uv[0] <  1.05f);
        assert(vertices[i].uv[1] <  1.05f);
        assert(vertices[i].texturearray_i < TEXTUREARRAYS_SIZE);
        assert(vertices[i].texture_i < MAX_FILES_IN_SINGLE_TEXARRAY);
    }
}
#endif
