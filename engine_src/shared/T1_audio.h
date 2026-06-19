#ifndef T1_AUDIO_H
#define T1_AUDIO_H

#if T1_AUDIO_ACTIVE == T1_ACTIVE

#include "T1_stdint.h"

typedef struct {
    i16 * samples_buffer;
    u64 play_cursor;
    u64 callback_runs;
    u32 platform_buffer_size_bytes;
    u32 global_buffer_size_bytes;
    u32 global_samples_size;
    f32 tone_frequency;
    f32 volume;
    f32 sfx_volume;
    f32 music_volume;
    f32 sample_rate;
} T1AudioSettings;

extern T1AudioSettings * T1_audio_s;

void T1_audio_init(
    void * (* arg_malloc_function)(u64 size));

void T1_audio_consume_int16_samples(
    i16 * recipient,
    const u32 samples_to_copy);

void T1_audio_add(
    i16 * data,
    const u32 data_size,
    const f32 volume_mult);

void T1_audio_add_at_offset(
    i16 * data,
    const u32 data_size,
    const u64 play_cursor_offset,
    const f32 volume_mult);

void T1_audio_copy(
    i16 * data,
    const u32 data_size,
    const b8 is_music);

void T1_audio_copy_at_offset(
    i16 * samples,
    const u32 samples_size,
    const u64 play_cursor_offset,
    const b8 is_music);

void
T1_audio_add_permasound_to_global_buffer_at_offset(
    const s32 permasound_id,
    const u64 play_cursor_offset,
    const f32 volume_mult);

void
T1_audio_add_offset_permasound_to_global_buffer_at_offset(
    const s32 permasound_id,
    const u64 permasound_offset,
    const u64 play_cursor_offset,
    const f32 volume_mult);

void
T1_audio_add_permasound_to_global_buffer(
    const s32 permasound_id,
    const f32 volume_f32);

void
T1_audio_copy_permasound_to_global_buffer_at_offset(
    const s32 permasound_id,
    const u64 play_cursor_offset,
    const b8 is_music);

void
T1_audio_copy_offset_permasound_to_global_buffer_at_offset(
    const s32 permasound_id,
    const u64 permasound_offset,
    const u64 play_cursor_offset,
    const u32 samples_to_copy_size,
    const b8 is_music);

void
T1_audio_copy_permasound_to_global_buffer(
    const s32 permasound_id,
    const f32 volume_mult);

void
T1_audio_clear_global_buffer(void);

s32
T1_audio_get_permasound_id_or_register_new(
    const char * for_resource_name);

void
T1_audio_register_samples_to_permasound(
    const s32 permasound_id,
    i16 * samples,
    const s32 samples_size);
#elif T1_AUDIO_ACTIVE == T1_INACTIVE
// Pass
#else
#error "T1_AUDIO_ACTIVE undefined"
#endif // T1_AUDIO_ACTIVE

#endif // T1_AUDIO_H
