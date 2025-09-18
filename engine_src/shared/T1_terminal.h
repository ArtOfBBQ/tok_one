#if TERMINAL_ACTIVE

#ifndef TERMINAL_H
#define TERMINAL_H

#include "T1_mem.h"
#include "T1_engine_globals.h"
#include "T1_userinput.h"
#include "T1_zsprite.h"
#include "T1_text.h"
#include "T1_clientlogic.h"
#include "T1_wav.h"
#include "T1_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

extern bool8_t terminal_active;

void terminal_init(
    void (* terminal_enter_fullscreen_fncptr)(void));

void destroy_terminal_objects(void);
void terminal_redraw_backgrounds(void);
void terminal_render(void);
void terminal_sendchar(uint32_t to_send);
void terminal_commit_or_activate(void);

#ifdef __cplusplus
}
#endif

#endif // TERMINAL_H

#endif // TERMINAL_ACTIVE
