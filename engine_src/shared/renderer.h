#ifndef RENDERER_H
#define RENDERER_H

#include "inttypes.h"

#include "common.h"
#include "window_size.h"
#include "simd.h"
#include "platform_layer.h"
#include "cpu_gpu_shared_types.h"
#include "cpu_to_gpu_types.h"
#include "zpolygon.h"
#include "particle.h"

#ifdef __cplusplus
extern "C" {
#endif

void init_renderer(void);
void free_renderer(void);

void hardware_render(
    GPUVertex * next_gpu_workload,
    uint32_t * next_workload_size,
    GPULightCollection * lights_for_gpu,
    uint64_t elapsed_nanoseconds);

//void software_render(
//    Vertex * next_gpu_workload,
//    uint32_t * next_gpu_workload_size,
//    uint64_t elapsed_nanoseconds);

#ifdef __cplusplus
}
#endif

#endif // RENDERER_H
