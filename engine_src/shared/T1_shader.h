#ifndef T1_SHADER_H
#define T1_SHADER_H

#include "T1_stdint.h"

/*
Checks that shader_source stays inside T1's MSL subset (see RENDERING in
T1.h), so T1 can translate it for other platforms. *good is 0 if it doesn't,
and error_recip (cap 512+, may be NULL) lists the first problems with their
line numbers in shader_source.
*/
void T1_shader_check(
    const c8 * shader_source,
    u8 * good,
    c8 * error_recip,
    u32 error_recip_cap);

/*
Translates shader_source (as for T1_gpu_try_shader_source, it's checked
first) to GLSL 4.50 for Vulkan, with T1's settings and GPU headers in front
like on Metal. Every entry point becomes a unit of its own, recip gets them
one after another, each starting with a line "//T1_UNIT vert|frag|comp
name". The descriptor layout a Vulkan backend must match is in T1_shader.c.
*/
void T1_shader_to_glsl(
    const c8 * shader_source,
    c8 * recip,
    u32 recip_cap,
    u8 * good,
    c8 * error_recip,
    u32 error_recip_cap);

#endif // T1_SHADER_H
