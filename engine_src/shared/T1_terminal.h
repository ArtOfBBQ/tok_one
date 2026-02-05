#if T1_TERMINAL_ACTIVE == T1_ACTIVE

#ifndef T1_TERMINAL_H
#define T1_TERMINAL_H

#include "T1_mem.h"
#include "T1_global.h"
#include "T1_io.h"
#include "T1_zsprite.h"
#include "T1_text.h"
#include "T1_clientlogic.h"
#include "T1_wav.h"
#include "T1_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

extern bool8_t T1_terminal_active;

void
T1_terminal_init(
    void (* terminal_enter_fullscreen_fncptr)(void));

void
T1_terminal_destroy_all(void);

void
T1_terminal_redraw_backgrounds(void);

void
T1_terminal_render(void);

void
T1_terminal_sendchar(uint32_t to_send);

void
T1_terminal_commit_or_activate(void);

#ifdef __cplusplus
}
#endif

#endif // T1_TERMINAL_H

#elif T1_TERMINAL_ACTIVE == T1_INACTIVE
#else
#error
#endif // T1_TERMINAL_ACTIVE
