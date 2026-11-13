/*
Text in any language with T1: T1_font_glyph() asks the operating system's
fonts for one glyph at a time (CoreText on macOS, GDI on Windows), and this
file lays them out and blends them into its own pixels, like examples/noise.

What it shows of T1:
- T1_font_glyph(): a Unicode codepoint (not its UTF-8 bytes: this file
  decodes those itself) and a size in pixels give the glyph's coverage
  (0 to 255, row by row from the top), its place from the pen and how far
  the pen moves on. A font with the glyph is found for you: the lines below
  need several (Latin, Japanese, Greek, Cyrillic, symbols)
- *good is 0 when no installed font has the codepoint: we draw '?' then
- drawing a glyph takes the OS a while (tens of microseconds, the first one
  from a font more), so we draw each one once and keep it (THE GLYPHS)

What it doesn't: T1 gives glyphs, not text shaping. Every glyph goes left
to right on its own, so scripts that join or reorder their letters (Arabic,
Hebrew, Devanagari) come out as separate letters in the wrong order, and
there are no kerning pairs or ligatures. The Arabic line below shows it.

Build and run, from the tok_one folder:
    gcc concat.c -o concat && ./concat
    ./build/examples/text

(concat builds every example, with
    gcc -std=c11 -O3 examples/text/text.c examples/text/T1.c
        -o build/examples/text)
*/
#include <stdio.h>
#include <stdlib.h>

#include "T1.h"

#define WIDTH  960
#define HEIGHT 540

static uint8_t rgba[WIDTH * HEIGHT * 4];

/*
UTF-8: the codepoint at text, and how many bytes it takes. Broken UTF-8 is
U+FFFD (the replacement character), one byte at a time
*/
static uint32_t utf8_decode(const char * text, uint32_t * bytes) {
    const uint8_t * s = (const uint8_t *)text;
    *bytes = 1;
    if (s[0] < 0x80) { return s[0]; }

    uint32_t n = 0;
    uint32_t codepoint = 0;
    uint32_t least = 0;
    if (s[0] >= 0xC2 && s[0] <= 0xDF) {
        n = 2; codepoint = s[0] & 0x1F; least = 0x80;
    } else if ((s[0] & 0xF0) == 0xE0) {
        n = 3; codepoint = s[0] & 0x0F; least = 0x800;
    } else if (s[0] >= 0xF0 && s[0] <= 0xF4) {
        n = 4; codepoint = s[0] & 0x07; least = 0x10000;
    } else {
        return 0xFFFD;
    }
    for (uint32_t i = 1; i < n; i++) {
        // (the text's 0 at the end isn't 10xxxxxx: this stops there)
        if ((s[i] & 0xC0) != 0x80) { return 0xFFFD; }
        codepoint = (codepoint << 6) | (s[i] & 0x3F);
    }
    if (
        codepoint < least ||
        codepoint > 0x10FFFF ||
        (codepoint >= 0xD800 && codepoint <= 0xDFFF))
    {
        return 0xFFFD;
    }
    *bytes = n;
    return codepoint;
}

/*
THE GLYPHS: each codepoint and size drawn once by T1_font_glyph(), then
kept. A real app would keep them in a texture, or a hash table if it has
many; a list is enough for one screen
*/
#define GLYPH_SIDE_CAP 160
#define GLYPHS_CAP 256

typedef struct {
    uint32_t codepoint;
    float size;
    uint32_t w;
    uint32_t h;
    float left;
    float top;
    float advance;
    uint8_t missing; // no font has it: draw a '?'
    uint8_t alpha[GLYPH_SIDE_CAP * GLYPH_SIDE_CAP];
} Glyph;

static Glyph glyphs[GLYPHS_CAP];
static uint32_t glyphs_size = 0;
static uint32_t glyphs_missing = 0;
static uint64_t glyphs_us = 0;

static const Glyph * get_glyph(uint32_t codepoint, float size) {
    for (uint32_t i = 0; i < glyphs_size; i++) {
        if (glyphs[i].codepoint == codepoint && glyphs[i].size == size) {
            if (glyphs[i].missing) {
                return codepoint == '?' ? NULL : get_glyph('?', size);
            }
            return &glyphs[i];
        }
    }
    if (glyphs_size == GLYPHS_CAP) { return NULL; }

    Glyph * glyph = &glyphs[glyphs_size];
    glyph->codepoint = codepoint;
    glyph->size = size;
    uint8_t good = 0;
    uint64_t start_us = T1_time_us();
    T1_font_glyph(
        codepoint,
        size,
        glyph->alpha,
        sizeof(glyph->alpha),
        &glyph->w,
        &glyph->h,
        &glyph->left,
        &glyph->top,
        &glyph->advance,
        &good);
    glyphs_us += T1_time_us() - start_us;
    glyphs_size++;

    if (!good) {
        // no font has it (or it's too big for us): a '?' instead, and we
        // remember that, so the OS isn't asked again
        glyph->missing = 1;
        glyphs_missing++;
        return codepoint == '?' ? NULL : get_glyph('?', size);
    }
    return glyph;
}

// one glyph's coverage over the picture, in a colour
static void blend_glyph(
    const Glyph * glyph,
    float pen_x,
    float baseline,
    const uint8_t * color)
{
    // the glyph's top left in the picture (y goes down here, up in T1's)
    int32_t left = (int32_t)(pen_x + glyph->left + 0.5f);
    int32_t top = (int32_t)(baseline - glyph->top + 0.5f);
    for (uint32_t y = 0; y < glyph->h; y++) {
        int32_t py = top + (int32_t)y;
        if (py < 0 || py >= HEIGHT) { continue; }
        for (uint32_t x = 0; x < glyph->w; x++) {
            int32_t px = left + (int32_t)x;
            if (px < 0 || px >= WIDTH) { continue; }
            uint32_t a = glyph->alpha[y * glyph->w + x];
            uint8_t * pixel = rgba + (py * WIDTH + px) * 4;
            for (uint32_t c = 0; c < 3; c++) {
                pixel[c] = (uint8_t)(
                    (pixel[c] * (255 - a) + color[c] * a + 127) / 255);
            }
        }
    }
}

/*
A line of UTF-8 text, its pen starting at x on the baseline: each glyph
where the last one moved the pen to. Returns where the pen ends
*/
static float draw_text(
    const char * text,
    float x,
    float baseline,
    float size,
    const uint8_t * color)
{
    for (uint32_t i = 0; text[i] != '\0';) {
        uint32_t bytes;
        uint32_t codepoint = utf8_decode(text + i, &bytes);
        i += bytes;

        const Glyph * glyph = get_glyph(codepoint, size);
        if (glyph == NULL) { continue; }
        blend_glyph(glyph, x, baseline, color);
        x += glyph->advance;
    }
    return x;
}

typedef struct {
    const char * text;
    float size;
    uint8_t color[3];
} Line;

static const Line lines[] = {
    {"Hello, world!", 64.0f, {255, 214, 150}},
    {"こんにちは、世界", 56.0f, {255, 255, 255}},
    {"漢字 · Ελληνικά · Русский · Español", 40.0f, {170, 200, 255}},
    {"☃ ♫ → ∞ ≈ ★ ✓ ½", 40.0f, {150, 230, 170}},
    {"مرحبا  (letters only: no shaping)", 32.0f, {230, 160, 160}},
    // U+E000 is private: no font has a glyph for it, so it draws as '?'
    {"A codepoint no font has: \xEE\x80\x80", 28.0f, {150, 150, 170}},
};

int main(void) {
    char * fatal_error = NULL;
    T1_init(malloc, malloc, free, &fatal_error);
    T1_gpu_init(&fatal_error);
    T1Tex window_tex = T1_tex_new(NULL, WIDTH, HEIGHT,
        /* compress_if_possible: */ 0, &fatal_error);
    T1_window_open(window_tex, &fatal_error);
    if (fatal_error != NULL) {
        printf("%s\n", fatal_error);
        return 1;
    }

    // a dark background, then each line below the last
    for (uint32_t i = 0; i < WIDTH * HEIGHT; i++) {
        rgba[i * 4 + 0] = 20;
        rgba[i * 4 + 1] = 22;
        rgba[i * 4 + 2] = 32;
        rgba[i * 4 + 3] = 255;
    }
    float baseline = 50.0f;
    for (uint32_t i = 0; i < sizeof(lines) / sizeof(lines[0]); i++) {
        baseline += lines[i].size * 1.4f;
        draw_text(lines[i].text, 40.0f, baseline, lines[i].size,
            lines[i].color);
    }
    printf(
        "%u glyphs asked of the OS in %llu microseconds, %u missing\n",
        glyphs_size,
        (unsigned long long)glyphs_us,
        glyphs_missing);

    // the picture doesn't change: T1 shows it every frame
    T1_tex_update(window_tex, rgba);
    while (T1_frame_update()) {}

    return 0;
}
