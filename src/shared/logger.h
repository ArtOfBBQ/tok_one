#ifndef LOGGER_H
#define LOGGER_H

// #define LOGGER_SILENCE
#ifndef LOGGER_SILENCE
#include "stdio.h"
#endif

#include <dlfcn.h>

#include "common.h"
#include "memorystore.h"

#define LOG_SIZE 5000000

//#define log_assert(condition)
//#define log_append(string)
//#define log_append_char(num)
//#define log_append_float(num)
//#define log_append_int(num)
//#define log_append_uint(num)

#define log_assert(condition) internal_log_assert(condition, #condition, __FILE__, __LINE__, __func__)
#define log_append(string) internal_log_append(string, __func__)
#define log_append_char(num) internal_log_append_char(num, __func__)
#define log_append_float(num) internal_log_append_float(num, __func__)
#define log_append_int(num) internal_log_append_int(num, __func__)
#define log_append_uint(num) internal_log_append_uint(num, __func__)


#ifdef __cplusplus
extern "C" {
#endif
extern char application_name[100];

void __attribute__((no_instrument_function))
__cyg_profile_func_enter(
    void *this_fn,
    void *call_site);

void __attribute__((no_instrument_function))
__cyg_profile_func_exit(
    void *this_fn,
    void *call_site);

extern char * assert_failed_message;
extern bool32_t application_running;

/*
Allocates memory. This is only necessary in C99
*/
void setup_log(void);

/*
don't use the internal_ functions, use the macros that call them.
*/
void internal_log_append(
    const char * to_append,
    const char * caller_function_name);

/*
don't use the internal_ functions, use the macros that call them.
*/
void __attribute__((no_instrument_function))
internal_log_append_int(
    const int32_t to_append,
    const char * caller_function_name);

/*
don't use the internal_ functions, use the macros that call them.
*/
void __attribute__((no_instrument_function))
internal_log_append_uint(
    const uint32_t to_append,
    const char * caller_function_name);


/*
don't use the internal_ functions, use the macros that call them.
*/
void __attribute__((no_instrument_function))
internal_log_append_uint(
    const uint32_t to_append,
    const char * caller_function_name);

/*
don't use the internal_ functions, use the macros that call them.
*/
void __attribute__((no_instrument_function))
internal_log_append_char(
    const char to_append,
    const char * caller_function_name);

/*
don't use the internal_ functions, use the macros that call them.
*/
void __attribute__((no_instrument_function))
internal_log_append_float(
    const float to_append,
    const char * caller_function_name);


void __attribute__((no_instrument_function))
get_log_backtrace(
    char * return_value,
    uint32_t return_value_capacity);

void __attribute__((no_instrument_function))
add_profiling_stats_to_log(void);

/*
dump the entire debug log to debuglog.txt
*/
void __attribute__((no_instrument_function))
log_dump(bool32_t * good);

/*
Dump the entire debug log to debuglog.txt,
then crash the application.
I use this instead of assert(0).
*/
void __attribute__((no_instrument_function))
log_dump_and_crash(void);

/*
Assert something, but use the GUI to report on failure
instead of crashing the app
*/
void __attribute__((no_instrument_function))
internal_log_assert(
    bool32_t condition,
    const char * str_condition,
    const char * file_name,
    const int line_number,
    const char * func_name);

#ifdef __cplusplus
}
#endif

#endif // LOGGER_H
