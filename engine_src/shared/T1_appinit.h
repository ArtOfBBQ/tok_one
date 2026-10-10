#ifndef INIT_APPLICATION_H
#define INIT_APPLICATION_H

#include <string.h> // strlcat

#include "T1_stdint.h"

extern u32 block_drawinmtkview;

void T1_appinit_before_gpu_init(
    void (* callback_newthread_entry_fptr)(s32),
    void (* callback_update_fptr)(u64),
    void (* callback_onwindowresize_fptr)(void),
    void (* callback_onappclose_fptr)(void),
    void (* callback_evaluate_terminal_command)(
        char * command, char * response, u32),
    char ** const fatal_error);
void T1_appinit_after_gpu_init_step1(
    char ** const fatal_error);
void T1_appinit_after_gpu_init_step2(
    s32 throwaway_threadarg);

#endif // INIT_APPLICATION_H
