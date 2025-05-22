#ifndef RENDERER_H
#define RENDERER_H

#include <inttypes.h>

#include "common.h"
#include "window_size.h"
#include "simd.h"
#include "platform_layer.h"
#include "cpu_gpu_shared_types.h"
#include "cpu_to_gpu_types.h"
#include "lines.h"
#include "zpolygon.h"
#include "particle.h"
#include "userinput.h"

#ifdef __cplusplus
extern "C" {
#endif

void renderer_init(void);

void renderer_hardware_render(
    GPUDataForSingleFrame * frame_data,
    uint64_t elapsed_nanoseconds);

#ifdef __cplusplus
}
#endif

#endif // RENDERER_H
