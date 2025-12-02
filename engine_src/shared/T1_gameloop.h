#ifndef T1_GAMELOOP_H
#define T1_GAMELOOP_H

#include "T1_terminal.h"
#include "T1_logger.h"
#include "T1_engineglobals.h"
#include "T1_cpu_gpu_shared_types.h"
#include "T1_io.h"
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

extern bool32_t T1_gameloop_active;
extern bool32_t T1_loading_textures;

void T1_gameloop_init(void);

void T1_gameloop_update_before_render_pass(T1GPUFrame * frame_data);
void T1_gameloop_update_after_render_pass(void);
// client_logic_after_copy

#ifdef __cplusplus
}
#endif

#endif // T1_GAMELOOP_H

