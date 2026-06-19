#if T1_TERM_ACTIVE == T1_ACTIVE

#ifndef T1_TERM_H
#define T1_TERM_H

#include "T1_public_types.h"

#ifdef __cplusplus
extern "C" {
#endif


extern b8 T1_term_active;

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
T1_term_sendchar(u32 to_send);

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
