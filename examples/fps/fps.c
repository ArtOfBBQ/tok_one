/*
How fast can a software renderer go? T1 caps blitting loops at 60 frames
per second by default, this example removes the cap and draws its own
frames per second counter.

This folder has its own copy of T1.h and T1.c, like your app would: change
the settings at the top of T1.h (concat starts this copy over, naming the
window "fps").

Build and run, from the tok_one folder:
    gcc concat.c -o concat && ./concat
    ./build/examples/fps

(concat builds every example, with
    gcc -std=c11 -O2 examples/fps/fps.c examples/fps/T1.c
        -o build/examples/fps)
*/
#include <stdio.h>
#include <stdlib.h>

#include "T1.h"

// Scaled to fit the window
#define WIDTH  280
#define HEIGHT 200

static uint8_t rgba[WIDTH * HEIGHT * 4];

// 3x5 pixel digits, 3 bits per row, top row in the highest bits
static const uint16_t digit_bits[10] = {
    0x7B6F, 0x2C97, 0x73E7, 0x73CF, 0x5BC9,
    0x79CF, 0x79EF, 0x7249, 0x7BEF, 0x7BCF,
};

static void draw_digit(uint32_t digit, uint32_t left, uint32_t top) {
    const uint32_t scale = 4;
    for (uint32_t y = 0; y < 5 * scale; y++) {
        for (uint32_t x = 0; x < 3 * scale; x++) {
            uint32_t bit = 14 - ((y / scale) * 3 + (x / scale));
            if ((digit_bits[digit] >> bit) & 1) {
                uint8_t * pixel = rgba + ((top + y) * WIDTH + left + x) * 4;
                pixel[0] = pixel[1] = pixel[2] = 255;
            }
        }
    }
}

static void draw_number(uint32_t number, uint32_t left, uint32_t top) {
    char text[16];
    int length = snprintf(text, sizeof(text), "%u", number);
    for (int i = 0; i < length; i++) {
        draw_digit((uint32_t)(text[i] - '0'), left + (uint32_t)i * 16, top);
    }
}

int main(void) {
    char * fatal_error = NULL;
    T1_init(malloc, malloc, free, &fatal_error);
    T1_window_open(&fatal_error);
    if (fatal_error != NULL) {
        printf("%s\n", fatal_error);
        return 1;
    }

    T1_window_set_fps_cap(0);

    uint64_t second_start_us = T1_os_get_current_time_us();
    uint32_t frames_this_second = 0;
    uint32_t fps = 0;

    while (!T1_window_close_requested()) {
        T1_window_poll_events();

        uint64_t now_us = T1_os_get_current_time_us();
        if (now_us - second_start_us >= 1000000) {
            fps = frames_this_second;
            printf("%u fps\n", fps);
            fflush(stdout);
            frames_this_second = 0;
            second_start_us = now_us;
        }
        frames_this_second++;

        // Moves with time, not frames, so it looks the same at any fps
        uint32_t t = (uint32_t)(now_us / 10000);
        for (uint32_t y = 0; y < HEIGHT; y++) {
            for (uint32_t x = 0; x < WIDTH; x++) {
                uint8_t * pixel = rgba + (y * WIDTH + x) * 4;
                pixel[0] = (uint8_t)((x + t) ^ y);
                pixel[1] = (uint8_t)((y + t) ^ x);
                pixel[2] = (uint8_t)(x ^ y ^ (t / 2));
            }
        }
        draw_number(fps, 8, 8);

        T1_window_blit_rgba(rgba, WIDTH, HEIGHT);
    }

    T1_shutdown();
    return 0;
}
