#include "logger.h"

static char log[LOG_SIZE];
static char current_function[100];
static uint32_t log_i = 0;

void internal_log_append_uint(
    const uint32_t to_append,
    const char * caller_function_name)
{
    char * converted =
        (char *)"<unimplemented number conversion\n";
    internal_log_append(
        converted,
        caller_function_name);
}

void internal_log_append_int(
    const int32_t to_append,
    const char * caller_function_name)
{
    char * converted =
        (char *)"<unimplemented number conversion\n";
    internal_log_append(
        converted,
        caller_function_name);
}

void internal_log_append_float(
    const float to_append,
    const char * caller_function_name)
{
    char * converted =
        (char *)"<unimplemented float conversion\n";
    internal_log_append(
        converted,
        caller_function_name);
}

void internal_log_append(
    const char * to_append,
    const char * caller_function_name)
{
    #ifndef LOGGER_SILENCE 
    uint32_t initial_log_i = log_i;
    #endif
    
    /*
    If we're in a different function than last time,
    add the function name to the log
    */
    if (!are_equal_strings(
        current_function,
        caller_function_name))
    {
        log[log_i] = '[';
        log_i++;
        
        uint32_t i = 0;
        while (caller_function_name[i] != '\0') {
            
            current_function[i] = caller_function_name[i];        
            log[log_i] = caller_function_name[i];
            log_i += 1;
            i += 1;
        }
        current_function[i] = '\0';
        
        log[log_i] = ']';
        log_i++;
        log[log_i] = '\n';
        log_i++;
    }    
    
    uint32_t i = 0;
    while (to_append[i] != '\0') {
        
        log[log_i] = to_append[i];
        log_i += 1;
        i += 1;
    }
    
    #ifndef LOGGER_SILENCE 
    printf(
        "%s\n",
        log + initial_log_i);
    #endif
}

void log_dump() {
    log[log_i] = '\0';
    log_i += 1;
    
    platform_write_file(
        /* filepath_destination: */
            "log.txt",
        /* const char * output: */
            log,
        /* output_size: */
            log_i + 1);
}

void log_dump_and_crash() {
    log_dump();
    assert(0);
}

