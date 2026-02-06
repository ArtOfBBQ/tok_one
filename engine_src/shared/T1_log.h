#ifndef T1_LOGGER_H
#define T1_LOGGER_H

#include <stddef.h>
#include "T1_std.h"

#if T1_LOG_SILENCE == T1_INACTIVE
#include <stdio.h>
#define T1_log_append(string) internal_log_append(string, __func__)
#define T1_log_append_char(num) internal_log_append_char(num, __func__)
#define T1_log_append_float(num) internal_log_append_float(num, __func__)
#define T1_log_append_int(num) internal_log_append_int(num, __func__)
#define T1_log_append_uint(num) internal_log_append_uint(num, __func__)
#elif T1_LOG_SILENCE == T1_ACTIVE
#define T1_log_append(string)
#define T1_log_append_char(num)
#define T1_log_append_float(num)
#define T1_log_append_int(num)
#define T1_log_append_uint(num)
#else
#error
#endif

#if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
#include <stdio.h>
#include <assert.h>
#define T1_log_assert(condition) T1_log_assert_internal(condition, #condition, __FILE__, __LINE__, __func__)
#elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
#define T1_log_assert(condition)
#else
#error
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern char crashed_top_of_screen_msg[256];
extern bool32_t T1_logger_app_running;

/*
Allocates memory. You need to pass a chunk of memory of LOG_SIZE bytes
example with c standard library: setup_log(malloc(LOG_SIZE));
*/
void
T1_logger_init(
    void * (* malloc_function)(size_t size),
    uint32_t (* create_mutex_function)(void),
    void (* mutex_lock_function)(const uint32_t mutex_id),
    void (* mutex_unlock_function)(const uint32_t mutex_id));

#if T1_LOG_SILENCE == T1_INACTIVE
/*
don't use the internal_ functions, use the macros that call them.
*/
void T1_log_internal_append(
    const char * to_append,
    const char * caller_function_name);

/*
don't use the internal_ functions, use the macros that call them.
*/
void
internal_log_append_int(
    const int32_t to_append,
    const char * caller_function_name);

/*
don't use the internal_ functions, use the macros that call them.
*/
void
internal_log_append_uint(
    const uint32_t to_append,
    const char * caller_function_name);


/*
don't use the internal_ functions, use the macros that call them.
*/
void
internal_log_append_uint(
    const uint32_t to_append,
    const char * caller_function_name);

/*
don't use the internal_ functions, use the macros that call them.
*/
void
internal_log_append_char(
    const char to_append,
    const char * caller_function_name);

/*
don't use the internal_ functions, use the macros that call them.
*/
void
internal_log_append_float(
    const float to_append,
    const char * caller_function_name);
#elif T1_LOG_SILENCE == T1_ACTIVE
#else
#error
#endif

/*
dump the entire debug log to debuglog.txt
*/
void log_dump(bool32_t * good);

/*
Dump the entire debug log to debuglog.txt,
then crash the application. This can be used even in release mode.
*/
void
log_dump_and_crash(const char * crash_message);


#if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
/*
Assert something, but use the GUI to report on failure
instead of crashing the app
*/
void
T1_log_assert_internal(
    bool32_t condition,
    const char * str_condition,
    const char * file_name,
    const int line_number,
    const char * func_name);
#elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
#else
#error
#endif

#ifdef __cplusplus
}
#endif

#endif // T1_LOGGER_H
