// functions we must implement
// #include "../shared/platform_layer.h" 

// shared functionality we can use
#include "../shared/common.h"
// #include "../shared/userinput.h"
#include "../shared/window_size.h"
// #include "../shared/vertex_types.h"
// #include "../shared/zpolygon.h"
// #include "../shared/software_renderer.h"
// #include "../shared/bitmap_renderer.h"
// #include "../shared/clientlogic.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <GL/gl.h>

static bool32_t handle_SDL_event(
    SDL_Event * event)
{
    // do stuff!
    switch (event->type)
    {
        case SDL_QUIT: {
            return 1;
        }
        break;
    }

    return 0;
}

int main(int argc, const char * argv[]) 
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_ShowSimpleMessageBox(
            /* Uint32 flags: */
                SDL_MESSAGEBOX_INFORMATION,
            /* const char *title: */
                "Error",
            /* const char *message: */
                "Failed to initialize SDL",
            /* SDL_Window *window: */
                0);
        return 0;
    }
    
    SDL_Window * window = SDL_CreateWindow(
        "TOK ONE",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        640,
        480,
        SDL_WINDOW_OPENGL);
    
    SDL_GLContext Context =
        SDL_GL_CreateContext(window); 
    
    
    #ifdef SDL_OPENGL
    assert(0);
    #endif

    #ifdef SDL_FULLSCREEN
    assert(0);
    #endif
    
    float red = 1.0f;
    while (1) {
        SDL_Event event;
        SDL_WaitEvent(&event);
        if (handle_SDL_event(&event)) {
            break;
        }
        glViewport(
            0,
            0,
            window_width,
            window_height);
        glClearColor(red, 0.0f, 1.0f, 1.0f);
        red -= 0.03f;
        if (red < 0.0f) { red = 1.0f; }
        glClear(GL_COLOR_BUFFER_BIT);
        SDL_GL_SwapWindow(window);
    }
    
    SDL_Quit();
    return 1;
}

