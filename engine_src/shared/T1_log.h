#ifndef T1_LOGGER_H
#define T1_LOGGER_H

#include "T1_stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

extern u8 T1_log_app_running;
extern char * T1_log_crash_msg;

/*
Allocates memory. You need to pass a chunk of memory of LOG_SIZE bytes
example with c standard library: setup_log(malloc(LOG_SIZE));
*/
void
T1_logger_init(
    void * (* malloc_function)(u64 size),
    u32 (* create_mutex_function)(void),
    void (* mutex_lock_function)(const u32 mutex_id),
    void (* mutex_unlock_function)(const u32 mutex_id));

#if T1_LOG_PRINTF == T1_ACTIVE
void T1_log_append(const char * to_append);
void T1_log_append_s32(s32 to_append);
void T1_log_append_u32(u32 to_append);
void T1_log_append_f32(f32 to_append);
void T1_log_append_c8(c8 to_append);
#elif T1_LOG_PRINTF == T1_INACTIVE
#else
#error
#endif

/*
dump the entire debug log to debuglog.txt
*/
void T1_log_dump(u8 * good);

/*
Dump the entire debug log to debuglog.txt,
then crash the application. This can be used even in release mode.
*/
void
T1_log_dump_and_crash(const char * crash_message);


#if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
/*
Assert something, but use the GUI to report on failure
instead of crashing the app
*/
void T1_log_assert(u8 condition);
void T1_log_warn(u8 condition);
#elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
#else
#error
#endif

#ifdef __cplusplus
}
#endif

#endif // T1_LOGGER_H
