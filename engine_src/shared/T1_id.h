#ifndef T1_ID_H
#define T1_ID_H

#include "T1_stdint.h"

#ifdef __cplusplus
extern "C" {
#endif


#define T1_ID_FPS_COUNTER 0
#define T1_ID_DEBUG_TEXT 1

#define T1_ID_MAX 8000

#define T1_ID_FIRST_NONUI 1011

#define T1_ID_LAST_UI_TOUCH 1000
#define T1_ID_FIRST_NONUI_TOUCH 1001

// retired objects are unaffected by animations
#define T1_ID_RETIRED_BUT_NOT_DELETED 20501

extern s32 T1_id_retired;

s32 T1_id_next_ui_element_id(void);
s32 T1_id_next_nonui_id(void);
s32 T1_id_next_ui_element_touch_id(void);
void T1_id_clear_ui_element_touch_ids(void);
s32 T1_id_next_nonui_touch_id(void);

#ifdef __cplusplus
}
#endif

#endif // T1_ID_H
