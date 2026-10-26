#ifndef T1_ASSERT_H
#define T1_ASSERT_H

#include "T1_stdint.h"

/*
T1_assert() is libc's assert() when T1_ASSERTS_ACTIVE is T1_ACTIVE, and
compiled out when it's T1_INACTIVE, so conditions mustn't have side effects.
*/
#if T1_ASSERTS_ACTIVE == T1_ACTIVE
#include <assert.h>
#define T1_assert(condition) assert(condition)
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
