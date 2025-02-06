#include "platform_layer.h"

typedef struct OSMutexID {
    pthread_mutex_t mutex;
    bool32_t initialized;
} OSMutexID;

static OSMutexID * mutexes = NULL;
static uint32_t next_mutex_id = 0;

void platform_layer_init(
    void ** unmanaged_memory_store,
    const uint32_t aligned_to)
{
    mutexes = *unmanaged_memory_store;
    
    size_t size = sizeof(OSMutexID) * MUTEXES_SIZE;
    while (size % aligned_to != 0) {
        size += 1;
    }
    
    *unmanaged_memory_store = (void *)(
        ((char *)*unmanaged_memory_store) + size);
}

uint32_t platform_init_mutex_and_return_id(void) {
    
    log_assert(next_mutex_id + 1 < MUTEXES_SIZE);
    log_assert(!mutexes[next_mutex_id].initialized);
    int mutex_init_error_value = pthread_mutex_init(
        &(mutexes[next_mutex_id].mutex),
        NULL);
    
    log_assert(mutex_init_error_value == 0);
    
    uint32_t return_value = next_mutex_id;
    
    mutexes[next_mutex_id].initialized = true;
    
    next_mutex_id++;
    return return_value;
}

/*
Attempt to lock a mutex and return True if succesful
*/
bool32_t platform_mutex_trylock(const uint32_t mutex_id)
{
    /*
    If successful, pthread_mutex_trylock() will return zero.
    Otherwise, an error number will be returned to indicate the error.
    */
    log_assert(mutexes[mutex_id].initialized);
    
    int return_val = pthread_mutex_trylock(&mutexes[mutex_id].mutex);
    
    #ifndef LOGGER_IGNORE_ASSERTS
    if (return_val != 0) {
        // EINVAL = The value specified by mutex is invalid
        log_assert(return_val != EINVAL);
        
        // EPERM = Operation not permitted
        log_assert(return_val != EPERM);
        
        // EBUSY = Mutex is already locked
        log_assert(return_val == EBUSY);
        
    }
    #endif
    
    return return_val == 0;
}

void platform_assert_mutex_locked(const uint32_t mutex_id) {
    #ifndef LOGGER_IGNORE_ASSERTS
    int return_val = pthread_mutex_trylock(&mutexes[mutex_id].mutex);
    log_assert(return_val == EBUSY);
    #endif
}

/*
returns whether or not a mutex was locked, and locks the mutex if it
was unlocked
*/
void platform_mutex_lock(
    const uint32_t mutex_id)
{
    //    #ifdef PROFILER_ACTIVE
    //    profiler_start("platform_mutex_lock()");
    //    #endif
    
    log_assert(mutex_id < MUTEXES_SIZE);
    log_assert(mutexes[mutex_id].initialized);
    int return_value = pthread_mutex_lock(&(mutexes[mutex_id].mutex));
    
    #ifdef LOGGER_IGNORE_ASSERTS
    (void)return_value;
    #endif
    
    log_assert(return_value == 0);
    
    //    #ifdef PROFILER_ACTIVE
    //    profiler_end("platform_mutex_lock()");
    //    #endif
    return;
}

void platform_mutex_unlock(const uint32_t mutex_id) {
    log_assert(mutex_id < MUTEXES_SIZE);
    log_assert(mutexes[mutex_id].initialized);
    #ifndef LOGGER_IGNORE_ASSERTS
    int return_value =
    #endif
        pthread_mutex_unlock(&(mutexes[mutex_id].mutex));
    
    log_assert(return_value == 0);
}
