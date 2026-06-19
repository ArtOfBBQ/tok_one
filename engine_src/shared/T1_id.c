#include "T1_id.h"

#include "T1_log.h"


// an object id identifies a group of graphical items (like 2 images and
// a bunch of letters that combine to make a button)
s32 T1_id_retired = T1_ID_RETIRED_BUT_NOT_DELETED + 1;

static s32 T1_zspriteid_latest_nonui = T1_ID_FIRST_NONUI;
static s32 T1_zspriteid_latest_ui = 2;

static s32 T1_zspriteid_latest_nonui_touch = T1_ID_FIRST_NONUI_TOUCH;
static s32 T1_zspriteid_latest_ui_touch = 2;

s32 T1_id_next_ui_element_id(void) {
    T1_log_assert(T1_zspriteid_latest_ui + 1 < T1_ID_FIRST_NONUI);
    return T1_zspriteid_latest_ui++;
}

s32 T1_id_next_nonui_id(void) {
    T1_log_assert(
        T1_zspriteid_latest_nonui + 1 >= T1_ID_FIRST_NONUI);
    T1_log_assert(T1_zspriteid_latest_nonui + 1 < T1_ID_MAX);
    return T1_zspriteid_latest_nonui++;
}

s32
T1_id_next_ui_element_touch_id(void)
{
    T1_log_assert(T1_zspriteid_latest_ui_touch >= 0);
    T1_log_assert(T1_zspriteid_latest_ui_touch < T1_ID_FIRST_NONUI_TOUCH);
    return T1_zspriteid_latest_ui_touch++;
}

void T1_id_clear_ui_element_touch_ids(void) {
    T1_zspriteid_latest_ui_touch = 0;
}

s32 T1_id_next_nonui_touch_id(void) {
    T1_log_assert(T1_zspriteid_latest_nonui_touch >=
        T1_ID_FIRST_NONUI_TOUCH);
    T1_log_assert(T1_zspriteid_latest_nonui_touch < INT32_MAX);
    return T1_zspriteid_latest_nonui_touch++;
}
