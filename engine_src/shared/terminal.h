#ifndef TERMINAL_H
#define TERMINAL_H

#include "memorystore.h"
#include "window_size.h"
#include "userinput.h"
#include "zpolygon.h"
#include "text.h"
#include "clientlogic.h"
#include "wav.h"
#include "audio.h"

#ifdef __cplusplus
extern "C" {
#endif

extern bool32_t terminal_active;

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
