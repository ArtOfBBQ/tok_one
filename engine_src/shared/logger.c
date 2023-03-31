#include "logger.h"

char application_name[100];
char crashed_top_of_screen_msg[256];

#ifndef IGNORE_LOGGER
static bool32_t logger_activated = false;
#endif

static char * app_log;
static uint32_t log_i = 0;

#ifdef __cplusplus
extern "C" {
#endif

void setup_log(void) {

    log_assert(application_name != NULL);
    
    // create a log for debug text
    app_log = (char *)malloc_from_unmanaged(LOG_SIZE);
}

void
internal_log_append_uint(
    const uint32_t to_append,
    const char * caller_function_name)
{
    char converted[1000];
    uint_to_string(
        /* const uint32_t input: */
            to_append,
        /* char * recipient: */
            converted,
        /* const uint32_t recipient_size: */
            1000);
    
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
    int_to_string(
        /* const int32_t input: */
            to_append,
        /* char * recipient: */
            converted,
        /* const uint32_t recipient_size: */
            1000);
    
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
    float_to_string(
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
    if (application_running) {
        printf("%s", to_append);
    }
    #endif
    
    if (
        caller_function_name != NULL)
    {
        char * prefix = (char *)"[";
        uint32_t prefix_length = get_string_length(prefix);
        if (log_i + prefix_length >= LOG_SIZE) { return; }
        strcpy_capped(
            /* recipient: */
                app_log + log_i,
            /* recipient_size: */
                LOG_SIZE,
            /* origin: */
                prefix);
        log_i += prefix_length;
        if (log_i >= LOG_SIZE) { return; }
        
        uint32_t func_length = get_string_length(
        caller_function_name);
        if (log_i + func_length < LOG_SIZE) { return; }
        
        strcpy_capped(
            /* recipient: */
                app_log + log_i,
            /* recipient_size: */
                LOG_SIZE,
            /* origin: */
                caller_function_name);
        log_i += func_length;
        if (log_i >= LOG_SIZE) { return; }
        
        char * glue = (char *)"]: ";
        uint32_t glue_length = get_string_length(glue);
        if (log_i + glue_length >= LOG_SIZE) { return; }
        strcpy_capped(
            /* recipient: */
                app_log + log_i,
            /* recipient_size: */
                LOG_SIZE,
            /* origin: */
                glue);
        log_i += glue_length;
        if (log_i >= LOG_SIZE) { return; }
    }
    
    uint32_t to_append_length = get_string_length(to_append);
    if (log_i + to_append_length >= LOG_SIZE) { return; }
    strcpy_capped(
        /* recipient: */
            app_log + log_i,
        /* recipient_size: */
            LOG_SIZE,
        /* origin: */
            to_append);
    log_i += to_append_length;
    if (log_i >= LOG_SIZE) { return; }
}

void log_dump(bool32_t * good) {
    
    if (app_log == NULL) { return; }
    app_log[log_i + 1] = '\0';
    
    platform_write_file_to_writables(
        /* filepath_destination : */
            (char *)"log.txt",
        /* const char * output  : */
            app_log,
        /* output_size          : */
            log_i + 1,
        /* good                 : */
            good);
}

void
log_dump_and_crash(const char * crash_message) {
    bool32_t log_dump_succesful = false;
    log_dump(&log_dump_succesful);
    strcpy_capped(crashed_top_of_screen_msg, 256, crash_message);  
    application_running = false;
    
    #ifndef LOGGER_SILENCE
    printf("DUMP & CRASHED: %s\n", crash_message);
    #endif
}

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
    char assert_failed_msg[256];
    
    strcpy_capped(
        assert_failed_msg,
        256,
        "Assertion failed: (");
    strcat_capped(
        assert_failed_msg,
        256,
        str_condition);
    strcat_capped(
        assert_failed_msg,
        256,
        "), function ");
    strcat_capped(
        assert_failed_msg,
        256,
        func_name);
    strcat_capped(
        assert_failed_msg,
        256,
        ", file ");
    strcat_capped(
        assert_failed_msg,
        256,
        file_name);
    strcat_capped(
        assert_failed_msg,
        256,
        ", line ");
    strcat_int_capped(
        assert_failed_msg,
        256,
        line_number);
    
    log_dump_and_crash(assert_failed_msg);
}
