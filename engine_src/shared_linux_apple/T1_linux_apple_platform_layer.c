#include "T1_platform_layer.h"

typedef struct OSMutexID {
    pthread_mutex_t mutex;
    bool8_t initialized;
} OSMutexID;

static OSMutexID * mutexes = NULL;
static uint32_t next_mutex_id = 0;

void T1_platform_init(
    void ** unmanaged_memory_store,
    const uint32_t aligned_to)
{
    mutexes = *unmanaged_memory_store;
    
    size_t size = sizeof(OSMutexID) * T1_MUTEXES_SIZE;
    while (size % aligned_to != 0) {
        size += 1;
    }
    
    T1_std_memset(mutexes, 0, sizeof(OSMutexID) * T1_MUTEXES_SIZE);
    
    *unmanaged_memory_store = (void *)(
        ((char *)*unmanaged_memory_store) + size);
}

uint32_t
T1_platform_init_mutex_and_return_id(void)
{
    T1_log_assert(
        next_mutex_id + 1 < T1_MUTEXES_SIZE);
    T1_log_assert(
        !mutexes[next_mutex_id].initialized);
    
    int mutex_init_error_value = pthread_mutex_init(
        &(mutexes[next_mutex_id].mutex),
        NULL);
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    T1_log_assert(mutex_init_error_value == 0);
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    (void)mutex_init_error_value;
    #else
    #error
    #endif
    
    uint32_t return_value = next_mutex_id;
    
    mutexes[next_mutex_id].initialized = true;
    
    next_mutex_id++;
    return return_value;
}

/*
Attempt to lock a mutex and return True if succesful
*/
bool32_t T1_platform_mutex_trylock(const uint32_t mutex_id)
{
    /*
    If successful, pthread_mutex_trylock() will return zero.
    Otherwise, an error number will be returned to indicate the error.
    */
    T1_log_assert(mutexes[mutex_id].initialized);
    
    int return_val = pthread_mutex_trylock(&mutexes[mutex_id].mutex);
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    if (return_val != 0) {
        // EINVAL = The value specified by mutex is invalid
        T1_log_assert(return_val != EINVAL);
        
        // EPERM = Operation not permitted
        T1_log_assert(return_val != EPERM);
        
        // EBUSY = Mutex is already locked
        T1_log_assert(return_val == EBUSY);
        
    }
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    return return_val == 0;
}

void T1_platform_assert_mutex_locked(const uint32_t mutex_id) {
    #if LOGGER_IGNORE_ASSERTS
    int return_val = pthread_mutex_trylock(&mutexes[mutex_id].mutex);
    log_assert(return_val == EBUSY);
    #else
    (void)mutex_id;
    #endif
}

/*
returns whether or not a mutex was locked, and locks the mutex if it
was unlocked
*/
void T1_platform_mutex_lock(
    const uint32_t mutex_id)
{
    T1_log_assert(mutex_id < T1_MUTEXES_SIZE);
    T1_log_assert(mutexes[mutex_id].initialized);
    int return_value = pthread_mutex_lock(&(mutexes[mutex_id].mutex));
    
    #if T1_LOGGER_ASSERTS_ACTIVE
    log_assert(return_value == 0);
    #else
    (void)return_value;
    #endif
    
    return;
}

void T1_platform_mutex_unlock(const uint32_t mutex_id) {
    T1_log_assert(mutex_id < T1_MUTEXES_SIZE);
    T1_log_assert(mutexes[mutex_id].initialized);
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    int return_value =
        pthread_mutex_unlock(&(mutexes[mutex_id].mutex));
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    pthread_mutex_unlock(&(mutexes[mutex_id].mutex));
    #else
    #error
    #endif
    
    T1_log_assert(return_value == 0);
}
