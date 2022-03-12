// functions we must implement
#include "../shared/platform_layer.h"

// functions for us to use
#include "../shared_windows_android/opengl.h"
#include "../shared/software_renderer.h"
#include "../shared/zpolygon.h"
#include "../shared/bool_types.h"
#include "../shared_windows_macos/platform_read_file.c"
#include "../shared/static_redefinitions.h"

#include <windows.h>
#include <gl/gl.h>

// TODO: remove these
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// Vertex * next_gpu_workload;
// uint32_t next_gpu_workload_size;

uint32_t application_running = true;

// this magical cruft is necessary and I think
// you should carefully read the documentation
// and get used to the meaning if you really want to learn
// openGL // windows programming. I'm just going to type
// it out and get it working for now, don't actually
// understand the initialization options
// if you try to wglCreateContext() without the
// weird pixel option settings first, it will fail
void win32_init_opengl(HWND window) {
    HDC window_dc = GetDC(window);
    
    PIXELFORMATDESCRIPTOR desired = {};
    desired.nSize = sizeof(desired);
    desired.nVersion = 1;
    desired.dwFlags =
        PFD_SUPPORT_OPENGL |
        PFD_DRAW_TO_WINDOW |
        PFD_DOUBLEBUFFER;
    desired.cColorBits = 24;
    desired.cAlphaBits = 8;
    desired.iLayerType = PFD_MAIN_PLANE;
    
    int suggest_pixel_format_i = ChoosePixelFormat(
        window_dc,
        &desired);
    PIXELFORMATDESCRIPTOR suggested;
    DescribePixelFormat(
        window_dc,
        suggest_pixel_format_i,
        sizeof(suggested),
        &suggested);
    SetPixelFormat(
        window_dc,
        suggest_pixel_format_i,
        &suggested);
    
    HGLRC openglrc = wglCreateContext(window_dc);
    if (wglMakeCurrent(window_dc, openglrc)) {
        
        printf("wglMakeCurrent() succeeded\n");
        OpenGLInfo opengl_info = get_opengl_info();
        printf("got opengl_info\n");

        // this is how you can load an extension function
        // manually - it seems quite convoluted to me
        // you need typedefs (search for wgl_swap at the
        // at the top of the file) in addition to this
        manual_wgl_swap_interval_ext
            * loaded_wgl_swap_interval_ext =
                wglGetProcAddress("wglSwapIntervalEXT");
        if (loaded_wgl_swap_interval_ext) {
            printf("wgl swap interval function pointer (extension) was manually loaded!\n");
            loaded_wgl_swap_interval_ext(1);
        } else {
            printf("ERROR - wgl_swap_interval_ext wasn't avaialble on this platform\n");
        }
       
        printf("dynamically load OpenGL extension functions...\n"); 
        glCompileShader =
            (ptr_gl_compile_shader *)
            wglGetProcAddress("glCompileShader");
        assert(glCompileShader != NULL);
        glGetShaderiv =
            (ptr_gl_get_shader_iv *)
            wglGetProcAddress("glGetShaderiv");
        assert(glGetShaderiv != NULL);
        glGetShaderInfoLog =
            (ptr_gl_get_shader_info_log *)
            wglGetProcAddress("glGetShaderInfoLog");
        glCreateShader =
            (ptr_gl_create_shader *)
            wglGetProcAddress("glCreateShader");
        assert(glCreateShader != NULL);
        glCreateProgram =
            (ptr_gl_create_program *)
            wglGetProcAddress("glCreateProgram");
        assert(glCreateProgram != NULL);
        glLinkProgram =
            (ptr_gl_link_program *)
            wglGetProcAddress("glLinkProgram");
        assert(glLinkProgram != NULL);
        glShaderSource =
            (ptr_gl_shader_source *)
            wglGetProcAddress("glShaderSource");
        assert(glShaderSource != NULL);
        glUseProgram =
            (ptr_gl_use_program *)
            wglGetProcAddress("glUseProgram");
        assert(glUseProgram != NULL);
        glGenBuffers =
            (ptr_gl_gen_buffers *)
            wglGetProcAddress("glGenBuffers");
        assert(glGenBuffers != NULL);
        glAttachShader =
            (ptr_gl_attach_shader *)
            wglGetProcAddress("glAttachShader");
        assert(glAttachShader != NULL);
        glBindBuffer =
            wglGetProcAddress("glBindBuffer");
        assert(glBindBuffer != NULL);
        glBufferData =
            wglGetProcAddress("glBufferData");
        assert(glBufferData != NULL);
        glGenVertexArrays =
            (ptr_gl_gen_vertex_arrays *)
            wglGetProcAddress("glGenVertexArrays");
        assert(glGenVertexArrays != NULL);
        glBindVertexArray =
            (ptr_gl_bind_vertex_array *)
            wglGetProcAddress("glBindVertexArray");
        assert(glBindVertexArray != NULL);
        glVertexAttribPointer =
            (ptr_gl_vertex_attrib_pointer *)
            wglGetProcAddress("glVertexAttribPointer");
        assert(glVertexAttribPointer != NULL);
        glEnableVertexAttribArray =
            (ptr_gl_enable_vertex_attrib_array *)
            wglGetProcAddress("glEnableVertexAttribArray");
        assert(glEnableVertexAttribArray != NULL);
        printf("finished dynamically loading OpenGL extension functions...\n"); 
        
    } else {
        printf("failed wglmakecurrent\n");
        assert(false);
    }
}

// Ask windows to paint our window with opengl
void opengl_update_window(HWND window) {
    
    glViewport(0, 0, window_width, window_height);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    uint32_t current_workload_size = 0;
    software_render(
        gpu_workload_buffer,
        &current_workload_size);
    
    glUseProgram(program_id);
    glBindVertexArray(VAO);
    glBufferData(
        GL_ARRAY_BUFFER,
        VERTEX_BUFFER_SIZE * sizeof(Vertex),
        gpu_workload_buffer,
        GL_DYNAMIC_DRAW);
    glDrawArrays(
        GL_TRIANGLES,
        0,
        current_workload_size);
    
    HDC device_context = GetDC(window);
    SwapBuffers(device_context);
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
    LRESULT return_value = 0;
    
    switch (message_id)
    {
        case WM_SIZE:
        {
            printf("got a WM_SIZE message...\n");
            break;
        }
        case WM_DESTROY:
        {
            application_running = false;
            break;
        }
        case WM_CLOSE:
        {
            application_running = false;
            break;
        }
        case WM_ACTIVATEAPP:
        {
            printf("got a WM_ACTIVEATEAPP message...\n");
            break;
        }
        case WM_PAINT:
        {
            opengl_update_window(window_handle);
            break;
        }
        default:
        {
            // printf("got an unhandled message...\n");
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
    WNDCLASS window_params = {};
    window_params.style =
        CS_OWNDC | CS_HREDRAW | CS_HREDRAW;
    window_params.lpfnWndProc = MainWindowCallback;
    window_params.hInstance = hInstance;
    // window_params.hIcon = ? // TODO: set icon here
    window_params.lpszClassName = "hello3dgfxwindowclass";
    
    if (RegisterClass(&window_params)) {
        
        printf("RegisterClass was succesful\n");
        HWND window_handle =
            CreateWindowEx(
                /* DWORD dwExStyle:     */ 0,
                /* LPCTSTR lpClassName: */
                    window_params.lpszClassName,
                /* LPCTSTR lpWindowName:*/ "hello3dgfx",
                /* DWORD dwStyle:       */
                    WS_OVERLAPPEDWINDOW |
                    WS_VISIBLE |
                    CS_OWNDC |
                    WS_MAXIMIZE,
                /* int x:               */ CW_USEDEFAULT,
                /* int y:               */ CW_USEDEFAULT,
                /* int nWidth:          */ CW_USEDEFAULT,
                /* int nHeight:         */ CW_USEDEFAULT,
                /* HWND hWndParent:     */ 0,
                /* HMENU hMenu:         */ 0,
                /* HINSTANCE hInstance: */ hInstance,
                /* LPVOID lpParam:      */ 0);
        
        if (window_handle) {
            printf(
                "window_handle was created succesfully.\n");
            
            RECT window_rect;
            if (GetWindowRect(
                window_handle,
                &window_rect))
            {
                window_width =
                    window_rect.right - window_rect.left;
                window_height =
                    window_rect.bottom - window_rect.top;
            } else {
                printf("ERROR - can't deduce window size\n");
                assert(0);
            }
            printf("init_projection_constants()..\n");
            init_projection_constants();
            printf("init_renderer()..\n");
            init_renderer();
            // printf("malloc Vertex buffer...\n");
            // next_gpu_workload =
            //     malloc(sizeof(Vertex) * 500000);
            
            printf("Win32InitOpenGL()..\n");
            win32_init_opengl(window_handle); 
            
            opengl_compile_shaders();
            
            printf("start game loop...\n");
            while (application_running)
            {
                MSG message;
                BOOL message_result = GetMessage(
                    &message,
                    0,
                    0,
                    0);
                if (message_result) {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }                
            }
        } else {
            printf("window_handle creation failed!\n");
            assert(0);
        }
    } else {
        printf("ERROR: RegisterClass() failed!\n");
    }
    
    printf("end of program.\n");
    
    return 0;
}

