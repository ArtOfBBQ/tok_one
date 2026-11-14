#ifndef T1_H
#define T1_H

#include <stdint.h>
#include <stddef.h>

#include "T1_macro_settings.h"
#include "T1_types_public.h"

/*
APPLICATION STARTUP
T1_init(malloc, malloc, free, &fatal_error);
T1_gpu_init(&fatal_error);
T1_gpu_try_shader_source(...); // only for your own pipelines
T1Tex window_tex = T1_tex_new_rgba(width, height, &fatal_error);
T1_window_open(window_tex, &fatal_error);

T1 allocates only through your functions (aligned like malloc()).
alloc_forever's memory is never freed, alloc_temp's goes back to free_temp.
*/
void T1_init(
    void * (* alloc_forever)(size_t size),
    void * (* alloc_temp)(size_t size),
    void   (* free_temp)(void * ptr),
    char ** const fatal_error);
void T1_gpu_init(
    char ** const fatal_error);

/*
Compiles your shaders (T1's macro settings and GPU headers put in front) and
builds your pipelines from them. On failure *good is 0, error_recip (NULL,
or a cap of at least 512) says why, and the last good shaders stay. Until
one succeeds, only clear passes run.
*/
void T1_gpu_try_shader_source(
    const char * shader_source,
    uint8_t * good,
    char * error_recip,
    const uint32_t error_recip_cap,
    char ** const fatal_error);

// The first frame after this reports T1_frame_resized()
void T1_window_open(
    T1Tex tex,
    char ** const fatal_error);
// from the next frame on
void T1_window_set_tex(T1Tex tex);
void T1_window_toggle_fullscreen(void);
uint8_t T1_window_is_fullscreen(void);

/*
FATAL ERRORS
Pass &fatal_error (a char * set to NULL). A function does nothing if it's
already set, so check once after several calls. The first error stays, T1
owns the string, and T1 is unusable after one.
*/

/*
MAIN LOOP
while (T1_frame_update()) {
    if (T1_frame_resized()) { rebuild what depends on the window size }
    update(T1_frame_elapsed_us());
}

T1_frame_update() runs your passes, shows the window's T1Tex (scaled to fit,
black bands around), handles input and waits for the display. While the
window is hidden it waits, and that time doesn't count as elapsed. Returns 0
when the app should quit or the GPU was lost. Main thread only.
*/
uint8_t  T1_frame_update(void);
// The GPU is gone with everything on it: the app ends
uint8_t  T1_gpu_lost(void);
// A recent frame's GPU time (0 until one is known): compare on one GPU only
uint64_t T1_gpu_frame_us(void);
// "" if unknown, '?' for non-ASCII
const char * T1_gpu_name(void);
// 0: frames keep running (about 60 a second) while the window is hidden
void T1_window_set_pause_when_hidden(uint8_t pause);
uint8_t  T1_frame_resized(void);
uint64_t T1_frame_elapsed_us(void);
// In the screen's own pixels, the mouse's too (NULL for what you don't need)
void T1_window_get_size(uint32_t * width, uint32_t * height);

/*
INPUT (KEYBOARD, MOUSE, GAMEPAD)
Taps are cleared every frame: consume them on the frame they end. The
mouse is in window pixels, (0, 0) at the bottom left; a stick is -1 to 1.
*/
uint8_t   T1_io_key_is_down(T1IOKey key);
uint8_t   T1_io_key_consume_tap_began_frame(T1IOKey key);
uint8_t   T1_io_any_key_consume_tap_began_frame(void);
uint8_t   T1_io_key_consume_short_tap_this_frame(T1IOKey key);
uint8_t   T1_io_key_consume_long_tap_this_frame(T1IOKey key);
float     T1_io_get_pos_x_this_frame(T1IOKey key);
float     T1_io_get_pos_y_this_frame(T1IOKey key);
uint8_t   T1_io_consume_mouse_drag(float * delta_x, float * delta_y);

/*
TEXTURES
A texture takes updates in the format it was made for, and keeps them so.
BC1 (8 bytes a 4x4 block, colour) and BC3 (16, colour and alpha) are an
eighth and a quarter of rgba8's memory: sides in multiples of 4, only
sampled (no pass draws into, writes or reads them). A GPU that can't sample
BC fails T1_tex_new_bc*() in builds with asserts; release builds keep them
as rgba8 there. Updates take x, y, width and height in multiples of 4 (or
up to the edge), copy the pixels and return at once.
*/
T1Tex T1_tex_new_rgba(
    uint32_t width,
    uint32_t height,
    char ** const fatal_error);
T1Tex T1_tex_new_bc1(
    uint32_t width,
    uint32_t height,
    char ** const fatal_error);
T1Tex T1_tex_new_bc3(
    uint32_t width,
    uint32_t height,
    char ** const fatal_error);
void T1_tex_update_from_rgba(
    T1Tex tex,
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    const uint8_t * rgba);
void T1_tex_update_from_bc1(
    T1Tex tex,
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    const uint8_t * bc1);
void T1_tex_update_from_bc3(
    T1Tex tex,
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    const uint8_t * bc3);
void     T1_tex_delete(T1Tex tex);
uint32_t T1_tex_get_width(T1Tex tex);
uint32_t T1_tex_get_height(T1Tex tex);
// Like T1_buf_read_start(): rgba8 pixels, (0, 0) at the top left
uint32_t T1_tex_read_start(
    T1Tex tex,
    uint32_t x,
    uint32_t y,
    uint32_t w,
    uint32_t h);

/*
RENDERING
Passes run in the order you made them.

Shaders are a subset of MSL (T1_SHADER_CHECK_ACTIVE refuses the rest and
says where):
- no #include or "using namespace"
- floats only (no half), no typedefs but typedef struct {...} Name;
- shared constants at program scope with constant
- no pointers: buffers are entry point parameters,
  "const device MyStruct * things [[ buffer(T1_BUFS_I) ]]", used as
  things[i] (copy one into a local, or pass the whole buffer on under the
  same name, never &things[i])
- mul(v, m) for the row vector v times m, mul(a, b) for a then b, never *
- no implicit conversions between bool, ints and floats; both sides of ?:
  of one type
- a condition is one bool (vectors: any() or all()), % only on integers
- both sides of ||, && and ?: may run: nothing there may write; ?: only
  between numbers and vectors
- every path of a function that returns a value returns one; no recursion
- a for's variable has a name of its own among those around it
- = { ... } lists every element; no array parameters or array assignment
- structs at program scope
- threads write a buffer's vector one component at a time. Threadgroup
  vectors take literal indices only; a threadgroup write is indexed by the
  thread's position (or inside an if on it), and something must write it
- textures: T1_sample_nearest(), T1_sample_linear(),
  T1_sample_linear_repeat() (textures, tex, uv), T1_read(), on the
  T1Textures at texture(0)
- a compute pass's "write_tex": T1_write_tex_read(), _write(), _size()

A pipeline's function must exist in the loaded shaders, else *good is 0.

Pass properties (u32):
- "off": 1 skips the pass, it keeps its place
- draw: "vertex_count", "vertex_start" (no geometry is bound: build it from
  vertex_id), "color_tex[0]" to "color_tex[3]" (one size), "depth_tex" (a
  depth buffer its size, shared by passes with the same one, cleared by the
  first of the frame)
- compute: once per pixel of "write_tex", or per element of the grid
  "threads[0]" * "threads[1]" * "threads[2]" (0 counts as 1), or of
  "threads_buf" at "threads_buf_offset" (3 u32s read on the GPU, in groups,
  at most 65535 a side; "threads_dims" 1 to 3)
- draw and compute: "params[0]" to "params[7]" (T1_PARAMS_I), "buf[0]" to
  "buf[11]". Every pass gets T1GPUFrameConsts at T1_FRAME_CONSTS_I.

COMPUTE GROUPS
By the kernel position's type: uint 256 x 1 x 1, uint2 16 x 16 x 1, uint3
8 x 8 x 4. Threadgroup memory (at most 32 KB, uninitialized) is declared at
the top of a kernel's body. T1_threadgroup_barrier() only at the top level
of a kernel's body; a kernel with one needs a grid of whole groups.

COMPUTE ATOMICS
On a kernel's own "device uint * buf", returning the old value (store:
nothing), ordering nothing else; don't mix with plain buf[i] in a pass:
    T1_atomic_load(buf, i)                T1_atomic_store(buf, i, value)
    T1_atomic_add(buf, i, value)          T1_atomic_exchange(buf, i, value)
    T1_atomic_min(buf, i, value)          T1_atomic_max(buf, i, value)
    T1_atomic_and(buf, i, value)          T1_atomic_or(buf, i, value)
    T1_atomic_xor(buf, i, value)
    T1_atomic_compare_exchange(buf, i, expected, value)
*/
#define T1_BLEND_NONE                0
// tested against depth, doesn't write it
#define T1_BLEND_ADDITIVE            1
#define T1_BLEND_PREMULTIPLIED_ALPHA 2
#define T1_BLEND_STRAIGHT_ALPHA      3
#define T1_CULL_NONE  0
#define T1_CULL_BACK  1
#define T1_CULL_FRONT 2
uint32_t T1_pipeline_new_draw(
    const char * vertex_fn,
    const char * fragment_fn,
    uint8_t blend,
    uint8_t cull,
    uint8_t * good);
uint32_t T1_pipeline_new_compute(
    const char * kernel_fn,
    uint8_t * good);

uint32_t T1_pass_new_clear(
    T1Tex tex,
    float r,
    float g,
    float b,
    float a);
uint32_t T1_pass_new_draw(uint32_t pipeline);
uint32_t T1_pass_new_compute(uint32_t pipeline);
void     T1_pass_set_u32(uint32_t pass, const char * property, uint32_t value);
void     T1_pass_delete(uint32_t pass);
uint8_t  T1_pass_exists(uint32_t pass);
void     T1_pipeline_delete(uint32_t pipeline);

/*
BUFFERS
Zeroed at first; a pass's "buf[k]" is buffer(T1_BUFS_I + k). Writes are
copied at once and seen in order from the next frame on. Offsets and sizes
are multiples of 4. Metal's float3 takes 16 bytes.
*/
T1Buf T1_buf_new(uint64_t size, char ** const fatal_error);
void  T1_buf_write(
    T1Buf buf,
    uint64_t offset,
    const void * data,
    uint64_t size);
void  T1_buf_delete(T1Buf buf);

/*
The bytes as they are after the next frame's passes. start gives
T1_BUF_READ_NONE if too many are waiting; finish returns 0 until they're
there, then copies them to recip (once).
*/
uint32_t T1_buf_read_start(T1Buf buf, uint64_t offset, uint64_t size);
uint8_t  T1_buf_read_finish(uint32_t read, void * recip);

/*
FILES
T1_res_path() and T1_writable_path() give full paths (of the dir for ""),
the rest take full paths.
*/
void T1_res_path(const char * filename, char * recip, uint32_t recip_cap);
void T1_writable_path(
    const char * filename,
    char * recip,
    uint32_t recip_cap);
uint8_t T1_file_exists(const char * path);
uint64_t T1_file_size(const char * path);
// recip gets a 0 after the file: its cap must be larger
void T1_file_read(
    const char * path,
    char * recip,
    uint64_t recip_cap,
    char ** const fatal_error);
void T1_file_write(
    const char * path,
    const char * data,
    uint32_t size,
    uint8_t * good);
void T1_file_delete(const char * path);
void T1_file_copy(
    const char * src_path, const char * dst_path);
// each name followed by a 0
void T1_dir_filenames(
    const char * path,
    char * filenames,
    uint32_t filenames_cap);

/*
OPERATING SYSTEM
*/
uint64_t T1_time_us(void);
uint32_t T1_cpu_core_count(void);

void T1_thread_start(
    void *(*function_to_run)(void *), void * arg_on_heap);
uint32_t T1_mutex_new(void);
uint8_t T1_mutex_trylock(uint32_t mutex_id);
void T1_mutex_lock(uint32_t mutex_id);
void T1_mutex_unlock(uint32_t mutex_id);

// to stderr, no newline added
void T1_write_error(const char * text);

/*
FONTS
The system's fonts. A glyph is drawn as coverage (w * h bytes from the top)
into alpha; left and top place it from the pen on the baseline (y up). size
is ascent plus descent in pixels. *good is 0 if no font has the codepoint
or alpha_cap is too small (w and h then say how much); alpha NULL only
measures. Windows: codepoints up to 0xFFFF. One thread at a time.
T1_font_set_language() (BCP 47, NULL: the system's) picks Chinese, Japanese
or Korean shapes for shared codepoints.
*/
void T1_font_set_language(const char * language);
void T1_font_glyph(
    uint32_t codepoint,
    float size,
    uint8_t * alpha,
    uint32_t alpha_cap,
    uint32_t * w,
    uint32_t * h,
    float * left,
    float * top,
    float * advance,
    uint8_t * good);

/*
AUDIO
16-bit stereo at T1_AUDIO_RATE. fill makes every sample, frames * 2 int16_t
(left first), on T1's audio thread: it must never wait (no locks, allocation,
files or printing). Call T1_audio_start() once; *good is 0 without a device.
*/
#define T1_AUDIO_RATE 44100
#if T1_AUDIO_ACTIVE == T1_ACTIVE
typedef void (* T1AudioFill)(int16_t * out, uint32_t frames, void * user);
void T1_audio_start(T1AudioFill fill, void * user, uint8_t * good);
#elif T1_AUDIO_ACTIVE == T1_INACTIVE
#else
#error
#endif

/*
TOKENS
A tokenizer by rules, each giving its tokens a kind of your choosing:
- literal: exact text
- run: a byte of first_set, then any from rest_set. Sets are like
  "A-Za-z0-9_"; '^' first negates; "" is empty
- delimited: start, then up to stop (included if stop_in_token); the byte
  after escape never ends it
The longest match wins, then the rule added first. Discarded tokens aren't
kept; unmatched bytes make T1_TOKEN_UNMATCHED tokens. Call T1_token_build()
after the last rule. Tokens point into the text you ran on and last until
the next run. Lines count from 1. A delimited token's value leaves out start
and stop. Past the last token is a T1_TOKEN_END token of size 0.
*/
#define T1_TOKEN_UNMATCHED UINT32_MAX
#define T1_TOKEN_END       (UINT32_MAX - 1)
// never 0
uint32_t T1_token_new(void);
void     T1_token_delete(uint32_t tokenizer);
void     T1_token_add_literal(
    uint32_t tokenizer,
    const char * text,
    uint32_t kind,
    uint8_t discard);
void     T1_token_add_run(
    uint32_t tokenizer,
    const char * first_set,
    const char * rest_set,
    uint32_t kind,
    uint8_t discard);
void     T1_token_add_delimited(
    uint32_t tokenizer,
    const char * start,
    const char * stop,
    char escape,
    uint8_t stop_in_token,
    uint32_t kind,
    uint8_t discard);
void     T1_token_build(uint32_t tokenizer);
void     T1_token_run(
    uint32_t tokenizer,
    const char * text,
    uint64_t text_size);
uint32_t T1_token_count(uint32_t tokenizer);
uint32_t T1_token_kind(uint32_t tokenizer, uint32_t i);
// not 0-terminated
const char * T1_token_text(uint32_t tokenizer, uint32_t i);
const char * T1_token_value(uint32_t tokenizer, uint32_t i);
uint32_t T1_token_size(uint32_t tokenizer, uint32_t i);
uint32_t T1_token_line(uint32_t tokenizer, uint32_t i);
uint8_t  T1_token_equals(uint32_t tokenizer, uint32_t i, const char * str);
// cut short to fit
void     T1_token_copy(
    uint32_t tokenizer,
    uint32_t i,
    char * recip,
    uint32_t recip_cap);
// f64: 12, -0.5, .5, 1e-3; s64: -12, 0x1F
double   T1_token_parse_f64(uint32_t tokenizer, uint32_t i, uint8_t * good);
int64_t  T1_token_parse_s64(uint32_t tokenizer, uint32_t i, uint8_t * good);

#endif // T1_H
