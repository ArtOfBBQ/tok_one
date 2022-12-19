#ifndef INIT_APPLICATION_H
#define INIT_APPLICATION_H

#include "shared/common.h"
#include "shared/logger.h"
#include "shared/tok_random.h"
#include "shared/lightsource.h"
#include "shared/text.h"
#include "shared/userinput.h"
#include "shared/window_size.h"
#include "shared/scheduled_animations.h"
#include "shared/software_renderer.h"
#include "clientlogic.h"

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t block_drawinmtkview;

void init_application(void);

#ifdef __cplusplus
}
#endif

#endif // INIT_APPLICATION_H
