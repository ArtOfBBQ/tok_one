#ifndef INIT_APPLICATION_H
#define INIT_APPLICATION_H

#include "T1_stdint.h"

void T1_init(
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
