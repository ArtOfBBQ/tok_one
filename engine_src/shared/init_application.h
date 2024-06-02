#ifndef INIT_APPLICATION_H
#define INIT_APPLICATION_H

#include "common.h"
#include "logger.h"
#include "tok_random.h"
#include "audio.h"
#include "lightsource.h"
#include "text.h"
#include "uielement.h"
#include "userinput.h"
#include "window_size.h"
#include "scheduled_animations.h"
#include "renderer.h"
#include "gameloop.h"
#include "objparser.h"
#include "clientlogic_macro_settings.h"

#include "clientlogic.h"

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t block_drawinmtkview;

void init_application_before_gpu_init(void);
void init_application_after_gpu_init(void);

void shared_shutdown_application(void);

#ifdef __cplusplus
}
#endif

#endif // INIT_APPLICATION_H
