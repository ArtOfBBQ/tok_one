#ifndef T1_AUDIO_H
#define T1_AUDIO_H

#if T1_AUDIO_ACTIVE == T1_ACTIVE

#include "T1_types_public.h"
#include "T1_stdint.h"
#include "T1_macro_settings.h"

typedef struct {
    s16 * samples_buffer;
    u64 play_cursor;
    u64 callback_runs;
    u32 platform_buffer_size_bytes;
    u32 global_buffer_size_bytes;
    u32 global_samples_size;
    f32 tone_frequency;
    f32 sample_rate;
} T1AudioSettings;

extern T1AudioSettings * T1_audio_s;
typedef struct {
    float volume;
    float sfx_volume;
    float music_volume;
} T1AudioSettingsFullyPublic;

extern T1AudioSettingsFullyPublic * T1_audio_state;

void T1_audio_init(
    void * (* arg_malloc_function)(size_t size));

void T1_audio_consume_int16_samples(
    s16 * recipient,
    const u32 samples_to_copy);

void T1_audio_add(
    s16 * data,
    const u32 data_size,
    const f32 volume_mult);

void T1_audio_add_at_offset(
    s16 * data,
    const u32 data_size,
    const u64 play_cursor_offset,
    const f32 volume_mult);

void T1_audio_copy(
    s16 * data,
    const u32 data_size,
    const b8 is_music);

void T1_audio_copy_at_offset(
    s16 * samples,
    const u32 samples_size,
    const u64 play_cursor_offset,
    const b8 is_music);

void T1_audio_add_permasound_to_global_buffer_at_offset(
    s32 permasound_id,
    u64 play_cursor_offset,
    f32 volume_mult);

void T1_audio_add_offset_permasound_to_global_buffer_at_offset(
    const s32 permasound_id,
    const u64 permasound_offset,
    const u64 play_cursor_offset,
    const f32 volume_mult);

void T1_audio_play(
    s32 permasound_id,
    f32 volume_f32,
    u64 pause_us);

void T1_audio_copy_permasound_to_global_buffer_at_offset(
    s32 permasound_id,
    u64 play_cursor_offset,
    b8 is_music);

void T1_audio_stream(
    s32 permasound_id,
    u64 permasound_offset,
    u64 play_cursor_offset,
    u32 samples_to_copy_size,
    b8  is_music);

void T1_audio_copy_permasound_to_global_buffer(
    const s32 permasound_id,
    const f32 volume_mult);

u64 T1_audio_play_cursor(void);

void T1_audio_stop_all(void);

s32 T1_audio_sound_id(
    const char * for_resource_name,
    char ** const fatal_error);

void T1_audio_set_samples(
    s32 permasound_id,
    s16 * samples,
    s32 samples_size);
#elif T1_AUDIO_ACTIVE == T1_INACTIVE
// Pass
#else
#error "T1_AUDIO_ACTIVE undefined"
#endif // T1_AUDIO_ACTIVE

#endif // T1_AUDIO_H
