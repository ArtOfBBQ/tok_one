#ifndef INIT_APPLICATION_H
#define INIT_APPLICATION_H

#include <string.h> // strlcat

#include "T1_stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

extern u32 block_drawinmtkview;

void T1_appinit_before_gpu_init(
    u8 * success,
    char * error_message);
void T1_appinit_after_gpu_init_step1(
    u8 * success,
    char * error_message);
void T1_appinit_after_gpu_init_step2(
    s32 throwaway_threadarg);

void T1_appinit_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // INIT_APPLICATION_H
