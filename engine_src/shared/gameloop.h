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

void shared_gameloop_update(GPUDataForSingleFrame * frame_data);

#ifdef __cplusplus
}
#endif

#endif // GAMELOOP_H

