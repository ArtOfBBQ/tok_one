#ifndef RENDERER_H
#define RENDERER_H

#include <inttypes.h>

#include "T1_std.h"
#include "T1_engineglobals.h"
#include "T1_simd.h"
#include "T1_platform_layer.h"
#include "T1_cpu_gpu_shared_types.h"
#include "T1_cpu_to_gpu_types.h"
#include "T1_lines.h"
#include "T1_zsprite.h"
#include "T1_particle.h"
#include "T1_uiinteraction.h"

#ifdef __cplusplus
extern "C" {
#endif

void renderer_init(void);

void renderer_hardware_render(
    T1GPUFrame * frame_data,
    uint64_t elapsed_nanoseconds);

#ifdef __cplusplus
}
#endif

#endif // RENDERER_H
