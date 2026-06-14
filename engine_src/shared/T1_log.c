#include "T1_log.h"
#include "T1_std.h"

#if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
#include <stdio.h>
#include <assert.h>
#elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
#else
#error
#endif


uint8_t T1_log_app_running = false;
char * T1_log_crash_msg = NULL;

#define T1_LOG_CRASH_STRING_SIZE 256
#define LOG_SIZE 500000
typedef struct {
    void * (* malloc)(size_t);
    uint32_t (* create_mutex)(void);
    void (* mutex_lock)(const uint32_t);
    void (* mutex_unlock)(const uint32_t);
    uint32_t mutex_id;
    char * full;
    uint32_t full_i;
} T1LogState;

static T1LogState * T1_log_s = NULL;

#ifdef __cplusplus
extern "C" {
#endif

void T1_logger_init(
    void * (* arg_logger_malloc_func)(size_t size),
    uint32_t (* arg_logger_create_mutex_func)(void),
    void (* arg_logger_mutex_lock_func)(const uint32_t mutex_id),
    void (* arg_logger_mutex_unlock_func)(const uint32_t mutex_id))
{
    T1_log_s = arg_logger_malloc_func(sizeof(T1LogState));
    T1_std_memset(T1_log_s, 0, sizeof(T1LogState));
    
    T1_log_crash_msg = arg_logger_malloc_func(
        T1_LOG_CRASH_STRING_SIZE);
    T1_std_memset(T1_log_crash_msg, 0, T1_LOG_CRASH_STRING_SIZE);
    
    T1_log_s->malloc = arg_logger_malloc_func;
    T1_log_s->create_mutex = arg_logger_create_mutex_func;
    T1_log_s->mutex_lock = arg_logger_mutex_lock_func;
    T1_log_s->mutex_unlock = arg_logger_mutex_unlock_func;
    
    // create a log for debug text
    T1_log_s->full_i = 0;
    
    if (T1_log_s->create_mutex != NULL) {
        T1_log_s->mutex_id = T1_log_s->create_mutex();
    }
}

#if T1_LOG_PRINTF == T1_ACTIVE
void
T1_log_internal_append_uint(
    const uint32_t to_append,
    const char * caller_function_name)
{
    char converted[1000];
    T1_std_uint_to_string(
        /* const uint32_t input: */
            to_append,
        /* char * recipient: */
            converted);
    
    T1_log_internal_append(
        converted,
        caller_function_name);
}

void
T1_log_internal_append_char(
    const char to_append,
    const char * caller_function_name)
{
    char to_append_array[2];
    to_append_array[0] = to_append;
    to_append_array[1] = '\0';
    
    T1_log_internal_append(
        to_append_array,
        caller_function_name);
}

void
T1_log_internal_append_int(
    const int32_t to_append,
    const char * caller_function_name)
{
    char converted[1000];
    T1_std_int_to_string(
        /* const int32_t input: */
            to_append,
        /* char * recipient: */
            converted);
    
    T1_log_internal_append(
        converted,
        caller_function_name);
}

void
T1_log_internal_append_float(
    const float to_append,
    const char * caller_function_name)
{
    char float_str[1000];
    T1_std_float_to_string(
        /* const int32_t input: */
            to_append,
        /* char * recipient: */
            float_str,
        /* const uint32_t recipient_size: */
            1000);
    
    T1_log_internal_append(
        float_str,
        caller_function_name);
}

void
T1_log_internal_append(
    const char * to_append,
    const char * caller_function_name)
{
    (void)to_append;
    
    printf("%s", to_append);
    
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
        
        T1_std_strcpy_cap(
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
        
        T1_std_strcpy_cap(
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
        T1_std_strcpy_cap(
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
    T1_std_strcpy_cap(
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
#elif T1_LOG_PRINTF == T1_INACTIVE
#else
#error
#endif

void T1_log_dump(uint8_t * good) {
    
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

void
T1_log_dump_and_crash(
    const char * crash_message)
{
    uint8_t log_dump_succesful = false;
    T1_log_dump(&log_dump_succesful);
    
    if (T1_log_app_running) {
        unsigned int i = 0;
        while (
            crash_message[i] != '\0' &&
            i < (T1_LOG_CRASH_STRING_SIZE-1))
        {
            T1_log_crash_msg[i] = crash_message[i];
            i++;
        }
        T1_log_crash_msg[i] = '\0';
    }
    
    #if T1_LOG_PRINTF == T1_ACTIVE
    printf("DUMP & CRASHED: %s\n", crash_message);
    #elif T1_LOG_PRINTF == T1_INACTIVE
    #else
    #error
    #endif
    
    T1_log_app_running = false;
}

#if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
void
T1_log_assert_internal(
    uint8_t condition,
    const char * str_condition,
    const char * file_name,
    const int line_number,
    const char * func_name)
{
    if (
        condition ||
        !T1_log_app_running)
    {
        return;
    }
    
    #if T1_LOG_PRINTF == T1_ACTIVE
    printf(
        "\n*****\nfailed condition (%s::%s::%i: %s\n*****\n",
        file_name != NULL ? file_name : "NULL",
        func_name != NULL ? func_name : "NULL",
        line_number,
        str_condition != NULL ? str_condition : "NULL");
    #elif T1_LOG_PRINTF == T1_INACTIVE
    #else
    #error
    #endif
    
    assert(str_condition != NULL);
    assert(str_condition[0] != '\0');
    
    //Assertion failed: (0), function main, file test.c, line 6.
    char assert_failed_msg[512];
    
    T1_std_strcpy_cap(
        assert_failed_msg,
        512,
        "Assertion failed: (");
    T1_std_strcat_cap(
        assert_failed_msg,
        512,
        str_condition);
    T1_std_strcat_cap(
        assert_failed_msg,
        256,
        "), function ");
    T1_std_strcat_cap(
        assert_failed_msg,
        512,
        func_name);
    T1_std_strcat_cap(
        assert_failed_msg,
        512,
        ", file ");
    T1_std_strcat_cap(
        assert_failed_msg,
        512,
        file_name);
    T1_std_strcat_cap(
        assert_failed_msg,
        512,
        ", line ");
    T1_std_strcat_int_cap(
        assert_failed_msg,
        512,
        line_number);
    
    T1_log_dump_and_crash(assert_failed_msg);
}

void
T1_log_warn_internal(
    uint8_t condition,
    const char * str_condition,
    const char * file_name,
    const int line_number,
    const char * func_name)
{
    if (condition) { return; }
    
    #if T1_LOG_PRINTF == T1_ACTIVE
    printf(
        "\n*****\nWARN CONDITION (%s::%s::%i: %s\n*****\n",
        file_name != NULL ? file_name : "NULL",
        func_name != NULL ? func_name : "NULL",
        line_number,
        str_condition != NULL ? str_condition : "NULL");
    #elif T1_LOG_PRINTF == T1_INACTIVE
    (void)line_number;
    (void)file_name;
    (void)func_name;
    (void)str_condition;
    #else
    #error
    #endif
    
    T1_log_append("WARN CONDITION from function: ");
    T1_log_append(str_condition);
    T1_log_append("\n");
}
#elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
#else
#error
#endif

typedef struct TimerResults {
    uint64_t inprogress_start;
    uint64_t inprogress_end;
    uint64_t averaged_result_so_far;
    uint32_t runs;
} TimerResults;
