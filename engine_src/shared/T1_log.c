#include "T1_log.h"
#include "T1_std.h"

#if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
#include <stdio.h>
#include <assert.h>
#elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
#else
#error
#endif


u8 T1_log_app_running = false;
c8 * T1_log_crash_msg = NULL;

#define T1_LOG_CRASH_STRING_SIZE 256
#define LOG_SIZE 500000
typedef struct {
    void * (* malloc)(u64);
    u32 (* create_mutex)(void);
    void (* mutex_lock)(u32);
    void (* mutex_unlock)(u32);
    u32 mutex_id;
    c8 * full;
    u32 full_i;
} T1LogState;

static T1LogState * T1_log_s = NULL;

#ifdef __cplusplus
extern "C" {
#endif

void T1_log_init(
    void * (* arg_log_malloc_func)(u64 size),
    u32 (* arg_log_create_mutex_func)(void),
    void (* arg_log_mutex_lock_func)(const u32 mutex_id),
    void (* arg_log_mutex_unlock_func)(const u32 mutex_id))
{
    T1_log_s = arg_log_malloc_func(sizeof(T1LogState));
    T1_std_memset(T1_log_s, 0, sizeof(T1LogState));
    
    T1_log_crash_msg = arg_log_malloc_func(
        T1_LOG_CRASH_STRING_SIZE);
    T1_std_memset(T1_log_crash_msg, 0, T1_LOG_CRASH_STRING_SIZE);
    
    T1_log_s->malloc = arg_log_malloc_func;
    T1_log_s->create_mutex = arg_log_create_mutex_func;
    T1_log_s->mutex_lock = arg_log_mutex_lock_func;
    T1_log_s->mutex_unlock = arg_log_mutex_unlock_func;
    
    // create a log for debug text
    T1_log_s->full_i = 0;
    
    if (T1_log_s->create_mutex != NULL) {
        T1_log_s->mutex_id = T1_log_s->create_mutex();
    }
}

#if T1_LOG_PRINTF == T1_ACTIVE
void T1_log_append_u32(u32 to_append)
{
    c8 converted[1000];
    T1_std_u32_to_string(
        /* const u32 input: */
            to_append,
        /* c8 * recipient: */
            converted);
    
    T1_log_append(converted);
}

void T1_log_append_c8(c8 to_append)
{
    c8 to_append_array[2];
    to_append_array[0] = to_append;
    to_append_array[1] = '\0';
    
    T1_log_append(to_append_array);
}

void T1_log_append_s32(s32 to_append)
{
    c8 converted[1000];
    T1_std_s32_to_string(
        /* const s32 input: */
            to_append,
        /* c8 * recipient: */
            converted);
    
    T1_log_append(converted);
}

void T1_log_append_f32(f32 to_append)
{
    c8 f32_str[1000];
    T1_std_f32_to_string(
        /* const s32 input: */
            to_append,
        /* c8 * recipient: */
            f32_str,
        /* const u32 recipient_size: */
            1000);
    
    T1_log_append(f32_str);
}

void T1_log_append(
    const c8 * to_append)
{
    (void)to_append;
    
    printf("%s", to_append);
    
    // logger_mutex_lock_func(logger_mutex_id);
}
#elif T1_LOG_PRINTF == T1_INACTIVE
void T1_log_append_u32(u32 to_append) {}
void T1_log_append_c8(c8 to_append) {}
void T1_log_append_s32(s32 to_append) {}
void T1_log_append_f32(f32 to_append) {}
void T1_log_append(const c8 * to_append) {}
#else
#error
#endif

void T1_log_dump(u8 * good) {
    *good = true;
}

void T1_log_dump_and_crash(
    const c8 * crash_message)
{
    u8 log_dump_succesful = false;
    T1_log_dump(&log_dump_succesful);
    
    if (T1_log_app_running) {
        u32 i = 0;
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
void T1_log_assert(u8 condition)
{
    if (condition || !T1_log_app_running) { return; }
    
    //Assertion failed: (0), function main, file test.c, line 6.
    c8 assert_failed_msg[512];
    assert_failed_msg[0] = '\0';
    
    T1_std_strcpy_cap(
        assert_failed_msg,
        512,
        "Assertion failed.");
    
    T1_log_dump_and_crash(assert_failed_msg);
}

void T1_log_warn(u8 condition)
{
    if (condition) { return; }
    
    // #if T1_LOG_PRINTF == T1_ACTIVE
    T1_log_append("WARN CONDITION triggered\n");
    //    #elif T1_LOG_PRINTF == T1_INACTIVE
    //    #else
    //    #error
    //    #endif
}
#elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
#else
#error
#endif

typedef struct TimerResults {
    u64 inprogress_start;
    u64 inprogress_end;
    u64 averaged_result_so_far;
    u32 runs;
} TimerResults;
