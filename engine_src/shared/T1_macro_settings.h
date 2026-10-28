#ifndef T1_MACRO_SETTINGS_H
#define T1_MACRO_SETTINGS_H

// These macro definitions change the behavior of your app when you compile.
#define T1_APP_NAME "T1 app"

#define T1_ACTIVE 1
#define T1_INACTIVE 2

#define T1_ASSERTS_ACTIVE 1

// Check that shaders stay in T1's MSL subset when they load (see RENDERING)
#define T1_SHADER_CHECK_ACTIVE 1

#define T1_GAMEPAD_ACTIVE 1
#define T1_TEXTURES_ACTIVE 1
#define T1_MIPMAPS_ACTIVE 2
#define T1_AUDIO_ACTIVE 2
#define T1_ENGINE_SAVEFILE_ACTIVE 2

#ifdef NS_BLOCK_ASSERTIONS
#ifndef NDEBUG
#define NDEBUG
#endif
#endif

// set to 3 for triple-buffering
#define T1_FRAMES_CAP 2

#define T1_TEXARRAYS_CAP 29
#define T1_TEX_SLICES_CAP 250

// the max # of perma stored sounds in your app
#define T1_ALL_PERMASOUNDS_CAP 75
// 60million samples = 120MB     60...000
#define T1_ALL_AUDIOSAMPLES_SIZE 45000000

#define T1_INITIAL_WINDOW_HEIGHT 1000.0f
#define T1_INITIAL_WINDOW_WIDTH  1400.0f // 800.0f// 375.0f
#define T1_INITIAL_WINDOW_LEFT    200.0f
#define T1_INITIAL_WINDOW_BOTTOM  400.0f

#endif // T1_MACRO_SETTINGS_H
