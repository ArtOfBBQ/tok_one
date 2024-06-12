#ifndef TOK_DIRECTSOUND_H
#define TOK_DIRECTSOUND_H

#include <windows.h>
#include <dsound.h>

void init_directsound(
    HWND top_window_handle,
    const uint32_t audio_buffer_size,
    unsigned int * success);

void playsquarewave(void);

#endif // TOK_DIRECTSOUND_H

