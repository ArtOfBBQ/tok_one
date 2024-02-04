#ifndef APPLE_AUDIO_H
#define APPLE_AUDIO_H

#include <AudioToolbox/AudioToolbox.h>

#include "audio.h"

void start_audio_loop(void);
//
//void audio_callback(
//    void * in_user_data,
//    AudioQueueRef queue,
//    AudioQueueBufferRef buffer);

#endif // APPLE_AUDIO_H
