#ifndef T1_IO_H
#define T1_IO_H

#include <stdint.h>
#include <stddef.h>

#include "T1_public_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    u64 timestamp;
    s32 touch_id_pierce;
    s32 touch_id_top;
    f32 screen_x;
    f32 screen_y;
    u8 checked_touch_ids;
    u8 handled;
} T1IOEvent;

// indexes in the interactions array
#define T1_IO_LAST_GPU_DATA               0
#define T1_IO_LAST_TOUCH_START            1
#define T1_IO_LAST_TOUCH_END              2
#define T1_IO_LAST_LCLICK_START           3
#define T1_IO_LAST_LCLICK_END             4
#define T1_IO_LAST_TOUCH_OR_LCLICK_START  5
#define T1_IO_LAST_TOUCH_OR_LCLICK_END    6
#define T1_IO_LAST_RCLICK_START           7
#define T1_IO_LAST_RCLICK_END             8
#define T1_IO_LAST_MOUSE_MOVE             9
#define T1_IO_LAST_TOUCH_MOVE            10
#define T1_IO_LAST_MOUSE_OR_TOUCH_MOVE   11

// permanent size of the interactions array
#define T1_IO_EVENTS_CAP 12
#define T1_IO_KEYMAP_CAP 1000

typedef struct {
    T1IOEvent events[T1_IO_EVENTS_CAP];
    f32 mouse_scroll_pos;
    u8 keymap[T1_IO_KEYMAP_CAP];
} T1IOState;

extern T1IOState * T1_io;

void
T1_io_init(
    void *(* arg_malloc_func)(u64));

void
T1_io_event_construct(
    T1IOEvent * to_construct);

void
T1_io_event_register(
    T1IOEvent * touch_record);

void
T1_io_register_keyup(u32 key_id);

void
T1_io_register_keydown(u32 key_id);

void
T1_io_register_mousescroll(const f32 amount);

#ifdef __cplusplus
}
#endif

#endif // T1_IO_H
