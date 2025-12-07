#ifndef T1_AUDIO_H
#define T1_AUDIO_H

#include "T1_logger.h"

#if T1_AUDIO_ACTIVE == T1_ACTIVE

typedef struct {
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
} T1SoundSettings;

extern T1SoundSettings * T1_audio_state;

void T1_audio_init(
    void * (* arg_malloc_function)(size_t size));

void T1_audio_consume_int16_samples(
    int16_t * recipient,
    const uint32_t samples_to_copy);

void T1_audio_add(
    int16_t * data,
    const uint32_t data_size,
    const float volume_mult);
void T1_audio_add_at_offset(
    int16_t * data,
    const uint32_t data_size,
    const uint64_t play_cursor_offset,
    const float volume_mult);

void T1_audio_copy(
    int16_t * data,
    const uint32_t data_size,
    const bool32_t is_music);
void T1_audio_copy_at_offset(
    int16_t * samples,
    const uint32_t samples_size,
    const uint64_t play_cursor_offset,
    const bool32_t is_music);

void T1_audio_add_permasound_to_global_buffer_at_offset(
    const int32_t permasound_id,
    const uint64_t play_cursor_offset,
    const float volume_mult);
void T1_audio_add_offset_permasound_to_global_buffer_at_offset(
    const int32_t permasound_id,
    const uint64_t permasound_offset,
    const uint64_t play_cursor_offset,
    const float volume_mult);
void T1_audio_add_permasound_to_global_buffer(
    const int32_t permasound_id,
    const float volume_float);

void T1_audio_copy_permasound_to_global_buffer_at_offset(
    const int32_t permasound_id,
    const uint64_t play_cursor_offset,
    const bool32_t is_music);
void T1_audio_copy_offset_permasound_to_global_buffer_at_offset(
    const int32_t permasound_id,
    const uint64_t permasound_offset,
    const uint64_t play_cursor_offset,
    const uint32_t samples_to_copy_size,
    const bool32_t is_music);
void T1_audio_copy_permasound_to_global_buffer(
    const int32_t permasound_id,
    const float volume_mult);

void T1_audio_clear_global_buffer(void);

int32_t T1_audio_get_permasound_id_or_register_new(
    const char * for_resource_name);

void T1_audio_register_samples_to_permasound(
    const int32_t permasound_id,
    int16_t * samples,
    const int32_t samples_size);
#elif T1_AUDIO_ACTIVE == T1_INACTIVE
// Pass
#else
#error "T1_AUDIO_ACTIVE undefined"
#endif // T1_AUDIO_ACTIVE

#endif // T1_AUDIO_H
