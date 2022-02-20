// functions we must implement
#include "../shared/platform_layer.h"

// functions for us to use
#include "../shared/software_renderer.h"
#include "../shared/box.h"

#include "windows.h"
#include "stdio.h"
#include "stdlib.h"
#include "assert.h"

/*
This functionality must be provided by the platform because
of iOS, where reading your own app's files is a security
ordeal
*/
FileBuffer * platform_read_file(char * filename) {
    
    FileBuffer * return_value = malloc(sizeof(FileBuffer));
    
    FILE * textfile = fopen(
        filename,
        "rb");
    
    fseek(textfile, 0, SEEK_END);
    unsigned long fsize = (unsigned long)ftell(textfile);
    fseek(textfile, 0, SEEK_SET);
    
    char * buffer = malloc(fsize);
    
    size_t bytes_read = fread(
        /* ptr: */
            buffer,
        /* size of each element to be read: */
            1,
        /* nmemb (no of members) to read: */
            fsize,
        /* stream: */
            textfile);
    
    fclose(textfile);
    if (bytes_read != fsize) {
        printf("Error - expected bytes read equal to fsize\n");
        return NULL;
    }
    
    return_value->contents = buffer;
    return_value->size = bytes_read;
    
    return return_value;
}

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
            printf("got a WM_DESTROY message...\n");
            break;
        }
        case WM_CLOSE:
        {
            printf("got a WM_CLOSE message...\n");
            break;
        }
        case WM_ACTIVATEAPP:
        {
            printf("got a WM_ACTIVEATEAPP message...\n");
            break;
        }
        default:
        {
            printf("got an unhandled message...\n");
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

int CALLBACK WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    WNDCLASS window_params = {};
    window_params.style = CS_OWNDC | CS_HREDRAW | CS_HREDRAW;
    window_params.lpfnWndProc = MainWindowCallback;
    window_params.hInstance = hInstance;
    // window_params.hIcon = ? // TODO: set icon here
    window_params.lpszClassName = "hello3dgfxwindowclass";
    
    if (RegisterClass(&window_params)) {

        printf("RegisterClass was succesful\n");
        HWND window_handle =
            CreateWindowEx(
                /* DWORD dwExStyle:     */ 0,
                /* LPCTSTR lpClassName: */ window_params.lpszClassName,
                /* LPCTSTR lpWindowName:*/ "hello3dgfx",
                /* DWORD dwStyle:       */
                    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                /* int x:               */ CW_USEDEFAULT,
                /* int y:               */ CW_USEDEFAULT,
                /* int nWidth:          */ CW_USEDEFAULT,
                /* int nHeight:         */ CW_USEDEFAULT,
                /* HWND hWndParent:     */ 0,
                /* HMENU hMenu:         */ 0,
                /* HINSTANCE hInstance: */ hInstance,
                /* LPVOID lpParam:      */ 0);
        
        if (window_handle) {
            printf("window_handle was created succesfully.\n");
            for (;;)
            {
                MSG message;
                BOOL message_result = GetMessage(&message, 0, 0, 0);
                if (message_result) {
                    printf("got a message..\n");
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                } else {
                    break;
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

/*
int main() {
    assert(box == NULL);
    
    z_constants_init();
    free_renderer();
    
    printf("hello, windows!\n");
    printf("WINDOW_WIDTH: %f\n", WINDOW_WIDTH);
    printf("WINDOW_HEIGHT: %f\n", WINDOW_HEIGHT);
    
    ColoredVertex * next_workload = 
        malloc(sizeof(ColoredVertex) * 5);
    uint32_t next_workload_size = 0;
    software_render(next_workload, &next_workload_size);
    
    printf(
        "vertices obtained in 1st frame: %u\n",
        next_workload_size);
    
    free(next_workload);
    free_renderer();
    
    return 0;
}
*/
