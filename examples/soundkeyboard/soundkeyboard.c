/*
Keyboard input and sound with T1. The keys A S D F G H J K play a C major
scale, the keycaps at the bottom light up while held, and a panel shows the
sound T1 is about to play: the left channel in orange, the right in blue.
The arrow keys move the panel.

What it shows of T1:
- audio: T1_audio_start(fill) once, and T1 calls fill on its audio thread
  whenever the speakers need more. T1 makes no sound of its own: this file
  is the whole mixer (THE MIXER below), a few dozen lines: notes made at
  startup, voices that play them, the main thread telling fill which keys
  were tapped through C11 atomics, and fill keeping a copy of what it
  played for the panel
- keys: T1_io_key_consume_tap_began_frame() for a press (once per tap),
  T1_io_key_is_down() for as long as a key is held
- T1_frame_elapsed_us(): movement that doesn't depend on the frame rate
- no shaders: the app draws its own pixels and uploads them to the
  window's T1Tex, like examples/noise
- T1_font_glyph() for the keycaps' letters: the operating system's font,
  drawn once and kept (see examples/text)

T1_audio_start() only exists with T1_AUDIO_ACTIVE: concat turns it on in
this folder's copy of T1.h. In your own app, set it at the top of T1.h.

Build and run, from the tok_one folder:
    gcc concat.c -o concat && ./concat
    ./build/examples/soundkeyboard

(concat builds every example, with
    gcc -std=c11 -O3 examples/soundkeyboard/soundkeyboard.c
        examples/soundkeyboard/T1.c -o build/examples/soundkeyboard)
*/
#include <math.h>
#include <stdatomic.h>
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

// T1 plays 16-bit stereo at T1_AUDIO_RATE: samples go left, right, ...
#define NOTE_FRAMES (T1_AUDIO_RATE / 2)
#define NOTES_SIZE 8

static uint8_t rgba[WIDTH * HEIGHT * 4];
// what the panel draws: a copy of what fill played last
#define SCOPE_FRAMES (PANEL_WIDTH * FRAMES_PER_COLUMN)
static int16_t samples[SCOPE_FRAMES * 2];

static const float note_frequencies[NOTES_SIZE] = {
    261.63f, 293.66f, 329.63f, 349.23f, 392.00f, 440.00f, 493.88f, 523.25f,
};
static const T1IOKey note_keys[NOTES_SIZE] = {
    T1_IO_KEYBOARD_A, T1_IO_KEYBOARD_S, T1_IO_KEYBOARD_D, T1_IO_KEYBOARD_F,
    T1_IO_KEYBOARD_G, T1_IO_KEYBOARD_H, T1_IO_KEYBOARD_J, T1_IO_KEYBOARD_K,
};
static const char * note_names[NOTES_SIZE] = {
    "C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5",
};

/*
THE MIXER: what T1 asks for, made here. A note is a sound (its samples,
made at startup, never changed after: both threads may read them). A voice
plays one, from where it is; fill adds up the voices playing.

The main thread and the audio thread share only atomics: taps[i] counts
how often note i was tapped (the main thread adds, fill reads), so fill
starts a voice for each tap it hasn't seen yet. And scope[] is a copy of
what fill played last, which the main thread draws. Nothing waits on
anything: fill must never block (see AUDIO in T1.h)
*/
static int16_t note_samples[NOTES_SIZE][NOTE_FRAMES * 2];
static atomic_uint taps[NOTES_SIZE];
static atomic_short scope[SCOPE_FRAMES * 2];

// only fill touches these
#define VOICES_CAP 16
typedef struct {
    int32_t note; // -1: free
    uint32_t frame; // how far it played
} Voice;
static Voice voices[VOICES_CAP];
static unsigned taps_seen[NOTES_SIZE];

static void start_voice(int32_t note) {
    // a free voice, or (16 notes ringing) the one that played longest
    uint32_t pick = 0;
    for (uint32_t v = 0; v < VOICES_CAP; v++) {
        if (voices[v].note < 0) { pick = v; break; }
        if (voices[v].frame > voices[pick].frame) { pick = v; }
    }
    voices[pick].note = note;
    voices[pick].frame = 0;
}

/*
The notes' sum as 16 bits: as it is up to 3/4 of full scale, above that
bent smoothly toward full scale instead of cut off. Cut off, a loud sum
stays flat at full scale while it's over, and those flat tops crackle when
many notes ring at once. Only the mixer can do this: once a sample is 16
bits, how far over it was is lost
*/
static int16_t limit(int32_t sum) {
    const float knee = 24576.0f;
    const float room = 32767.0f - knee;
    float size = sum < 0 ? -(float)sum : (float)sum;
    if (size > knee) {
        const float over = size - knee;
        size = knee + room * over / (over + room); // never reaches 32767
    }
    return (int16_t)(sum < 0 ? -size : size);
}

// T1 calls this on its audio thread: frames frames, left and right
static void fill(int16_t * out, uint32_t frames, void * user) {
    (void)user;
    // the taps since the last call
    for (int32_t i = 0; i < NOTES_SIZE; i++) {
        const unsigned tapped = atomic_load(&taps[i]);
        while (taps_seen[i] != tapped) {
            start_voice(i);
            taps_seen[i]++;
        }
    }
    for (uint32_t f = 0; f < frames; f++) {
        // 32 bits: overlapping notes add up past 16, see limit()
        int32_t left = 0;
        int32_t right = 0;
        for (uint32_t v = 0; v < VOICES_CAP; v++) {
            Voice * voice = &voices[v];
            if (voice->note < 0) { continue; }
            const int16_t * note = note_samples[voice->note];
            left += note[voice->frame * 2 + 0];
            right += note[voice->frame * 2 + 1];
            voice->frame++;
            if (voice->frame == NOTE_FRAMES) { voice->note = -1; }
        }
        out[f * 2 + 0] = limit(left);
        out[f * 2 + 1] = limit(right);
    }
    // the panel's copy: the last SCOPE_FRAMES frames played
    const uint32_t n = frames < SCOPE_FRAMES ? frames : SCOPE_FRAMES;
    const uint32_t keep = SCOPE_FRAMES - n; // older ones move up
    for (uint32_t i = 0; i < keep * 2; i++) {
        atomic_store_explicit(&scope[i],
            atomic_load_explicit(&scope[i + n * 2], memory_order_relaxed),
            memory_order_relaxed);
    }
    for (uint32_t i = 0; i < n * 2; i++) {
        atomic_store_explicit(&scope[keep * 2 + i],
            out[(frames - n) * 2 + i], memory_order_relaxed);
    }
}

#define CAP_WIDTH  48
#define CAP_HEIGHT 58
#define CAP_GAP     8

/*
Drawing: plain C into rgba. The keycaps' letters are the operating
system's font (T1_font_glyph()), each one drawn once and kept: asking the
OS takes tens of microseconds, too long for every letter of every frame
*/
#define GLYPH_SIDE_CAP 64
#define GLYPHS_CAP 32

typedef struct {
    char c;
    float size;
    uint32_t w;
    uint32_t h;
    float left; // from the pen
    float top; // above the baseline
    float advance;
    uint8_t good; // 0: no font has it, drawn as nothing
    uint8_t alpha[GLYPH_SIDE_CAP * GLYPH_SIDE_CAP];
} Glyph;

static Glyph glyphs[GLYPHS_CAP];
static uint32_t glyphs_size = 0;

static const Glyph * get_glyph(char c, float size) {
    for (uint32_t i = 0; i < glyphs_size; i++) {
        if (glyphs[i].c == c && glyphs[i].size == size) {
            return &glyphs[i];
        }
    }
    if (glyphs_size == GLYPHS_CAP) { return NULL; }

    Glyph * glyph = &glyphs[glyphs_size++];
    glyph->c = c;
    glyph->size = size;
    // ASCII's codes are its Unicode codepoints
    T1_font_glyph(
        (uint32_t)(uint8_t)c,
        size,
        glyph->alpha,
        sizeof(glyph->alpha),
        &glyph->w,
        &glyph->h,
        &glyph->left,
        &glyph->top,
        &glyph->advance,
        &glyph->good);
    return glyph;
}

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

// ASCII text centred on x, on the baseline, blended over what's there
static void draw_text(
    const char * text,
    float x,
    float baseline,
    float size,
    uint8_t r,
    uint8_t g,
    uint8_t b)
{
    float width = 0.0f;
    for (uint32_t i = 0; text[i] != '\0'; i++) {
        const Glyph * glyph = get_glyph(text[i], size);
        if (glyph != NULL) { width += glyph->advance; }
    }

    const uint8_t color[3] = { r, g, b };
    float pen = x - width * 0.5f;
    for (uint32_t i = 0; text[i] != '\0'; i++) {
        const Glyph * glyph = get_glyph(text[i], size);
        if (glyph == NULL) { continue; }
        if (glyph->good) {
            // its top left in the picture (y goes down here, up in T1's)
            int32_t left = (int32_t)(pen + glyph->left + 0.5f);
            int32_t top = (int32_t)(baseline - glyph->top + 0.5f);
            for (uint32_t gy = 0; gy < glyph->h; gy++) {
                for (uint32_t gx = 0; gx < glyph->w; gx++) {
                    int32_t px = left + (int32_t)gx;
                    int32_t py = top + (int32_t)gy;
                    if (px < 0 || py < 0 || px >= WIDTH || py >= HEIGHT) {
                        continue;
                    }
                    uint32_t a = glyph->alpha[gy * glyph->w + gx];
                    uint8_t * pixel = rgba + (py * WIDTH + px) * 4;
                    for (uint32_t c = 0; c < 3; c++) {
                        pixel[c] = (uint8_t)(
                            (pixel[c] * (255 - a) + color[c] * a + 127) / 255);
                    }
                }
            }
        }
        pen += glyph->advance;
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
        char letter[2] = { (char)note_keys[i], '\0' };
        float middle = (float)left + CAP_WIDTH * 0.5f;
        float text_top = (float)(top + (down ? 10 : 6));
        draw_text(
            letter, middle, text_top + 26.0f, 34.0f,
            down ? 30 : 225, down ? 24 : 228, down ? 20 : 240);
        draw_text(
            note_names[i], middle, text_top + 41.0f, 15.0f,
            down ? 90 : 130, down ? 50 : 136, down ? 20 : 165);
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

    // What fill played last (a few milliseconds ago: what you hear)
    for (uint32_t i = 0; i < SCOPE_FRAMES * 2; i++) {
        samples[i] = atomic_load_explicit(&scope[i], memory_order_relaxed);
    }
    draw_channel(panel_left, panel_top, 1, 80, 160, 255);
    draw_channel(panel_left, panel_top, 0, 255, 160, 60);

    draw_keyboard();
}

// A sine wave that fades out, the same in the left and right channel
static void make_note(float frequency, int16_t * note) {
    for (uint32_t i = 0; i < NOTE_FRAMES; i++) {
        float t = (float)i / (float)T1_AUDIO_RATE;
        float fade = expf(-6.0f * t);
        float value = sinf(2.0f * 3.14159265f * frequency * t) * fade;
        int16_t sample = (int16_t)(value * 12000.0f);
        note[i * 2 + 0] = sample;
        note[i * 2 + 1] = sample;
    }
}

int main(void) {
    char * fatal_error = NULL;
    T1_init(malloc, malloc, free, &fatal_error);
    T1_gpu_init(&fatal_error);
    T1Tex window_tex = T1_tex_new_rgba(WIDTH, HEIGHT,
        &fatal_error);
    T1_window_open(window_tex, &fatal_error);

    if (fatal_error != NULL) {
        printf("%s\n", fatal_error);
        return 1;
    }

    // the notes before the audio starts: fill reads them from then on
    for (uint32_t i = 0; i < NOTES_SIZE; i++) {
        make_note(note_frequencies[i], note_samples[i]);
    }
    for (uint32_t v = 0; v < VOICES_CAP; v++) { voices[v].note = -1; }
    uint8_t audio_good = 0;
    T1_audio_start(fill, NULL, &audio_good);
    if (!audio_good) { printf("no audio device: the keys stay silent\n"); }

    float panel_left = (WIDTH - PANEL_WIDTH) / 2.0f;
    float panel_top = (HEIGHT - PANEL_HEIGHT) / 2.0f;

    while (T1_frame_update()) {
        /*
        A tap is reported for the frame it began in, and cleared by the
        next T1_frame_update(): consume it now or lose it
        */
        for (uint32_t i = 0; i < NOTES_SIZE; i++) {
            if (T1_io_key_consume_tap_began_frame(note_keys[i])) {
                // fill sees it the next time T1 calls it, a few ms on
                atomic_fetch_add(&taps[i], 1);
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
        T1_tex_update_from_rgba(window_tex, 0, 0, WIDTH, HEIGHT, rgba);
    }

    return 0;
}
