#define SHARED_APPLE_PLATFORM

#include "../shared/platform_layer.h"

void platform_update_gpu_texture(
    int32_t texturearray_i,
    int32_t texture_i,
    DecodedImage * with_img)
{
    [apple_gpu_delegate
        updateTextureArray: texturearray_i
        atSlice: texture_i
        withImg: with_img];
}

