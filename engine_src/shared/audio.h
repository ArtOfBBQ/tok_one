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
    float sfx_volume;
    float music_volume;
    float sample_rate;
} SoundSettings;

extern SoundSettings * sound_settings;

void init_audio(
    void * (* arg_malloc_function)(size_t size));

void add_audio(
    int16_t * data,
    const uint32_t data_size);
void add_audio_at_offset(
    int16_t * data,
    const uint32_t data_size,
    const uint64_t play_cursor_offset);

void copy_audio(
    int16_t * data,
    const uint32_t data_size,
    const bool32_t is_music);
void copy_audio_at_offset(
    int16_t * samples,
    const uint32_t samples_size,
    const uint64_t play_cursor_offset,
    const bool32_t is_music);

void add_permasound_to_global_buffer_at_offset(
    const int32_t permasound_id,
    const uint64_t play_cursor_offset);
void add_offset_permasound_to_global_buffer_at_offset(
    const int32_t permasound_id,
    const uint64_t permasound_offset,
    const uint64_t play_cursor_offset);
void add_permasound_to_global_buffer(
    const int32_t permasound_id);

void copy_permasound_to_global_buffer_at_offset(
    const int32_t permasound_id,
    const uint64_t play_cursor_offset,
    const bool32_t is_music);
void copy_offset_permasound_to_global_buffer_at_offset(
    const int32_t permasound_id,
    const uint64_t permasound_offset,
    const uint64_t play_cursor_offset,
    const uint32_t samples_to_copy_size,
    const bool32_t is_music);
void copy_permasound_to_global_buffer(
    const int32_t permasound_id);

void clear_global_soundbuffer(void);

int32_t get_permasound_id_or_register_new(
    const char * for_resource_name);

void register_samples_to_permasound(
    const int32_t permasound_id,
    int16_t * samples,
    const int32_t samples_size);

#endif // AUDIO_H
