#ifndef INIT_APPLICATION_H
#define INIT_APPLICATION_H

#include <stddef.h>

#include "T1_stdint.h"

void T1_init(
    void * (* alloc_forever)(size_t size),
    void * (* alloc_temp)(size_t size),
    void   (* free_temp)(void * ptr),
    char ** const fatal_error);

void T1_gpu_init(
    char ** const fatal_error);

void T1_gpu_try_shader_source(
    const char * shader_source,
    u8 * good,
    char * error_recip,
    const u32 error_recip_cap,
    char ** const fatal_error);

void T1_shutdown(void);

#endif // INIT_APPLICATION_H
