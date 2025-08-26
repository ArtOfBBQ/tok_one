#ifndef GAMELOOP_H
#define GAMELOOP_H

#include "T1_terminal.h"
#include "T1_logger.h"
#include "T1_engine_globals.h"
#include "T1_cpu_gpu_shared_types.h"
#include "T1_userinput.h"
#include "T1_platform_layer.h"
#include "T1_texture_array.h"
#include "T1_text.h"
#include "T1_uielement.h"
#include "T1_renderer.h"
#include "T1_lightsource.h"
#include "T1_scheduled_animations.h"
#include "T1_clientlogic.h"
#include "T1_profiler.h"


#ifdef __cplusplus
extern "C" {
#endif

extern bool32_t gameloop_active;
extern bool32_t loading_textures;

void gameloop_init(void);

void gameloop_update_before_render_pass(GPUFrame * frame_data);
void gameloop_update_after_render_pass(void);
// client_logic_after_copy

#ifdef __cplusplus
}
#endif

#endif // GAMELOOP_H

