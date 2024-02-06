#include "audio.h"

static void * (* malloc_function)(size_t size) = NULL;

SoundSettings * sound_settings = NULL;

#define PERMASOUND_NAME_MAX 64
typedef struct PermaSound {
    char name[PERMASOUND_NAME_MAX];
    int32_t allsamples_head_i;
    int32_t allsamples_tail_i;
} PermaSound;

static PermaSound * all_permasounds = NULL;
static int32_t all_permasounds_size = 0;

#define ALL_AUDIOSAMPLES_SIZE 10000000
static int16_t * all_samples = NULL;
int32_t all_samples_size = 0;

void init_audio(
    void * (* arg_malloc_function)(size_t size))
{
    malloc_function = arg_malloc_function;
    
    sound_settings = arg_malloc_function(sizeof(SoundSettings));
    sound_settings->tone_frequency = 261.6f * 2; // 261.6 ~= Middle C frequency
    sound_settings->volume = 1.0f;
    sound_settings->sample_rate = 44100.0f;
    sound_settings->play_cursor = 0;
    sound_settings->callback_runs = 0;
    // sound_settings->platform_buffer_size_bytes = platform_buffer_size;
    sound_settings->global_buffer_size_bytes = 41000 * 180 * 2;
    log_assert(sound_settings->global_buffer_size_bytes % 2 == 0);
    sound_settings->global_samples_size =
        sound_settings->global_buffer_size_bytes / 2;
    
    sound_settings->samples_buffer = arg_malloc_function(
        sound_settings->global_buffer_size_bytes);
    
    memset(
        sound_settings->samples_buffer,
        0,
        sound_settings->global_buffer_size_bytes);
    
    all_permasounds = (PermaSound *)arg_malloc_function(
        sizeof(PermaSound) * ALL_PERMASOUNDS_SIZE);
    
    for (uint32_t i = 0; i < ALL_PERMASOUNDS_SIZE; i++) {
        all_permasounds[i].name[0] = '\0';
        all_permasounds[i].allsamples_head_i = -1;
        all_permasounds[i].allsamples_tail_i = -1;
    }
    
    all_samples = arg_malloc_function(
        sizeof(int16_t) * ALL_AUDIOSAMPLES_SIZE);
}

void add_audio(
    int16_t * data,
    const uint32_t data_size)
{
    assert(data_size < sound_settings->global_buffer_size_bytes);
    
    for (uint32_t i = 0; i < data_size; i++) {
        sound_settings->samples_buffer[
            (sound_settings->play_cursor + i + 500) %
                sound_settings->global_samples_size] += data[i];
    }
}

void add_permasound_to_global_buffer(
    const int32_t permasound_id)
{
    log_assert(permasound_id >= 0);
    log_assert(all_permasounds[permasound_id].allsamples_tail_i >
        all_permasounds[permasound_id].allsamples_head_i);
    log_assert(
        all_permasounds[permasound_id].allsamples_head_i >= 0);
    log_assert(
        all_permasounds[permasound_id].allsamples_tail_i >= 0);
    
    add_audio(
        /* int16_t * data: */
            all_samples + all_permasounds[permasound_id].allsamples_head_i,
        /* const uint32_t data_size: */
            (uint32_t)all_permasounds[permasound_id].allsamples_tail_i -
                (uint32_t)all_permasounds[permasound_id].allsamples_head_i);
}

int32_t get_permasound_id_or_register_new(
    const char * for_resource_name)
{
    for (int32_t i = 0; i < all_permasounds_size; i++) {
        if (are_equal_strings(all_permasounds[i].name, for_resource_name)) {
            return i;
        }
    }
    
    strcpy_capped(
        all_permasounds[all_permasounds_size].name,
        PERMASOUND_NAME_MAX,
        for_resource_name);
    all_permasounds_size += 1;
    return all_permasounds_size - 1;
}

void register_samples_to_permasound(
    const int32_t permasound_id,
    int16_t * samples,
    const int32_t samples_size)
{
    log_assert(samples_size + all_samples_size <= ALL_AUDIOSAMPLES_SIZE);
    log_assert(all_permasounds[permasound_id].allsamples_head_i < 0);
    log_assert(all_permasounds[permasound_id].allsamples_tail_i < 0);
    memcpy(
        /* void * dst: */
            all_samples + all_samples_size,
        /* const void * src: :*/
            samples,
        /* size_t n: */
            samples_size);
    
    all_permasounds[permasound_id].allsamples_head_i = all_samples_size;
    all_samples_size += samples_size;
    all_permasounds[permasound_id].allsamples_tail_i = all_samples_size - 1;
}
