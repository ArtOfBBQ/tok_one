#if T1_TERM_ACTIVE == T1_ACTIVE

#ifndef T1_TERM_H
#define T1_TERM_H

#include "T1_stdint.h"

void T1_term_init(
    void (* callback_eval_command_fptr)(
        char *, char *, u32),
    void (* enter_fullscreen_fncptr)(void));

void T1_term_update(void);

// Commands T1 doesn't know are passed to the handler
void T1_term_set_command_handler(
    void (* handler)(char * command, char * response, u32 response_cap));

void T1_term_manually_deactivate(void);

#endif // T1_TERM_H

#elif T1_TERM_ACTIVE == T1_INACTIVE
#else
#error
#endif // T1_TERM_ACTIVE
