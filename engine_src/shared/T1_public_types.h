#ifndef T1_PUBLIC_TYPES_H
#define T1_PUBLIC_TYPES_H

#include "T1_stdint.h"

#define T1_TEX_NONE UINT16_MAX
typedef u16 T1Tex;

typedef enum : u16 {
    T1_IO_KEYBOARD_LEFTARROW = 0,
    T1_IO_KEYBOARD_RIGHTARROW = 1,
    T1_IO_KEYBOARD_UPARROW = 2,
    T1_IO_KEYBOARD_DOWNARROW = 3,
    T1_IO_KEYBOARD_ENTER = 4,
    T1_IO_KEYBOARD_ESCAPE = 5,
    T1_IO_KEYBOARD_TAB = 6,
    T1_IO_KEYBOARD_SPACEBAR = 7,
    T1_IO_KEYBOARD_BACKSPACE = 8,
    T1_IO_KEYBOARD_YENSIGN = 9,
    T1_IO_KEYBOARD_KANABUTTON = 10,
    T1_IO_KEYBOARD_ROMAJIBUTTON = 11,
    T1_IO_KEYBOARD_ALT = 12,
    T1_IO_KEYBOARD_WINKEY = 13,
    T1_IO_KEYBOARD_CAPSLOCK = 14,
    T1_IO_KEYBOARD_CONTROL = 15,
    T1_IO_KEYBOARD_SHIFT = 16,
    T1_IO_KEYBOARD_EXCLAM = 33,
    T1_IO_KEYBOARD_DOUBLEQUOTE = 34,
    T1_IO_KEYBOARD_NUMBERSIGN = 35,
    T1_IO_KEYBOARD_DOLLARSIGN = 36,
    T1_IO_KEYBOARD_PERCENTSIGN = 37,
    T1_IO_KEYBOARD_AMPERSAND = 38,
    T1_IO_KEYBOARD_APOSTROPHE = 39,
    T1_IO_KEYBOARD_LEFTPARENTHESIS = 40,
    T1_IO_KEYBOARD_RIGHTPARENTHESIS = 41,
    T1_IO_KEYBOARD_ASTERISK = 42,
    T1_IO_KEYBOARD_PLUS = 43,
    T1_IO_KEYBOARD_COMMA = 44,
    T1_IO_KEYBOARD_MINUS = 45,
    T1_IO_KEYBOARD_FULLSTOP = 46,
    T1_IO_KEYBOARD_BACKSLASH = 47,
    T1_IO_KEYBOARD_0 = 48,
    T1_IO_KEYBOARD_1 = 49,
    T1_IO_KEYBOARD_2 = 50,
    T1_IO_KEYBOARD_3 = 51,
    T1_IO_KEYBOARD_4 = 52,
    T1_IO_KEYBOARD_5 = 53,
    T1_IO_KEYBOARD_6 = 54,
    T1_IO_KEYBOARD_7 = 55,
    T1_IO_KEYBOARD_8 = 56,
    T1_IO_KEYBOARD_9 = 57,
    T1_IO_KEYBOARD_COLON = 58,
    T1_IO_KEYBOARD_SEMICOLON = 59,
    T1_IO_KEYBOARD_LESSTHAN = 60,
    T1_IO_KEYBOARD_EQUALS = 61,
    T1_IO_KEYBOARD_GREATERTHAN = 62,
    T1_IO_KEYBOARD_QUESTIONMARK = 63,
    T1_IO_KEYBOARD_AT = 64,
    T1_IO_KEYBOARD_A = 65,
    T1_IO_KEYBOARD_B = 66,
    T1_IO_KEYBOARD_C = 67,
    T1_IO_KEYBOARD_D = 68,
    T1_IO_KEYBOARD_E = 69,
    T1_IO_KEYBOARD_F = 70,
    T1_IO_KEYBOARD_G = 71,
    T1_IO_KEYBOARD_H = 72,
    T1_IO_KEYBOARD_I = 73,
    T1_IO_KEYBOARD_J = 74,
    T1_IO_KEYBOARD_K = 75,
    T1_IO_KEYBOARD_L = 76,
    T1_IO_KEYBOARD_M = 77,
    T1_IO_KEYBOARD_N = 78,
    T1_IO_KEYBOARD_O = 79,
    T1_IO_KEYBOARD_P = 80,
    T1_IO_KEYBOARD_Q = 81,
    T1_IO_KEYBOARD_R = 82,
    T1_IO_KEYBOARD_S = 83,
    T1_IO_KEYBOARD_T = 84,
    T1_IO_KEYBOARD_U = 85,
    T1_IO_KEYBOARD_V = 86,
    T1_IO_KEYBOARD_W = 87,
    T1_IO_KEYBOARD_X = 88,
    T1_IO_KEYBOARD_Y = 89,
    T1_IO_KEYBOARD_Z = 90,
    T1_IO_KEYBOARD_OPENSQUAREBRACKET = 91,
    T1_IO_KEYBOARD_CLOSESQUAREBRACKET = 93,
    T1_IO_KEYBOARD_HAT = 94,
    T1_IO_KEYBOARD_UNDERSCORE = 95,
    T1_IO_KEYBOARD_OPENCURLYBRACKET = 123,
    T1_IO_KEYBOARD_PIPE = 124,
    T1_IO_KEYBOARD_RIGHTCURLYBRACKET = 125,
    T1_IO_KEYBOARD_TILDE = 126,
    T1_IO_KEYBOARD_F1 = 136,
    T1_IO_KEYBOARD_F2 = 137,
    T1_IO_KEYBOARD_F3 = 138,
    T1_IO_KEYBOARD_F4 = 139,
    T1_IO_KEYBOARD_F5 = 140,
    T1_IO_KEYBOARD_F6 = 141,
    T1_IO_KEYBOARD_F7 = 142,
    T1_IO_KEYBOARD_F8 = 143,
    T1_IO_KEYBOARD_F9 = 144,
    T1_IO_KEYBOARD_F10 = 145,
    T1_IO_KEYBOARD_F11 = 146,
    T1_IO_KEYBOARD_F12 = 147,
    T1_IO_KEYBOARD_HOME = 148,
    T1_IO_KEYBOARD_PAGEUP = 149,
    T1_IO_KEYBOARD_END = 150,
    T1_IO_KEYBOARD_PAGEDOWN = 151,
    T1_IO_KEYBOARD_ABOVE_KEYBOARD_BOUNDS = 152,
    T1_IO_MOUSE_LCLICK = 153,
    T1_IO_MOUSE_MCLICK = 154,
    T1_IO_MOUSE_RCLICK = 155,
    T1_IO_MOUSE_WHEEL_UP = 156,
    T1_IO_MOUSE_WHEEL_DOWN = 157,
    T1_IO_KEY_ABOVEBOUNDS, // must be last
} T1IOKey;

typedef struct {
    s32 reserved_and_tex;
    s32 touch_id;
    s32 padding[2];
} T1GPUTexQuads32;

typedef struct {
    f32 xyz[3];
    f32 offset_xy[2];
    f32 wh[2];
    f32 rgba[4];
    f32 padding[5];
} T1GPUTexQuadf32;

typedef struct
{
    u64 timestamp;
    u32 cam_rv_i;
    u32 lights_size;
    s32 perlin_texturearray_i;
    s32 perlin_texture_i;
    f32 rgb_add[3];
    #if T1_FOG_ACTIVE == T1_ACTIVE
    f32 fog_color[3];
    f32 fog_factor;
    #elif T1_FOG_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    f32 nonblur_pct;
    f32 blur_pct;
    f32 color_quantization;
    #if T1_SHADOWS_ACTIVE == T1_ACTIVE
    f32 in_shadow_multipliers[3];
    #elif T1_SHADOWS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    f32 padding[6];
} T1GPUPostProcConsts;

typedef struct {
    T1GPUPostProcConsts postproc_consts;
    
    u64 elapsed;
    u64 this_frame_timestamp_us;
    u64 last_resize_request_us;
    
    u32 startup_bytes_to_load;
    u32 startup_bytes_loaded;
    
    f32 last_clickray_origin[3];
    f32 last_clickray_direction[3];
    f32 last_clickray_collision[3];
    
    f32 timedelta_mult;
    f32 window_wh[2];
    f32 window_left;
    f32 window_bottom;
    
    u8 draw_mouseptr;
    u8 draw_imputed_normals;
    u8 draw_triangles;
    u8 draw_axes;
    u8 draw_fps;
    u8 draw_touch_id;
    u8 show_profiler;
    u8 pause_profiler;
    u8 block_mouse;
    u8 block_render_view_pos_updates;
    u8 fullscreen;
    u8 clientlogic_early_startup_finished;
    u8 upcoming_fullscreen_request;
} T1Globals;

typedef struct {
    T1GPUTexQuadf32 f32s;
    T1GPUTexQuads32 s32s;
    f32             font_height;
    s32             highlight_i;
    s32             highlight_size;
    s32             opaque_back_active;
} T1TextFontSettings;

typedef struct {
    f32 offset_xyz[3];
    s32 T1_id;
    u8 one_frame_only;
    u8 committed;
    u8 visible;
    u8 deleted;
} T1CPUTexQuad;

typedef struct {
    T1GPUTexQuadf32 f32s;
    T1GPUTexQuads32 s32s;
} T1GPUTexQuad;

typedef struct {
    // you can make a group of lights and/or texquads by
    // giving them the same positive object_id, then make
    // ScheduledAnimations that affect the entire group
    // set to -1 to not be a party of any group
    union {
        s32 T1_id;
        f32 flt_T1_id;
    };
    union {
        u32 deleted;
        f32 flt_deleted;
    };
    union {
        u32 committed;
        f32 flt_committed;
    };
    f32 xyz[3];
    f32 xyz_angle[3];
    f32 xyz_offset[3];
    f32 RGBA[4];
    f32 reach; // light's reach
    f32 diffuse;     // how much diffuse light does this radiate?
    f32 specular;
    s32 shadow_map_depth_texture_i;
    s32 shadow_map_render_view_i;
    f32 simd_padding[3];
} T1zLight; // 17 f32s = 68 bytes

#endif // T1_PUBLIC_TYPES_H
