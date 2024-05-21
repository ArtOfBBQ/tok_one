// functions for us to use
#include <windows.h>
#include <gl/gl.h>

#include "opengl_extensions.h"
#include "opengl.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "common.h"


static unsigned int application_running = 1;
static unsigned int window_width = 200;
static unsigned int window_height = 200;

static HWND window_handle;

static void fetch_extension_func_address(
    void ** extptr,
    char * func_name)
{
    if (*extptr != NULL) {
        MessageBox(
            /* HWND hWnd: */
                window_handle,
            /* LPCSTR lpText: */
                "Tried to fetch a PROC address that was already non-null",
            /* LPCSTR lpCaption: */
                "Error",
            /* UINT uType: */
                MB_OK);
        application_running = 0;
        return;
    }
    
    *extptr = (void *)wglGetProcAddress(func_name);
    if (extptr == NULL) {
        DWORD error = 0;
        error = GetLastError();
        char errcode[128];
        strcpy_capped(
            errcode,
            128,
            "Failed to get pointer to extension function."
            "Error code: ");
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
        application_running = 0;
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
        case WM_QUIT: {
            printf("got a WM_QUIT message...\n");
            application_running = 0;
            DestroyWindow(window_handle);
            break;
        }
        case WM_CLOSE: {
            printf("got a WM_CLOSE message...\n");
            application_running = 0;
            DestroyWindow(window_handle);
            break;
        }
        case WM_SIZE: {
            printf("got a WM_SIZE message...\n");
            break;
        }
        case WM_DESTROY: {
            printf("got a WM_DESTROY message...\n");
            application_running = 0;
            break;
        }
        case WM_ACTIVATEAPP: {
            printf("got a WM_ACTIVEATEAPP message...\n");
            break;
        }
        case WM_PAINT: {
            printf("got a WM_PAINT message...\n");
            break;
        }
        case WM_ENABLE: {
            printf("got a WM_ENABLE message...\n");
            break;
        }
        case WM_SETFOCUS: {
            printf("got a WM_SETFOCUS message...\n");
            break;
        }
        case WM_KILLFOCUS: {
            printf("got a WM_KILLFOCUS message...\n");
            break;
        }
        default: {
            printf(".");
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
// applications only
int CALLBACK WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
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
    #endif
    
    /*
    ATOM RegisterClassA(
      [in] const WNDCLASSA *lpWndClass
    );
    
    typedef struct {
    UINT style;
    WNDPROC lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HBRUSH hbrBackground;
    LPCTSTR lpszMenuName;
    LPCTSTR lpszClassName;
    } WNDCLASS, *PWNDCLASS;
    */ 
    WNDCLASS window_params;
    memset(&window_params, 0, sizeof(WNDCLASSA));
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
    
    HWND window_handle = CreateWindowEx(
	/* DWORD dwExStyle:     */ 
            WS_EX_CONTROLPARENT | WS_EX_WINDOWEDGE,
	/* LPCTSTR lpClassName: */
            window_params.lpszClassName,
	/* LPCTSTR lpWindowName:*/ 
            "tok_one",
	/* DWORD dwStyle:       */
            WS_OVERLAPPEDWINDOW |
            WS_VISIBLE |
	    WS_SIZEBOX |
	    WS_CLIPCHILDREN |
	    WS_CLIPSIBLINGS, 
	/* int x:               */ 
            CW_USEDEFAULT,
	/* int y:               */ 
            CW_USEDEFAULT,
	/* int nWidth:          */ 
            CW_USEDEFAULT,
	/* int nHeight:         */ 
            CW_USEDEFAULT,
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
    
    RECT window_rect;
    if (
        GetWindowRect(
        window_handle,
            &window_rect))
    {
        window_width = window_rect.right - window_rect.left;
        window_height = window_rect.bottom - window_rect.top;
    } else {
        MessageBox(
            window_handle,
            "Can't deduce window size",
            "Error",
            0);
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
    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
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
    memset(
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
        fetch_extension_func_address(
            (void **)&extptr_wglCreateContextAttribsARB,
            "wglCreateContextAttribsARB");
        fetch_extension_func_address(
            (void **)&extptr_glGetUniformLocation,
            "glGetUniformLocation");
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
    int gl33_attribs[7];
    gl33_attribs[0] = WGL_CONTEXT_MAJOR_VERSION_ARB;
    gl33_attribs[1] = 3;
    gl33_attribs[2] = WGL_CONTEXT_MINOR_VERSION_ARB;
    gl33_attribs[3] = 3;
    /*
    The attribute name WGL_CONTEXT_PROFILE_MASK_ARB requests an OpenGL
    context supporting a specific <profile> of the API. If the
    WGL_CONTEXT_CORE_PROFILE_BIT_ARB bit is set in the attribute value,
    then a context implementing the <core> profile of OpenGL is
    returned.
    */
    gl33_attribs[4] = WGL_CONTEXT_PROFILE_MASK_ARB;
    gl33_attribs[5] = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
    gl33_attribs[6] = 0;
    
    HGLRC gl33_context = extptr_wglCreateContextAttribsARB(
        /* HDC hDC */
            device_context,
        /* HGLRC hShareContext */
            0,
        /* const int* attribList: */
            gl33_attribs);
    
    if (gl33_context == 0) {
        MessageBoxA(0, "Failed to init gl33 context!\n", "Error", MB_OK);
        application_running = false;
        return 0;
    }
    
    if (!wglMakeCurrent(device_context, gl33_context)) {
        MessageBox(
            0,
            "Failed to make gl33 context current!\n",
            "Error",
            MB_OK);
        application_running = false;
        return 0;
    }
    
    #if 0
    opengl_compile_shaders(
        /* char * vertex_shader_source: */
            
        /* uint32_t vertex_shader_source_size: */
            
        /* char * fragment_shader_source: */
            
        /* uint32_t fragment_shader_source_size: */
            );
    #endif
    
    while (application_running)
    {
        MSG message;
	for (int i = 0; i < 30; i++)
	{
	    /*
	    If wMsgFilterMin and wMsgFilterMax are both zero,
            PeekMessage returns all available messages 
            (that is, no range filtering is performed).
	    */
	    BOOL got_message =
		PeekMessage(
		    &message, window_handle, 0, 0, PM_REMOVE);
	    if (message.message == WM_QUIT) {
		application_running = 0;
		break;
	    }
	    if (!got_message || !application_running) {
		break;
	    }
	    
	    TranslateMessage(&message);
	    DispatchMessage(&message);
	}
        
        glClearColor(1.0f, 0.25f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        SwapBuffers(device_context);
    }
    
    return 0;
}
