#ifndef INIT_APPLICATION_H
#define INIT_APPLICATION_H

#include <stdint.h>
#include <string.h> // strlcat

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t block_drawinmtkview;

void T1_appinit_before_gpu_init(
    uint8_t * success,
    char * error_message);
void T1_appinit_after_gpu_init_step1(
    uint8_t * success,
    char * error_message);
void T1_appinit_after_gpu_init_step2(
    int32_t throwaway_threadarg);

void T1_appinit_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // INIT_APPLICATION_H
