#ifndef T1_ZPSPRITEID_H
#define T1_ZPSPRITEID_H

#ifdef __cplusplus
extern "C" {
#endif

#include "T1_log.h"

#define T1_ZSPRITEID_FPS_COUNTER 0
#define T1_ZSPRITEID_DEBUG_TEXT 1

#define T1_ZSPRITE_ID_MAX 8000

#define T1_ZSPRITEID_FIRST_NONUI 1011

#define T1_ZSPRITEID_LAST_UI_TOUCH 1000
#define T1_ZSPRITEID_FIRST_NONUI_TOUCH 1001

// retired objects are unaffected by animations
#define T1_ZSPRITEID_RETIRED_BUT_NOT_DELETED 20501

extern int32_t T1_zspriteid_retired;

int32_t
T1_zspriteid_next_ui_element_id(void);

int32_t
T1_zspriteid_next_nonui_id(void);

int32_t
T1_zspriteid_next_ui_element_touch_id(void);

void
T1_zspriteid_clear_ui_element_touch_ids(void);

int32_t
T1_zspriteid_next_nonui_touch_id(void);

#ifdef __cplusplus
}
#endif

#endif // T1_ZPSPRITEID_H
