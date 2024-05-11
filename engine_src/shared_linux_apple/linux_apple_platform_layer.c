#include "platform_layer.h"

#define MUTEXES_SIZE 80
static pthread_mutex_t mutexes[MUTEXES_SIZE];
static uint32_t next_mutex_id = 0;
/*
creates a mutex and return the ID of said mutex for you to store
*/
uint32_t platform_init_mutex_and_return_id(void) {
    
    log_assert(next_mutex_id + 1 < MUTEXES_SIZE);
    pthread_mutex_init(&(mutexes[next_mutex_id]), NULL);
    uint32_t return_value = next_mutex_id;
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
    return pthread_mutex_trylock(&mutexes[mutex_id]) == 0;
}

/*
returns whether or not a mutex was locked, and locks the mutex if it
was unlocked
*/
void platform_mutex_lock(
    const uint32_t mutex_id)
{
    log_assert(mutex_id < MUTEXES_SIZE);
    int return_value = pthread_mutex_lock(&(mutexes[mutex_id]));
    
    #ifdef LOGGER_IGNORE_ASSERTS
    (void)return_value;
    #endif
    
    log_assert(return_value == 0);
    return;
}

int32_t platform_mutex_unlock(const uint32_t mutex_id) {
    log_assert(mutex_id < MUTEXES_SIZE);
    int return_value = pthread_mutex_unlock(&(mutexes[mutex_id]));
    log_assert(return_value == 0);
    
    return return_value;
}
