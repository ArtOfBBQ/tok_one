#ifndef APPLE_AUDIO_H
#define APPLE_AUDIO_H

#include "T1_audio.h"

#if T1_AUDIO_ACTIVE == T1_ACTIVE
void T1_apple_audio_init(u8 * good);

void T1_apple_audio_start_loop(void);
#elif T1_AUDIO_ACTIVE == T1_INACTIVE
#else
#error
#endif

#endif // APPLE_AUDIO_H
