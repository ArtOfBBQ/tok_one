#include "../shared/platform_layer.h"

// functions for us to use
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

uint32_t application_running = true;
Vertex * next_gpu_workload;
uint32_t next_gpu_workload_size;

// openGL nonsense
// if you use a "loader library" like GLU it saves you
// this trouble amongst other things
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
typedef char GLchar;

// These typedefs are function pointers
// they weren't in OpenGL v1.0, but were added as
// extensions
// you need query for their availability and set the
// pointers during runtime with wglGetProcAddress()
// to make it work without a library
typedef BOOL WINAPI manual_wgl_swap_interval_ext(int interval);

typedef void WINAPI ptr_gl_attach_shader(GLuint program, GLuint shader);
typedef void ptr_gl_compile_shader(GLuint shader_id);
typedef GLuint ptr_gl_create_shader(GLenum type);
typedef GLuint ptr_gl_create_program(void);
typedef void ptr_gl_link_program(GLuint program_id);
typedef void ptr_gl_shader_source(GLuint shader_id, GLsizei count, GLchar **string, GLint * length);
typedef void ptr_gl_use_program(GLuint program_id);

static ptr_gl_compile_shader *glCompileShader;
static ptr_gl_create_shader *glCreateShader;
static ptr_gl_create_program *glCreateProgram;
static ptr_gl_link_program *glLinkProgram;
static ptr_gl_shader_source *glShaderSource;
static ptr_gl_attach_shader *glAttachShader;
static ptr_gl_use_program *glUseProgram;


// info about what OpenGL functionality
// is/isnt available on platform
typedef struct OpenGLInfo
{
    // TODO: some of these string fields (like extensions)
    // are parsed into bool fields and then not used
    // anymore. They might be useful
    // for debugging right now but should be deleted
    char * vendor;
    char * renderer;
    char * version;
    char * shading_language_version;
    char * extensions; 
    
    bool32_t EXT_texture_sRGB_decode;
    bool32_t GL_ARB_framebuffer_sRGB;
} OpenGLInfo;

// TODO: seems out of place here
// should be moved to a string manipulation header
// or something?
static bool32_t are_equal_strings(
    char * str1,
    char * str2,
    size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (str1[i] != str2[i]) {
            return false;
        }
    }
    
    return true;
}

// TODO: move openGL related stuff to a different file
// maybe think about it when we start an android platform
// layer and find out what can be re-used
OpenGLInfo get_opengl_info() {
    OpenGLInfo return_value = {};
    
    return_value.vendor = (char *)glGetString(GL_VENDOR);
    return_value.renderer =
        (char *)glGetString(GL_RENDERER);
    return_value.version = (char *)glGetString(GL_VERSION);
    return_value.shading_language_version =
        (char *)glGetString(GL_VENDOR);
    return_value.vendor = (char *)glGetString(GL_VENDOR);
    return_value.extensions =
        (char *)glGetString(GL_EXTENSIONS);

    printf("opengl version: %s\n", return_value.version);
    
    char * at = return_value.extensions; 
    char * end = at;
    while (*end) {
        while (*at == ' ') {
            at++;
        }
        end = at;
        while (*end && *end != ' ') { end++; }
        
        if (
            are_equal_strings(
                at,
                "EXT_texture_sRGB_decode",
                end - at))
        {
            return_value.EXT_texture_sRGB_decode = true;
        } else if (
            are_equal_strings(
                at,
                "GL_ARB_framebuffer_sRGB;",
                end - at))
        {
            return_value.GL_ARB_framebuffer_sRGB = true;
        } else if (
            are_equal_strings(
                at,
                "put your expected extension string here",
                end - at))
        {
            // reserved
        }
        
        at = end;
    }
    
    return return_value;
}


// Ask windows to paint our window with opengl
void opengl_update_window(HWND window) {
    
    next_gpu_workload_size = 0;
    software_render(
        next_gpu_workload,
        &next_gpu_workload_size);
    
    glViewport(0, 0, window_width, window_height);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glBegin(GL_TRIANGLES);
    
    for (
        uint32_t i = 0;
        i < next_gpu_workload_size;
        i++)
    {
        if (next_gpu_workload[i].texture_i >= 0) {
            glColor3f(
                0.25f,
                0.5f,
                1.0f);
        } else {
            glColor3f(
                next_gpu_workload[i].RGBA[0],
                next_gpu_workload[i].RGBA[1],
                next_gpu_workload[i].RGBA[2]);
        }
        glVertex2f(
            next_gpu_workload[i].x,
            next_gpu_workload[i].y);
    }
    glEnd();
    
    // We don't need this paint struct,
    // we just want to get the device_context
    // so this seems wrong but I don't know how
    // else to get it
    // PAINTSTRUCT paint;
    // HDC device_context = BeginPaint(
    //     /* HWND handle: */ window_handle,
    //     /* [out] LPPAINTSTRUCT lpPaint: */ &paint);
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

// TODO: move opengl stuff elsewhere
// this magical cruft is necessary and I think
// you should carefully read the documentation
// and memorize the meaning if you really want to learn
// openGL // windows programming. I'm just going to type
// it out and get it working for now, don't actually
// understand the initialization options
// if you try to wglCreateContext() without the
// weird pixel option settings first, it will fail
void Win32InitOpenGL(HWND window) {
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
        
        glCompileShader =
            (ptr_gl_compile_shader *)
            wglGetProcAddress("glCompileShader");
        assert(glCompileShader != NULL);
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
        glAttachShader =
            (ptr_gl_attach_shader *)
            wglGetProcAddress("glAttachShader");
        assert(glAttachShader != NULL);
        
    } else {
        printf("failed wglmakecurrent\n");
        assert(false);
    }
}

void opengl_compile_shaders() {
    GLuint vertex_shader_id = glCreateShader(
        GL_VERTEX_SHADER);
    glShaderSource(
        /* shader handle: */ vertex_shader_id,
        /* shader count : */ 1,
        /* shader source: */ "int main()",
        /* source length: */ 5);
    glCompileShader(vertex_shader_id);
    
    GLuint fragment_shader_id = glCreateShader(
        GL_FRAGMENT_SHADER);
    glShaderSource(
        /* shader handle: */ fragment_shader_id,
        /* shader count : */ 1,
        /* shader source: */ "int main()",
        /* source length: */ 5);
    glCompileShader(fragment_shader_id);
    
    GLuint program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader_id);
    
    glLinkProgram(program_id);
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
            printf("malloc Vertex buffer...\n");
            next_gpu_workload =
                malloc(sizeof(Vertex) * 50000);
            
            printf("Win32InitOpenGL()..\n");
            Win32InitOpenGL(window_handle); 

            // opengl_compile_shaders();
            
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

