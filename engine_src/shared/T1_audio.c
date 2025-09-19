#include "T1_audio.h"

#if T1_AUDIO_ACTIVE == T1_ACTIVE

T1SoundSettings * T1_sound_settings = NULL;

#define PERMASOUND_NAME_MAX 64
typedef struct PermaSound {
    char name[PERMASOUND_NAME_MAX];
    int32_t allsamples_head_i;
    int32_t allsamples_tail_i;
} PermaSound;

static PermaSound * all_permasounds = NULL;
static int32_t all_permasounds_size = 0;

// 60million samples = 120MB  60...000
#define ALL_AUDIOSAMPLES_SIZE 60000000
static int16_t * all_samples = NULL;
int32_t all_samples_size = 0;

void T1_audio_init(
    void * (* arg_malloc_function)(size_t size))
{
    T1_sound_settings                 = arg_malloc_function(sizeof(T1SoundSettings));
    T1_sound_settings->tone_frequency = 261.6f * 2; // 261.6 ~= Middle C frequency
    T1_sound_settings->volume         = 0.08f;
    T1_sound_settings->sample_rate    = 44100.0f;
    T1_sound_settings->play_cursor    = 0;
    T1_sound_settings->callback_runs  = 0;
    // sound_settings->platform_buffer_size_bytes = platform_buffer_size;
    T1_sound_settings->global_buffer_size_bytes = 41000 * 45 * 2;
    log_assert(T1_sound_settings->global_buffer_size_bytes % 2 == 0);
    T1_sound_settings->global_samples_size = T1_sound_settings->
        global_buffer_size_bytes / 2;
    
    T1_sound_settings->samples_buffer = arg_malloc_function(
        T1_sound_settings->global_buffer_size_bytes);
    
    T1_audio_clear_global_buffer();
    
    all_permasounds = (PermaSound *)arg_malloc_function(
        sizeof(PermaSound) * ALL_PERMASOUNDS_SIZE);
    
    for (uint32_t i = 0; i < ALL_PERMASOUNDS_SIZE; i++) {
        all_permasounds[i].name[0]           = '\0';
        all_permasounds[i].allsamples_head_i =   -1;
        all_permasounds[i].allsamples_tail_i =   -1;
    }
    
    all_samples = arg_malloc_function(
        sizeof(int16_t) * ALL_AUDIOSAMPLES_SIZE);
    assert(all_samples != NULL);
    T1_std_memset(all_samples, 0, sizeof(int16_t) * ALL_AUDIOSAMPLES_SIZE);
}

//__attribute__((noinline))
void T1_audio_consume_int16_samples(
    int16_t * recipient,
    const uint32_t samples_to_copy)
{
    for (uint32_t _ = 0; _ < samples_to_copy; _++) {
        uint32_t next_i = (T1_sound_settings->play_cursor) %
            T1_sound_settings->global_samples_size;
        
        int32_t new_val = (int32_t)(
            (float)T1_sound_settings->samples_buffer[next_i] *
                T1_sound_settings->volume);
        
        T1_sound_settings->samples_buffer[next_i] = 0;
        
        new_val = new_val > INT16_MAX ? INT16_MAX : new_val;
        new_val = new_val < INT16_MIN ? INT16_MIN : new_val;
        *recipient++ = (int16_t)new_val;
        T1_sound_settings->play_cursor += 1;
    }
    
    log_assert(T1_sound_settings->play_cursor < UINT64_MAX - 1000000);
}

#define DEFAULT_WRITING_OFFSET 12
void T1_audio_add_at_offset(
    int16_t * data,
    const uint32_t data_size,
    const uint64_t play_cursor_offset,
    const float volume_mult)
{
    assert(data_size < (T1_sound_settings->global_buffer_size_bytes / 2));
    
    for (uint32_t i = 0; i < data_size; i++) {
        float new_value = data[i];
        new_value = new_value * volume_mult;
        new_value += T1_sound_settings->samples_buffer[
            (T1_sound_settings->play_cursor + i + play_cursor_offset) %
                    T1_sound_settings->global_samples_size];
        new_value = new_value > INT16_MAX ? INT16_MAX : new_value;
        new_value = new_value < INT16_MIN ? INT16_MIN : new_value;
        T1_sound_settings->samples_buffer[
            (T1_sound_settings->play_cursor + i + play_cursor_offset) %
                T1_sound_settings->global_samples_size] = (int16_t)new_value;
    }
}

void T1_audio_add(
    int16_t * data,
    const uint32_t data_size,
    const float volume_mult)
{
    T1_audio_add_at_offset(
        /* int16_t * data: */
            data,
        /* const uint32_t data_size: */
            data_size,
        /* const uint64_t play_cursor_offset: */
            DEFAULT_WRITING_OFFSET,
        /* const float volume_mult: */
            volume_mult);
}

void T1_audio_copy(
    int16_t * data,
    const uint32_t data_size,
    const bool32_t is_music)
{
    assert(data_size < T1_sound_settings->global_buffer_size_bytes);
    
    T1_audio_copy_at_offset(
        /* int16_t * data: */
            data,
        /* const uint32_t data_size: */
            data_size,
        /* const uint64_t play_cursor_offset: */
            DEFAULT_WRITING_OFFSET,
        /* const bool32_t is_music: */
            is_music);
}

void T1_audio_copy_at_offset(
    int16_t * samples,
    const uint32_t samples_size,
    const uint64_t play_cursor_offset,
    const bool32_t is_music)
{
    assert(samples_size < T1_sound_settings->global_buffer_size_bytes);
    
    for (uint32_t i = 0; i < samples_size; i++) {
        int32_t new_value = (int32_t)(samples[i] * (
            is_music ?
                T1_sound_settings->music_volume :
                T1_sound_settings->sfx_volume));
        new_value = new_value > INT16_MAX ? INT16_MAX : new_value;
        new_value = new_value < INT16_MIN ? INT16_MIN : new_value;
        T1_sound_settings->samples_buffer[
            (i + play_cursor_offset) %
                T1_sound_settings->global_samples_size] = (int16_t)new_value;
    }
}

void T1_audio_add_permasound_to_global_buffer_at_offset(
    const int32_t permasound_id,
    const uint64_t play_cursor_offset,
    const float volume_mult)
{
    log_assert(permasound_id >= 0);
    log_assert(all_permasounds[permasound_id].allsamples_tail_i >
        all_permasounds[permasound_id].allsamples_head_i);
    log_assert(
        all_permasounds[permasound_id].allsamples_head_i >= 0);
    log_assert(
        all_permasounds[permasound_id].allsamples_tail_i >= 0);
    
    T1_audio_add_at_offset(
        /* int16_t * data: */
            all_samples + all_permasounds[permasound_id].allsamples_head_i,
        /* const uint32_t data_size: */
            (uint32_t)all_permasounds[permasound_id].allsamples_tail_i -
                (uint32_t)all_permasounds[permasound_id].allsamples_head_i,
        /* const uint32_t play_cursor_offset: */
            play_cursor_offset,
        /* volume_mult: */
            volume_mult);
}

void T1_audio_add_offset_permasound_to_global_buffer_at_offset(
    const int32_t permasound_id,
    const uint64_t permasound_offset,
    const uint64_t play_cursor_offset,
    const float volume_mult)
{
    log_assert(permasound_id >= 0);
    log_assert(all_permasounds[permasound_id].allsamples_tail_i >
        all_permasounds[permasound_id].allsamples_head_i +
            (int32_t)permasound_offset);
    log_assert(
        all_permasounds[permasound_id].allsamples_head_i >= 0);
    log_assert(
        all_permasounds[permasound_id].allsamples_tail_i >= 0);
    
    T1_audio_add_at_offset(
        /* int16_t * data: */
            all_samples + all_permasounds[permasound_id].
                allsamples_head_i + permasound_offset,
        /* const uint32_t data_size: */
            (uint32_t)all_permasounds[permasound_id].allsamples_tail_i -
                (uint32_t)all_permasounds[permasound_id].allsamples_head_i -
                    (uint32_t)permasound_offset,
        /* const uint32_t play_cursor_offset: */
            play_cursor_offset,
        /* const float volume_mult: */
            volume_mult);
}

void T1_audio_add_permasound_to_global_buffer(
    const int32_t permasound_id,
    const float volume_mult)
{
    T1_audio_add_permasound_to_global_buffer_at_offset(
        /* const int32_t permasound_id: */
            permasound_id,
        /* const uint64_t play_cursor_offset: */
            DEFAULT_WRITING_OFFSET,
        /* const float volume_mult: */
            volume_mult);
}

void T1_audio_copy_permasound_to_global_buffer_at_offset(
    const int32_t permasound_id,
    const uint64_t play_cursor_offset,
    const bool32_t is_music)
{
    log_assert(permasound_id >= 0);
    log_assert(all_permasounds[permasound_id].allsamples_tail_i >
        all_permasounds[permasound_id].allsamples_head_i);
    log_assert(
        all_permasounds[permasound_id].allsamples_head_i >= 0);
    log_assert(
        all_permasounds[permasound_id].allsamples_tail_i >= 0);
    
    T1_audio_copy_at_offset(
        /* int16_t * data: */
            all_samples + all_permasounds[permasound_id].allsamples_head_i,
        /* const uint32_t data_size: */
            (uint32_t)all_permasounds[permasound_id].allsamples_tail_i -
                (uint32_t)all_permasounds[permasound_id].allsamples_head_i,
        /* const uint32_t play_cursor_offset: */
            play_cursor_offset,
        /* const bool32_t is_music: */
            is_music);
}

void T1_audio_copy_offset_permasound_to_global_buffer_at_offset(
    const int32_t permasound_id,
    const uint64_t permasound_offset,
    const uint64_t play_cursor_offset,
    const uint32_t samples_to_copy_size,
    const bool32_t is_music)
{
    log_assert(permasound_id >= 0);
    log_assert(all_permasounds[permasound_id].allsamples_tail_i >
        all_permasounds[permasound_id].allsamples_head_i +
            (int32_t)permasound_offset);
    log_assert(
        all_permasounds[permasound_id].allsamples_head_i >= 0);
    log_assert(
        all_permasounds[permasound_id].allsamples_tail_i >= 0);
    
    T1_audio_copy_at_offset(
        /* int16_t * data: */
            all_samples +
                all_permasounds[permasound_id].allsamples_head_i +
                    permasound_offset,
        /* const uint32_t data_size: */
            samples_to_copy_size,
        /* const uint32_t play_cursor_offset: */
            play_cursor_offset,
        /* const bool32_t is_music: */
            is_music);
}

void T1_audio_copy_permasound_to_global_buffer(
    const int32_t permasound_id,
    const float volume_mult)
{
    T1_audio_add_permasound_to_global_buffer_at_offset(
        /* const int32_t permasound_id: */
            permasound_id,
        /* const uint64_t play_cursor_offset: */
            DEFAULT_WRITING_OFFSET,
        /* const float volume_mult: */
            volume_mult);
}

void T1_audio_clear_global_buffer(void)
{
    T1_std_memset(
        T1_sound_settings->samples_buffer,
        0,
        T1_sound_settings->global_buffer_size_bytes);
}

int32_t T1_audio_get_permasound_id_or_register_new(
    const char * for_resource_name)
{
    for (int32_t i = 0; i < all_permasounds_size; i++) {
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
    const int32_t permasound_id,
    int16_t * samples,
    const int32_t samples_size)
{
    log_assert(samples_size + all_samples_size <= ALL_AUDIOSAMPLES_SIZE);
    log_assert(all_permasounds[permasound_id].allsamples_head_i < 0);
    log_assert(all_permasounds[permasound_id].allsamples_tail_i < 0);
    T1_std_memcpy(
        /* void * dst: */
            all_samples + all_samples_size,
        /* const void * src: :*/
            samples,
        /* size_t n (in bytes, so 2x samples): */
            (size_t)samples_size * 2);
    
    all_permasounds[permasound_id].allsamples_head_i = all_samples_size;
    all_samples_size += samples_size;
    all_permasounds[permasound_id].allsamples_tail_i = all_samples_size - 1;
    log_assert(
        (all_permasounds[permasound_id].allsamples_tail_i -
            all_permasounds[permasound_id].allsamples_head_i) + 1 ==
                samples_size);
}
#elif T1_AUDIO_ACTIVE == T1_INACTIVE
// Pass
#else
#error "T1_AUDIO_ACTIVE undefined!"
#endif // T1_AUDIO_ACTIVE
