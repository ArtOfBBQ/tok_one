#ifndef T1_GAMELOOP_H
#define T1_GAMELOOP_H

#include "T1_term.h"
#include "T1_logger.h"
#include "T1_global.h"
#include "T1_cpu_gpu_shared_types.h"
#include "T1_io.h"
#include "T1_platform_layer.h"
#include "T1_tex_array.h"
#include "T1_text.h"
#include "T1_ui_widget.h"
#include "T1_render.h"
#include "T1_zlight.h"
#include "T1_zsprite_anim.h"
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

