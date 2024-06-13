#ifndef TOK_DIRECTSOUND_H
#define TOK_DIRECTSOUND_H

#include <windows.h>
#include <dsound.h>

void init_directsound(
    HWND top_window_handle,
    const uint32_t audio_buffer_size,
    unsigned int * success);

void play_sound_bytes(
    const int16_t * sound,
    const uint32_t sound_size);

void consume_some_global_soundbuffer_bytes(void);

void start_audio_loop(void);

#endif // TOK_DIRECTSOUND_H

