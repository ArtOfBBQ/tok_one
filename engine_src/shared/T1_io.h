#ifndef T1_IO_H
#define T1_IO_H

#include "T1_std.h"
#include "T1_lightsource.h"
#include "T1_platform_layer.h"

#define T1_IO_KEYMAP_CAP 1000

#define T1_IO_KEY_LEFTARROW 0
#define T1_IO_KEY_RIGHTARROW 1
#define T1_IO_KEY_UPARROW 2
#define T1_IO_KEY_DOWNARROW 3
#define T1_IO_KEY_ENTER 4
#define T1_IO_KEY_ESCAPE 5
#define T1_IO_KEY_TAB 6
#define T1_IO_KEY_SPACEBAR 7
#define T1_IO_KEY_BACKSPACE 8
#define T1_IO_KEY_YENSIGN 9
#define T1_IO_KEY_KANABUTTON 10
#define T1_IO_KEY_ROMAJIBUTTON 11
#define T1_IO_KEY_ALT 12
#define T1_IO_KEY_WINKEY 13
#define T1_IO_KEY_CAPSLOCK 14
#define T1_IO_KEY_CONTROL 15
#define T1_IO_KEY_SHIFT 16
#define T1_IO_KEY_EXCLAM 33
#define T1_IO_KEY_DOUBLEQUOTE 34
#define T1_IO_KEY_NUMBERSIGN 35
#define T1_IO_KEY_DOLLARSIGN 36
#define T1_IO_KEY_PERCENTSIGN 37
#define T1_IO_KEY_AMPERSAND 38
#define T1_IO_KEY_APOSTROPHE 39
#define T1_IO_KEY_LEFTPARENTHESIS 40
#define T1_IO_KEY_RIGHTPARENTHESIS 41
#define T1_IO_KEY_ASTERISK 42
#define T1_IO_KEY_PLUS 43
#define T1_IO_KEY_COMMA 44
#define T1_IO_KEY_MINUS 45
#define T1_IO_KEY_FULLSTOP 46
#define T1_IO_KEY_BACKSLASH 47
#define T1_IO_KEY_0 48
#define T1_IO_KEY_1 49
#define T1_IO_KEY_2 50
#define T1_IO_KEY_3 51
#define T1_IO_KEY_4 52
#define T1_IO_KEY_5 53
#define T1_IO_KEY_6 54
#define T1_IO_KEY_7 55
#define T1_IO_KEY_8 56
#define T1_IO_KEY_9 57
#define T1_IO_KEY_COLON 58
#define T1_IO_KEY_SEMICOLON 59
#define T1_IO_KEY_LESSTHAN 60
#define T1_IO_KEY_EQUALS 61
#define T1_IO_KEY_GREATERTHAN 62
#define T1_IO_KEY_QUESTIONMARK 63
#define T1_IO_KEY_AT 64
#define T1_IO_KEY_A 65
#define T1_IO_KEY_B 66
#define T1_IO_KEY_C 67
#define T1_IO_KEY_D 68
#define T1_IO_KEY_E 69
#define T1_IO_KEY_F 70
#define T1_IO_KEY_G 71
#define T1_IO_KEY_H 72
#define T1_IO_KEY_I 73
#define T1_IO_KEY_J 74
#define T1_IO_KEY_K 75
#define T1_IO_KEY_L 76
#define T1_IO_KEY_M 77
#define T1_IO_KEY_N 78
#define T1_IO_KEY_O 79
#define T1_IO_KEY_P 80
#define T1_IO_KEY_Q 81
#define T1_IO_KEY_R 82
#define T1_IO_KEY_S 83
#define T1_IO_KEY_T 84
#define T1_IO_KEY_U 85
#define T1_IO_KEY_V 86
#define T1_IO_KEY_W 87
#define T1_IO_KEY_X 88
#define T1_IO_KEY_Y 89
#define T1_IO_KEY_Z 90
#define T1_IO_KEY_OPENSQUAREBRACKET  91
#define T1_IO_KEY_CLOSESQUAREBRACKET 93
#define T1_IO_KEY_HAT 94
#define T1_IO_KEY_UNDERSCORE 95
#define T1_IO_KEY_OPENCURLYBRACKET 123
#define T1_IO_KEY_PIPE 124
#define T1_IO_KEY_RIGHTCURLYBRACKET 125
#define T1_IO_KEY_TILDE 126
#define T1_IO_KEY_F1 136
#define T1_IO_KEY_F2 137
#define T1_IO_KEY_F3 138
#define T1_IO_KEY_F4 139
#define T1_IO_KEY_F5 140
#define T1_IO_KEY_F6 141
#define T1_IO_KEY_F7 142
#define T1_IO_KEY_F8 143
#define T1_IO_KEY_F9 144
#define T1_IO_KEY_F10 145
#define T1_IO_KEY_F11 146
#define T1_IO_KEY_F12 147
#define T1_IO_KEY_HOME 148
#define T1_IO_KEY_PAGEUP 149
#define T1_IO_KEY_END 150
#define T1_IO_KEY_PAGEDOWN 151

#ifdef __cplusplus
extern "C" {
#endif

void T1_io_init(
    void *(* arg_malloc_func)(size_t));

typedef struct {
    int32_t touch_id_pierce;
    int32_t touch_id_top;
    bool32_t checked_touch_ids;
    float screen_x;
    float screen_y;
    uint64_t timestamp;
    bool32_t handled;
} T1IOEvent;

void T1_io_event_construct(T1IOEvent * to_construct);

// indexes in the interactions array
#define T1_IO_LAST_GPU_DATA               0
#define T1_IO_LAST_TOUCH_START            1
#define T1_IO_LAST_TOUCH_END              2
#define T1_IO_LAST_LCLICK_START           3
#define T1_IO_LAST_LCLICK_END             4
#define T1_IO_LAST_TOUCH_OR_LCLICK_START  5
#define T1_IO_LAST_TOUCH_OR_LCLICK_END    6
#define T1_IO_LAST_RCLICK_START           7
#define T1_IO_LAST_RCLICK_END             8
#define T1_IO_LAST_MOUSE_MOVE             9
#define T1_IO_LAST_TOUCH_MOVE            10
#define T1_IO_LAST_MOUSE_OR_TOUCH_MOVE   11
extern T1IOEvent * T1_io_events;

extern bool32_t * T1_io_keymap;
extern float T1_io_mouse_scroll_pos;

void T1_io_event_register(T1IOEvent * touch_record);

void T1_io_register_keyup(uint32_t key_id);
void T1_io_register_keydown(uint32_t key_id);
void T1_io_register_mousescroll(const float amount);

#ifdef __cplusplus
}
#endif

#endif // T1_IO
