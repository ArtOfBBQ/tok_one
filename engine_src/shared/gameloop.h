#ifndef GAMELOOP_H
#define GAMELOOP_H

#include "logger.h"
#include "window_size.h"
#include "vertex_types.h"
#include "platform_layer.h"
#include "texture_array.h"
#include "text.h"
#include "bitmap_renderer.h"
#include "software_renderer.h"
#include "lightsource.h"
#include "scheduled_animations.h"
#include "clientlogic.h"


#ifdef __cplusplus
extern "C" {
#endif

void shared_gameloop_update(
    Vertex * vertices_for_gpu,
    uint32_t * vertices_for_gpu_size);

#ifdef __cplusplus
}
#endif

#endif // GAMELOOP_H

