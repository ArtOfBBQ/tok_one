#ifndef T1_LOGGER_H
#define T1_LOGGER_H

#include "T1_stdint.h"

/*
Allocates memory. You need to pass a chunk of memory of LOG_SIZE bytes
example with c standard library: setup_log(malloc(LOG_SIZE));
*/
void T1_log_init(
    void * (* malloc_func)(size_t size),
    u32 (* create_mutex_func)(void),
    void (* mutex_lock_func)(const u32 mutex_id),
    void (* mutex_unlock_func)(const u32 mutex_id));

#if T1_LOG_PRINTF_ACTIVE == T1_ACTIVE
void T1_log_append(const char * to_append);
void T1_log_append_s32(s32 to_append);
void T1_log_append_u32(u32 to_append);
void T1_log_append_f32(f32 to_append);
void T1_log_append_c8(c8 to_append);
#elif T1_LOG_PRINTF_ACTIVE == T1_INACTIVE
#define T1_log_append(x)
#define T1_log_append_s32(x)
#define T1_log_append_u32(x)
#define T1_log_append_f32(x)
#define T1_log_append_c8(x)
#else
#error
#endif

/*
Prints crash_message and crashes right away, so the debugger stops where
things went wrong. Also works with asserts inactive.
*/
void T1_log_crash(const char * crash_message);

#if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
// Crashes if condition is false, see T1_log_crash()
void T1_log_assert(u8 condition);
void T1_log_warn_if_false(u8 condition, const char * msg);
#elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
#define T1_log_assert(x)
#define T1_log_warn_if_false(x, y)
#else
#error
#endif

#endif // T1_LOGGER_H
