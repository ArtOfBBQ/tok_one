#ifndef T1_IO_H
#define T1_IO_H

#include <stdint.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef enum : uint8_t {
    T1_IO_KEY_LEFTARROW = 0,
    T1_IO_KEY_RIGHTARROW = 1,
    T1_IO_KEY_UPARROW = 2,
    T1_IO_KEY_DOWNARROW = 3,
    T1_IO_KEY_ENTER = 4,
    T1_IO_KEY_ESCAPE = 5,
    T1_IO_KEY_TAB = 6,
    T1_IO_KEY_SPACEBAR = 7,
    T1_IO_KEY_BACKSPACE = 8,
    T1_IO_KEY_YENSIGN = 9,
    T1_IO_KEY_KANABUTTON = 10,
    T1_IO_KEY_ROMAJIBUTTON = 11,
    T1_IO_KEY_ALT = 12,
    T1_IO_KEY_WINKEY = 13,
    T1_IO_KEY_CAPSLOCK = 14,
    T1_IO_KEY_CONTROL = 15,
    T1_IO_KEY_SHIFT = 16,
    T1_IO_KEY_EXCLAM = 33,
    T1_IO_KEY_DOUBLEQUOTE = 34,
    T1_IO_KEY_NUMBERSIGN = 35,
    T1_IO_KEY_DOLLARSIGN = 36,
    T1_IO_KEY_PERCENTSIGN = 37,
    T1_IO_KEY_AMPERSAND = 38,
    T1_IO_KEY_APOSTROPHE = 39,
    T1_IO_KEY_LEFTPARENTHESIS = 40,
    T1_IO_KEY_RIGHTPARENTHESIS = 41,
    T1_IO_KEY_ASTERISK = 42,
    T1_IO_KEY_PLUS = 43,
    T1_IO_KEY_COMMA = 44,
    T1_IO_KEY_MINUS = 45,
    T1_IO_KEY_FULLSTOP = 46,
    T1_IO_KEY_BACKSLASH = 47,
    T1_IO_KEY_0 = 48,
    T1_IO_KEY_1 = 49,
    T1_IO_KEY_2 = 50,
    T1_IO_KEY_3 = 51,
    T1_IO_KEY_4 = 52,
    T1_IO_KEY_5 = 53,
    T1_IO_KEY_6 = 54,
    T1_IO_KEY_7 = 55,
    T1_IO_KEY_8 = 56,
    T1_IO_KEY_9 = 57,
    T1_IO_KEY_COLON = 58,
    T1_IO_KEY_SEMICOLON = 59,
    T1_IO_KEY_LESSTHAN = 60,
    T1_IO_KEY_EQUALS = 61,
    T1_IO_KEY_GREATERTHAN = 62,
    T1_IO_KEY_QUESTIONMARK = 63,
    T1_IO_KEY_AT = 64,
    T1_IO_KEY_A = 65,
    T1_IO_KEY_B = 66,
    T1_IO_KEY_C = 67,
    T1_IO_KEY_D = 68,
    T1_IO_KEY_E = 69,
    T1_IO_KEY_F = 70,
    T1_IO_KEY_G = 71,
    T1_IO_KEY_H = 72,
    T1_IO_KEY_I = 73,
    T1_IO_KEY_J = 74,
    T1_IO_KEY_K = 75,
    T1_IO_KEY_L = 76,
    T1_IO_KEY_M = 77,
    T1_IO_KEY_N = 78,
    T1_IO_KEY_O = 79,
    T1_IO_KEY_P = 80,
    T1_IO_KEY_Q = 81,
    T1_IO_KEY_R = 82,
    T1_IO_KEY_S = 83,
    T1_IO_KEY_T = 84,
    T1_IO_KEY_U = 85,
    T1_IO_KEY_V = 86,
    T1_IO_KEY_W = 87,
    T1_IO_KEY_X = 88,
    T1_IO_KEY_Y = 89,
    T1_IO_KEY_Z = 90,
    T1_IO_KEY_OPENSQUAREBRACKET = 91,
    T1_IO_KEY_CLOSESQUAREBRACKET = 93,
    T1_IO_KEY_HAT = 94,
    T1_IO_KEY_UNDERSCORE = 95,
    T1_IO_KEY_OPENCURLYBRACKET = 123,
    T1_IO_KEY_PIPE = 124,
    T1_IO_KEY_RIGHTCURLYBRACKET = 125,
    T1_IO_KEY_TILDE = 126,
    T1_IO_KEY_F1 = 136,
    T1_IO_KEY_F2 = 137,
    T1_IO_KEY_F3 = 138,
    T1_IO_KEY_F4 = 139,
    T1_IO_KEY_F5 = 140,
    T1_IO_KEY_F6 = 141,
    T1_IO_KEY_F7 = 142,
    T1_IO_KEY_F8 = 143,
    T1_IO_KEY_F9 = 144,
    T1_IO_KEY_F10 = 145,
    T1_IO_KEY_F11 = 146,
    T1_IO_KEY_F12 = 147,
    T1_IO_KEY_HOME = 148,
    T1_IO_KEY_PAGEUP = 149,
    T1_IO_KEY_END = 150,
    T1_IO_KEY_PAGEDOWN = 151,
} T1IOKey;

typedef struct {
    uint64_t timestamp;
    int32_t touch_id_pierce;
    int32_t touch_id_top;
    float screen_x;
    float screen_y;
    uint8_t checked_touch_ids;
    uint8_t handled;
} T1IOEvent;

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

// permanent size of the interactions array
#define T1_IO_EVENTS_CAP 12
#define T1_IO_KEYMAP_CAP 1000

typedef struct {
    T1IOEvent events[T1_IO_EVENTS_CAP];
    float mouse_scroll_pos;
    uint8_t keymap[T1_IO_KEYMAP_CAP];
} T1IOState;

extern T1IOState * T1_io;

void
T1_io_init(
    void *(* arg_malloc_func)(size_t));

void
T1_io_event_construct(
    T1IOEvent * to_construct);

void
T1_io_event_register(
    T1IOEvent * touch_record);

void
T1_io_register_keyup(uint32_t key_id);

void
T1_io_register_keydown(uint32_t key_id);

void
T1_io_register_mousescroll(const float amount);

#ifdef __cplusplus
}
#endif

#endif // T1_IO_H
