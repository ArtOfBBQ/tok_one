#ifndef T1_AUDIO_H
#define T1_AUDIO_H

#include "T1_stdint.h"
#include "T1_macro_settings.h"

#if T1_AUDIO_ACTIVE == T1_ACTIVE

/*
How far ahead of what's heard the platform holds what fill made, the same
on every platform: T1_AUDIO_PLATFORM_CHUNKS chunks of T1_AUDIO_CHUNK_FRAMES
frames (a frame is a left and a right sample), 1500 frames, about 34 ms.
macOS's audio queue gets that many buffers of that size, Windows' WASAPI
is kept topped up to that many frames
*/
#define T1_AUDIO_CHUNK_FRAMES 750
#define T1_AUDIO_PLATFORM_CHUNKS 2
#define T1_AUDIO_PLATFORM_FRAMES \
    (T1_AUDIO_CHUNK_FRAMES * T1_AUDIO_PLATFORM_CHUNKS)

typedef void (* T1AudioFill)(int16_t * out, uint32_t frames, void * user);

// (see T1.h)
void T1_audio_start(T1AudioFill fill, void * user, u8 * good);

/*
The platform's audio thread, for each buffer it hands the device: the
client's fill makes frames frames into out (silence before
T1_audio_start())
*/
void T1_audio_fill_platform(s16 * out, u32 frames);

#elif T1_AUDIO_ACTIVE == T1_INACTIVE
// Pass
#else
#error "T1_AUDIO_ACTIVE undefined"
#endif // T1_AUDIO_ACTIVE

#endif // T1_AUDIO_H
