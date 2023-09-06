#ifndef GAMELOOP_H
#define GAMELOOP_H

#include "terminal.h"
#include "logger.h"
#include "window_size.h"
#include "cpu_gpu_shared_types.h"
#include "platform_layer.h"
#include "texture_array.h"
#include "text.h"
#include "uielement.h"
#include "renderer.h"
#include "lightsource.h"
#include "scheduled_animations.h"
#include "clientlogic.h"


#ifdef __cplusplus
extern "C" {
#endif

void shared_gameloop_init(void);

void shared_gameloop_update(
    GPUVertex * vertices_for_gpu,
    uint32_t * vertices_for_gpu_size,
    GPULightCollection * lights_for_gpu,
    GPUCamera * camera_for_gpu,
    GPUProjectionConstants * projection_constants_for_gpu);

#ifdef __cplusplus
}
#endif

#endif // GAMELOOP_H
