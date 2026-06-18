#include "T1_io.h"
#include "T1_platform_layer.h"
#include "T1_std.h"
#include "T1_log.h"


T1IOState * T1_io = NULL;

void
T1_io_init(
    void *(* arg_malloc_func)(size_t))
{
    T1_io = (T1IOState *)(arg_malloc_func(
        sizeof(T1IOState)));
    
    for (
        uint32_t m = 0;
        m < T1_IO_EVENTS_CAP;
        m++)
    {
        T1_io_event_construct(&T1_io->events[m]);
    }
    
    for (uint32_t i = 0; i < T1_IO_KEYMAP_CAP; i++) {
        T1_io->keymap[i] = false;
    }
}

void
T1_io_event_construct(
    T1IOEvent * to_construct)
{
    to_construct->touch_id_top = -1;
    to_construct->touch_id_pierce = -1;
    to_construct->screen_x = 0;
    to_construct->screen_y = 0;
    to_construct->timestamp = 0;
    to_construct->handled = true;
}

void
T1_io_event_register(
    T1IOEvent * touch_record)
{
    uint64_t timestamp = T1_os_get_current_time_us();
    
    touch_record->screen_x =
        T1_io->events[T1_IO_LAST_GPU_DATA].screen_x;
    touch_record->screen_y =
        T1_io->events[T1_IO_LAST_GPU_DATA].screen_y;
    touch_record->touch_id_top =
        T1_io->events[T1_IO_LAST_GPU_DATA].touch_id_top;
    touch_record->touch_id_pierce =
        T1_io->events[T1_IO_LAST_GPU_DATA].
            touch_id_pierce;
    touch_record->timestamp = timestamp;
    touch_record->checked_touch_ids = false;
    touch_record->handled = false;
}

void
T1_io_register_keyup(
    uint32_t key_id)
{
    T1_log_assert(key_id < T1_IO_KEYMAP_CAP);
    
    T1_io->keymap[key_id] = false;
}

void
T1_io_register_keydown(
    uint32_t key_id)
{
    T1_log_assert(key_id < T1_IO_KEYMAP_CAP);
    
    T1_io->keymap[key_id] = true;
}

void
T1_io_register_mousescroll(
    float amount)
{
    T1_io->mouse_scroll_pos += amount;
    
    if (T1_io->mouse_scroll_pos < -40.0f) {
        T1_io->mouse_scroll_pos = -40.0f;
    }
    if (T1_io->mouse_scroll_pos > 40.0f) {
        T1_io->mouse_scroll_pos = 40.0f;
    }
}
