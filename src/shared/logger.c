#include "logger.h"

static char * log;
static uint32_t log_i = 0;

#define MAX_TIMED_FUNCTION_NAME 40
#define MAX_SIMULTANEOUS_FUNCTION_RUNS 4000
typedef struct TimedFunction {
    uint64_t function_address;
    char function_name[MAX_TIMED_FUNCTION_NAME];
    uint32_t times_ran;
    uint64_t time_tally;
    uint64_t started_at[MAX_SIMULTANEOUS_FUNCTION_RUNS];
} TimedFunction;

#define TIMED_FUNCTION_LINK_SIZE 10
typedef struct TimedFunctionLink {
    TimedFunction linked_list[TIMED_FUNCTION_LINK_SIZE];
} TimedFunctionLink;

#define TIMED_FUNCTION_MAP_SIZE 4096
TimedFunctionLink * timed_function_map = NULL;

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
        assert(entry_i < TIMED_FUNCTION_MAP_SIZE);
        
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
       
        // record +1 run for this function address
        timed_function_map[entry_i]
            .linked_list[link_i]
            .function_address =
                (uint64_t)this_fn;
        timed_function_map[entry_i]
            .linked_list[link_i]
            .times_ran += 1;
        
        // record when this function started running 
        uint32_t start_i = 0;
        while (
            timed_function_map[entry_i]
                .linked_list[link_i]
                .started_at[start_i] != 0)
        {
            if (
                start_i >= MAX_SIMULTANEOUS_FUNCTION_RUNS)
            {
                #ifndef LOGGER_SILENCE 
                printf(
                    "too many simultaneous runs of the same timed function. You can probably just add some memory for the linked list by increasing MAX_SIMULTANEOUS_FUNCTION_RUNS (currently %u) in logger.c\n",
                    MAX_SIMULTANEOUS_FUNCTION_RUNS);
                #endif
                application_running = false;
                assert(0);
            }
            start_i += 1;
        }
        
        timed_function_map[entry_i]
            .linked_list[link_i]
            .started_at[start_i] =
                platform_get_current_time_microsecs();
    }
    
    void __attribute__((no_instrument_function))
    __cyg_profile_func_exit(
        void *this_fn,
        void *call_site)
    {
        if (timed_function_map == NULL) { return; }
        uint32_t entry_i =
            (uint64_t)this_fn & (TIMED_FUNCTION_MAP_SIZE - 1);
        assert(entry_i < TIMED_FUNCTION_MAP_SIZE);
       
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
        assert(found_link);
        
        // find out when this func started running 
        uint32_t start_i = MAX_SIMULTANEOUS_FUNCTION_RUNS;
        while (
            timed_function_map[entry_i]
                .linked_list[link_i]
                .started_at[start_i] == 0)
        {
            if (start_i == 0) {
                #ifndef LOGGER_SILENCE 
                printf(
                    "function exited with no rec of it start\n");
                #endif
                application_running = false;
                assert(0);
            }
            start_i -= 1;
        }
        
        assert(
            timed_function_map[entry_i]
            .linked_list[link_i]
            .started_at[start_i] > 0);
        
        timed_function_map[entry_i]
            .linked_list[link_i]
            .time_tally +=
                (platform_get_current_time_microsecs() -
                    timed_function_map[entry_i]
                        .linked_list[link_i]
                        .started_at[start_i]);
        
        timed_function_map[entry_i]
            .linked_list[link_i]
            .started_at[start_i] = 0;
    }
#ifdef __cplusplus
}
#endif

void __attribute__((no_instrument_function))
register_function_name(
    uint64_t address,
    char * name)
{
    #ifndef LOGGER_SILENCE
    printf(
        "register function: %s at address %llu\n",
        name,
        address);
    #endif
    assert(timed_function_map != NULL);
    
    uint32_t entry_i =
        address & (TIMED_FUNCTION_MAP_SIZE - 1);
    assert(entry_i < TIMED_FUNCTION_MAP_SIZE);
    
    #ifndef LOGGER_SILENCE
    printf("check if already registered\n");
    #endif
    for (
        uint32_t link_i = 0;
        link_i < TIMED_FUNCTION_LINK_SIZE;
        link_i++)
    {
        #ifndef LOGGER_SILENCE
        printf("checking link: %u\n", link_i);
        #endif
        
        if (
            timed_function_map[entry_i]
                .linked_list[link_i]
                .function_address == address)
        {
            uint32_t i = 0;
            while (name[i] != '\0') {
                timed_function_map[entry_i]
                    .linked_list[link_i]
                    .function_name[i] =
                        name[i];
                assert(i < MAX_TIMED_FUNCTION_NAME);
            }
            
            timed_function_map[entry_i]
                .linked_list[link_i]
                .function_name[i] =
                    name[i + 1] = '\0';
            
            
            #ifndef LOGGER_SILENCE
            printf("address was registered, added name\n");
            #endif
            return;
        }
    }
    
    #ifndef LOGGER_SILENCE
    printf("was unregistered, add entry...\n");
    #endif
    for (
        uint32_t link_i = 0;
        link_i < TIMED_FUNCTION_LINK_SIZE;
        link_i++)
    {
        #ifndef LOGGER_SILENCE
        printf("checking link: %u\n", link_i);
        #endif
        
        if (
            timed_function_map[entry_i]
                .linked_list[link_i]
                .function_address
            == 0)
        {
            #ifndef LOGGER_SILENCE
            printf("found open spot..\n");
            #endif
            
            timed_function_map[entry_i]
                .linked_list[link_i]
                .function_address = address;
            
            #ifndef LOGGER_SILENCE
            printf("copying name string..\n");
            #endif
            uint32_t i = 0;
            while (name[i] != '\0') {
                timed_function_map[entry_i]
                    .linked_list[link_i]
                    .function_name[i] = name[i];
                if (i >= (MAX_TIMED_FUNCTION_NAME - 1)) {
                    printf(
                        "name %s is exceeding MAX_TIMED_FUNCTION_NAME: %u\n",
                        name,
                        MAX_TIMED_FUNCTION_NAME);
                    assert(0);
                }
                i++;
            }
            
            #ifndef LOGGER_SILENCE
            printf(
                "registered at %llu, link %u\n",
                timed_function_map[entry_i]
                    .linked_list[link_i]
                    .function_address,
                link_i);
            #endif
            return;
        }
    }
    
    
    #ifndef LOGGER_SILENCE
    printf(
        "failed to find spot for %s at address %llu\n",
        name,
        address);
    #endif
    assert(0);
}

void __attribute__((no_instrument_function))
setup_log() {
    // create a log for debug text
    log = (char *)malloc(LOG_SIZE);
    
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
            
            for (
                uint32_t k = 0;
                k < MAX_SIMULTANEOUS_FUNCTION_RUNS;
                k++)
            {
                timed_function_map[i]
                    .linked_list[j]
                    .started_at[k] = 0;
            }
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
    while (to_append[i] != '\0') {
        
        log[log_i] = to_append[i];
        log_i += 1;
        assert(log_i < LOG_SIZE);
        i += 1;
    }
    
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
        log_append("\n");
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

