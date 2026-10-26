/*
The smallest T1 app: no GPU setup and no shaders. We draw our own pixels,
T1 gives us a window to show them in.

This folder has its own copy of T1.h and T1.c, like your app would: change
the settings at the top of T1.h (concat starts this copy over, naming the
window "noise").

Build and run, from the tok_one folder:
    gcc concat.c -o concat && ./concat
    ./build/examples/noise

(concat builds every example, with
    gcc -std=c11 -O2 examples/noise/noise.c examples/noise/T1.c
        -o build/examples/noise)
*/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "T1.h"

// SVGA, scaled to fit the window
#define WIDTH  800
#define HEIGHT 600

static uint8_t rgba[WIDTH * HEIGHT * 4];

// A random value from 0 to 1 for each point of an integer grid
static float hash(int32_t x, int32_t y) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return (float)(h >> 8) / 16777215.0f;
}

static float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

// Smoothly blends the random values of the 4 grid points around (x, y)
static float value_noise(float x, float y) {
    float floor_x = floorf(x);
    float floor_y = floorf(y);
    int32_t grid_x = (int32_t)floor_x;
    int32_t grid_y = (int32_t)floor_y;

    // smoothstep instead of a straight line hides the grid
    float tx = x - floor_x;
    float ty = y - floor_y;
    tx = tx * tx * (3.0f - 2.0f * tx);
    ty = ty * ty * (3.0f - 2.0f * ty);

    float top = lerp(hash(grid_x, grid_y), hash(grid_x + 1, grid_y), tx);
    float bottom = lerp(
        hash(grid_x, grid_y + 1), hash(grid_x + 1, grid_y + 1), tx);
    return lerp(top, bottom, ty);
}

/*
Fractal Brownian motion: layers ("octaves") of value noise, each twice as
detailed and half as strong as the one before. Each octave drifts by the
same amount on its own grid, so the fine detail moves slower than the big
shapes and the clouds seem to evolve.
*/
static float fbm(float x, float y, float drift) {
    float sum = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;
    float max_sum = 0.0f;
    for (int octave = 0; octave < 6; octave++) {
        sum += amplitude * value_noise(
            x * frequency + drift,
            y * frequency - drift * 0.5f);
        max_sum += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    return sum / max_sum;
}

int main(void) {
    char * fatal_error = NULL;
    T1_init(malloc, malloc, free, &fatal_error);
    T1_window_open(&fatal_error);
    if (fatal_error != NULL) {
        printf("%s\n", fatal_error);
        return 1;
    }

    for (uint32_t frame = 0; !T1_window_close_requested(); frame++) {
        T1_window_poll_events();

        float drift = (float)frame * 0.01f;
        for (uint32_t y = 0; y < HEIGHT; y++) {
            for (uint32_t x = 0; x < WIDTH; x++) {
                float v = fbm((float)x / 160.0f, (float)y / 160.0f, drift);
                v = v * v * (3.0f - 2.0f * v); // more contrast

                // From a night blue to a warm sunset glow
                uint8_t * pixel = rgba + (y * WIDTH + x) * 4;
                pixel[0] = (uint8_t)(255.0f * lerp(0.05f, 1.00f, v));
                pixel[1] = (uint8_t)(255.0f * lerp(0.08f, 0.75f, v * v));
                pixel[2] = (uint8_t)(255.0f * lerp(0.25f, 0.45f, v));
            }
        }

        T1_window_blit_rgba(rgba, WIDTH, HEIGHT);
    }

    T1_shutdown();
    return 0;
}
