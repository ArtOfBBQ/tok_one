/*
The browser's platform layer: T1 compiled to WebAssembly (wasm32), in a web
page. The page is T1_web.html (this folder), which loads the app's .wasm
and gives it what a browser has: the window is a canvas that
fills the page, input comes from DOM events, the GPU is WebGPU (see
T1_gpu_webgpu.c), files are the page's (see FILES below).

The C side calls JavaScript through imports (T1_WEB_IMPORT, module "t1"),
all of them in T1_web.html. Imports take numbers only: strings and arrays go
as a pointer and a size into the wasm memory, which JS reads and writes.

THE LOOP: a browser runs nothing of the page while JS or wasm runs, so the
client's blocking loop (while (T1_frame_update())) can't wait the way it
does elsewhere. T1_os_window_pump_events() calls t1.wait(), an async JS
function: the browser's JavaScript Promise Integration (JSPI) suspends the
whole wasm call stack there and resumes it when the wait is over (the
display's next frame, or a timeout). Meanwhile the page handles its events,
which JS queues; after the wait, T1 takes them one by one (t1.next_event()),
like Windows' message pump.

The app is built for wasm's threads (wasm32-wasip1-threads): its memory is
a shared one that JS makes, so that other threads (the audio's, see AUDIO)
can run its code on the same memory. The page must be cross-origin isolated
for that (tools/serve.c sends the headers).

Threads are the C library's (pthreads, wasi-libc): each is a Web Worker
running another instance of the app on the same memory (t1_thread_js in
T1_web.html).
They can't call what only the page has (the GPU, the window, files, fonts):
the page says so if they try. The browser lets the page's own
thread (the main thread) never block, so a mutex it finds held spins until
it's free, other threads sleep (see MUTEXES). The C library's own locks
(malloc's) don't know that: a thread holding malloc's lock while the main
thread wants it would crash the main thread. (TODO, if that ever shows)
*/

#include "T1_os.h"

#include "T1_assert.h"
#include "T1_global.h"
#include "T1_io.h"
#include "T1_mem.h"
#include "T1_std.h"
#include "T1_types_public.h"

#define T1_WEB_IMPORT(name) \
    __attribute__((import_module("t1"), import_name(#name)))

T1_WEB_IMPORT(time_us) f64 T1_web_time_us(void);
T1_WEB_IMPORT(write_error) void T1_web_write_error(
    const char * text, u32 size);
T1_WEB_IMPORT(wait) void T1_web_wait(f64 max_wait_us);
T1_WEB_IMPORT(next_event) s32 T1_web_next_event(void * event);
T1_WEB_IMPORT(visible) s32 T1_web_visible(void);
T1_WEB_IMPORT(scale) f64 T1_web_scale(void);
T1_WEB_IMPORT(window_open) void T1_web_window_open(
    const char * title, u32 title_size);
T1_WEB_IMPORT(toggle_fullscreen) void T1_web_toggle_fullscreen(void);
T1_WEB_IMPORT(core_count) s32 T1_web_core_count(void);
T1_WEB_IMPORT(file_size) f64 T1_web_file_size(const char * path, u32 size);
T1_WEB_IMPORT(file_read) f64 T1_web_file_read(
    const char * path, u32 path_size, void * recip, f64 recip_cap);
T1_WEB_IMPORT(file_write) s32 T1_web_file_write(
    const char * path, u32 path_size, const void * data, u32 data_size);
T1_WEB_IMPORT(file_delete) void T1_web_file_delete(
    const char * path, u32 size);
T1_WEB_IMPORT(file_list) void T1_web_file_list(
    const char * dir, u32 dir_size, char * recip, u32 recip_cap);
T1_WEB_IMPORT(font_set_language) void T1_web_font_set_language(
    const char * code, u32 size);
T1_WEB_IMPORT(font_glyph) s32 T1_web_font_glyph(
    u32 codepoint,
    f32 size,
    u8 * alpha,
    u32 alpha_cap,
    void * measures);
#if T1_GAMEPAD_ACTIVE == T1_ACTIVE
T1_WEB_IMPORT(gamepad) s32 T1_web_gamepad(void * state);
#elif T1_GAMEPAD_ACTIVE == T1_INACTIVE
#else
#error
#endif

// what T1_web_next_event() fills in, see T1_WEB_EVENT_*
typedef struct {
    s32 kind;
    s32 key;
    f32 x;
    f32 y;
} T1WebEvent;

#define T1_WEB_EVENT_KEYDOWN    1
#define T1_WEB_EVENT_KEYUP      2
#define T1_WEB_EVENT_MOUSEMOVE  3 // x, y: points from the bottom left
#define T1_WEB_EVENT_WHEEL      4 // y: notches, up positive
#define T1_WEB_EVENT_RESIZE     5 // x, y: the canvas's size in points
#define T1_WEB_EVENT_BLUR       6 // the page lost the keyboard
#define T1_WEB_EVENT_FULLSCREEN 7 // key: 1 in, 0 out

#include <pthread.h>

typedef struct {
    b8  quit_requested;
    b8  window_opened;
} T1OSWebState;

static T1OSWebState T1_os_web_s;

static u32 T1_os_web_strlen(const char * text) {
    return text == NULL ? 0 : (u32)T1_std_strlen(text);
}

/*
TIME AND ERRORS
*/

u64 T1_os_get_current_time_us(void) {
    // performance.now(): since the page opened, microseconds or coarser
    return (u64)T1_web_time_us();
}

void T1_os_write_error(const char * text) {
    // JS collects it until a newline, then it goes to the console
    T1_web_write_error(text, T1_os_web_strlen(text));
}

/*
THE MACHINE
*/

u32 T1_os_get_page_size(void) {
    // WebAssembly's memory grows in pages of 64 KB
    return 65536;
}

u32 T1_os_get_cpu_logical_core_count(void) {
    s32 cores = T1_web_core_count();
    return cores > 0 ? (u32)cores : 1;
}

void T1_os_close_app(void) {
    // T1_frame_update() then returns 0, and the client's loop ends
    T1_os_web_s.quit_requested = 1;
}

f32 T1_os_get_screen_backing_scale_factor(void) {
    // window.devicePixelRatio: pixels per CSS pixel, T1's points
    f32 scale = (f32)T1_web_scale();
    return scale > 0.0f ? scale : 1.0f;
}

/*
THREADS
*/

// 1 on the threads T1_os_start_thread() made, 0 on the main thread
static _Thread_local u8 T1_os_web_on_worker = 0;

// What a new thread runs: its own copy, so threads started at the same time
// can't overwrite each other's (the thread frees it)
typedef struct {
    void *(* function_to_run)(void *);
    void * argument;
} T1OSThreadStart;

static void * T1_os_web_thread_main(void * thread_start) {
    T1OSThreadStart start = *(T1OSThreadStart *)thread_start;
    T1_mem_free_managed(thread_start);
    T1_os_web_on_worker = 1;
    
    char * dud_error = 0;
    u64 T1_thread_id = T1_os_get_T1_thread_id(&dud_error);
    T1_assert(dud_error == 0);
    (void)T1_thread_id;
    
    void * out = start.function_to_run(start.argument);
    
    // the thread ends: its T1 thread id goes to the next thread
    T1_os_forget_raw_thread_id((u64)(uintptr_t)pthread_self());
    return out;
}

void T1_os_start_thread(
    void *(*function_to_run)(void *),
    void * argument)
{
    T1OSThreadStart * start = T1_mem_malloc_managed(sizeof(T1OSThreadStart));
    start->function_to_run = function_to_run;
    start->argument = argument;
    
    pthread_t thread;
    s32 result = pthread_create(&thread, NULL, T1_os_web_thread_main, start);
    T1_assert(result == 0);
    if (result != 0) {
        T1_crash("T1_os_start_thread(): the browser started no thread");
    }
    // nobody joins our threads: their resources go when they end
    pthread_detach(thread);
}

u64 T1_os_get_T1_thread_id(char ** const fatal_error) {
    if (*fatal_error != 0) { return 0; }
    // (a pthread_t is a pointer here, never 0)
    return T1_os_raw_thread_id_to_T1_thread_id(
        (u64)(uintptr_t)pthread_self(),
        fatal_error);
}

/*
MUTEXES: a word each, 0 free and 1 held, taken by an atomic compare and
swap. A thread that finds it held sleeps on it (wasm's atomic wait) until
the unlock wakes it; the main thread may not sleep, it tries again until
it's free. With asserts, each one's owner (a pthread_t, 0 for none) catches
locking it twice on a thread and unlocking it from another
*/
static u32 T1_os_web_mutexes[T1_MUTEXES_CAP];
static uintptr_t T1_os_web_mutex_owners[T1_MUTEXES_CAP];
static u32 T1_os_web_mutexes_size = 0;

u32 T1_os_init_mutex_and_return_id(void) {
    u32 id = __atomic_fetch_add(&T1_os_web_mutexes_size, 1, __ATOMIC_SEQ_CST);
    T1_assert(id < T1_MUTEXES_CAP);
    __atomic_store_n(&T1_os_web_mutexes[id], 0, __ATOMIC_SEQ_CST);
    return id;
}

static b8 T1_os_web_mutex_take(u32 mutex_id) {
    u32 expected = 0;
    return __atomic_compare_exchange_n(
        &T1_os_web_mutexes[mutex_id],
        &expected,
        1,
        /* weak: */ 0,
        __ATOMIC_ACQUIRE,
        __ATOMIC_RELAXED);
}

u8 T1_os_mutex_trylock(u32 mutex_id) {
    T1_assert(mutex_id < T1_os_web_mutexes_size);
    // like pthread_mutex_trylock(): 0 if it's held, by us too
    if (!T1_os_web_mutex_take(mutex_id)) { return 0; }
    T1_os_web_mutex_owners[mutex_id] = (uintptr_t)pthread_self();
    return 1;
}

void T1_os_mutex_lock(u32 mutex_id) {
    T1_assert(mutex_id < T1_os_web_mutexes_size);
    // this thread holds it already: it would wait for itself forever
    T1_assert(T1_os_web_mutex_owners[mutex_id] != (uintptr_t)pthread_self());
    while (!T1_os_web_mutex_take(mutex_id)) {
        if (T1_os_web_on_worker) {
            // sleeps while it's still 1 (no timeout)
            __builtin_wasm_memory_atomic_wait32(
                (int *)&T1_os_web_mutexes[mutex_id], 1, -1);
        }
    }
    T1_os_web_mutex_owners[mutex_id] = (uintptr_t)pthread_self();
}

void T1_os_mutex_unlock(u32 mutex_id) {
    T1_assert(mutex_id < T1_os_web_mutexes_size);
    // only the thread that locked it may unlock it
    T1_assert(T1_os_web_mutex_owners[mutex_id] == (uintptr_t)pthread_self());
    T1_os_web_mutex_owners[mutex_id] = 0;
    __atomic_store_n(&T1_os_web_mutexes[mutex_id], 0, __ATOMIC_RELEASE);
    __builtin_wasm_memory_atomic_notify((int *)&T1_os_web_mutexes[mutex_id], 1);
}

/*
FILES: the page has no file system, T1_web.html keeps one in memory. Paths
start with "/res" (the app's resources: the files next to the page on its
server, fetched the first time they're asked for, read-only) or
"/writables" (kept in the browser's storage for this page, IndexedDB:
written there as they change, read back before main() runs)
*/

void T1_os_get_dir_separator(char * recipient) {
    recipient[0] = '/';
    recipient[1] = '\0';
}

void T1_os_get_res_dir(char * recip, u32 recip_cap) {
    T1_std_strcpy_cap(recip, recip_cap, "/res");
}

void T1_os_get_writables_dir(char * recipient, u32 recipient_size) {
    T1_std_strcpy_cap(recipient, recipient_size, "/writables");
}

u8 T1_os_file_exists(const char * filepath) {
    return T1_web_file_size(filepath, T1_os_web_strlen(filepath)) >= 0.0;
}

u64 T1_os_get_filesize(const char * filepath) {
    f64 size = T1_web_file_size(filepath, T1_os_web_strlen(filepath));
    return size > 0.0 ? (u64)size : 0;
}

void T1_os_del_file(const char * filepath) {
    T1_web_file_delete(filepath, T1_os_web_strlen(filepath));
}

void T1_os_write_file(
    const char * filepath_destination,
    const char * output,
    u32 output_size,
    u8 * good)
{
    *good = (u8)T1_web_file_write(
        filepath_destination,
        T1_os_web_strlen(filepath_destination),
        output,
        output_size);
}

void T1_os_copy_file(
    const char * filepath_source,
    const char * filepath_destination)
{
    u64 size = T1_os_get_filesize(filepath_source);
    if (!T1_os_file_exists(filepath_source)) { return; }
    u8 * data = T1_mem_malloc_managed(size + 1);
    if (data == NULL) { return; }
    T1_web_file_read(
        filepath_source,
        T1_os_web_strlen(filepath_source),
        data,
        (f64)(size + 1));
    u8 good = 0;
    T1_os_write_file(filepath_destination, (char *)data, (u32)size, &good);
    T1_mem_free_managed(data);
}

b8 T1_os_mkdir_if_not_exist(const char * dirname) {
    // folders are only parts of paths here, they always exist
    (void)dirname;
    return 1;
}

void T1_os_get_filenames_in(
    const char * directory,
    char * filenames,
    const u32 filenames_cap)
{
    // names one after the other, each 0-terminated, like the other OSes
    T1_web_file_list(
        directory,
        T1_os_web_strlen(directory),
        filenames,
        filenames_cap);
}

void T1_os_read_file(
    const char * filepath,
    char * recip,
    u32 * recip_size,
    u64 recip_cap,
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return; }
    *recip_size = 0;
    if (recip_cap == 0) { return; }

    // like macOS and Windows: up to recip_cap bytes, with a 0 after them
    f64 read = T1_web_file_read(
        filepath,
        T1_os_web_strlen(filepath),
        recip,
        (f64)recip_cap);
    if (read < 0.0) {
        recip[0] = '\0';
        *fatal_error = "T1_os_read_file(): no such file";
        return;
    }
    *recip_size = (u32)read;
    recip[*recip_size] = '\0';
}

/*
THE WINDOW: the page's canvas, the whole page
*/

static void T1_os_web_release_keys(void) {
    for (u32 key = 0; key <= T1_IO_MOUSE_OTHERCLICK4; key++) {
        if (
            key != T1_IO_KEYBOARD_ABOVEBOUNDS &&
            key != T1_IO_MOUSE &&
            T1_io_key_is_down((T1IOKey)key))
        {
            T1_io_register_keyup(key, /* debounces: */ 0);
        }
    }
}

static void T1_os_web_handle_event(const T1WebEvent * e) {
    switch (e->kind) {
        case T1_WEB_EVENT_KEYDOWN:
            if (e->key >= 0 && e->key < T1_IO_KEY_ABOVEBOUNDS) {
                T1_io_register_keydown((u32)e->key, /* debounces: */ 0);
            }
            break;
        case T1_WEB_EVENT_KEYUP:
            if (e->key >= 0 && e->key < T1_IO_KEY_ABOVEBOUNDS) {
                T1_io_register_keyup((u32)e->key, /* debounces: */ 0);
            }
            break;
        case T1_WEB_EVENT_MOUSEMOVE:
            if (!T1_global->block_mouse) {
                T1_io_register_key_move_to_pos(T1_IO_MOUSE, e->x, e->y);
            }
            break;
        case T1_WEB_EVENT_WHEEL: {
            // the same steps as on macOS, where a notch is about 1.0
            f32 delta = e->y;
            const f32 step = 0.1f;
            while (delta > step) {
                T1_io_register_keyup_force_up_short(T1_IO_MOUSE_WHEEL_UP);
                delta -= step;
            }
            while (delta < -step) {
                T1_io_register_keyup_force_up_short(T1_IO_MOUSE_WHEEL_DOWN);
                delta += step;
            }
            break;
        }
        case T1_WEB_EVENT_RESIZE:
            T1_global_update_window_size(
                e->x, e->y, T1_os_get_current_time_us());
            break;
        case T1_WEB_EVENT_BLUR:
            T1_os_web_release_keys();
            break;
        case T1_WEB_EVENT_FULLSCREEN:
            T1_global->fullscreen = e->key != 0;
            break;
        default:
            T1_assert(0);
            break;
    }
}

void T1_os_window_open(char ** const fatal_error) {
    if (*fatal_error != 0) { return; }
    if (!T1_global) {
        *fatal_error = "Tried to T1_window_open() before T1_init()";
        return;
    }

    // the page's title, then the canvas's size comes as a resize event
    T1_web_window_open(T1_APP_NAME, T1_os_web_strlen(T1_APP_NAME));
    T1_os_web_s.window_opened = 1;
    T1_global_update_window_pos(0.0f, 0.0f);

    T1WebEvent e;
    while (T1_web_next_event(&e)) { T1_os_web_handle_event(&e); }
}

/*
A page can go fullscreen only right after the user pressed a key or clicked
(the browser's rule): asked at another time, T1_web.html goes fullscreen at
the next key press or click instead
*/
void T1_os_toggle_fullscreen(void) {
    T1_web_toggle_fullscreen();
}

void T1_os_enter_fullscreen(void) {
    if (!T1_global->fullscreen) { T1_web_toggle_fullscreen(); }
}

void T1_os_window_pump_events(const u64 max_wait_us) {
    // suspends the app (see THE LOOP at the top), then takes the events
    T1_web_wait((f64)max_wait_us);

    T1WebEvent e;
    while (T1_web_next_event(&e)) { T1_os_web_handle_event(&e); }
}

u8 T1_os_window_close_requested(void) {
    // a closed tab ends everything at once, only the app asks to close
    return T1_os_web_s.quit_requested;
}

u8 T1_os_window_visible(void) {
    // the tab isn't in the background or minimized
    return T1_web_visible() != 0;
}

/*
GAMEPADS: the browser's Gamepad API, the first pad with the "standard"
layout (an Xbox controller's: A B X Y, bumpers, triggers, back, start,
sticks, the d-pad, home)
*/

#if T1_GAMEPAD_ACTIVE == T1_ACTIVE
// what T1_web_gamepad() fills in
typedef struct {
    u32 buttons_down; // bit b: button b of the standard layout
    f32 axes[4]; // left x, left y, right x, right y: down is positive
} T1WebGamepad;

// XInput's dead zone (7849 of 32767), like on Windows
#define T1_WEB_THUMB_DEADZONE 0.2395f

static f32 T1_os_web_thumb_axis(f32 value) {
    if (value > -T1_WEB_THUMB_DEADZONE && value < T1_WEB_THUMB_DEADZONE) {
        return 0.0f;
    }
    f32 out = value > 0.0f ?
        (value - T1_WEB_THUMB_DEADZONE) / (1.0f - T1_WEB_THUMB_DEADZONE) :
        (value + T1_WEB_THUMB_DEADZONE) / (1.0f - T1_WEB_THUMB_DEADZONE);
    return T1_std_clampf(out, -1.0f, 1.0f);
}

void T1_os_poll_gamepad_events(void) {
    // the standard layout's buttons, in its order
    static const T1IOKey keys[17] = {
        T1_IO_GAMEPAD_A,
        T1_IO_GAMEPAD_B,
        T1_IO_GAMEPAD_X,
        T1_IO_GAMEPAD_Y,
        T1_IO_GAMEPAD_LSHOULDER,
        T1_IO_GAMEPAD_RSHOULDER,
        T1_IO_GAMEPAD_LTRIGGER,
        T1_IO_GAMEPAD_RTRIGGER,
        T1_IO_GAMEPAD_OPTIONS, // back, select, share
        T1_IO_GAMEPAD_MENU, // start
        T1_IO_GAMEPAD_LTHUMBSTICKBTN,
        T1_IO_GAMEPAD_RTHUMBSTICKBTN,
        T1_IO_GAMEPAD_DPAD_UP,
        T1_IO_GAMEPAD_DPAD_DOWN,
        T1_IO_GAMEPAD_DPAD_LEFT,
        T1_IO_GAMEPAD_DPAD_RIGHT,
        T1_IO_GAMEPAD_HOME,
    };

    // no pad: every button up, like a pad that disconnected on Windows
    T1WebGamepad pad;
    T1_std_memset(&pad, 0, sizeof(T1WebGamepad));
    T1_web_gamepad(&pad);

    for (u32 b = 0; b < 17; b++) {
        if ((pad.buttons_down >> b) & 1) {
            T1_io_register_keydown(keys[b], /* debounces: */ 1);
        } else {
            T1_io_register_keyup(keys[b], /* debounces: */ 1);
        }
    }

    // T1's y is up, like macOS's GameController
    T1_io_register_key_move_to_pos(
        T1_IO_GAMEPAD_LTHUMBSTICK,
        T1_os_web_thumb_axis(pad.axes[0]),
        T1_os_web_thumb_axis(-pad.axes[1]));
    T1_io_register_key_move_to_pos(
        T1_IO_GAMEPAD_RTHUMBSTICK,
        T1_os_web_thumb_axis(pad.axes[2]),
        T1_os_web_thumb_axis(-pad.axes[3]));
}
#elif T1_GAMEPAD_ACTIVE == T1_INACTIVE
#else
#error
#endif

/*
FONTS: the browser draws the glyph (a 2D canvas's fillText), with its own
fallback through every font it has. T1_web.html sizes the font so that its
ascent plus descent is size
*/

void T1_os_font_set_language(const char * code) {
    T1_web_font_set_language(code, T1_os_web_strlen(code));
}

void T1_os_font_glyph(
    u32 codepoint,
    f32 size,
    u8 * alpha,
    u32 alpha_cap,
    u32 * w,
    u32 * h,
    f32 * left,
    f32 * top,
    f32 * advance,
    u8 * good)
{
    // w, h (as u32s), left, top, advance
    union { u32 u; f32 f; } measures[5];
    T1_std_memset(measures, 0, sizeof(measures));
    s32 result = T1_web_font_glyph(
        codepoint, size, alpha, alpha != NULL ? alpha_cap : 0, measures);
    // 0: no glyph, 1: measured (alpha too small or NULL), 2: drawn
    if (result == 0) { return; }
    *w = measures[0].u;
    *h = measures[1].u;
    *left = measures[2].f;
    *top = measures[3].f;
    *advance = measures[4].f;
    *good = alpha == NULL || result == 2 || *w == 0 || *h == 0;
}

/*
AUDIO: like on Windows, T1's audio thread (an ordinary T1 thread) keeps a
ring of samples topped up with the client's fill, T1_AUDIO_PLATFORM_FRAMES
ahead of what's heard. The browser's audio thread (an AudioWorklet in
T1_web.html, which runs no wasm) takes 128 frames at a time from the ring
and wakes the audio thread after each (an atomic notify on read_frames).
The AudioContext runs at T1_AUDIO_RATE (the browser converts to the
device's); browsers keep it silent until the user's first key press or
click
*/

#if T1_AUDIO_ACTIVE == T1_ACTIVE
#include "T1_audio.h"

// sets the AudioWorklet up (suspends, like t1.wait()): 1 if it's there
T1_WEB_IMPORT(audio_init) s32 T1_web_audio_init(
    s16 * ring, u32 ring_frames, u32 * read_frames, u32 * written_frames);

// a power of 2, room for what's kept ahead and a block the worklet takes
#define T1_WEB_AUDIO_RING_FRAMES 4096
// what the audio thread fills at a time, at most
#define T1_WEB_AUDIO_CHUNK_FRAMES 256

typedef struct {
    s16 ring[T1_WEB_AUDIO_RING_FRAMES * 2];
    s16 chunk[T1_WEB_AUDIO_CHUNK_FRAMES * 2];
    // frames since the start, the worklet's and the audio thread's
    u32 read_frames;
    u32 written_frames;
} T1WebAudio;

static T1WebAudio * T1_web_audio = NULL;

static void * T1_os_web_audio_thread(void * argument) {
    (void)argument;
    T1WebAudio * a = T1_web_audio;
    while (1) {
        const u32 read = __atomic_load_n(&a->read_frames, __ATOMIC_ACQUIRE);
        const u32 written = a->written_frames;
        const u32 ahead = written - read;
        if (ahead >= T1_AUDIO_PLATFORM_FRAMES) {
            // until the worklet took some (or 10 ms, in case it stopped)
            __builtin_wasm_memory_atomic_wait32(
                (int *)&a->read_frames, (int)read, 10000000);
            continue;
        }
        u32 frames = T1_AUDIO_PLATFORM_FRAMES - ahead;
        if (frames > T1_WEB_AUDIO_CHUNK_FRAMES) {
            frames = T1_WEB_AUDIO_CHUNK_FRAMES;
        }
        T1_audio_fill_platform(a->chunk, frames);
        for (u32 f = 0; f < frames; f++) {
            const u32 at = ((written + f) & (T1_WEB_AUDIO_RING_FRAMES - 1)) * 2;
            a->ring[at] = a->chunk[f * 2];
            a->ring[at + 1] = a->chunk[f * 2 + 1];
        }
        __atomic_store_n(&a->written_frames, written + frames, __ATOMIC_RELEASE);
    }
    return NULL;
}

void T1_os_audio_init(u8 * good) {
    *good = 0;
    T1_web_audio = T1_mem_malloc_unmanaged(sizeof(T1WebAudio));
    if (T1_web_audio == NULL) { return; }
    T1_std_memset(T1_web_audio, 0, sizeof(T1WebAudio));
    *good = (u8)T1_web_audio_init(
        T1_web_audio->ring,
        T1_WEB_AUDIO_RING_FRAMES,
        &T1_web_audio->read_frames,
        &T1_web_audio->written_frames);
}

void T1_os_audio_start_loop(void) {
    T1_os_start_thread(T1_os_web_audio_thread, NULL);
}
#elif T1_AUDIO_ACTIVE == T1_INACTIVE
#else
#error
#endif
