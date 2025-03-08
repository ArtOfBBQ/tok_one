#ifndef USERINPUT_H
#define USERINPUT_H

#include "common.h"
#include "lightsource.h"
#include "platform_layer.h"

#define KEYPRESS_MAP_SIZE 1000

#define TOK_KEY_LEFTARROW 0
#define TOK_KEY_RIGHTARROW 1
#define TOK_KEY_UPARROW 2
#define TOK_KEY_DOWNARROW 3
#define TOK_KEY_ENTER 4
#define TOK_KEY_ESCAPE 5
#define TOK_KEY_TAB 6
#define TOK_KEY_SPACEBAR 7
#define TOK_KEY_BACKSPACE 8
#define TOK_KEY_YENSIGN 9
#define TOK_KEY_KANABUTTON 10
#define TOK_KEY_ROMAJIBUTTON 11
#define TOK_KEY_ALT 12
#define TOK_KEY_WINKEY 13
#define TOK_KEY_CAPSLOCK 14
#define TOK_KEY_CONTROL 15
#define TOK_KEY_SHIFT 16
#define TOK_KEY_EXCLAM 33
#define TOK_KEY_DOUBLEQUOTE 34
#define TOK_KEY_NUMBERSIGN 35
#define TOK_KEY_DOLLARSIGN 36
#define TOK_KEY_PERCENTSIGN 37
#define TOK_KEY_AMPERSAND 38
#define TOK_KEY_APOSTROPHE 39
#define TOK_KEY_LEFTPARENTHESIS 40
#define TOK_KEY_RIGHTPARENTHESIS 41
#define TOK_KEY_ASTERISK 42
#define TOK_KEY_PLUS 43
#define TOK_KEY_COMMA 44
#define TOK_KEY_MINUS 45
#define TOK_KEY_FULLSTOP 46
#define TOK_KEY_BACKSLASH 47
#define TOK_KEY_0 48
#define TOK_KEY_1 49
#define TOK_KEY_2 50
#define TOK_KEY_3 51
#define TOK_KEY_4 52
#define TOK_KEY_5 53
#define TOK_KEY_6 54
#define TOK_KEY_7 55
#define TOK_KEY_8 56
#define TOK_KEY_9 57
#define TOK_KEY_COLON 58
#define TOK_KEY_SEMICOLON 59
#define TOK_KEY_LESSTHAN 60
#define TOK_KEY_EQUALS 61
#define TOK_KEY_GREATERTHAN 62
#define TOK_KEY_QUESTIONMARK 63
#define TOK_KEY_AT 64
#define TOK_KEY_A 65
#define TOK_KEY_B 66
#define TOK_KEY_C 67
#define TOK_KEY_D 68
#define TOK_KEY_E 69
#define TOK_KEY_F 70
#define TOK_KEY_G 71
#define TOK_KEY_H 72
#define TOK_KEY_I 73
#define TOK_KEY_J 74
#define TOK_KEY_K 75
#define TOK_KEY_L 76
#define TOK_KEY_M 77
#define TOK_KEY_N 78
#define TOK_KEY_O 79
#define TOK_KEY_P 80
#define TOK_KEY_Q 81
#define TOK_KEY_R 82
#define TOK_KEY_S 83
#define TOK_KEY_T 84
#define TOK_KEY_U 85
#define TOK_KEY_V 86
#define TOK_KEY_W 87
#define TOK_KEY_X 88
#define TOK_KEY_Y 89
#define TOK_KEY_Z 90
#define TOK_KEY_OPENSQUAREBRACKET  91
#define TOK_KEY_CLOSESQUAREBRACKET 93
#define TOK_KEY_HAT 94
#define TOK_KEY_UNDERSCORE 95
#define TOK_KEY_OPENCURLYBRACKET 123
#define TOK_KEY_PIPE 124
#define TOK_KEY_RIGHTCURLYBRACKET 125
#define TOK_KEY_TILDE 126
#define TOK_KEY_F1 136
#define TOK_KEY_F2 137
#define TOK_KEY_F3 138
#define TOK_KEY_F4 139
#define TOK_KEY_F5 140
#define TOK_KEY_F6 141
#define TOK_KEY_F7 142
#define TOK_KEY_F8 143
#define TOK_KEY_F9 144
#define TOK_KEY_F10 145
#define TOK_KEY_F11 146
#define TOK_KEY_F12 147
#define TOK_KEY_HOME 148
#define TOK_KEY_PAGEUP 149
#define TOK_KEY_END 150
#define TOK_KEY_PAGEDOWN 151

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Interaction {
    int32_t touchable_id_pierce;
    int32_t touchable_id_top;
    bool32_t checked_touchables;
    float screen_x;
    float screen_y;
    uint64_t timestamp;
    bool32_t handled;
} Interaction;
void construct_interaction(Interaction * to_construct);

// indexes in the interactions array
#define INTR_LAST_GPU_DATA                      0
#define INTR_PREVIOUS_TOUCH_START               1
#define INTR_PREVIOUS_TOUCH_END                 2
#define INTR_PREVIOUS_LEFTCLICK_START           3
#define INTR_PREVIOUS_LEFTCLICK_END             4
#define INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START  5
#define INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END    6
#define INTR_PREVIOUS_RIGHTCLICK_START          7
#define INTR_PREVIOUS_RIGHTCLICK_END            8
#define INTR_PREVIOUS_MOUSE_MOVE                9
#define INTR_PREVIOUS_TOUCH_MOVE               10
#define INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE      11
// permanent size of the interactions array
#define USER_INTERACTIONS_SIZE                 12
extern Interaction * user_interactions;

void register_interaction(Interaction * touch_record);

extern bool32_t * keypress_map; //[KEYPRESS_MAP_SIZE];

extern float mouse_scroll_pos;

void register_keyup(uint32_t key_id);
void register_keydown(uint32_t key_id);
void register_mousescroll(const float amount);

#ifdef __cplusplus
}
#endif

#endif // USERINPUT_H

