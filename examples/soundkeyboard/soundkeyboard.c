/*
Keyboard input and sound with T1. The keys A S D F G H J K play a C major
scale, the keycaps at the bottom light up while held, and a panel shows the
sound T1 is about to play: the left channel in orange, the right in blue.
The arrow keys move the panel.

What it shows of T1:
- sounds: register each with T1_audio_sound_id(), give it its samples once
  with T1_audio_set_samples(), then T1_audio_play() it any time
- T1_audio_peek(): the mix that's about to play
- keys: T1_io_key_consume_tap_began_frame() for a press (once per tap),
  T1_io_key_is_down() for as long as a key is held
- T1_frame_elapsed_us(): movement that doesn't depend on the frame rate
- no shaders: the app draws its own pixels and uploads them to the
  window's T1Tex, like examples/noise

The audio functions only exist with T1_AUDIO_ACTIVE: concat turns it on in
this folder's copy of T1.h. In your own app, set it at the top of T1.h.

Build and run, from the tok_one folder:
    gcc concat.c -o concat && ./concat
    ./build/examples/soundkeyboard

(concat builds every example, with
    gcc -std=c11 -O3 examples/soundkeyboard/soundkeyboard.c
        examples/soundkeyboard/T1.c -o build/examples/soundkeyboard)
*/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "T1.h"

#if T1_AUDIO_ACTIVE != T1_ACTIVE
#error "This example needs T1_AUDIO_ACTIVE, see the top of T1.h"
#endif

#define WIDTH  640
#define HEIGHT 360

// The panel shows 1024 frames (about 23 ms), every 4th of them
#define PANEL_WIDTH  256
#define PANEL_HEIGHT 128
#define FRAMES_PER_COLUMN 4

// The arrow keys move the panel this far per second
#define PANEL_SPEED 240.0f

// T1 plays 16-bit stereo at 44.1 kHz: samples go left, right, left, ...
#define SAMPLE_RATE 44100
#define NOTE_FRAMES (SAMPLE_RATE / 2)
#define NOTES_SIZE 8

static uint8_t rgba[WIDTH * HEIGHT * 4];
static int16_t samples[PANEL_WIDTH * FRAMES_PER_COLUMN * 2];
static int16_t note_samples[NOTE_FRAMES * 2];

static const float note_frequencies[NOTES_SIZE] = {
    261.63f, 293.66f, 329.63f, 349.23f, 392.00f, 440.00f, 493.88f, 523.25f,
};
static const T1IOKey note_keys[NOTES_SIZE] = {
    T1_IO_KEYBOARD_A, T1_IO_KEYBOARD_S, T1_IO_KEYBOARD_D, T1_IO_KEYBOARD_F,
    T1_IO_KEYBOARD_G, T1_IO_KEYBOARD_H, T1_IO_KEYBOARD_J, T1_IO_KEYBOARD_K,
};
// A sound's name is how T1 knows it, T1_audio_sound_id() gives its id
static const char * note_names[NOTES_SIZE] = {
    "C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5",
};
static int32_t note_sounds[NOTES_SIZE];

#define CAP_WIDTH  48
#define CAP_HEIGHT 58
#define CAP_GAP     8

/*
Drawing: plain C into rgba, nothing here is T1's. The font has only the
characters on the keycaps, 5 x 7 pixels each
*/
typedef struct {
    char c;
    uint8_t rows[7];
} Glyph;

static const Glyph glyphs[] = {
    {'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'B', {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}},
    {'C', {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}},
    {'D', {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}},
    {'E', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}},
    {'F', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}},
    {'G', {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}},
    {'H', {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'J', {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C}},
    {'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
    {'S', {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}},
    {'4', {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}},
    {'5', {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}},
};

static void put_pixel(
    int32_t x,
    int32_t y,
    uint8_t r,
    uint8_t g,
    uint8_t b)
{
    if (x < 0 || y < 0 || x >= WIDTH || y >= HEIGHT) { return; }

    uint8_t * pixel = rgba + (y * WIDTH + x) * 4;
    pixel[0] = r;
    pixel[1] = g;
    pixel[2] = b;
    pixel[3] = 255;
}

static void fill_rect(
    int32_t left,
    int32_t top,
    int32_t width,
    int32_t height,
    uint8_t r,
    uint8_t g,
    uint8_t b)
{
    for (int32_t y = top; y < top + height; y++) {
        for (int32_t x = left; x < left + width; x++) {
            put_pixel(x, y, r, g, b);
        }
    }
}

static void draw_char(
    char c,
    int32_t left,
    int32_t top,
    int32_t scale,
    uint8_t r,
    uint8_t g,
    uint8_t b)
{
    for (uint32_t i = 0; i < sizeof(glyphs) / sizeof(glyphs[0]); i++) {
        if (glyphs[i].c != c) { continue; }

        for (int32_t row = 0; row < 7; row++) {
            for (int32_t column = 0; column < 5; column++) {
                if (glyphs[i].rows[row] & (0x10 >> column)) {
                    fill_rect(
                        left + column * scale,
                        top + row * scale,
                        scale,
                        scale,
                        r, g, b);
                }
            }
        }
        return;
    }
}

static int32_t sample_to_y(int16_t sample, int32_t panel_top) {
    int32_t half = PANEL_HEIGHT / 2 - 2;
    return panel_top + PANEL_HEIGHT / 2 - ((int32_t)sample * half) / 32768;
}

// channel 0 is left, 1 is right
static void draw_channel(
    int32_t panel_left,
    int32_t panel_top,
    uint32_t channel,
    uint8_t r,
    uint8_t g,
    uint8_t b)
{
    int32_t previous_y = sample_to_y(samples[channel], panel_top);
    for (int32_t column = 0; column < PANEL_WIDTH; column++) {
        int16_t sample = samples[column * FRAMES_PER_COLUMN * 2 + channel];
        int32_t y = sample_to_y(sample, panel_top);
        int32_t from = y < previous_y ? y : previous_y;
        int32_t to = y < previous_y ? previous_y : y;
        for (int32_t at = from; at <= to; at++) {
            put_pixel(panel_left + column, at, r, g, b);
        }
        previous_y = y;
    }
}

static void draw_keyboard(void) {
    int32_t row_width = NOTES_SIZE * CAP_WIDTH + (NOTES_SIZE - 1) * CAP_GAP;
    int32_t row_left = (WIDTH - row_width) / 2;
    int32_t top = HEIGHT - CAP_HEIGHT - 12;

    for (int32_t i = 0; i < NOTES_SIZE; i++) {
        int32_t left = row_left + i * (CAP_WIDTH + CAP_GAP);
        // true from the key's press until its release
        uint8_t down = T1_io_key_is_down(note_keys[i]);

        fill_rect(
            left, top + 4, CAP_WIDTH, CAP_HEIGHT - 4,
            down ? 150 : 30, down ? 90 : 32, down ? 30 : 44);
        fill_rect(
            left, top + (down ? 4 : 0), CAP_WIDTH, CAP_HEIGHT - 8,
            down ? 255 : 54, down ? 160 : 58, down ? 60 : 78);

        // T1IOKeys of letters are their ASCII codes
        char letter = (char)note_keys[i];
        int32_t text_top = top + (down ? 10 : 6);
        draw_char(
            letter,
            left + (CAP_WIDTH - 5 * 4) / 2,
            text_top,
            4,
            down ? 30 : 225, down ? 24 : 228, down ? 20 : 240);

        int32_t name_left = left + (CAP_WIDTH - (2 * 5 * 2 + 2)) / 2;
        for (int32_t c = 0; c < 2; c++) {
            draw_char(
                note_names[i][c],
                name_left + c * (5 * 2 + 2),
                text_top + 30,
                2,
                down ? 90 : 130, down ? 50 : 136, down ? 20 : 165);
        }
    }
}

static void draw(int32_t panel_left, int32_t panel_top) {
    fill_rect(0, 0, WIDTH, HEIGHT, 18, 20, 28);

    fill_rect(
        panel_left - 2, panel_top - 2,
        PANEL_WIDTH + 4, PANEL_HEIGHT + 4,
        90, 96, 120);
    fill_rect(
        panel_left, panel_top,
        PANEL_WIDTH, PANEL_HEIGHT,
        8, 9, 14);
    fill_rect(
        panel_left, panel_top + PANEL_HEIGHT / 2,
        PANEL_WIDTH, 1,
        40, 44, 60);

    // The samples that play next, starting at T1_audio_play_cursor()
    T1_audio_peek(samples, PANEL_WIDTH * FRAMES_PER_COLUMN * 2);
    draw_channel(panel_left, panel_top, 1, 80, 160, 255);
    draw_channel(panel_left, panel_top, 0, 255, 160, 60);

    draw_keyboard();
}

// A sine wave that fades out, the same in the left and right channel
static void make_note(
    float frequency,
    const char * name,
    int32_t * sound_id,
    char ** const fatal_error)
{
    for (uint32_t i = 0; i < NOTE_FRAMES; i++) {
        float t = (float)i / (float)SAMPLE_RATE;
        float fade = expf(-6.0f * t);
        float value = sinf(2.0f * 3.14159265f * frequency * t) * fade;
        int16_t sample = (int16_t)(value * 20000.0f);
        note_samples[i * 2 + 0] = sample;
        note_samples[i * 2 + 1] = sample;
    }

    // T1 copies the samples, note_samples is free for the next note
    *sound_id = T1_audio_sound_id(name, fatal_error);
    if (*fatal_error != NULL) { return; }
    T1_audio_set_samples(*sound_id, note_samples, NOTE_FRAMES * 2);
}

int main(void) {
    char * fatal_error = NULL;
    T1_init(malloc, malloc, free, &fatal_error);
    T1_gpu_init(&fatal_error);
    T1Tex window_tex = T1_tex_new(NULL, WIDTH, HEIGHT, &fatal_error);
    T1_window_open(window_tex, &fatal_error);

    for (uint32_t i = 0; i < NOTES_SIZE; i++) {
        make_note(
            note_frequencies[i],
            note_names[i],
            &note_sounds[i],
            &fatal_error);
    }

    if (fatal_error != NULL) {
        printf("%s\n", fatal_error);
        return 1;
    }

    float panel_left = (WIDTH - PANEL_WIDTH) / 2.0f;
    float panel_top = (HEIGHT - PANEL_HEIGHT) / 2.0f;

    while (T1_frame_update()) {
        /*
        A tap is reported for the frame it began in, and cleared by the
        next T1_frame_update(): consume it now or lose it
        */
        for (uint32_t i = 0; i < NOTES_SIZE; i++) {
            if (T1_io_key_consume_tap_began_frame(note_keys[i])) {
                // volume 1, no pause before it plays
                T1_audio_play(note_sounds[i], 1.0f, 0);
            }
        }

        // T1_frame_elapsed_us() is 0 on the first frame
        float step = PANEL_SPEED * (float)T1_frame_elapsed_us() / 1000000.0f;
        if (T1_io_key_is_down(T1_IO_KEYBOARD_LEFTARROW))  { panel_left -= step; }
        if (T1_io_key_is_down(T1_IO_KEYBOARD_RIGHTARROW)) { panel_left += step; }
        if (T1_io_key_is_down(T1_IO_KEYBOARD_UPARROW))    { panel_top -= step; }
        if (T1_io_key_is_down(T1_IO_KEYBOARD_DOWNARROW))  { panel_top += step; }

        float max_left = (float)(WIDTH - PANEL_WIDTH - 2);
        float max_top = (float)(HEIGHT - PANEL_HEIGHT - 2);
        if (panel_left < 2.0f) { panel_left = 2.0f; }
        if (panel_top < 2.0f) { panel_top = 2.0f; }
        if (panel_left > max_left) { panel_left = max_left; }
        if (panel_top > max_top) { panel_top = max_top; }

        draw((int32_t)panel_left, (int32_t)panel_top);
        // The window shows window_tex from the next T1_frame_update() on
        T1_tex_update(window_tex, rgba);
    }

    T1_shutdown();
    return 0;
}
