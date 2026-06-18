#ifndef T1_PUBLIC_TYPES_H
#define T1_PUBLIC_TYPES_H

typedef enum : unsigned char {
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
    int reserved_and_tex;
    int touch_id;
    int padding[2];
} T1GPUTexQuadi32;

typedef struct {
    float xyz[3];
    float offset_xy[2];
    float wh[2];
    float rgba[4];
    float padding[5];
} T1GPUTexQuadf32;

typedef struct
{
    float rgb_add[3];
    #if T1_FOG_ACTIVE == T1_ACTIVE
    float fog_color[3];
    #elif T1_FOG_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    // float screen_width;
    // float screen_height;
    float nonblur_pct;
    float blur_pct;
    float color_quantization;
    #if T1_FOG_ACTIVE == T1_ACTIVE
    float fog_factor;
    #elif T1_FOG_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    #if T1_SHADOWS_ACTIVE == T1_ACTIVE
    float in_shadow_multipliers[3];
    #elif T1_SHADOWS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    unsigned int cam_rv_i;
    unsigned int timestamp;
    unsigned int lights_size;
    int perlin_texturearray_i;
    int perlin_texture_i;
    float padding[6];
} T1GPUPostProcConsts;

typedef struct {
    T1GPUPostProcConsts postproc_consts;
    
    unsigned long elapsed;
    unsigned long this_frame_timestamp_us;
    unsigned long last_resize_request_us;
    
    unsigned int startup_bytes_to_load;
    unsigned int startup_bytes_loaded;
    
    float last_clickray_origin[3];
    float last_clickray_direction[3];
    float last_clickray_collision[3];
    
    unsigned int pixelation_div;
    
    float timedelta_mult;
    float window_height;
    float window_width;
    float window_left;
    float window_bottom;
    
    unsigned char draw_mouseptr;
    unsigned char draw_imputed_normals;
    unsigned char draw_triangles;
    unsigned char draw_axes;
    unsigned char draw_fps;
    unsigned char draw_top_touchable_id;
    unsigned char show_profiler;
    unsigned char pause_profiler;
    unsigned char block_mouse;
    unsigned char block_render_view_pos_updates;
    unsigned char fullscreen;
    unsigned char clientlogic_early_startup_finished;
    unsigned char upcoming_fullscreen_request;
} T1Globals;

typedef struct FontSettings {
    T1GPUTexQuadf32 f32;
    T1GPUTexQuadi32 i32;
    float           font_height;
    int             highlight_i;
    int             highlight_size;
    int             opaque_back_active;
} T1TextFontSettings;

typedef struct {
    float offset_xyz[3];
    int T1_id;
    unsigned char one_frame_only;
    unsigned char committed;
    unsigned char visible;
    unsigned char deleted;
} T1CPUTexQuad;

typedef struct {
    T1GPUTexQuadf32 f32;
    T1GPUTexQuadi32 i32;
} T1GPUTexQuad;

typedef struct {
    // you can make a group of lights and/or texquads by
    // giving them the same positive object_id, then make
    // ScheduledAnimations that affect the entire group
    // set to -1 to not be a party of any group
    union {
        int T1_id;
        float flt_T1_id;
    };
    union {
        unsigned int deleted;
        float flt_deleted;
    };
    union {
        unsigned int committed;
        float flt_committed;
    };
    float xyz[3];
    float xyz_angle[3];
    float xyz_offset[3];
    float RGBA[4];
    float reach; // light's reach
    float diffuse;     // how much diffuse light does this radiate?
    float specular;
    int shadow_map_depth_texture_i;
    int shadow_map_render_view_i;
    float simd_padding[3];
} T1zLight; // 17 floats = 68 bytes

#endif // T1_PUBLIC_TYPES_H
