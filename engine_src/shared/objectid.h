#ifndef OBJECT_ID_H
#define OBJECT_ID_H

#ifdef __cplusplus
extern "C" {
#endif

#include "logger.h"

#define MAX_OBJECT_ID 12000

#define FIRST_NONUI_OBJECT_ID 211

#define LAST_UI_TOUCHABLE_ID 100
#define FIRST_NONUI_TOUCHABLE_ID 101

// retired objects are unaffected by animations
#define RETIRED_BUT_NOT_DELETED_ID  10501

extern int32_t retired_object_id;

// extern int32_t latest_ui_element_object_id;
// extern int32_t latest_nonui_object_id;

int32_t next_ui_element_object_id(void);
int32_t next_nonui_object_id(void);

int32_t next_ui_element_touchable_id(void);
void clear_ui_element_touchable_ids(void);
int32_t next_nonui_touchable_id(void);

#ifdef __cplusplus
}
#endif

#endif // OBJECT_ID_H
