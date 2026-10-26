#ifndef T1_ID_H
#define T1_ID_H

#include "T1_stdint.h"

#define T1_ID_FPS_COUNTER 0
#define T1_ID_DEBUG_TEXT 1

#define T1_ID_MAX 8000

#define T1_ID_FIRST_NONUI 1011

#define T1_ID_LAST_UI_TOUCH 1000
#define T1_ID_FIRST_NONUI_TOUCH 1001

// retired objects are unaffected by animations
#define T1_ID_RETIRED_BUT_NOT_DELETED 20501

extern u32 T1_id_retired;

u32  T1_id_next_nonui_id(void);
// the id must no longer be used by any object, it will be handed out again
void T1_id_release(u32 T1_id);
u32  T1_id_next_ui_element_touch_id(void);
void T1_id_clear_ui_element_touch_ids(void);
u32  T1_id_next_nonui_touch_id(void);

#endif // T1_ID_H
