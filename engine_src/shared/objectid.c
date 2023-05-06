#include "objectid.h"

// an object id identifies a group of graphical items (like 2 images and
// a bunch of letters that combine to make a button)
int32_t retired_object_id = RETIRED_BUT_NOT_DELETED_ID + 1;

int32_t latest_nonui_object_id = FIRST_NONUI_OBJECT_ID;
int32_t latest_ui_element_object_id = 1;

int32_t latest_nonui_touchable_id = FIRST_NONUI_TOUCHABLE_ID;
int32_t latest_ui_element_touchable_id = 0;

int32_t next_ui_element_object_id(void) {
    log_assert(latest_ui_element_object_id + 1 < FIRST_NONUI_OBJECT_ID);
    return latest_ui_element_object_id++;
}

int32_t next_nonui_object_id(void) {
    log_assert(latest_nonui_object_id + 1 >= FIRST_NONUI_OBJECT_ID);
    log_assert(latest_nonui_object_id + 1 < MAX_OBJECT_ID);
    return latest_nonui_object_id++;
}

int32_t next_ui_element_touchable_id(void) {
    log_assert(latest_ui_element_touchable_id >= 0);
    log_assert(latest_ui_element_touchable_id < FIRST_NONUI_TOUCHABLE_ID);
    return latest_ui_element_touchable_id++;
}

int32_t next_nonui_touchable_id(void) {
    log_assert(latest_nonui_touchable_id >= FIRST_NONUI_TOUCHABLE_ID);
    log_assert(latest_nonui_touchable_id < INT32_MAX);
    return latest_nonui_touchable_id++;
}
