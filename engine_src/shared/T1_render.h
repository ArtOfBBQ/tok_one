#ifndef T1_RENDER_H
#define T1_RENDER_H

#include <inttypes.h>

#include "T1_cpu_to_gpu.h"

#ifdef __cplusplus
extern "C" {
#endif

void T1_render_init(void);

void T1_render_update(
    T1GPUFrame * frame_data,
    u64 elapsed_us);

#ifdef __cplusplus
}
#endif

#endif // T1_RENDER_H
