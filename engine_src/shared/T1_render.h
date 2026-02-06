#ifndef T1_RENDER_H
#define T1_RENDER_H

#include <inttypes.h>

#include "T1_std.h"
#include "T1_linalg3d.h"
#include "T1_global.h"
#include "T1_simd.h"
#include "T1_platform_layer.h"
#include "T1_cpu_gpu_shared_types.h"
#include "T1_cpu_to_gpu.h"
#include "T1_texquad.h"
#include "T1_zsprite.h"
#include "T1_particle.h"
#include "T1_io.h"
#include "T1_frame_anim.h"

#ifdef __cplusplus
extern "C" {
#endif

void T1_render_init(void);

void T1_render_update(
    T1GPUFrame * frame_data,
    uint64_t elapsed_us);

#ifdef __cplusplus
}
#endif

#endif // T1_RENDER_H
