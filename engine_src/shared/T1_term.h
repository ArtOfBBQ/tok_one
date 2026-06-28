#if T1_TERM_ACTIVE == T1_ACTIVE

#ifndef T1_TERM_H
#define T1_TERM_H

#ifdef __cplusplus
extern "C" {
#endif

void
T1_term_init(
    void (* enter_fullscreen_fncptr)(void));

void
T1_term_update(void);

#ifdef __cplusplus
}
#endif

#endif // T1_TERM_H

#elif T1_TERM_ACTIVE == T1_INACTIVE
#else
#error
#endif // T1_TERM_ACTIVE
