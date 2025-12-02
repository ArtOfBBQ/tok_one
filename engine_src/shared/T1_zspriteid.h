#ifndef OBJECT_ID_H
#define OBJECT_ID_H

#ifdef __cplusplus
extern "C" {
#endif

#include "T1_logger.h"

#define T1_FPS_COUNTER_ZSPRITE_ID 0
#define T1_DEBUG_TEXT_ZSPRITE_ID 1

#define T1_ZSPRITE_ID_MAX 8000

#define T1_FIRST_NONUI_ZSPRITE_ID 1011

#define T1_LAST_UI_TOUCHABLE_ID 1000
#define FIRST_NONUI_TOUCHABLE_ID 1001

// retired objects are unaffected by animations
#define RETIRED_BUT_NOT_DELETED_ID 20501

extern int32_t T1_zspriteid_retired_zsprite_id;

int32_t T1_zspriteid_next_ui_element_id(void);
int32_t T1_zspriteid_next_nonui_id(void);

int32_t T1_zspriteid_next_ui_element_touch_id(void);
void T1_zspriteid_clear_ui_element_touch_ids(void);
int32_t T1_zspriteid_next_nonui_touch_id(void);

#ifdef __cplusplus
}
#endif

#endif // OBJECT_ID_H
