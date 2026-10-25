#ifndef T1_PUBLIC_TYPES_H
#define T1_PUBLIC_TYPES_H

#include <stdint.h>

#include "T1_types_public_gpucpu.h"

#define T1_ID_NONE 0xFFFFFFFF
// retired objects are unaffected by animations
#define T1_ID_RETIRED_BUT_NOT_DELETED 20501

#define T1_TOUCH_ID_NONE 0xFFFFFFFF

#define T1_ANIM_NO_EFFECT 0x0FFFFFFF
#define T1_ANIM_HIT_EVERYTHING 0x0AAAAAAA

// T1_anim_new() modes: animate to a value, or by an amount
#define T1_ANIM_MODE_TO 1
#define T1_ANIM_MODE_BY 2
// never returned by T1_anim_new()
#define T1_ANIM_NONE 0xFFFFFFFF

// Basic quads and cubes are predefined, they can be used without registering
// an .obj file.
#define T1_BASIC_QUAD_MESH_ID 0
#define T1_BASIC_CUBE_MESH_ID 1

typedef uint16_t T1Tex;

typedef enum : uint16_t {
    T1_IO_KEYBOARD_LEFTARROW     =  0,
    T1_IO_KEYBOARD_RIGHTARROW    =  1,
    T1_IO_KEYBOARD_UPARROW       =  2,
    T1_IO_KEYBOARD_DOWNARROW     =  3,
    T1_IO_KEYBOARD_ENTER         =  4,
    T1_IO_KEYBOARD_ESCAPE        =  5,
    T1_IO_KEYBOARD_TAB           =  6,
    T1_IO_KEYBOARD_SPACEBAR      =  7,
    T1_IO_KEYBOARD_BACKSPACE     =  8,
    T1_IO_KEYBOARD_YENSIGN       =  9,
    T1_IO_KEYBOARD_KANA          = 10,
    T1_IO_KEYBOARD_ROMAJI        = 11,
    T1_IO_KEYBOARD_ALT           = 12,
    T1_IO_KEYBOARD_WINKEY        = 13,
    T1_IO_KEYBOARD_CAPSLOCK      = 14,
    T1_IO_KEYBOARD_CONTROL       = 15,
    T1_IO_KEYBOARD_SHIFT         = 16,
    T1_IO_KEYBOARD_UNKNOWNBTN    = 17,
    T1_IO_KEYBOARD_EXCLAM        = 33,
    T1_IO_KEYBOARD_DOUBLEQUOTE   = 34,
    T1_IO_KEYBOARD_NUMBERSIGN    = 35,
    T1_IO_KEYBOARD_DOLLARSIGN    = 36,
    T1_IO_KEYBOARD_PERCENTSIGN   = 37,
    T1_IO_KEYBOARD_AMPERSAND     = 38,
    T1_IO_KEYBOARD_APOSTROPHE    = 39,
    T1_IO_KEYBOARD_LEFTPAREN     = 40,
    T1_IO_KEYBOARD_RIGHTPAREN    = 41,
    T1_IO_KEYBOARD_ASTERISK      = 42,
    T1_IO_KEYBOARD_PLUS          = 43,
    T1_IO_KEYBOARD_COMMA         = 44,
    T1_IO_KEYBOARD_MINUS         = 45,
    T1_IO_KEYBOARD_FULLSTOP      = 46,
    T1_IO_KEYBOARD_BACKSLASH     = 47,
    T1_IO_KEYBOARD_0             = 48,
    T1_IO_KEYBOARD_1             = 49,
    T1_IO_KEYBOARD_2             = 50,
    T1_IO_KEYBOARD_3             = 51,
    T1_IO_KEYBOARD_4             = 52,
    T1_IO_KEYBOARD_5             = 53,
    T1_IO_KEYBOARD_6             = 54,
    T1_IO_KEYBOARD_7             = 55,
    T1_IO_KEYBOARD_8             = 56,
    T1_IO_KEYBOARD_9             = 57,
    T1_IO_KEYBOARD_COLON         = 58,
    T1_IO_KEYBOARD_SEMICOLON     = 59,
    T1_IO_KEYBOARD_LESSTHAN      = 60,
    T1_IO_KEYBOARD_EQUALS        = 61,
    T1_IO_KEYBOARD_GREATERTHAN   = 62,
    T1_IO_KEYBOARD_QMARK         = 63,
    T1_IO_KEYBOARD_AT            = 64,
    T1_IO_KEYBOARD_A             = 65,
    T1_IO_KEYBOARD_B             = 66,
    T1_IO_KEYBOARD_C             = 67,
    T1_IO_KEYBOARD_D             = 68,
    T1_IO_KEYBOARD_E             = 69,
    T1_IO_KEYBOARD_F             = 70,
    T1_IO_KEYBOARD_G             = 71,
    T1_IO_KEYBOARD_H             = 72,
    T1_IO_KEYBOARD_I             = 73,
    T1_IO_KEYBOARD_J             = 74,
    T1_IO_KEYBOARD_K             = 75,
    T1_IO_KEYBOARD_L             = 76,
    T1_IO_KEYBOARD_M             = 77,
    T1_IO_KEYBOARD_N             = 78,
    T1_IO_KEYBOARD_O             = 79,
    T1_IO_KEYBOARD_P             = 80,
    T1_IO_KEYBOARD_Q             = 81,
    T1_IO_KEYBOARD_R             = 82,
    T1_IO_KEYBOARD_S             = 83,
    T1_IO_KEYBOARD_T             = 84,
    T1_IO_KEYBOARD_U             = 85,
    T1_IO_KEYBOARD_V             = 86,
    T1_IO_KEYBOARD_W             = 87,
    T1_IO_KEYBOARD_X             = 88,
    T1_IO_KEYBOARD_Y             = 89,
    T1_IO_KEYBOARD_Z             = 90,
    T1_IO_KEYBOARD_OPENSQUARE    = 91,
    T1_IO_KEYBOARD_CLOSESQUARE   = 93,
    T1_IO_KEYBOARD_HAT           = 94,
    T1_IO_KEYBOARD_UNDERSCORE    = 95,
    T1_IO_KEYBOARD_NUMPADCLEAR   = 96,
    T1_IO_KEYBOARD_OPENCURLY     = 123,
    T1_IO_KEYBOARD_PIPE          = 124,
    T1_IO_KEYBOARD_RIGHTCURLY    = 125,
    T1_IO_KEYBOARD_TILDE         = 126,
    T1_IO_KEYBOARD_F1            = 136,
    T1_IO_KEYBOARD_F2            = 137,
    T1_IO_KEYBOARD_F3            = 138,
    T1_IO_KEYBOARD_F4            = 139,
    T1_IO_KEYBOARD_F5            = 140,
    T1_IO_KEYBOARD_F6            = 141,
    T1_IO_KEYBOARD_F7            = 142,
    T1_IO_KEYBOARD_F8            = 143,
    T1_IO_KEYBOARD_F9            = 144,
    T1_IO_KEYBOARD_F10           = 145,
    T1_IO_KEYBOARD_F11           = 146,
    T1_IO_KEYBOARD_F12           = 147,
    T1_IO_KEYBOARD_INSERT        = 148,
    T1_IO_KEYBOARD_HOME          = 149,
    T1_IO_KEYBOARD_PAGEUP        = 150,
    T1_IO_KEYBOARD_END           = 151,
    T1_IO_KEYBOARD_PAGEDOWN      = 152,
    T1_IO_KEYBOARD_ABOVEBOUNDS   = 153,
    T1_IO_MOUSE                  = 154,
    T1_IO_MOUSE_LCLICK           = 155,
    T1_IO_MOUSE_MCLICK           = 156,
    T1_IO_MOUSE_RCLICK           = 157,
    T1_IO_MOUSE_OTHERCLICK1      = 158,
    T1_IO_MOUSE_OTHERCLICK2      = 159,
    T1_IO_MOUSE_OTHERCLICK3      = 160,
    T1_IO_MOUSE_OTHERCLICK4      = 161,
    T1_IO_MOUSE_WHEEL_UP         = 162,
    T1_IO_MOUSE_WHEEL_DOWN       = 163,
    T1_IO_GAMEPAD_DPAD_LEFT      = 164,
    T1_IO_GAMEPAD_DPAD_RIGHT     = 165,
    T1_IO_GAMEPAD_DPAD_UP        = 166,
    T1_IO_GAMEPAD_DPAD_DOWN      = 167,
    T1_IO_GAMEPAD_LSHOULDER      = 168,
    T1_IO_GAMEPAD_RSHOULDER      = 169,
    T1_IO_GAMEPAD_LTRIGGER       = 170,
    T1_IO_GAMEPAD_RTRIGGER       = 171,
    T1_IO_GAMEPAD_LTHUMBSTICKBTN = 172,
    T1_IO_GAMEPAD_RTHUMBSTICKBTN = 173,
    T1_IO_GAMEPAD_LTHUMBSTICK    = 174,
    T1_IO_GAMEPAD_RTHUMBSTICK    = 175,
    T1_IO_GAMEPAD_A              = 176,
    T1_IO_GAMEPAD_B              = 177,
    T1_IO_GAMEPAD_X              = 178,
    T1_IO_GAMEPAD_Y              = 179,
    T1_IO_GAMEPAD_HOME           = 180,
    T1_IO_GAMEPAD_MENU           = 181,
    T1_IO_GAMEPAD_OPTIONS        = 182,
    T1_IO_KEY_ABOVEBOUNDS, // must be last
} T1IOKey;

typedef struct {
    T1GPUPostProcConsts postproc_consts;
    
    uint64_t elapsed;
    uint64_t this_frame_timestamp_us;
    uint64_t last_resize_request_us;
    
    uint32_t startup_bytes_to_load;
    uint32_t startup_bytes_loaded;
    
    float timedelta_mult;
    float window_wh[2];
    float window_left;
    float window_bottom;
    
    uint8_t draw_mouseptr;
    uint8_t draw_imputed_normals;
    uint8_t draw_triangles;
    uint8_t draw_axes;
    uint8_t draw_fps;
    uint8_t draw_touch_id;
    uint8_t block_mouse;
    uint8_t block_render_view_pos_updates;
    uint8_t fullscreen;
    uint8_t upcoming_fullscreen_request;
} T1Globals;

typedef struct {
    float volume;
    float sfx_volume;
    float music_volume;
} T1AudioSettingsFullyPublic;

typedef enum : uint8_t {
    T1_EASINGTYPE_NONE = 0,
    T1_EASINGTYPE_ALWAYS_1,
    T1_EASINGTYPE_INOUT_SINE,
    T1_EASINGTYPE_OUT_QUADRATIC,
    T1_EASINGTYPE_EASEOUT_ELASTIC_ZERO_TO_ONE,
    T1_EASINGTYPE_SINGLE_BOUNCE_ZERO_TO_ZERO,
    T1_EASINGTYPE_DOUBLE_BOUNCE_ZERO_TO_ZERO,
    T1_EASINGTYPE_QUADRUPLE_BOUNCE_ZERO_TO_ZERO,
    T1_EASINGTYPE_OCTUPLE_BOUNCE_ZERO_TO_ZERO,
    T1_EASINGTYPE_SINGLE_PULSE_ZERO_TO_ZERO,
    T1_EASINGTYPE_OCTUPLE_PULSE_ZERO_TO_ZERO,
    T1_EASINGTYPE_OUTOFBOUNDS
} T1EasingType;

#endif // T1_PUBLIC_TYPES_H
