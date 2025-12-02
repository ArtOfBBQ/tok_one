#ifndef INIT_APPLICATION_H
#define INIT_APPLICATION_H

#include <string.h> // strlcat

#include "T1_std.h"
#include "T1_meta.h"
#include "T1_logger.h"
#include "T1_profiler.h"
#include "T1_random.h"
#include "T1_audio.h"
#include "T1_lightsource.h"
#include "T1_text.h"
#include "T1_uielement.h"
#include "T1_io.h"
#include "T1_engineglobals.h"
#include "T1_scheduled_animations.h"
#include "T1_renderer.h"
#include "T1_gameloop.h"
#include "T1_tokenizer.h"
#include "T1_mtlparser.h"
#include "T1_objparser.h"

#include "T1_clientlogic.h"

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t block_drawinmtkview;

void T1_appinit_before_gpu_init(
    bool32_t * success,
    char * error_message);
void T1_appinit_after_gpu_init(int32_t throwaway_threadarg);

void T1_appinit_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // INIT_APPLICATION_H
