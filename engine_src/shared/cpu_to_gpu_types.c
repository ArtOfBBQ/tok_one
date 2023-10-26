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
        
        if (vertices[i].texturearray_i >= TEXTUREARRAYS_SIZE) 
        {
            printf(
                "(engine check) corrupted texturearray_i: %u, "
                "texture_i %i and texturearray_i %i\n",
                i,
                vertices[i].texture_i,
                vertices[i].texturearray_i);
            assert(0);
        }
        
        if (vertices[i].texture_i >= MAX_FILES_IN_SINGLE_TEXARRAY)
        {
            printf(
                "(engine check) corrupted texturearray_i: %u"
                ", texture_i %i and texturearray_i %i\n",
                i,
                vertices[i].texture_i,
                vertices[i].texturearray_i);
            assert(0);
        }
    }
}
#endif

