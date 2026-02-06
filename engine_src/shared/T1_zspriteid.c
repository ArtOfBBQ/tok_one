#include "T1_zspriteid.h"

// an object id identifies a group of graphical items (like 2 images and
// a bunch of letters that combine to make a button)
int32_t T1_zspriteid_retired = T1_ZSPRITEID_RETIRED_BUT_NOT_DELETED + 1;

static int32_t T1_zspriteid_latest_nonui = T1_ZSPRITEID_FIRST_NONUI;
static int32_t T1_zspriteid_latest_ui = 2;

static int32_t T1_zspriteid_latest_nonui_touch = T1_ZSPRITEID_FIRST_NONUI_TOUCH;
static int32_t T1_zspriteid_latest_ui_touch = 2;

int32_t T1_zspriteid_next_ui_element_id(void) {
    T1_log_assert(T1_zspriteid_latest_ui + 1 < T1_ZSPRITEID_FIRST_NONUI);
    return T1_zspriteid_latest_ui++;
}

int32_t T1_zspriteid_next_nonui_id(void) {
    T1_log_assert(
        T1_zspriteid_latest_nonui + 1 >= T1_ZSPRITEID_FIRST_NONUI);
    T1_log_assert(T1_zspriteid_latest_nonui + 1 < T1_ZSPRITE_ID_MAX);
    return T1_zspriteid_latest_nonui++;
}

int32_t
T1_zspriteid_next_ui_element_touch_id(void)
{
    T1_log_assert(T1_zspriteid_latest_ui_touch >= 0);
    T1_log_assert(T1_zspriteid_latest_ui_touch < T1_ZSPRITEID_FIRST_NONUI_TOUCH);
    return T1_zspriteid_latest_ui_touch++;
}

void T1_zspriteid_clear_ui_element_touch_ids(void) {
    T1_zspriteid_latest_ui_touch = 0;
}

int32_t T1_zspriteid_next_nonui_touch_id(void) {
    T1_log_assert(T1_zspriteid_latest_nonui_touch >=
        T1_ZSPRITEID_FIRST_NONUI_TOUCH);
    T1_log_assert(T1_zspriteid_latest_nonui_touch < INT32_MAX);
    return T1_zspriteid_latest_nonui_touch++;
}
