#include "T1_id.h"

#include "T1.h"

// an object id identifies a group of graphical items (like 2 images and
// a bunch of letters that combine to make a button)
u32 T1_id_retired = T1_ID_RETIRED_BUT_NOT_DELETED + 1;

static u32 T1_id_latest_nonui = T1_ID_FIRST_NONUI;
static u32 T1_id_latest_ui = 2;

static u32 T1_touch_id_latest_nonui = T1_ID_FIRST_NONUI_TOUCH;
static u32 T1_touch_id_latest_ui = 2;

u32 T1_id_next_ui_element_id(void) {
    T1_assert(T1_id_latest_ui + 1 < T1_ID_FIRST_NONUI);
    return T1_id_latest_ui++;
}

u32 T1_id_next_nonui_id(void) {
    T1_assert(
        T1_id_latest_nonui + 1 >= T1_ID_FIRST_NONUI);
    T1_assert(T1_id_latest_nonui + 1 < T1_ID_MAX);
    return T1_id_latest_nonui++;
}

u32 T1_id_next_ui_element_touch_id(void)
{
    T1_assert(T1_touch_id_latest_ui >= 0);
    T1_assert(T1_touch_id_latest_ui < T1_ID_FIRST_NONUI_TOUCH);
    return T1_touch_id_latest_ui++;
}

void T1_id_clear_ui_element_touch_ids(void) {
    T1_touch_id_latest_ui = 0;
}

u32 T1_id_next_nonui_touch_id(void) {
    T1_assert(T1_touch_id_latest_nonui >=
        T1_ID_FIRST_NONUI_TOUCH);
    T1_assert(T1_touch_id_latest_nonui < INT32_MAX);
    return T1_touch_id_latest_nonui++;
}
