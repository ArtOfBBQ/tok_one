/*
Gamepad input with T1: a drawing of a controller whose buttons light up
while they're held, and whose sticks move with yours. No shaders, we draw
our own pixels like examples/noise.

What it shows of T1:
- gamepad buttons are T1IOKeys like keyboard keys: T1_io_key_is_down()
  for as long as one is held, e.g. T1_IO_GAMEPAD_A
- the sticks are positions, like the mouse: T1_io_get_pos_x_this_frame()
  and _y_ of T1_IO_GAMEPAD_LTHUMBSTICK, from -1 to 1, y pointing up
- the triggers are buttons, pressed once they're pulled far enough

Gamepads need T1_GAMEPAD_ACTIVE (on unless you turn it off at the top of
T1.h). T1 looks for a gamepad every frame on macOS, and every 2 seconds on
Windows (Xbox-style controllers), so plugging one in later works too.

Build and run, from the tok_one folder:
    gcc concat.c -o concat && ./concat
    ./build/examples/gamepad

(concat builds every example, with
    gcc -std=c11 -O3 examples/gamepad/gamepad.c examples/gamepad/T1.c
        -o build/examples/gamepad)
*/
#include <stdio.h>
#include <stdlib.h>

#include "T1.h"

#if T1_GAMEPAD_ACTIVE != T1_ACTIVE
#error "This example needs T1_GAMEPAD_ACTIVE, see the top of T1.h"
#endif

#define WIDTH  640
#define HEIGHT 360

#define STICK_RADIUS 42
#define STICK_TRAVEL 28 // how far a stick's dot moves from its middle

static uint8_t rgba[WIDTH * HEIGHT * 4];

// Drawing: plain C into rgba, nothing here is T1's
static void put_pixel(int32_t x, int32_t y, const uint8_t * color) {
    if (x < 0 || y < 0 || x >= WIDTH || y >= HEIGHT) { return; }

    uint8_t * pixel = rgba + (y * WIDTH + x) * 4;
    pixel[0] = color[0];
    pixel[1] = color[1];
    pixel[2] = color[2];
    pixel[3] = 255;
}

static void fill_rect(
    int32_t left,
    int32_t top,
    int32_t width,
    int32_t height,
    const uint8_t * color)
{
    for (int32_t y = top; y < top + height; y++) {
        for (int32_t x = left; x < left + width; x++) {
            put_pixel(x, y, color);
        }
    }
}

// a disc, or a ring of the given thickness
static void fill_circle(
    int32_t cx,
    int32_t cy,
    int32_t radius,
    int32_t thickness,
    const uint8_t * color)
{
    int32_t inner = radius - thickness;
    for (int32_t y = -radius; y <= radius; y++) {
        for (int32_t x = -radius; x <= radius; x++) {
            int32_t d = x * x + y * y;
            if (d <= radius * radius && (thickness <= 0 || d >= inner * inner)) {
                put_pixel(cx + x, cy + y, color);
            }
        }
    }
}

static const uint8_t background[3] = {18, 20, 28};
static const uint8_t body[3] = {34, 37, 50};
static const uint8_t idle[3] = {62, 66, 88};
static const uint8_t lit[3] = {255, 160, 60};
static const uint8_t stick_dot[3] = {225, 228, 240};
static const uint8_t green[3] = {80, 210, 110};
static const uint8_t red[3] = {235, 80, 80};
static const uint8_t blue[3] = {80, 150, 255};
static const uint8_t yellow[3] = {240, 210, 70};

// idle, or color while the T1IOKey is held
static const uint8_t * key_color(T1IOKey key, const uint8_t * color) {
    return T1_io_key_is_down(key) ? color : idle;
}

static void draw_stick(
    int32_t cx,
    int32_t cy,
    T1IOKey stick,
    T1IOKey click)
{
    // -1 to 1 with y up, like T1's window positions: the screen's y is down
    float x = T1_io_get_pos_x_this_frame(stick);
    float y = T1_io_get_pos_y_this_frame(stick);

    fill_circle(cx, cy, STICK_RADIUS, 4, key_color(click, lit));
    fill_circle(
        cx + (int32_t)(x * STICK_TRAVEL),
        cy - (int32_t)(y * STICK_TRAVEL),
        14,
        0,
        stick_dot);
}

static void draw(void) {
    fill_rect(0, 0, WIDTH, HEIGHT, background);
    fill_rect(120, 90, 400, 210, body);

    // triggers and shoulder buttons along the top
    fill_rect(140, 22, 120, 26, key_color(T1_IO_GAMEPAD_LTRIGGER, lit));
    fill_rect(380, 22, 120, 26, key_color(T1_IO_GAMEPAD_RTRIGGER, lit));
    fill_rect(140, 58, 120, 20, key_color(T1_IO_GAMEPAD_LSHOULDER, lit));
    fill_rect(380, 58, 120, 20, key_color(T1_IO_GAMEPAD_RSHOULDER, lit));

    draw_stick(200, 160, T1_IO_GAMEPAD_LTHUMBSTICK, T1_IO_GAMEPAD_LTHUMBSTICKBTN);
    draw_stick(390, 245, T1_IO_GAMEPAD_RTHUMBSTICK, T1_IO_GAMEPAD_RTHUMBSTICKBTN);

    // the d-pad
    fill_rect(239, 204, 22, 22, key_color(T1_IO_GAMEPAD_DPAD_UP, lit));
    fill_rect(239, 248, 22, 22, key_color(T1_IO_GAMEPAD_DPAD_DOWN, lit));
    fill_rect(217, 226, 22, 22, key_color(T1_IO_GAMEPAD_DPAD_LEFT, lit));
    fill_rect(261, 226, 22, 22, key_color(T1_IO_GAMEPAD_DPAD_RIGHT, lit));

    // A B X Y where an Xbox controller has them
    fill_circle(450, 128, 15, 0, key_color(T1_IO_GAMEPAD_Y, yellow));
    fill_circle(418, 160, 15, 0, key_color(T1_IO_GAMEPAD_X, blue));
    fill_circle(482, 160, 15, 0, key_color(T1_IO_GAMEPAD_B, red));
    fill_circle(450, 192, 15, 0, key_color(T1_IO_GAMEPAD_A, green));

    // options (back/view) and menu (start), and the home (guide) button
    fill_rect(282, 152, 26, 14, key_color(T1_IO_GAMEPAD_OPTIONS, lit));
    fill_rect(332, 152, 26, 14, key_color(T1_IO_GAMEPAD_MENU, lit));
    fill_circle(320, 118, 11, 0, key_color(T1_IO_GAMEPAD_HOME, lit));
}

int main(void) {
    char * fatal_error = NULL;
    T1_init(malloc, malloc, free, &fatal_error);
    T1_gpu_init(&fatal_error);
    T1Tex window_tex = T1_tex_new(NULL, WIDTH, HEIGHT, &fatal_error);
    T1_window_open(window_tex, &fatal_error);
    if (fatal_error != NULL) {
        printf("%s\n", fatal_error);
        return 1;
    }

    // T1_frame_update() reads the gamepad, so each frame sees it as it is
    while (T1_frame_update()) {
        draw();
        T1_tex_update(window_tex, rgba);
    }

    T1_shutdown();
    return 0;
}
