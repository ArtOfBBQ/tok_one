#ifndef AUDIO_H
#define AUDIO_H

#include <string.h>

#include "clientlogic_macro_settings.h"
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

void add_permasound_to_global_buffer(
    const int32_t permasound_id);

int32_t get_permasound_id_or_register_new(
    const char * for_resource_name);

void register_samples_to_permasound(
    const int32_t permasound_id,
    int16_t * samples,
    const int32_t samples_size);

#endif // AUDIO_H
