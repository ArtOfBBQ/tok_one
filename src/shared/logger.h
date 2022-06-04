#ifndef LOGGER_H
#define LOGGER_H

// #define LOGGER_SILENCE
#ifndef LOGGER_SILENCE
#include "stdio.h"
#endif

#include "assert.h"

#include "common.h"
#include "platform_layer.h"

#define LOG_SIZE 50000

#define log_append(string) internal_log_append(string, __func__)
#define log_append_float(num) internal_log_append_float(num, __func__)
#define log_append_int(num) internal_log_append_int(num, __func__)
#define log_append_uint(num) internal_log_append_uint(num, __func__)

/*
don't use the internal_ functions, use the macros that call them.
*/
void internal_log_append(
    const char * to_append,
    const char * caller_function_name);

/*
don't use the internal_ functions, use the macros that call them.
*/
void internal_log_append_int(
    const int32_t to_append,
    const char * caller_function_name);

/*
don't use the internal_ functions, use the macros that call them.
*/
void internal_log_append_uint(
    const uint32_t to_append,
    const char * caller_function_name);


/*
don't use the internal_ functions, use the macros that call them.
*/
void internal_log_append_uint(
    const uint32_t to_append,
    const char * caller_function_name);

/*
don't use the internal_ functions, use the macros that call them.
*/
void internal_log_append_float(
    const float to_append,
    const char * caller_function_name);

/*
dump the entire debug log to debuglog.txt
*/
void log_dump();

/*
dump the entire debug log to debuglog.txt,
then crash the application
*/
void log_dump_and_crash();

#endif

