#if T1_TERM_ACTIVE == T1_ACTIVE

#ifndef T1_TERM_H
#define T1_TERM_H

#include "T1_std.h"

#ifdef __cplusplus
extern "C" {
#endif


extern bool8_t T1_term_active;

void
T1_term_init(
    void (* enter_fullscreen_fncptr)(void));

void
T1_term_destroy_all(void);

void
T1_term_redraw_backgrounds(void);

void
T1_term_render(void);

void
T1_term_sendchar(uint32_t to_send);

void
T1_term_commit_or_activate(void);

#ifdef __cplusplus
}
#endif

#endif // T1_TERM_H

#elif T1_TERM_ACTIVE == T1_INACTIVE
#else
#error
#endif // T1_TERM_ACTIVE
