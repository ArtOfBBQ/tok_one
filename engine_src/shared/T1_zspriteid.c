#include "T1_zspriteid.h"

// an object id identifies a group of graphical items (like 2 images and
// a bunch of letters that combine to make a button)
int32_t T1_zspriteid_retired_zsprite_id = RETIRED_BUT_NOT_DELETED_ID + 1;

static int32_t latest_nonui_object_id = T1_FIRST_NONUI_ZSPRITE_ID;
static int32_t latest_ui_element_object_id = 2;

static int32_t latest_nonui_touch_id = FIRST_NONUI_TOUCHABLE_ID;
static int32_t latest_ui_element_touch_id = 2;

int32_t T1_zspriteid_next_ui_element_id(void) {
    log_assert(latest_ui_element_object_id + 1 < T1_FIRST_NONUI_ZSPRITE_ID);
    return latest_ui_element_object_id++;
}

int32_t T1_zspriteid_next_nonui_id(void) {
    log_assert(
        latest_nonui_object_id + 1 >= T1_FIRST_NONUI_ZSPRITE_ID);
    log_assert(latest_nonui_object_id + 1 < T1_ZSPRITE_ID_MAX);
    return latest_nonui_object_id++;
}

int32_t T1_zspriteid_next_ui_element_touch_id(void) {
    log_assert(latest_ui_element_touch_id >= 0);
    log_assert(latest_ui_element_touch_id < FIRST_NONUI_TOUCHABLE_ID);
    return latest_ui_element_touch_id++;
}

void T1_zspriteid_clear_ui_element_touch_ids(void) {
    latest_ui_element_touch_id = 0;
}

int32_t T1_zspriteid_next_nonui_touch_id(void) {
    log_assert(latest_nonui_touch_id >= FIRST_NONUI_TOUCHABLE_ID);
    log_assert(latest_nonui_touch_id < INT32_MAX);
    return latest_nonui_touch_id++;
}
