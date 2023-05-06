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
#include "clientlogic.h"

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t block_drawinmtkview;

void init_application(
    const float window_left,
    const float window_width,
    const float window_bottom,
    const float window_height);

#ifdef __cplusplus
}
#endif

#endif // INIT_APPLICATION_H
