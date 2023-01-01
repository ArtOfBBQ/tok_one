#ifndef GAMELOOP_H
#define GAMELOOP_H

#include "logger.h"
#include "window_size.h"
#include "cpu_gpu_shared_types.h"
#include "platform_layer.h"
#include "texture_array.h"
#include "text.h"
#include "bitmap_renderer.h"
#include "renderer.h"
#include "lightsource.h"
#include "scheduled_animations.h"
#include "clientlogic.h"


#ifdef __cplusplus
extern "C" {
#endif

void shared_gameloop_update(
    GPU_Vertex * vertices_for_gpu,
    uint32_t * vertices_for_gpu_size,
    GPU_LightCollection * lights_for_gpu,
    GPU_Camera * camera_for_gpu,
    GPU_ProjectionConstants * projection_constants_for_gpu);

#ifdef __cplusplus
}
#endif

#endif // GAMELOOP_H

