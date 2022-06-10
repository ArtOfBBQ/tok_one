#include "logger.h"

static char * log;
static uint32_t log_i = 0;

#define MAX_TIMED_FUNCTION_NAME 80
typedef struct TimedFunction {
    uint64_t function_address;
    char function_name[MAX_TIMED_FUNCTION_NAME];
    uint32_t times_ran;
    uint64_t time_tally;
    uint64_t current_first_start_at;
    uint32_t currently_running;
} TimedFunction;

#define TIMED_FUNCTION_LINK_SIZE 5
typedef struct TimedFunctionLink {
    TimedFunction linked_list[TIMED_FUNCTION_LINK_SIZE];
} TimedFunctionLink;

#define TIMED_FUNCTION_MAP_SIZE 4096
TimedFunctionLink * timed_function_map = NULL;

/*
Let's make a backtrace circle

By 'backtrace circle' I mean a conceptually circular array
where the element at position 0 follows the final element etc.

So we store each function name wheenver it runs, and when
we get to the end of the array, we start storing at the front
again. If you request the last 5 functions when backtrace_i
is at 3, it would be elements 50,0,1,2,3
*/
typedef struct BacktraceEntry {
    char function_name[MAX_TIMED_FUNCTION_NAME];
} BacktrackeEntry;
#define BACKTRACE_CIRCLE_SIZE 50
BacktraceEntry * backtrace_circle = NULL;
uint32_t backtrace_i = 0;


#ifdef __cplusplus
extern "C" {
#endif
    void __attribute__((no_instrument_function))
    __cyg_profile_func_enter(
        void *this_fn,
        void *call_site)
    {
        if (timed_function_map == NULL) { return; }
        
        uint32_t entry_i =
            (uint64_t)this_fn & (TIMED_FUNCTION_MAP_SIZE - 1);
        
        uint32_t link_i = 0;
        while (
            timed_function_map[entry_i]
                .linked_list[link_i]
                .function_address
                    != (uint64_t)this_fn
            && timed_function_map[entry_i]
                .linked_list[link_i]
                .function_address
                    != 0)
        {
            link_i += 1;
            if (link_i >= TIMED_FUNCTION_LINK_SIZE) {
                #ifndef LOGGER_SILENCE 
                printf(
                    "too many timed function hashmap conflicts. You can probably just add some memory for the linked list by increasing TIMED_FUNCTION_LINK_SIZE (currently %u) in logger.c\n",
                    TIMED_FUNCTION_LINK_SIZE);
                #endif
                application_running = false;
                assert(0);
            }
        }

        // add this function name to the backtrace circle
        copy_strings(
            /* recipient: */
                backtrace_circle[backtrace_i]
                    .function_name,
            /* recipient_size: */
                MAX_TIMED_FUNCTION_NAME,
            /* origin: */
                timed_function_map[entry_i]
                    .linked_list[link_i]
                    .function_name);
        
        // record +1 run for this function address
        timed_function_map[entry_i]
            .linked_list[link_i]
            .function_address =
                (uint64_t)this_fn;
        timed_function_map[entry_i]
            .linked_list[link_i]
            .times_ran += 1;
        
        // record function name if necessary
        if (timed_function_map[entry_i]
                .linked_list[link_i]
                .function_name[0] == '\0')
        {
            Dl_info info;
            
            if (dladdr(this_fn, &info)) {
                // info.dli_fname;

                copy_strings(
                    /* recipient: */
                        timed_function_map[entry_i]
                            .linked_list[link_i]
                            .function_name,
                    /* recipient_size: */
                        MAX_TIMED_FUNCTION_NAME,
                    /* origin: */
                        info.dli_sname);
            }
        }
        
        // record when this function started running 
        timed_function_map[entry_i]
            .linked_list[link_i]
            .currently_running += 1;
        if (
            timed_function_map[entry_i]
                .linked_list[link_i]
                .currently_running == 1)
        {
            timed_function_map[entry_i]
                .linked_list[link_i]
                .current_first_start_at =
                    platform_get_current_time_microsecs();
        }
    }
    
    void __attribute__((no_instrument_function))
    __cyg_profile_func_exit(
        void *this_fn,
        void *call_site)
    {
        if (timed_function_map == NULL) { return; }
        uint32_t entry_i =
            (uint64_t)this_fn & (TIMED_FUNCTION_MAP_SIZE - 1);
        
        bool32_t found_link = false; 
        uint32_t link_i = 0;
        for (
            ;
            link_i < TIMED_FUNCTION_LINK_SIZE;
            link_i++)
        {
            if (
                timed_function_map[entry_i]
                    .linked_list[link_i]
                    .function_address
                        == (uint64_t)this_fn)
            {
                found_link = true;
                break;
            }
        }

        if (!found_link) { return; }
        
        // find out when this func started running 
        if (
            timed_function_map[entry_i]
            .linked_list[link_i]
            .currently_running == 1)
        {
            timed_function_map[entry_i]
                .linked_list[link_i]
                .time_tally +=
                    (platform_get_current_time_microsecs() -
                        timed_function_map[entry_i]
                            .linked_list[link_i]
                            .current_first_start_at);
            
            timed_function_map[entry_i]
                .linked_list[link_i]
                .current_first_start_at = 0;
        }
        
        timed_function_map[entry_i]
            .linked_list[link_i]
            .currently_running -= 1;
    }
#ifdef __cplusplus
}
#endif

void __attribute__((no_instrument_function))
setup_log() {
    // create a log for debug text
    log = (char *)malloc(LOG_SIZE);
    
    backtrace_circle = (BacktraceEntry *)
        malloc(sizeof(BacktraceEntry) * BACKTRACE_CIRCLE_SIZE);
    for (uint32_t i = 0; i < BACKTRACE_CIRCLE_SIZE; i++) {
        backtrace_circle[i].function_name[0] = '\0';
    }
    
    // create a hashmap for the functions in our app 
    // this is used for backtrace and profiling
    timed_function_map =
        (TimedFunctionLink *)malloc(
            TIMED_FUNCTION_MAP_SIZE * sizeof(TimedFunctionLink)); 
    // initialize values
    for (
        uint32_t i = 0;
        i < TIMED_FUNCTION_MAP_SIZE;
        i++)
    {
        for (
            uint32_t j = 0;
            j < TIMED_FUNCTION_LINK_SIZE;
            j++)
        {
            timed_function_map[i]
                .linked_list[j]
                .function_address = 0;
            timed_function_map[i]
                .linked_list[j]
                .time_tally = 0;
            timed_function_map[i]
                .linked_list[j]
                .times_ran = 0;
            timed_function_map[i]
                .linked_list[j]
                .current_first_start_at = 0;
            timed_function_map[i]
                .linked_list[j]
                .currently_running = 0;
        }
    }
}

void __attribute__((no_instrument_function))
internal_log_append_uint(
    const uint32_t to_append,
    const char * caller_function_name)
{
    char converted[100];
    uint_to_string(
        /* const uint32_t input: */
            to_append,
        /* char * recipient: */
            converted,
        /* const uint32_t recipient_size: */
            100);
    
    internal_log_append(
        converted,
        caller_function_name);
}

void __attribute__((no_instrument_function))
internal_log_append_int(
    const int32_t to_append,
    const char * caller_function_name)
{
    char converted[100];
    int_to_string(
        /* const int32_t input: */
            to_append,
        /* char * recipient: */
            converted,
        /* const uint32_t recipient_size: */
            100);
    
    internal_log_append(
        converted,
        caller_function_name);
}

void __attribute__((no_instrument_function))
internal_log_append_float(
    const float to_append,
    const char * caller_function_name)
{
    float temp_above_decimal =
        (float)(int32_t)to_append;
    int32_t below_decimal =
        (uint32_t)((to_append - temp_above_decimal) * 10000);
    int32_t above_decimal = (int32_t)temp_above_decimal;
    
    char str_above_decimal[100];
    int_to_string(
        /* const int32_t input: */
            above_decimal,
        /* char * recipient: */
            str_above_decimal,
        /* const uint32_t recipient_size: */
            100);
    char str_below_decimal[100];
    int_to_string(
        /* const int32_t input: */
            below_decimal,
        /* char * recipient: */
            str_below_decimal,
        /* const uint32_t recipient_size: */
            100);
    
    internal_log_append(
        str_above_decimal,
        caller_function_name);

    internal_log_append(
        (char *)".",
        caller_function_name);
    
    internal_log_append(
        str_below_decimal,
        caller_function_name);

    internal_log_append(
        (char *)"f",
        caller_function_name);
}

void __attribute__((no_instrument_function))
internal_log_append(
    const char * to_append,
    const char * caller_function_name)
{
    #ifndef LOGGER_SILENCE 
    uint32_t initial_log_i = log_i;
    #endif
    
    uint32_t i = 0;
    copy_strings(
        /* recipient: */
            log,
        /* recipient_size: */
            LOG_SIZE,
        /* origin: */
            to_append);
    
    #ifndef LOGGER_SILENCE 
    printf(
        "%s",
        log + initial_log_i);
    #endif
}

void __attribute__((no_instrument_function))
add_profiling_stats_to_log()
{
    TimedFunction top30_timedfuncs[30];
    for (uint32_t i = 0; i < 30; i++) {
        top30_timedfuncs[i].times_ran = 0;
    }
    
    for (
        uint32_t i = 0;
        i < TIMED_FUNCTION_MAP_SIZE;
        i++)
    {
        for (
            uint32_t l = 0;
            l < TIMED_FUNCTION_LINK_SIZE;
            l++)
        {
            for (
                uint32_t j = 0;
                j < 30;
                j++)
            {
                if (
                    timed_function_map[i]
                        .linked_list[l]
                        .times_ran >
                            top30_timedfuncs[j].times_ran)
                {
                    top30_timedfuncs[j] =
                        timed_function_map[i].linked_list[l];
                    break;
                }
            }
        }
    }
    
    log_append("\n***TOP 30 FUNCTIONS***\n");
    for (uint32_t j = 0; j < 30; j++) {
        log_append("[");
        if (j < 10) {
            log_append(" ");
        }
        log_append_uint(j);
        log_append("] - ");
        log_append(top30_timedfuncs[j].function_name);
        log_append(" @");
        log_append_uint(top30_timedfuncs[j].function_address);
        log_append(" * times ran: ");
        log_append_uint(top30_timedfuncs[j].times_ran);
        log_append(", time spent: ");
        log_append_uint((uint32_t)top30_timedfuncs[j].time_tally);
        log_append("\n\n");
    }
}

void __attribute__((no_instrument_function))
log_dump() {
    
    add_profiling_stats_to_log();
    
    log[log_i] = '\0';
    log_i += 1;
    assert(log_i < LOG_SIZE);
    
    char full_filepath[1000];
    concat_strings(
        /* string_1             : */
            platform_get_application_path(),
        /* string_2             : */
            (char *)"/log.txt",
        /* output               : */
            full_filepath,
        /* output_size          : */
            1000);
    
    platform_write_file(
        /* filepath_destination : */
            full_filepath,
        /* const char * output  : */
            log,
        /* output_size          : */
            log_i + 1);
}

void __attribute__((no_instrument_function))
log_dump_and_crash() {
    log_dump();
    application_running = false;
}

