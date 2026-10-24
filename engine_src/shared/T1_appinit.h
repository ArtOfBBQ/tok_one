#ifndef INIT_APPLICATION_H
#define INIT_APPLICATION_H

#include "T1_stdint.h"

extern u32 block_drawinmtkview;

void T1_appinit_before_gpu_init(
    char ** const fatal_error);
void T1_appinit_after_gpu_init_step1(
    char ** const fatal_error);
void T1_appinit_after_gpu_init_step2(
    s32 throwaway_threadarg,
    char ** const fatal_error);

void T1_shutdown(void);

#endif // INIT_APPLICATION_H
