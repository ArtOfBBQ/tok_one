#ifndef AUDIO_H
#define AUDIO_H

#include <string.h>

#include "logger.h"

typedef struct SoundSettings {
    int16_t * samples_buffer;
    uint64_t play_cursor;
    uint64_t callback_runs;
    uint32_t platform_buffer_size_bytes;
    uint32_t global_buffer_size_bytes;
    uint32_t global_samples_size;
    float tone_frequency;
    float volume;
    float sample_rate;
} SoundSettings;

extern SoundSettings * sound_settings;

void init_audio(
    void * (* arg_malloc_function)(size_t size));

void add_audio(
    int16_t * data,
    const uint32_t data_size);

#endif // AUDIO_H
