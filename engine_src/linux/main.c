// Linux & OpenGL headers
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>

// tok one platform specific headers
#include "opengl.h"
#include "linuxkeyboard.h"

// tok one shared engine headers
#include "common.h"
#include "cpu_to_gpu_types.h"
#include "window_size.h"
#include "init_application.h"
#include "gameloop.h"
#include "userinput.h"

extern char application_path[128];
uint32_t application_running = 1;

GPUSharedDataCollection gpu_shared_data_collection;

#define GLX_CONTEXT_MAJOR_VERSION_ARB       0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB       0x2092
typedef GLXContext (*glXCreateContextAttribsARBProc)(
    Display*,
    GLXFBConfig,
    GLXContext,
    Bool,
    const int*);

// Helper to check for extension string presence.  Adapted from:
//   http://www.opengl.org/resources/features/OGLextensions/
static bool32_t isExtensionSupported(
    const char *extList,
    const char *extension)
{
    const char *start;
    const char *where, *terminator;
    
    /* Extension names should not have spaces. */
    where = strchr(extension, ' ');
    if (where || *extension == '\0') {
        return false;
    }
    
    /* It takes a bit of care to be fool-proof about parsing the
     OpenGL extensions string. Don't be fooled by sub-strings,
     etc. */
    for (start=extList;;) {
        where = strstr(start, extension);
        
        if (!where)
          break;
        
        terminator = where + strlen(extension);
        
        if ( where == start || *(where - 1) == ' ' )
          if ( *terminator == ' ' || *terminator == '\0' )
            return true;
        
        start = terminator;
    }
    
    return false;
}

static bool32_t ctxErrorOccurred = false;
static int ctxErrorHandler(Display *dpy, XErrorEvent *ev)
{
    ctxErrorOccurred = true;
    return 0;
}

int main(int argc, char* argv[])
{
    printf("starting TOK ONE with preallocation:\n");
    printf("UNMANAGED_MEMORY_SIZE: %u\n", UNMANAGED_MEMORY_SIZE);
    printf("MANAGED_MEMORY_SIZE: %u\n", MANAGED_MEMORY_SIZE);
    
    // on linux the relative application path is always in argv[0],
    // but it's followed by the application name
    strcpy_capped(application_path, 128, argv[0]);
    uint32_t char_i = 0;
    while (application_path[char_i] != '\0') { char_i++; }
    while (application_path[char_i] != '/')  { char_i--; }
    application_path[char_i] = '\0';
    
    init_application_before_gpu_init();
    
    printf("%s\n", "finished init_application()");
    
    Display *display = XOpenDisplay(NULL);
    Screen * screen = ScreenOfDisplay(display, 0);
    
    if (!display) {
        exit(1);
    }
    
    // Get a matching FB config
    static int visual_attribs[] =
    {
        GLX_X_RENDERABLE    , True,
        GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
        GLX_RENDER_TYPE     , GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
        GLX_RED_SIZE        , 8,
        GLX_GREEN_SIZE      , 8,
        GLX_BLUE_SIZE       , 8,
        GLX_ALPHA_SIZE      , 8,
        GLX_DEPTH_SIZE      , 24,
        GLX_STENCIL_SIZE    , 8,
        GLX_DOUBLEBUFFER    , True,
        //GLX_SAMPLE_BUFFERS  , 1,
        //GLX_SAMPLES         , 4,
        None
    };
    
    int glx_major, glx_minor;
    
    // FBConfigs were added in GLX version 1.3.
    if (
        !glXQueryVersion(
            display,
            &glx_major,
            &glx_minor) ||
        (
            (glx_major == 1) &&
            (glx_minor < 3)) ||
        (glx_major < 1))
    {
        exit(1);
    }
    
    int fbcount;
    GLXFBConfig* fbc = glXChooseFBConfig(
        display,
        DefaultScreen(display),
        visual_attribs, &fbcount);
    
    if (!fbc)
    {
        exit(1);
    }
    
    // Pick the FB config/visual with the most samples per pixel
    int best_fbc = -1;
    int worst_fbc = -1;
    int best_num_samp = -1;
    int worst_num_samp = 999;
    
    int i;
    for (i=0; i<fbcount; ++i)
    {
        XVisualInfo *vi = glXGetVisualFromFBConfig(display, fbc[i]);
        if (vi)
        {
            int samp_buf, samples;
            glXGetFBConfigAttrib(
                display,
                fbc[i],
                GLX_SAMPLE_BUFFERS,
                &samp_buf);
            glXGetFBConfigAttrib(
                display,
                fbc[i],
                GLX_SAMPLES,
                &samples);
            
            if ( best_fbc < 0 || samp_buf && samples > best_num_samp )
            best_fbc = i, best_num_samp = samples;
            if ( worst_fbc < 0 || !samp_buf || samples < worst_num_samp )
            worst_fbc = i, worst_num_samp = samples;
        }
        
        XFree( vi );
    }
    
    GLXFBConfig bestFbc = fbc[ best_fbc ];
    
    // Be sure to free the FBConfig list allocated by glXChooseFBConfig()
    XFree( fbc );
    
    // Get a visual
    XVisualInfo *vi = glXGetVisualFromFBConfig(
        display,
        bestFbc);
    
    int window_left = (int)window_globals->window_left;
    int window_top =
        (int)window_globals->window_height +
        (int)window_globals->window_bottom;
    XSetWindowAttributes swa;
    Colormap cmap;
    swa.colormap = cmap = XCreateColormap(
        display,
        RootWindow(display, vi->screen), 
        vi->visual,
        AllocNone);
    swa.background_pixmap = None ;
    swa.border_pixel      = 255;
    swa.event_mask        = StructureNotifyMask;
    swa.override_redirect = false;
    
    // x and y parameters are weirdly ignored
    // override_redirect fixes that, but 
    // comes with its own set of problems
    // trying to learn how to fix this
    Window win = XCreateWindow(
        /* display: */
            display,
        /* parent: */
            RootWindow(display, vi->screen),
        /* int x (left): */
            window_left, // will be ignored & overridden because X11 is cool
        /* int y (top): */
            window_top, // will be ignored & overridden because X11 is cool
        /* width: */
            window_globals->window_width,
        /* height: */
            window_globals->window_height,
        /* border_width: */
            0,
        /* depth: */
            vi->depth,
        /* class: */
            InputOutput, 
        /* visual: */
            vi->visual, 
        /* valuemask: */
            CWBorderPixel |
            CWColormap |
            CWEventMask |
            CWOverrideRedirect,
        /* attributes: */
            &swa);
    
    if (!win)
    {
        exit(1);
    }

    // This is incredible to me, but this is
    // how we re-request the ignored params
    // from before to get some control over
    // where the window is positioned
    XSizeHints my_hints;
    my_hints.flags = PPosition | PSize;
    my_hints.x = window_left;
    my_hints.y = window_top;
    my_hints.width = window_globals->window_width;
    my_hints.height = window_globals->window_height;
    XSetNormalHints(
        display,
        win,
        &my_hints);
    
    // Done with the visual info data
    XFree(vi);
    
    XStoreName(display, win, APPLICATION_NAME);
    
    XMapWindow(display, win);
    
    // Get the default screen's GLX extension list
    const char * glxExts = glXQueryExtensionsString(
        display,
        DefaultScreen(display));
    
    // NOTE: It is not necessary to create or make current to a context before
    // calling glXGetProcAddressARB
    glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
    glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)
        glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");
    
    GLXContext ctx = 0;
    
    // Install an X error handler so the application won't exit if GL 3.0
    // context allocation fails.
    //
    // Note this error handler is global.  All display connections in all
    // threads of a process use the same error handler, so be sure to guard
    // against other threads issuing X commands while this code is running.
    ctxErrorOccurred = false;
    int (* oldHandler)(Display *, XErrorEvent *) =
        XSetErrorHandler(&ctxErrorHandler);
    
    // Check for the GLX_ARB_create_context extension string and the function.
    // If either is not present, use GLX 1.3 context creation method.
    if (
        !isExtensionSupported(
            glxExts,
            "GLX_ARB_create_context") ||
        !glXCreateContextAttribsARB)
    {
        ctx = glXCreateNewContext(
            display,
            bestFbc,
            GLX_RGBA_TYPE,
            0,
            True);
    } else {
        // If it does, try to get a GL 3.0 context!
        int context_attribs[] =
          {
            GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
            GLX_CONTEXT_MINOR_VERSION_ARB, 0,
            // GLX_CONTEXT_FLAGS_ARB        ,
            // GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
            None
          };
        
        ctx = glXCreateContextAttribsARB(
            display,
            bestFbc,
            0,
            True,
            context_attribs
        );
        
        // Sync to ensure any errors generated are processed.
        XSync(display, False);
        if (!ctxErrorOccurred && ctx) {
          // Created GL 3.0 context
        } else {
            // Couldn't create GL 3.0 context.  Fall back to old-style 2.x
            // context. When a context version below 3.0 is requested,
            // implementations will return the newest context version compatible
            // with OpenGL versions less than version 3.0.
            // GLX_CONTEXT_MAJOR_VERSION_ARB = 1
            context_attribs[1] = 1;
            // GLX_CONTEXT_MINOR_VERSION_ARB = 0
            context_attribs[3] = 0;

            ctxErrorOccurred = false;

            // Failed to create GL 3.0 context
            // ... using old-style GLX context
            ctx = glXCreateContextAttribsARB(
                display,
                bestFbc,
                0, 
                True,
                context_attribs);
        }
    }
    
    // Sync to ensure any errors generated are processed.
    XSync(display, False);
    
    // Restore the original error handler
    XSetErrorHandler(oldHandler);
    
    if (ctxErrorOccurred || !ctx)
    {
        exit(1);
    }
    
    // Verifying that context is a direct context
    if (!glXIsDirect(display, ctx)) {
        // Indirect GLX rendering context obtained
    } else {
        // Direct GLX rendering context obtained
    }
    
    // Making context current
    glXMakeCurrent(display, win, ctx);
    
    // Jelle: compile shaders
    FileBuffer vertex_shader_source;
    vertex_shader_source.size_without_terminator = 
        platform_get_resource_size("vertex_shader.glsl");
    vertex_shader_source.contents = (char *)malloc_from_managed(
        vertex_shader_source.size_without_terminator + 1);
    platform_read_resource_file(
        /* const char * resource name: */
            "vertex_shader.glsl",
        /* FileBuffer * out_preallocatedbuffer: */
            &vertex_shader_source);
    
    FileBuffer fragment_shader_source;
    fragment_shader_source.size_without_terminator = 
        platform_get_resource_size("fragment_shader.glsl");
    fragment_shader_source.contents = (char *)malloc_from_managed(
        fragment_shader_source.size_without_terminator + 1);
    platform_read_resource_file(
        /* const char * resource name: */
            "fragment_shader.glsl",
        /* FileBuffer * out_preallocatedbuffer: */
            &fragment_shader_source);
    
    opengl_compile_shaders(
        /* char * vertex_shader_source: */
            vertex_shader_source.contents,
        /* uint32_t vertex_shader_source_size: */
            vertex_shader_source.size_without_terminator,
        /* char * fragment_shader_source: */
            fragment_shader_source.contents,
        /* uint32_t fragment_shader_source_size: */
            fragment_shader_source.size_without_terminator);
    
    XSelectInput(
        display,
        win,
        KeyPressMask |
        KeyReleaseMask |
        ButtonPressMask |
        ButtonReleaseMask |
        StructureNotifyMask);
    XEvent event;
    
    uint32_t current_frame_i  = 0;
    uint64_t lifetime_frame_i = 0;
    
    init_application_after_gpu_init();
    
    glClearColor(0.0f, 0.03f, 0.015, 1.0f);
    while (application_running) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        /* events */
        while (XPending(display)) {
            XNextEvent(display, &event);
            float mouse_x =
                event.xbutton.x;
            float mouse_y =
                window_globals->window_height - event.xbutton.y;
            
            if (event.type == KeyPress) {
                // keyboard key pressed
                uint32_t tok_one_key = linux_keycode_to_tokone_keycode(
                    event.xkey.keycode);
                register_keydown(tok_one_key);
            } else if (event.type == KeyRelease) {
                // keyboard key released
                uint32_t tok_one_key = linux_keycode_to_tokone_keycode(
                    event.xkey.keycode);
                register_keyup(tok_one_key);
            } else if (event.type == ButtonPress) {
                // mouse click
                // event.xbutton.x_root,
                // event.ybutton.y_root,
                
                switch (event.xbutton.button) {
                    case 1:
                        printf("Left Click: [%f, %f]\n",
                            mouse_x,
                            mouse_y);
                        register_interaction(
                            /* interaction : */
                                &user_interactions[
                                    INTR_PREVIOUS_LEFTCLICK_START],
                            /* screenspace_x: */
                                mouse_x,
                            /* screenspace_y: */
                                mouse_y);
                        
                        user_interactions[
                            INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START] =
                                user_interactions[
                                    INTR_PREVIOUS_LEFTCLICK_START];
                        user_interactions[INTR_PREVIOUS_MOUSE_MOVE] =
                            user_interactions[INTR_PREVIOUS_LEFTCLICK_START];
                        user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE] =
                            user_interactions[INTR_PREVIOUS_LEFTCLICK_START];
                        
                        user_interactions[INTR_PREVIOUS_TOUCH_END].
                            handled = true;
                        user_interactions[INTR_PREVIOUS_LEFTCLICK_END].
                            handled = true;
                        user_interactions[
                            INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END].
                                handled = true;
                        break;
                    case 2:
                        printf("Middle Click\n");
                        break;
                    case 3:
                        printf("Right Click\n");
                        break;
                    case 4:
                        printf("Scroll UP\n");
                        break;
                    case 5:
                        printf("Scroll Down\n");
                        break;
                }
            } else if (event.type == ButtonRelease) {
                switch (event.xbutton.button) {
                    case 1:
                        printf("Left Up: [%f, %f]\n",
                            mouse_x,
                            mouse_y);
                        register_interaction(
                            /* interaction : */
                                &user_interactions[
                                    INTR_PREVIOUS_LEFTCLICK_END],
                            /* screenspace_x: */
                                mouse_x,
                            /* screenspace_y: */
                                mouse_y);
                        
                        user_interactions[
                            INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END] =
                                user_interactions[
                                    INTR_PREVIOUS_LEFTCLICK_END];
                        user_interactions[INTR_PREVIOUS_MOUSE_MOVE] =
                            user_interactions[INTR_PREVIOUS_LEFTCLICK_END];
                        user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE] =
                            user_interactions[INTR_PREVIOUS_LEFTCLICK_END];
                        break;
                    case 2:
                        printf("Middle Up\n");
                        break;
                    case 3:
                        printf("Right Up\n");
                        break;
                    case 4:
                        printf("Scroll UP\n");
                        break;
                    case 5:
                        printf("Scroll Down\n");
                        break;
                }
            } else if (event.type == ConfigureNotify) {
                
                // Window configuration did change
                update_window_size(
                    /* width: */
                        event.xconfigure.width,
                    /* height: */
                        event.xconfigure.height,
                    /* at_timestamp_microsecs: */
                        platform_get_current_time_microsecs());
                
                update_window_position(
                    /* float left: */
                        event.xconfigure.x,
                    /* float bottom: */
                        screen->height -
                        window_globals->window_height -
                        event.xconfigure.y);
                
                glViewport(
                    0,
                    0,
                    window_globals->window_width,
                    window_globals->window_height);
            }
        }
        
        gpu_shared_data_collection.
            triple_buffers[current_frame_i].
            vertices_size = 0;
        gpu_shared_data_collection.
            triple_buffers[current_frame_i].
            light_collection->lights_size = 0;
        
        assert(current_frame_i >= 0);
        assert(current_frame_i <= 2);
        shared_gameloop_update(
            &gpu_shared_data_collection.triple_buffers[current_frame_i]);
        
        assert(current_frame_i >= 0);
        assert(current_frame_i <= 2);
        assert(
            gpu_shared_data_collection.triple_buffers[current_frame_i].
                vertices_size % 3 == 0);
        
        opengl_render_triangles(
            &gpu_shared_data_collection.triple_buffers[current_frame_i]); 
        
        // XSync(display, False);
        glXSwapBuffers(display, win);
        glFinish();
        
        lifetime_frame_i += 1;
        current_frame_i = lifetime_frame_i % 3;
    }
    
    glXMakeCurrent(display, 0, 0);
    glXDestroyContext(display, ctx);
    
    XDestroyWindow(display, win);
    XFreeColormap(display, cmap);
    XCloseDisplay(display);
    
    platform_close_application();
    
    return 0;
}

