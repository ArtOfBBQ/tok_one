#ifndef INIT_APPLICATION_H
#define INIT_APPLICATION_H

#include "shared/common.h"
#include "shared/logger.h"
#include "shared/tok_random.h"
#include "shared/lightsource.h"
#include "shared/text.h"
#include "shared/uielement.h"
#include "shared/userinput.h"
#include "shared/window_size.h"
#include "shared/scheduled_animations.h"
#include "shared/renderer.h"
#include "shared/gameloop.h"
#include "shared/objparser.h"
#include "clientlogic.h"

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t block_drawinmtkview;

void init_application(void);

void shared_shutdown_application(void);

#ifdef __cplusplus
}
#endif

#endif // INIT_APPLICATION_H
