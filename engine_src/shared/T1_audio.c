#include "T1_audio.h"

#include "T1_std.h"
#include "T1_log.h"

#define T1_AUDIO_ASSERTS_ACTIVE 2
#if T1_AUDIO_ASSERTS_ACTIVE == T1_ACTIVE
#include <assert.h>
#elif T1_AUDIO_ASSERTS_ACTIVE == T1_INACTIVE
#else
#error
#endif 

#if T1_AUDIO_ACTIVE == T1_ACTIVE

T1AudioSettings * T1_audio_s = NULL;

#define PERMASOUND_NAME_MAX 64
typedef struct PermaSound {
    char name[PERMASOUND_NAME_MAX];
    s32 allsamples_head_i;
    s32 allsamples_tail_i;
} PermaSound;

static PermaSound * all_permasounds = NULL;
static s32 all_permasounds_size = 0;

static i16 * all_samples = NULL;
s32 all_samples_size = 0;

void T1_audio_init(
    void * (* arg_malloc_function)(u64 size))
{
    T1_audio_s                 = arg_malloc_function(sizeof(T1AudioSettings));
    T1_audio_s->tone_frequency = 261.6f * 2; // 261.6 ~= Middle C frequency
    T1_audio_s->volume         = 0.08f;
    T1_audio_s->sample_rate    = 44100.0f;
    T1_audio_s->play_cursor    = 0;
    T1_audio_s->callback_runs  = 0;
    // sound_settings->platform_buffer_size_bytes = platform_buffer_size;
    T1_audio_s->global_buffer_size_bytes = 41000 * 45 * 2;
    T1_log_assert(T1_audio_s->global_buffer_size_bytes % 2 == 0);
    T1_audio_s->global_samples_size = T1_audio_s->
        global_buffer_size_bytes / 2;
    
    T1_audio_s->samples_buffer = arg_malloc_function(
        T1_audio_s->global_buffer_size_bytes);
    
    T1_audio_clear_global_buffer();
    
    all_permasounds = (PermaSound *)
        arg_malloc_function(
            sizeof(PermaSound) *
                T1_ALL_PERMASOUNDS_SIZE);
    
    for (
        u32 i = 0;
        i < T1_ALL_PERMASOUNDS_SIZE;
        i++)
    {
        all_permasounds[i].name[0] = '\0';
        all_permasounds[i].allsamples_head_i = -1;
        all_permasounds[i].allsamples_tail_i = -1;
    }
    
    all_samples = arg_malloc_function(
        sizeof(i16) *
            T1_ALL_AUDIOSAMPLES_SIZE);
    #if T1_AUDIO_ASSERTS_ACTIVE == T1_ACTIVE
    assert(all_samples != NULL);
    #elif T1_AUDIO_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    T1_std_memset(
        all_samples,
        0,
        sizeof(i16) *
            T1_ALL_AUDIOSAMPLES_SIZE);
}

//__attribute__((noinline))
void T1_audio_consume_int16_samples(
    i16 * recipient,
    const u32 samples_to_copy)
{
    for (u32 _ = 0; _ < samples_to_copy; _++) {
        u32 next_i = (T1_audio_s->play_cursor) %
            T1_audio_s->global_samples_size;
        
        s32 new_val = (s32)(
            (f32)T1_audio_s->samples_buffer[next_i] *
                T1_audio_s->volume);
        
        T1_audio_s->samples_buffer[next_i] = 0;
        
        new_val = new_val > INT16_MAX ? INT16_MAX : new_val;
        new_val = new_val < INT16_MIN ? INT16_MIN : new_val;
        *recipient++ = (i16)new_val;
        T1_audio_s->play_cursor += 1;
    }
    
    T1_log_assert(T1_audio_s->play_cursor < UINT64_MAX - 1000000);
}

#define DEFAULT_WRITING_OFFSET 12
void T1_audio_add_at_offset(
    i16 * data,
    const u32 data_size,
    const u64 play_cursor_offset,
    const f32 volume_mult)
{
    #if T1_AUDIO_ASSERTS_ACTIVE == T1_ACTIVE
    assert(data_size < (T1_audio_state->global_buffer_size_bytes / 2));
    #elif T1_AUDIO_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    for (u32 i = 0; i < data_size; i++) {
        f32 new_value = data[i];
        new_value = new_value * volume_mult;
        new_value += T1_audio_s->samples_buffer[
            (T1_audio_s->play_cursor + i + play_cursor_offset) %
                    T1_audio_s->global_samples_size];
        new_value = new_value > INT16_MAX ? INT16_MAX : new_value;
        new_value = new_value < INT16_MIN ? INT16_MIN : new_value;
        T1_audio_s->samples_buffer[
            (T1_audio_s->play_cursor + i + play_cursor_offset) %
                T1_audio_s->global_samples_size] = (i16)new_value;
    }
}

void T1_audio_add(
    i16 * data,
    const u32 data_size,
    const f32 volume_mult)
{
    T1_audio_add_at_offset(
        /* i16 * data: */
            data,
        /* const u32 data_size: */
            data_size,
        /* const u64 play_cursor_offset: */
            DEFAULT_WRITING_OFFSET,
        /* const f32 volume_mult: */
            volume_mult);
}

void T1_audio_copy(
    i16 * data,
    const u32 data_size,
    const b8 is_music)
{
    #if T1_AUDIO_ASSERTS_ACTIVE == T1_ACTIVE
    assert(data_size < T1_audio_state->global_buffer_size_bytes);
    #elif T1_AUDIO_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1_audio_copy_at_offset(
        /* i16 * data: */
            data,
        /* const u32 data_size: */
            data_size,
        /* const u64 play_cursor_offset: */
            DEFAULT_WRITING_OFFSET,
        /* const bool8_t is_music: */
            is_music);
}

void T1_audio_copy_at_offset(
    i16 * samples,
    const u32 samples_size,
    const u64 play_cursor_offset,
    const b8 is_music)
{
    #if T1_AUDIO_ASSERTS_ACTIVE == T1_ACTIVE
    assert(samples_size < T1_audio_state->global_buffer_size_bytes);
    #elif T1_AUDIO_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    for (u32 i = 0; i < samples_size; i++) {
        s32 new_value = (s32)(samples[i] * (
            is_music ?
                T1_audio_s->music_volume :
                T1_audio_s->sfx_volume));
        new_value = new_value > INT16_MAX ? INT16_MAX : new_value;
        new_value = new_value < INT16_MIN ? INT16_MIN : new_value;
        T1_audio_s->samples_buffer[
            (i + play_cursor_offset) %
                T1_audio_s->global_samples_size] = (i16)new_value;
    }
}

void T1_audio_add_permasound_to_global_buffer_at_offset(
    const s32 permasound_id,
    const u64 play_cursor_offset,
    const f32 volume_mult)
{
    T1_log_assert(permasound_id >= 0);
    T1_log_assert(all_permasounds[permasound_id].allsamples_tail_i >
        all_permasounds[permasound_id].allsamples_head_i);
    T1_log_assert(
        all_permasounds[permasound_id].allsamples_head_i >= 0);
    T1_log_assert(
        all_permasounds[permasound_id].allsamples_tail_i >= 0);
    
    T1_audio_add_at_offset(
        /* i16 * data: */
            all_samples + all_permasounds[permasound_id].allsamples_head_i,
        /* const u32 data_size: */
            (u32)all_permasounds[permasound_id].allsamples_tail_i -
                (u32)all_permasounds[permasound_id].allsamples_head_i,
        /* const u32 play_cursor_offset: */
            play_cursor_offset,
        /* volume_mult: */
            volume_mult);
}

void T1_audio_add_offset_permasound_to_global_buffer_at_offset(
    const s32 permasound_id,
    const u64 permasound_offset,
    const u64 play_cursor_offset,
    const f32 volume_mult)
{
    T1_log_assert(permasound_id >= 0);
    T1_log_assert(all_permasounds[permasound_id].allsamples_tail_i >
        all_permasounds[permasound_id].allsamples_head_i +
            (s32)permasound_offset);
    T1_log_assert(
        all_permasounds[permasound_id].allsamples_head_i >= 0);
    T1_log_assert(
        all_permasounds[permasound_id].allsamples_tail_i >= 0);
    
    T1_audio_add_at_offset(
        /* i16 * data: */
            all_samples + all_permasounds[permasound_id].
                allsamples_head_i + permasound_offset,
        /* const u32 data_size: */
            (u32)all_permasounds[permasound_id].allsamples_tail_i -
                (u32)all_permasounds[permasound_id].allsamples_head_i -
                    (u32)permasound_offset,
        /* const u32 play_cursor_offset: */
            play_cursor_offset,
        /* const f32 volume_mult: */
            volume_mult);
}

void T1_audio_add_permasound_to_global_buffer(
    const s32 permasound_id,
    const f32 volume_mult)
{
    T1_audio_add_permasound_to_global_buffer_at_offset(
        /* const s32 permasound_id: */
            permasound_id,
        /* const u64 play_cursor_offset: */
            DEFAULT_WRITING_OFFSET,
        /* const f32 volume_mult: */
            volume_mult);
}

void T1_audio_copy_permasound_to_global_buffer_at_offset(
    const s32 permasound_id,
    const u64 play_cursor_offset,
    const b8 is_music)
{
    T1_log_assert(permasound_id >= 0);
    T1_log_assert(all_permasounds[permasound_id].allsamples_tail_i >
        all_permasounds[permasound_id].allsamples_head_i);
    T1_log_assert(
        all_permasounds[permasound_id].allsamples_head_i >= 0);
    T1_log_assert(
        all_permasounds[permasound_id].allsamples_tail_i >= 0);
    
    T1_audio_copy_at_offset(
        /* i16 * data: */
            all_samples + all_permasounds[permasound_id].allsamples_head_i,
        /* const u32 data_size: */
            (u32)all_permasounds[permasound_id].allsamples_tail_i -
                (u32)all_permasounds[permasound_id].allsamples_head_i,
        /* const u32 play_cursor_offset: */
            play_cursor_offset,
        /* const bool32_t is_music: */
            is_music);
}

void T1_audio_copy_offset_permasound_to_global_buffer_at_offset(
    const s32 permasound_id,
    const u64 permasound_offset,
    const u64 play_cursor_offset,
    const u32 samples_to_copy_size,
    const b8 is_music)
{
    T1_log_assert(permasound_id >= 0);
    T1_log_assert(all_permasounds[permasound_id].allsamples_tail_i >
        all_permasounds[permasound_id].allsamples_head_i +
            (s32)permasound_offset);
    T1_log_assert(
        all_permasounds[permasound_id].allsamples_head_i >= 0);
    T1_log_assert(
        all_permasounds[permasound_id].allsamples_tail_i >= 0);
    
    T1_audio_copy_at_offset(
        /* i16 * data: */
            all_samples +
                all_permasounds[permasound_id].allsamples_head_i +
                    permasound_offset,
        /* const u32 data_size: */
            samples_to_copy_size,
        /* const u32 play_cursor_offset: */
            play_cursor_offset,
        /* const bool32_t is_music: */
            is_music);
}

void T1_audio_copy_permasound_to_global_buffer(
    const s32 permasound_id,
    const f32 volume_mult)
{
    T1_audio_add_permasound_to_global_buffer_at_offset(
        /* const s32 permasound_id: */
            permasound_id,
        /* const u64 play_cursor_offset: */
            DEFAULT_WRITING_OFFSET,
        /* const f32 volume_mult: */
            volume_mult);
}

void T1_audio_clear_global_buffer(void)
{
    T1_std_memset(
        T1_audio_s->samples_buffer,
        0,
        T1_audio_s->global_buffer_size_bytes);
}

s32 T1_audio_get_permasound_id_or_register_new(
    const char * for_resource_name)
{
    for (s32 i = 0; i < all_permasounds_size; i++) {
        if (
            T1_std_are_equal_strings(
                all_permasounds[i].name,
                for_resource_name))
        {
            return i;
        }
    }
    
    T1_std_strcpy_cap(
        all_permasounds[all_permasounds_size].name,
        PERMASOUND_NAME_MAX,
        for_resource_name);
    all_permasounds_size += 1;
    return all_permasounds_size - 1;
}

void T1_audio_register_samples_to_permasound(
    const s32 permasound_id,
    i16 * samples,
    const s32 samples_size)
{
    T1_log_assert(samples_size + all_samples_size <= T1_ALL_AUDIOSAMPLES_SIZE);
    T1_log_assert(all_permasounds[permasound_id].allsamples_head_i < 0);
    T1_log_assert(all_permasounds[permasound_id].allsamples_tail_i < 0);
    T1_std_memcpy(
        /* void * dst: */
            all_samples + all_samples_size,
        /* const void * src: :*/
            samples,
        /* u64 n (in bytes, so 2x samples): */
            (u64)samples_size * 2);
    
    all_permasounds[permasound_id].allsamples_head_i = all_samples_size;
    all_samples_size += samples_size;
    all_permasounds[permasound_id].allsamples_tail_i = all_samples_size - 1;
    T1_log_assert(
        (all_permasounds[permasound_id].allsamples_tail_i -
            all_permasounds[permasound_id].allsamples_head_i) + 1 ==
                samples_size);
}
#elif T1_AUDIO_ACTIVE == T1_INACTIVE
// Pass
#else
#error "T1_AUDIO_ACTIVE undefined!"
#endif // T1_AUDIO_ACTIVE
