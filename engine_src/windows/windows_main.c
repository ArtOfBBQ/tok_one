// functions for us to use
#include <windows.h>
#include <gl/gl.h>

#include <assert.h>

#include "tok_directsound.h"

#include "memorystore.h"

#include "opengl_extensions.h"
#include "tok_opengl.h"
#include "common.h"
#include "userinput.h"
#include "audio.h"

static unsigned int requesting_shutdown = false;

static HWND window_handle;

static DWORD window_style = 
    WS_OVERLAPPEDWINDOW |
    WS_VISIBLE |
    WS_SIZEBOX |
    WS_CLIPCHILDREN |
    WS_CLIPSIBLINGS;


BOOL (* extptr_wglSwapIntervalEXT)(int interval) = NULL;

void platform_close_application(void) {
    requesting_shutdown = true;
}

static void wait_x_microseconds(uint64_t microseconds)
{
    uint64_t start = platform_get_current_time_microsecs();
    while (platform_get_current_time_microsecs() - start < microseconds) {
        // Wait
    }
}

static float win32_screen_height = -1.0f;
static float flip_y_axis(
    const float input)
{
    if (win32_screen_height < 0.0f) {
        win32_screen_height = GetSystemMetrics(
            /* [in] int nIndex: */ SM_CYFULLSCREEN);
        if (win32_screen_height < 100.0f) {
            win32_screen_height = 600.0f;
        }
    }
    
    /*
    win32 docs:
    Points on the screen are described by x- and y-coordinate pairs.
    The x-coordinates increase to the right; y-coordinates increase
    from top to bottom.
    
    So we want to 0 to equal the screen height, and screen height to
    equal 0.
    */
    
    return (win32_screen_height - input);
}

void platform_gpu_update_viewport(void)
{
    printf("Updating opengl projection constants\n");
    *gpu_shared_data_collection.locked_pjc =
        window_globals->projection_constants;
    
    opengl_copy_projection_constants(
        /* GPUProjectionConstants * projection_constants: */
            gpu_shared_data_collection.locked_pjc);
}

void platform_gpu_copy_locked_vertices()
{
    
    opengl_copy_locked_vertices(
        gpu_shared_data_collection.locked_vertices);
}

static void fetch_extension_func_address(
    void ** extptr,
    char * func_name)
{
    if (*extptr != NULL) {
        printf(
            "%s\n",
            "Tried to fetch a PROC address that was already non-null");
        requesting_shutdown = true;
        return;
    }
    
    *extptr = (void *)wglGetProcAddress(func_name);
    
    if (*extptr == NULL) {
        // backup plan: load from windows .dll
        HMODULE module = LoadLibraryA("opengl32.dll");
        if (!module) {
            MessageBox(
                /* HWND hWnd: */
                    0,
                /* LPCSTR lpText: */
                    "Couldn't find opengl32.dll",
                /* LPCSTR lpCaption: */
                    "Error",
                /* UINT uType: */
                    MB_OK);
            return;
        }
        
        *extptr = GetProcAddress(module, func_name);
    }
    
    if (*extptr == NULL) {
        DWORD error = GetLastError();
        char errmsg[128];
        errmsg[0] = '\0';
        strcpy_capped(
            errmsg,
            128,
            "Failed to get pointer to extension function:\n");
        strcat_capped(
            errmsg,
            128,
            func_name);
        strcat_capped(
            errmsg,
            128,
            "\nError code:\n");
        strcat_uint_capped(errmsg, 128, error);
        printf("%s\n", errmsg);
        requesting_shutdown = true;
    }
}

static uint32_t microsoft_keycode_to_tokone_keycode(
    const uint32_t microsoft_key)
{
    #ifndef LOGGER_IGNORE_ASSERTS
    char err_msg[128];
    #endif
    
    printf("key %u - ", microsoft_key);
    switch (microsoft_key) {
        case 8:
            return TOK_KEY_BACKSPACE;
        case 9:
            return TOK_KEY_TAB;
        case 13:
            return TOK_KEY_ENTER;
        case 16:
            return TOK_KEY_SHIFT;
        case 17:
            return TOK_KEY_CONTROL;
        case 20:
            return TOK_KEY_CAPSLOCK;
        case 27:
            return TOK_KEY_ESCAPE;
        case 32:
            return TOK_KEY_SPACEBAR;
        case 49:
            return TOK_KEY_1;
        case 50:
            return TOK_KEY_2;
        case 51:
            return TOK_KEY_3;
        case 52:
            return TOK_KEY_4;
        case 53:
            return TOK_KEY_5;
        case 54:
            return TOK_KEY_6;
        case 55:
            return TOK_KEY_7;
        case 56:
            return TOK_KEY_8;
        case 57:
            return TOK_KEY_9;
        case 48:
            return TOK_KEY_0;
        case 81:
            return TOK_KEY_Q;
        case 87:
            return TOK_KEY_W;
        case 69:
            return TOK_KEY_E;
        case 82:
            return TOK_KEY_R;
        case 84:
            return TOK_KEY_T;
        case 89:
            return TOK_KEY_Y;
        case 85:
            return TOK_KEY_U;
        case 73:
            return TOK_KEY_I;
        case 79:
            return TOK_KEY_O;
        case 80:
            return TOK_KEY_P;
        case 65:
            return TOK_KEY_A;
        case 83:
            return TOK_KEY_S;
        case 68:
            return TOK_KEY_D;
        case 70:
            return TOK_KEY_F;
        case 71:
            return TOK_KEY_G;
        case 72:
            return TOK_KEY_H;
        case 74:
            return TOK_KEY_J;
        case 75:
            return TOK_KEY_K;
        case 76:
            return TOK_KEY_L;
        case 91:
            return TOK_KEY_WINKEY;
        case 186:
            return TOK_KEY_SEMICOLON;
        //case 222:
        //    return TOK_KEY_SINGLEQUOTE;
        case 90:
            return TOK_KEY_Z;
        case 88:
            return TOK_KEY_X;
        case 67:
            return TOK_KEY_C;
        case 86:
            return TOK_KEY_V;
        case 66:
            return TOK_KEY_B;
        case 78:
            return TOK_KEY_N;
        case 77:
            return TOK_KEY_M;
        case 188:
            return TOK_KEY_COMMA;
        case 190:
            return TOK_KEY_FULLSTOP;
        case 191:
            return TOK_KEY_BACKSLASH;
        case 37:
            return TOK_KEY_LEFTARROW;
        case 38:
            return TOK_KEY_UPARROW;
        case 39:
            return TOK_KEY_RIGHTARROW;
        case 40:
            return TOK_KEY_DOWNARROW;
        case 219:
            return TOK_KEY_OPENSQUAREBRACKET;
        case 221:
            return TOK_KEY_CLOSESQUAREBRACKET;
        default:
            printf("unregistered keycode: %u\n", microsoft_key);
            #ifndef LOGGER_IGNORE_ASSERTS
            strcpy_capped(err_msg, 128, "unhandled windows keycode: ");
            strcat_uint_capped(err_msg, 128, microsoft_key);
            strcat_capped(err_msg, 128, "\n");
            #endif
            break;
    }
    
    #ifndef LOGGER_IGNORE_ASSERTS
    log_dump_and_crash(err_msg);
    #endif
    
    return TOK_KEY_ESCAPE;
}

// It's tradition to copy-paste Full screen code from Raymond Chen
WINDOWPLACEMENT g_wpPrev = { sizeof(WINDOWPLACEMENT) };
void toggle_fullscreen(HWND hwnd, int x, int y)
{
    DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
    if (dwStyle & WS_OVERLAPPEDWINDOW) {
        window_globals->fullscreen = true;
        MONITORINFO mi = { sizeof(mi) };
        if (GetWindowPlacement(hwnd, &g_wpPrev) &&
            GetMonitorInfo(MonitorFromWindow(hwnd,
                MONITOR_DEFAULTTOPRIMARY), &mi))
        {
            SetWindowLong(hwnd, GWL_STYLE,
                dwStyle & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(hwnd, HWND_TOP,
                mi.rcMonitor.left, mi.rcMonitor.top,
                mi.rcMonitor.right - mi.rcMonitor.left,
                mi.rcMonitor.bottom - mi.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    } else {
        window_globals->fullscreen = false;
        SetWindowLong(
            hwnd,
            GWL_STYLE,
            dwStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(hwnd, &g_wpPrev);
        SetWindowPos(
            hwnd, NULL, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

void platform_toggle_fullscreen() {
    printf("toggle fullscreen!\n");
    toggle_fullscreen(window_handle, 0, 0);
}

void platform_enter_fullscreen(void) {
    if (window_globals->fullscreen) {
        platform_toggle_fullscreen();
    }
}

// Windows will send us messages when something
// happens to our window
LRESULT CALLBACK
MainWindowCallback(
    HWND window_handle,
    UINT message_id,
    WPARAM w_param,
    LPARAM l_param) 
{
    assert(window_handle != NULL);
    LRESULT return_value = 0;
    
    switch (message_id)
    {
        //case WM_QUIT: {
        //    application_running = 0;
        //    DestroyWindow(window_handle);
        //    break;
        //}
        case WM_LBUTTONDOWN: {
            register_interaction(
                /* interaction : */
                    &user_interactions[INTR_PREVIOUS_LEFTCLICK_START],
                /* screenspace_x: */
                    user_interactions[INTR_PREVIOUS_MOUSE_MOVE].
                        screen_x,
                /* screenspace_y: */
                    user_interactions[INTR_PREVIOUS_MOUSE_MOVE].
                        screen_y);
            
            user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START] =
                user_interactions[INTR_PREVIOUS_LEFTCLICK_START];
            
            user_interactions[INTR_PREVIOUS_TOUCH_END].
                handled = true;
            user_interactions[INTR_PREVIOUS_LEFTCLICK_END].
                handled = true;
            user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END].
                handled = true;
            break;
        }
        case WM_RBUTTONDOWN: {
            register_interaction(
                /* interaction : */
                    &user_interactions[INTR_PREVIOUS_RIGHTCLICK_START],
                /* screenspace_x: */
                    user_interactions[INTR_PREVIOUS_MOUSE_MOVE].
                        screen_x,
                /* screenspace_y: */
                    user_interactions[INTR_PREVIOUS_MOUSE_MOVE].
                        screen_y);
            
            user_interactions[INTR_PREVIOUS_RIGHTCLICK_END].
                handled = true;
            break;
        }
        case WM_RBUTTONUP: {
            register_interaction(
                /* interaction : */
                    &user_interactions[INTR_PREVIOUS_RIGHTCLICK_END],
                /* screenspace_x: */
                    user_interactions[INTR_PREVIOUS_MOUSE_MOVE].
                        screen_x,
                /* screenspace_y: */
                    user_interactions[INTR_PREVIOUS_MOUSE_MOVE].
                        screen_y);
            
            break;
        }
        case WM_LBUTTONUP: {
            register_interaction(
                /* interaction : */
                    &user_interactions[INTR_PREVIOUS_LEFTCLICK_END],
                /* screenspace_x: */
                    user_interactions[INTR_PREVIOUS_MOUSE_MOVE].
                        screen_x,
                /* screenspace_y: */
                    user_interactions[INTR_PREVIOUS_MOUSE_MOVE].
                        screen_y);
            
            user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END] =
                user_interactions[INTR_PREVIOUS_LEFTCLICK_END];
            
            break;
        }
        case WM_CLOSE: {
            requesting_shutdown = true;
            break;
        }
        case WM_MOVE: {
            
            update_window_position(
                /* float left: */
                    (float)(short)LOWORD(l_param),
                /* float bottom: */
                flip_y_axis(
                    (float)(short)HIWORD(l_param) +
                        window_globals->window_height));
            
            break;
        }
        case WM_SIZE: {
            
            RECT window_rect;
            GetWindowRect(window_handle, &window_rect);
            // BOOL adjusted = AdjustWindowRect(
            //     /* [in: out] LPRECT lpRect: */
            //         &window_rect,
            //     /* [in]      DWORD  dwStyle: */
            //         window_style,
            //     /* [in]      BOOL   bMenu: */
            //         false);
            
            POINT point = { 0, 0 };
            ClientToScreen(window_handle, &point);
            int titlebar_size = point.y - window_rect.top +
                GetSystemMetrics(SM_CYSIZEFRAME) +
                GetSystemMetrics(SM_CYEDGE) * 2;
            
            float window_width = window_rect.right - window_rect.left;
            float window_height =
                window_rect.bottom -
                window_rect.top -
                titlebar_size;
            
            delete_all_ui_elements();
            zpolygons_to_render->size = 0;
            particle_effects_size = 0;
            
            // sets window_globals->height/width/aspectratio etc
            update_window_size(
                /* width: */
                    window_width,
                /* height: */
                    window_height,
                /* at_timestamp_microsecs: */
                    platform_get_current_time_microsecs());
            
            glViewport(
                0,
                0,
                window_width,
                window_height);
            
            break;
        }
        case WM_DESTROY: {
            requesting_shutdown = true;
            break;
        }
        case WM_ACTIVATEAPP: {
            break;
        }
        case WM_PAINT: {
            break;
        }
        case WM_ENABLE: {
            break;
        }
        case WM_SETFOCUS: {
            break;
        }
        case WM_KILLFOCUS: {
            break;
        }
        case WM_SYSKEYDOWN: {
            break;
        }
        case WM_SYSKEYUP: {
            break;
        }
        case WM_KEYUP: {
            uint32_t key_code = w_param;
            uint32_t tok_key = microsoft_keycode_to_tokone_keycode(
                key_code);
            register_keyup(tok_key);
            break;
        }
        case WM_KEYDOWN: {
            uint32_t key_code = w_param;
            uint32_t tok_key = microsoft_keycode_to_tokone_keycode(
                key_code);
            register_keydown(tok_key);
            break;
        }
        default: {
            // use the default handler
            return_value = DefWindowProc(
                window_handle,
                message_id,
                w_param,
                l_param);
            break;
        }
    }
    
    return return_value;
}

// This is basically "int main()" for windows
// GUI applications only
int CALLBACK WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    application_running = 1;
    
    assert(sizeof(GPUPolygon) % 32 == 0);

    assert(MANAGED_MEMORY_SIZE < 500000000);
    assert(UNMANAGED_MEMORY_SIZE < 900000000);
    
    init_application_before_gpu_init();
    
    // You can spawn a console with this code snippet 
    FILE * fp = NULL;
    AllocConsole();
    freopen_s(&fp, "CONIN$", "r", stdin);
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    
    #ifdef UNICODE
    MessageBox(
        /* HWND hWnd: */
            0,
        /* LPCSTR lpText: */
            "UNICODE was defined in the preprocessor when compiling!",
        /* LPCSTR lpCaption: */
            "Compiler error",
        /* UINT uType: */
            MB_OK);
    return 0;
    #endif
     
    WNDCLASS window_params;
    memset_char(&window_params, 0, sizeof(WNDCLASSA));
    window_params.style = CS_CLASSDC | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    window_params.lpfnWndProc = MainWindowCallback;
    window_params.cbClsExtra = 0;
    window_params.cbWndExtra = 0;
    window_params.hInstance = hInstance;
    // window_params.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    window_params.hCursor = LoadCursorA(NULL, IDC_ARROW);
    window_params.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    window_params.lpszMenuName = NULL;
    window_params.lpszClassName = "tokoneWindowClass";
    
    ATOM reg_class_success = RegisterClass(
        /* const WNDCLASS *lpWndClass */
          &window_params);
    
    if (!reg_class_success) {
        DWORD error = GetLastError();
        char errcode[128];
        strcpy_capped(
            errcode,
            128,
            "Failed to RegisterClass() in the win32 API. Error code: ");
        strcat_uint_capped(errcode, 128, error);
        MessageBox(
            /* HWND hWnd: */
                0,
            /* LPCSTR lpText: */
                errcode,
            /* LPCSTR lpCaption: */
                "Error",
            /* UINT uType: */
                MB_OK);
        return 0;
    }
    
    int topleft_y =
        (window_globals->window_bottom + window_globals->window_height);
    
    window_handle = CreateWindowEx(
	/* DWORD dwExStyle:     */ 
            WS_EX_CONTROLPARENT | WS_EX_WINDOWEDGE,
	/* LPCTSTR lpClassName: */
            window_params.lpszClassName,
	/* LPCTSTR lpWindowName:*/ 
            "tok_one",
	/* DWORD dwStyle:       */
            window_style,
	/* int x:               */ 
            window_globals->window_left,
	/* int y:               */ 
            flip_y_axis(topleft_y),
	/* int nWidth:          */ 
            (int)window_globals->window_width,
	/* int nHeight:         */ 
            (int)window_globals->window_height,
	/* HWND hWndParent:     */ 
            0,
	/* HMENU hMenu:         */ 
            0,
	/* HINSTANCE hInstance: */ 
            hInstance,
	/* LPVOID lpParam:      */ 
            0);
    
    if (window_handle == NULL) {
        DWORD error = GetLastError();
	assert(error != 54321);
        char errcode[128];
        strcpy_capped(
            errcode,
            128,
            "Failed to CreateWindowEx() in the win32 API. Error code: ");
        strcat_uint_capped(
            errcode,
            128,
            error);
        MessageBox(
            /* HWND hWnd: */
                window_handle,
            /* LPCSTR lpText: */
                errcode,
            /* LPCSTR lpCaption: */
                "Error",
            /* UINT uType: */
                MB_OK);
	return 0;
    }
 
    /*
    The GetDC function retrieves a handle to a device context (DC) for the 
    client area of a specified window or for the entire screen. You can use
    the returned handle in subsequent GDI functions to draw in the DC. 
    The device context is an opaque data structure, whose values are used 
    internally by GDI.

    If the function succeeds, the return value is a handle to the DC for 
    the specified window's client area.

    If the function fails, the return value is NULL.
    */ 
    HDC device_context = GetDC(window_handle);
    if (!device_context) {
        MessageBox(
            0,
            "Failed to get the device context",
            "Error",
            MB_OK);
        return 0;
    }
    
    PIXELFORMATDESCRIPTOR pfd;
    memset_char(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize           = sizeof(PIXELFORMATDESCRIPTOR); // How can this ever be useful? Bizarre
    pfd.nVersion        = 1;
    pfd.dwFlags         = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;    // Flags
    pfd.iPixelType      = PFD_TYPE_RGBA;
    pfd.cColorBits      = 32;
    pfd.cRedBits        = 0;
    pfd.cRedShift       = 0;
    pfd.cGreenBits      = 0;
    pfd.cGreenShift     = 0;
    pfd.cBlueBits       = 0;
    pfd.cBlueShift      = 0;
    pfd.cAlphaBits      = 0;
    pfd.cAlphaShift     = 0;
    pfd.cAccumBits      = 0;
    pfd.cAccumRedBits   = 0;
    pfd.cAccumGreenBits = 0;
    pfd.cAccumBlueBits  = 0;
    pfd.cAccumAlphaBits = 0;
    pfd.cDepthBits      = 24;
    pfd.cStencilBits    = 8;
    pfd.cAuxBuffers     = 0;
    pfd.iLayerType      = PFD_MAIN_PLANE;
    pfd.bReserved       = 0;
    pfd.dwLayerMask     = 0;
    pfd.dwVisibleMask   = 0;
    pfd.dwDamageMask    = 0;
    
    /*
    int ChoosePixelFormat(
      HDC                         hdc,
      const PIXELFORMATDESCRIPTOR *ppfd);
    
    If the function fails, the return value is zero. 
    To get extended error information, call GetLastError. 
    */ 
    int pixel_format_index = ChoosePixelFormat(device_context, &pfd);
    if (!pixel_format_index) {
        DWORD error = 0;
        error = GetLastError();
        char errcode[128];
        strcpy_capped(
            errcode,
            128,
            "Failed to ChoosePixelFormat() in the "
            "win32 API. Error code: ");
        strcat_uint_capped(errcode, 128, error);
        MessageBox(
            /* HWND hWnd: */
                window_handle,
            /* LPCSTR lpText: */
                errcode,
            /* LPCSTR lpCaption: */
                "Error",
            /* UINT uType: */
                MB_OK);
        
        return 0;
    }
    
    PIXELFORMATDESCRIPTOR spf_actual;
    memset_char(
        &spf_actual,
        0,
        sizeof(PIXELFORMATDESCRIPTOR));
    int success = DescribePixelFormat(
        device_context,
        pixel_format_index,
        sizeof(PIXELFORMATDESCRIPTOR),
        &spf_actual);
    if (!success) {
        assert(0);
    }
    
    BOOL spf_success = SetPixelFormat(
        /* HDC                         hdc: */
        device_context,
        /* int                         format: */
        pixel_format_index,
        /* const PIXELFORMATDESCRIPTOR *ppfd: */
        &spf_actual);
    
    if (!spf_success) {
        DWORD error = 0;
        error = GetLastError();
        char errcode[128];
        strcpy_capped(
            errcode,
            128,
            "Failed to SetPixelFormat() in the win32 "
            "API. Error code: ");
        strcat_uint_capped(
            errcode,
            128,
            error);
        MessageBox(
            /* HWND hWnd: */
                window_handle,
            /* LPCSTR lpText: */
                errcode,
            /* LPCSTR lpCaption: */
                "Error",
            /* UINT uType: */
                MB_OK);
        return 0;
    }
    
    // This is not the rendering context we're hoping 
    // to use, but a dummy one that we make just so we
    // initialize OpenGL and start adding
    // extension functions. I comment this everywhere
    // but 'I cant believe this is actually the API'.
    // Just mindblowing
    HGLRC dummy_context = wglCreateContext(device_context);
    
    if (
        wglMakeCurrent(
            device_context,
            dummy_context))
    {
        // OpenGL extensions for all platforms
        init_opengl_extensions(fetch_extension_func_address);
        
        // This is more of a windows WGL extension than an opengl one
        extptr_wglSwapIntervalEXT = (void *)wglGetProcAddress(
            "wglSwapIntervalEXT");
        if (extptr_wglSwapIntervalEXT) {
            extptr_wglSwapIntervalEXT(1); // V-SYNC to screen refresh
        }
        
        if (!application_running) {
            printf(
                "%s\n",
                "1 or more OpenGL extension procedures failed to load, "
                "exiting...\n");
            
            wait_x_microseconds(3500000);
            return 0;
        }
    } else {
        printf(
            "Failed to wglMakeCurrent() the dummy "
            "rendering context\n");
	return 0;
    }
    
    
    /*
    relevant docs:
    <attribList> specifies a list of attributes for the context. The
    list consists of a sequence of <name,value> pairs terminated by the
    value 0. If an attribute is not specified in <attribList>, then the
    default value specified below is used instead. If an attribute is
    specified more than once, then the last value specified is used.
    */
    int gl46_attribs[7];
    gl46_attribs[0] = WGL_CONTEXT_MAJOR_VERSION_ARB;
    gl46_attribs[1] = 4;
    gl46_attribs[2] = WGL_CONTEXT_MINOR_VERSION_ARB;
    gl46_attribs[3] = 6;
    /*
    The attribute name WGL_CONTEXT_PROFILE_MASK_ARB requests an OpenGL
    context supporting a specific <profile> of the API. If the
    WGL_CONTEXT_CORE_PROFILE_BIT_ARB bit is set in the attribute value,
    then a context implementing the <core> profile of OpenGL is
    returned.
    */
    gl46_attribs[4] = WGL_CONTEXT_PROFILE_MASK_ARB;
    gl46_attribs[5] = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
    gl46_attribs[6] = 0;
    
    HGLRC gl46_context = extptr_wglCreateContextAttribsARB(
        /* HDC hDC */
            device_context,
        /* HGLRC hShareContext */
            0,
        /* const int* attribList: */
            gl46_attribs);
    
    if (gl46_context == 0) {
        MessageBoxA(0, "Failed to init gl46 context!\n", "Error", MB_OK);
        requesting_shutdown = true;
        return 0;
    }
    
    if (!wglMakeCurrent(device_context, gl46_context)) {
        MessageBox(
            0,
            "Failed to make gl46 context current!\n",
            "Error",
            MB_OK);
        requesting_shutdown = true;
        return 0;
    }
    
    assert(sound_settings->global_buffer_size_bytes > 0);
    unsigned int ds_success = 0;
    init_directsound(
        window_handle,
        44100 * 2 * 2,
        &ds_success);
    if (!ds_success) {
        MessageBox(
            /* HWND hWnd: */
                0,
            /* LPCSTR lpText: */
                "Attempting to continue without sound...",
            /* LPCSTR lpCaption: */
                "Error",
            /* UINT uType: */
                MB_OK);
    }
    
    FileBuffer vertex_shader_file;
    vertex_shader_file.size_without_terminator =
        platform_get_resource_size(
            /* const char * filename: */
                "vertex_shader.glsl");
    
    if (vertex_shader_file.size_without_terminator < 1) {
        MessageBox(
            0,
            "Missing file vertex_shader.glsl!\n",
            "Error",
            MB_OK);
        requesting_shutdown = true;
        return 0;
    }
    //vertex_shader_file.contents = malloc(
    //    vertex_shader_file.size_without_terminator + 1);
    vertex_shader_file.contents = malloc_from_managed(
        vertex_shader_file.size_without_terminator + 1);
    memset_char(
        vertex_shader_file.contents,
        0,
        vertex_shader_file.size_without_terminator + 1);
    platform_read_resource_file(
        /* const char * filename: */
            "vertex_shader.glsl",
        /* FileBuffer * out_preallocatedbuffer: */
            &vertex_shader_file);
    
    FileBuffer fragment_shader_file;
    fragment_shader_file.size_without_terminator =
        platform_get_resource_size(
            /* const char * filename: */
                "fragment_shader.glsl");
    if (fragment_shader_file.size_without_terminator < 1) {
        MessageBox(
            0,
            "Missing file fragment_shader.glsl!\n",
            "Error",
            MB_OK);
        requesting_shutdown = true;
        return 0;
    }
    fragment_shader_file.contents = malloc_from_managed(
        fragment_shader_file.size_without_terminator + 1);
    memset_char(
        fragment_shader_file.contents,
        0,
        fragment_shader_file.size_without_terminator + 1);
    platform_read_resource_file(
        /* const char * filename: */
            "fragment_shader.glsl",
        /* FileBuffer * out_preallocatedbuffer: */
            &fragment_shader_file);

    FileBuffer alphablending_fragment_shader_file;
    alphablending_fragment_shader_file.size_without_terminator =
        platform_get_resource_size(
            /* const char * filename: */
                "alphablending_fragment_shader.glsl");
    if (alphablending_fragment_shader_file.size_without_terminator < 1) {
        MessageBox(
            0,
            "Missing file alphablending_fragment_shader.glsl!\n",
            "Error",
            MB_OK);
        requesting_shutdown = true;
        return 0;
    }
    alphablending_fragment_shader_file.contents = malloc_from_managed(
        alphablending_fragment_shader_file.size_without_terminator + 1);
    memset_char(
        alphablending_fragment_shader_file.contents,
        0,
        alphablending_fragment_shader_file.size_without_terminator + 1);
    platform_read_resource_file(
        /* const char * filename: */
            "alphablending_fragment_shader.glsl",
        /* FileBuffer * out_preallocatedbuffer: */
            &alphablending_fragment_shader_file);
    
    opengl_init(
        vertex_shader_file.contents,
        fragment_shader_file.contents,
        alphablending_fragment_shader_file.contents);
    
    free_from_managed(vertex_shader_file.contents);
    free_from_managed(fragment_shader_file.contents);
    
    init_application_after_gpu_init();
    
    start_audio_loop();
    
    //const uint32_t square_wave_size = 44100 * 2;
    //int16_t * square_wave = malloc_from_unmanaged(
    //    square_wave_size * sizeof(int16_t));
    //for (uint32_t i = 0; i < square_wave_size; i += 2) {
    //    square_wave[i  ] = ((i % 256) > 127) * 150;
    //    square_wave[i+1] = ((i % 256) > 127) * 150;
    //}
    //
    //play_sound_bytes(
    //    square_wave,
    //    square_wave_size);
    
    uint32_t frame_i = 0;
    while (!requesting_shutdown)
    { 
        POINT mousepos;
        if (GetCursorPos(
          /* [out] LPPOINT lpPoint: */
            &mousepos))
        {
            // now mousepos.x is the screen location x and
            // mousepos.y is the screen location y of the cursor
            if (
                ScreenToClient(
                  /* [in] HWND    hWnd: */
                      window_handle,
                  /* LPPOINT lpPoint: */
                      &mousepos))
            {
                register_interaction(
                    /* interaction : */
                        &user_interactions[INTR_PREVIOUS_MOUSE_MOVE],
                    /* screenspace_x: */
                        (float)mousepos.x,
                    /* screenspace_y: */
                        (float)(window_globals->window_height - mousepos.y));
                
                user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE] =
                    user_interactions[INTR_PREVIOUS_MOUSE_MOVE];
            }
        } else {
            log_dump_and_crash(
                "Fatal: failed to query windows for the mouse location.");
            return 0;
        }
        
        MSG message;
        /*
        If wMsgFilterMin and wMsgFilterMax are both zero,
        PeekMessage returns all available messages 
        (that is, no range filtering is performed).
        */
        BOOL got_message = PeekMessage(
                &message,
                window_handle,
                0,
                0,
                PM_REMOVE);
        
        if (got_message < 0 || message.message == WM_QUIT) {
            application_running = 0;
        }
        
        if (!got_message || requesting_shutdown) {
            continue;
        }
        
        log_assert(!requesting_shutdown);
        
        TranslateMessage(&message);
        DispatchMessage(&message);
        
        
        shared_gameloop_update(
            /* GPUDataForSingleFrame * frame_data: */
                &gpu_shared_data_collection.triple_buffers[frame_i]);
        
        opengl_render_frame(
            &gpu_shared_data_collection.triple_buffers[frame_i]);
        
        frame_i += 1;
        frame_i %= 3;
        
        SwapBuffers(device_context);
        
        consume_some_global_soundbuffer_bytes();
    }
    
    // shutdown
    shared_shutdown_application();
    
    client_logic_shutdown();
    
    // bool32_t write_succesful = false;
    //log_dump(&write_succesful);
    //
    //if (!write_succesful) {
    //    log_append("ERROR - failed to store the log file on app termination..\n");
    //}
    
    return 0;
}

