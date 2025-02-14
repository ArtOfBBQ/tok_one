#include "logger.h"

bool32_t application_running = false;
#define CRASH_STRING_SIZE 256
char crashed_top_of_screen_msg[CRASH_STRING_SIZE];

#ifndef IGNORE_LOGGER
// static bool32_t logger_activated = false;
#endif

// These are function pointers to our injected dependencies
static void *   (* logger_malloc_func)(size_t) = NULL;
static uint32_t (* logger_create_mutex_func)(void) = NULL;
static void     (* logger_mutex_lock_func)(const uint32_t) = NULL;
static void     (* logger_mutex_unlock_func)(const uint32_t) = NULL;
static uint32_t logger_mutex_id = UINT32_MAX;

#define LOG_SIZE 5000000
static char * app_log = NULL;
static uint32_t log_i = 0;

#ifdef __cplusplus
extern "C" {
#endif

void logger_init(
    void * (* arg_logger_malloc_func)(size_t size),
    uint32_t (* arg_logger_create_mutex_func)(void),
    void (* arg_logger_mutex_lock_func)(const uint32_t mutex_id),
    void (* arg_logger_mutex_unlock_func)(const uint32_t mutex_id))
{
    logger_malloc_func       = arg_logger_malloc_func;
    logger_create_mutex_func = arg_logger_create_mutex_func;
    logger_mutex_lock_func   = arg_logger_mutex_lock_func;
    logger_mutex_unlock_func = arg_logger_mutex_unlock_func;
    
    // create a log for debug text
    app_log    = logger_malloc_func(LOG_SIZE);
    app_log[0] = '\0';
    log_i      = 0;
    
    if (logger_create_mutex_func != NULL) {
        logger_mutex_id = logger_create_mutex_func();
    }
}

#ifndef LOGGER_SILENCE
void
internal_log_append_uint(
    const uint32_t to_append,
    const char * caller_function_name)
{
    char converted[1000];
    common_uint_to_string(
        /* const uint32_t input: */
            to_append,
        /* char * recipient: */
            converted);
    
    internal_log_append(
        converted,
        caller_function_name);
}

void
internal_log_append_char(
    const char to_append,
    const char * caller_function_name)
{
    char to_append_array[2];
    to_append_array[0] = to_append;
    to_append_array[1] = '\0';
    
    internal_log_append(
        to_append_array,
        caller_function_name);
}

void
internal_log_append_int(
    const int32_t to_append,
    const char * caller_function_name)
{
    char converted[1000];
    common_int_to_string(
        /* const int32_t input: */
            to_append,
        /* char * recipient: */
            converted);
    
    internal_log_append(
        converted,
        caller_function_name);
}

void
internal_log_append_float(
    const float to_append,
    const char * caller_function_name)
{
    char float_str[1000];
    common_float_to_string(
        /* const int32_t input: */
            to_append,
        /* char * recipient: */
            float_str,
        /* const uint32_t recipient_size: */
            1000);
    
    internal_log_append(
        float_str,
        caller_function_name);
}

void
internal_log_append(
    const char * to_append,
    const char * caller_function_name)
{
    #ifndef LOGGER_SILENCE
    printf("%s", to_append);
    #endif
    
    // logger_mutex_lock_func(logger_mutex_id);
    
    #if 1
    (void)caller_function_name;
    #else
    if (
        caller_function_name != NULL)
    {
        char * prefix = (char *)"[";
        uint32_t prefix_length = common_get_string_length(prefix);
        if (log_i + prefix_length >= LOG_SIZE) {
            if (logger_mutex_unlock_func != NULL) {
                // logger_mutex_unlock_func(logger_mutex_id);
            }
            return;
        }
        
        common_strcpy_capped(
            /* recipient: */
                app_log + log_i,
            /* recipient_size: */
                LOG_SIZE,
            /* origin: */
                prefix);
        log_i += prefix_length;
        if (log_i >= LOG_SIZE) {
            // logger_mutex_unlock_func(logger_mutex_id);
            return;
        }
        
        uint32_t func_length = common_get_string_length(
        caller_function_name);
        if (log_i + func_length >= LOG_SIZE) {
            // logger_mutex_unlock_func(logger_mutex_id);
            return;
        }
        
        common_strcpy_capped(
            /* recipient: */
                app_log + log_i,
            /* recipient_size: */
                LOG_SIZE - log_i,
            /* origin: */
                caller_function_name);
        log_i += func_length;
        if (log_i >= LOG_SIZE) {
            if (logger_mutex_unlock_func != NULL) {
                // logger_mutex_unlock_func(logger_mutex_id);
            }
            return;
        }
        
        char * glue = (char *)"]: ";
        uint32_t glue_length = common_get_string_length(glue);
        if (log_i + glue_length >= LOG_SIZE) {
            if (logger_mutex_unlock_func != NULL) {
                // logger_mutex_unlock_func(logger_mutex_id);
            }
            return;
        }
        common_strcpy_capped(
            /* recipient: */
                app_log + log_i,
            /* recipient_size: */
                LOG_SIZE,
            /* origin: */
                glue);
        log_i += glue_length;
        if (log_i >= LOG_SIZE) {
            if (logger_mutex_unlock_func != NULL) {
                // logger_mutex_unlock_func(logger_mutex_id);
            }
            return;
        }
    }
    
    uint32_t to_append_length = common_get_string_length(to_append);
    if (log_i + to_append_length >= LOG_SIZE) {
        if (logger_mutex_unlock_func != NULL) {
            // logger_mutex_unlock_func(logger_mutex_id);
        }
        return;
    }
    common_strcpy_capped(
        /* recipient: */
            app_log + log_i,
        /* recipient_size: */
            LOG_SIZE,
        /* origin: */
            to_append);
    log_i += to_append_length;
    
    if (logger_mutex_unlock_func != NULL) {
        // logger_mutex_unlock_func(logger_mutex_id);
    }
    #endif
}
#endif

void log_dump(bool32_t * good) {
    
    // TODO: move this elsewhere so logger can avoid #including platform_layer.h
    //    if (app_log == NULL) { return; }
    //    app_log[log_i + 1] = '\0';
    //
    //    platform_write_file_to_writables(
    //        /* filepath_destination : */
    //            (char *)"log.txt",
    //        /* const char * output  : */
    //            app_log,
    //        /* output_size          : */
    //            log_i + 1,
    //        /* good                 : */
    //            good);
    
    *good = true;
}

#ifndef LOGGER_IGNORE_ASSERTS
void
log_dump_and_crash(char * crash_message) {
    bool32_t log_dump_succesful = false;
    log_dump(&log_dump_succesful);
    
    if (application_running) {
        unsigned int i = 0;
        while (crash_message[i] != '\0' && i < (CRASH_STRING_SIZE-1)) {
            crashed_top_of_screen_msg[i] = crash_message[i];
            i++;
        }
        crashed_top_of_screen_msg[i] = '\0';
    }
    
    application_running = false;
    
    #ifndef LOGGER_SILENCE
    printf("DUMP & CRASHED: %s\n", crash_message);
    #endif
    
    #ifdef IGNORE_LOGGER
    assert(0);
    #endif
}
#endif

#ifndef LOGGER_IGNORE_ASSERTS
void
internal_log_assert(
    bool32_t condition,
    const char * str_condition,
    const char * file_name,
    const int line_number,
    const char * func_name)
{
    if (condition || !application_running) { return; }
    
    #ifndef LOGGER_SILENCE
    printf(
        "\n*****\nfailed condition (%s::%s::%i: %s\n*****\n",
        file_name != NULL ? file_name : "NULL",
        func_name != NULL ? func_name : "NULL",
        line_number,
        str_condition != NULL ? str_condition : "NULL");
    #endif
    
    assert(str_condition != NULL);
    assert(str_condition[0] != '\0');
    
    //Assertion failed: (0), function main, file test.c, line 6.
    char assert_failed_msg[512];
    
    common_strcpy_capped(
        assert_failed_msg,
        512,
        "Assertion failed: (");
    common_strcat_capped(
        assert_failed_msg,
        512,
        str_condition);
    common_strcat_capped(
        assert_failed_msg,
        256,
        "), function ");
    common_strcat_capped(
        assert_failed_msg,
        512,
        func_name);
    common_strcat_capped(
        assert_failed_msg,
        512,
        ", file ");
    common_strcat_capped(
        assert_failed_msg,
        512,
        file_name);
    common_strcat_capped(
        assert_failed_msg,
        512,
        ", line ");
    common_strcat_int_capped(
        assert_failed_msg,
        512,
        line_number);
    
    log_dump_and_crash(assert_failed_msg);
}
#endif

typedef struct TimerResults {
    uint64_t inprogress_start;
    uint64_t inprogress_end;
    uint64_t averaged_result_so_far;
    uint32_t runs;
} TimerResults;
