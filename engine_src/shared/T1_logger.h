#ifndef LOGGER_H
#define LOGGER_H

#include <stddef.h>
#include "T1_std.h"

#ifndef LOGGER_SILENCE
#include <stdio.h>
#define log_append(string) internal_log_append(string, __func__)
#define log_append_char(num) internal_log_append_char(num, __func__)
#define log_append_float(num) internal_log_append_float(num, __func__)
#define log_append_int(num) internal_log_append_int(num, __func__)
#define log_append_uint(num) internal_log_append_uint(num, __func__)
#else
#define log_append(string)
#define log_append_char(num)
#define log_append_float(num)
#define log_append_int(num)
#define log_append_uint(num)
#endif

#ifdef LOGGER_IGNORE_ASSERTS
#define log_assert(condition)
#else
#include <stdio.h>
#include <assert.h>
#define log_assert(condition) internal_log_assert(condition, #condition, __FILE__, __LINE__, __func__)
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern char crashed_top_of_screen_msg[256];
extern bool32_t application_running;

/*
Allocates memory. You need to pass a chunk of memory of LOG_SIZE bytes
example with c standard library: setup_log(malloc(LOG_SIZE));
*/
void logger_init(
    void * (* malloc_function)(size_t size),
    uint32_t (* create_mutex_function)(void),
    void (* mutex_lock_function)(const uint32_t mutex_id),
    void (* mutex_unlock_function)(const uint32_t mutex_id));

#ifndef LOGGER_SILENCE
/*
don't use the internal_ functions, use the macros that call them.
*/
void internal_log_append(
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


#ifndef LOGGER_IGNORE_ASSERTS
/*
Assert something, but use the GUI to report on failure
instead of crashing the app
*/
void
internal_log_assert(
    bool32_t condition,
    const char * str_condition,
    const char * file_name,
    const int line_number,
    const char * func_name);
#endif

#ifdef __cplusplus
}
#endif

#endif // LOGGER_H
