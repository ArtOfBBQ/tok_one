/*
GPU particles: a compute pass moves 65536 particles stored in a T1Buf, and a
procedural draw pass turns each one into a small glowing quad. The CPU only
uploads the starting positions and, every frame, a few settings (the time
and the mouse position, which pulls the particles towards it). Once a
second it reads the first particle back from the GPU and prints it.

This folder has its own copy of T1.h and T1.c, like your app would: change
the settings at the top of T1.h (concat starts this copy over, naming the
window "particles").

Build and run, from the tok_one folder:
    gcc concat.c -o concat && ./concat
    ./build/examples/particles

(concat builds every example, with
    gcc -std=c11 -O2 examples/particles/particles.c examples/particles/T1.c
        -o build/examples/particles)
*/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "T1.h"

#define PARTICLES 65536

// The C and shader versions of these structs must match byte for byte
typedef struct {
    float pos[2]; // -1 to 1, like the window's normalized coordinates
    float vel[2];
    float color[4];
} Particle;

typedef struct {
    float mouse[2]; // where particles are pulled to
    float aspect; // the window's width / height
    float dt; // seconds since the last frame
} Settings;

static const char * shader_source =
"\n"
"typedef struct {\n"
"    float pos[2];\n"
"    float vel[2];\n"
"    float color[4];\n"
"} Particle;\n"
"\n"
"typedef struct {\n"
"    float mouse[2];\n"
"    float aspect;\n"
"    float dt;\n"
"} Settings;\n"
"\n"
"// one thread per particle: \"threads[0]\" is PARTICLES\n"
"kernel void step_particles(\n"
"    uint i [[ thread_position_in_grid ]],\n"
"    device Particle * particles [[ buffer(T1_BUFS_I + 0) ]],\n"
"    constant Settings * settings [[ buffer(T1_BUFS_I + 1) ]])\n"
"{\n"
"    device Particle * p = &particles[i];\n"
"    float2 pos = float2(p->pos[0], p->pos[1]);\n"
"    float2 vel = float2(p->vel[0], p->vel[1]);\n"
"    float2 to_mouse = float2(settings->mouse[0], settings->mouse[1]) - pos;\n"
"    float dist = max(length(to_mouse), 0.05f);\n"
"    vel += normalize(to_mouse) * (0.6f / dist) * settings->dt;\n"
"    vel *= 1.0f - 0.8f * settings->dt;\n"
"    pos += vel * settings->dt;\n"
"    p->pos[0] = pos.x;\n"
"    p->pos[1] = pos.y;\n"
"    p->vel[0] = vel.x;\n"
"    p->vel[1] = vel.y;\n"
"}\n"
"\n"
"struct ParticlePixel {\n"
"    float4 position [[ position ]];\n"
"    float2 corner;\n"
"    float4 color;\n"
"};\n"
"\n"
"// 6 vertices (2 triangles) per particle, \"vertex_count\" is PARTICLES * 6\n"
"vertex ParticlePixel particle_vertex(\n"
"    uint vid [[ vertex_id ]],\n"
"    const device Particle * particles [[ buffer(T1_BUFS_I + 0) ]],\n"
"    constant Settings * settings [[ buffer(T1_BUFS_I + 1) ]])\n"
"{\n"
"    const float2 corners[6] = {\n"
"        float2(-1, -1), float2(1, -1), float2(1, 1),\n"
"        float2(-1, -1), float2(1, 1), float2(-1, 1),\n"
"    };\n"
"    const device Particle * p = &particles[vid / 6];\n"
"    float2 corner = corners[vid % 6];\n"
"    float2 size = float2(0.006f / settings->aspect, 0.006f);\n"
"    ParticlePixel out;\n"
"    out.position = float4(\n"
"        float2(p->pos[0], p->pos[1]) + corner * size, 0.0f, 1.0f);\n"
"    out.corner = corner;\n"
"    out.color = float4(\n"
"        p->color[0], p->color[1], p->color[2], p->color[3]);\n"
"    return out;\n"
"}\n"
"\n"
"fragment float4 particle_fragment(ParticlePixel in [[ stage_in ]]) {\n"
"    float glow = max(1.0f - length(in.corner), 0.0f);\n"
"    return in.color * glow * glow;\n"
"}\n";

static float random_01(void) {
    return (float)rand() / (float)RAND_MAX;
}

int main(void) {
    char * fatal_error = NULL;
    T1_init(malloc, malloc, free, &fatal_error);
    T1_gpu_init(&fatal_error);

    uint8_t good = 0;
    char shader_error[512];
    T1_gpu_try_shader_source(
        shader_source, &good, shader_error, 512, &fatal_error);
    if (!good && fatal_error == NULL) {
        printf("%s\n", shader_error);
        return 1;
    }

    T1_window_open(&fatal_error);

    T1Buf particles = T1_buf_new(sizeof(Particle) * PARTICLES, &fatal_error);
    T1Buf settings_buf = T1_buf_new(sizeof(Settings), &fatal_error);
    if (fatal_error != NULL) {
        printf("%s\n", fatal_error);
        return 1;
    }

    // A ring of particles, slowly circling, colored by angle
    Particle * start = malloc(sizeof(Particle) * PARTICLES);
    for (uint32_t i = 0; i < PARTICLES; i++) {
        float angle = random_01() * 6.2831853f;
        float radius = 0.3f + random_01() * 0.5f;
        start[i].pos[0] = cosf(angle) * radius;
        start[i].pos[1] = sinf(angle) * radius;
        start[i].vel[0] = -sinf(angle) * 0.3f;
        start[i].vel[1] = cosf(angle) * 0.3f;
        start[i].color[0] = 0.5f + 0.5f * cosf(angle);
        start[i].color[1] = 0.5f + 0.5f * cosf(angle + 2.094f);
        start[i].color[2] = 0.5f + 0.5f * cosf(angle + 4.189f);
        start[i].color[3] = 1.0f;
    }
    T1_buf_write(particles, 0, start, sizeof(Particle) * PARTICLES);
    free(start); // T1_buf_write() copied it

    uint32_t step = T1_pipeline_new_compute("step_particles", &good);
    uint32_t draw = T1_pipeline_new_draw(
        "particle_vertex",
        "particle_fragment",
        T1_BLEND_ADDITIVE,
        T1_CULL_NONE,
        &good);

    // The frame: move the particles, clear the window, draw them
    uint32_t step_pass = T1_pass_new_compute(step);
    T1_pass_set_u32(step_pass, "buf[0]", particles);
    T1_pass_set_u32(step_pass, "buf[1]", settings_buf);
    T1_pass_set_u32(step_pass, "threads[0]", PARTICLES);

    T1_pass_new_clear(T1_TEX_WINDOW, 0.0f, 0.0f, 0.02f, 1.0f);

    uint32_t draw_pass = T1_pass_new_draw(draw);
    T1_pass_set_u32(draw_pass, "color_tex[0]", T1_TEX_WINDOW);
    T1_pass_set_u32(draw_pass, "buf[0]", particles);
    T1_pass_set_u32(draw_pass, "buf[1]", settings_buf);
    T1_pass_set_u32(draw_pass, "vertex_count", PARTICLES * 6);

    uint32_t read = T1_BUF_READ_NONE;
    uint64_t next_read_us = 0;
    
    while (!T1_window_close_requested()) {
        T1_window_poll_events();
        if (!T1_window_visible()) { continue; }
        T1_frame_begin();

        if (T1_frame_should_update()) {
            float w = (float)T1_window_get_width();
            float h = (float)T1_window_get_height();

            // window points, y up, to -1..1
            Settings settings;
            settings.mouse[0] =
                T1_io_get_pos_x_this_frame(T1_IO_MOUSE) / w * 2.0f - 1.0f;
            settings.mouse[1] =
                T1_io_get_pos_y_this_frame(T1_IO_MOUSE) / h * 2.0f - 1.0f;
            settings.aspect = w / h;
            settings.dt = (float)T1_frame_elapsed_us() / 1000000.0f;
            if (settings.dt > 0.05f) { settings.dt = 0.05f; }
            T1_buf_write(settings_buf, 0, &settings, sizeof(Settings));
            
            // takes a frame or two, the loop goes on meanwhile
            if (read == T1_BUF_READ_NONE) {
                uint64_t now_us = T1_os_get_current_time_us();
                if (now_us >= next_read_us) {
                    read = T1_buf_read_start(particles, 0, sizeof(Particle));
                    next_read_us = now_us + 1000000;
                }
            } else {
                Particle first;
                if (T1_buf_read_finish(read, &first)) {
                    read = T1_BUF_READ_NONE;
                    printf(
                        "particle 0 is at (%.2f, %.2f)\n",
                        first.pos[0],
                        first.pos[1]);
                    fflush(stdout);
                }
            }
        }

        T1_frame_render();
        T1_window_present();
    }

    T1_shutdown();
    return 0;
}
