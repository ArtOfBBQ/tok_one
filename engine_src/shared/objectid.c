#include "objectid.h"

// an object id identifies a group of graphical items (like 2 images and
// a bunch of letters that combine to make a button)
int32_t retired_object_id = RETIRED_BUT_NOT_DELETED_ID + 1;
int32_t latest_object_id = 1;

int32_t latest_touchable_id = 0;

int32_t next_object_id(void) {
    log_assert(latest_object_id + 1 < MAX_OBJECT_ID);
    return latest_object_id++;
}

int32_t next_touchable_id(void) {
    log_assert(latest_touchable_id < INT32_MAX);
    return latest_touchable_id++;
}

