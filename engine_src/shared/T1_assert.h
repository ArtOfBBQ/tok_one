#ifndef T1_ASSERT_H
#define T1_ASSERT_H

#include "T1_stdint.h"

/*
T1_assert(): when T1_ASSERTS_ACTIVE is T1_ACTIVE, a false condition prints
where it was and what ("T1_shader.c:120: T1_assert(n < 8) failed") and
crashes right away (a trap: the debugger stops on it), like libc's assert()
without libc. Compiled out when it's T1_INACTIVE, so conditions mustn't have
side effects.
*/
#if T1_ASSERTS_ACTIVE == T1_ACTIVE
void T1_assert_failed(
    const char * file, uint32_t line, const char * condition);
#define T1_assert(condition) \
    ((condition) ? (void)0 : \
        T1_assert_failed(__FILE__, __LINE__, #condition))
#elif T1_ASSERTS_ACTIVE == T1_INACTIVE
#define T1_assert(condition) ((void)0)
#else
#error
#endif

/*
Prints crash_message and crashes right away, so the debugger stops where
things went wrong. For errors T1 can't recover from, also works with
asserts inactive.
*/
void T1_crash(const char * crash_message);

#endif // T1_ASSERT_H
