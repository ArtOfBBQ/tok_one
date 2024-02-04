#include "audio.h"

static void * (* malloc_function)(size_t size) = NULL;

SoundSettings * sound_settings = NULL;

void init_audio(
    void * (* arg_malloc_function)(size_t size))
{
    malloc_function = arg_malloc_function;
    
    sound_settings = arg_malloc_function(sizeof(SoundSettings));
    sound_settings->tone_frequency = 261.6f * 2; // 261.6 ~= Middle C frequency
    sound_settings->volume = 2.0f;
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
    
    int16_t sign = 1;
    for (
        uint32_t i = 0;
        i < sound_settings->global_buffer_size_bytes / 2;
        i++)
    {
        sound_settings->samples_buffer[i] = sign * 250;
        if (i % 200 == 0) {
            sign *= -1;
        }
    }
}

void add_audio(
    int16_t * data,
    const uint32_t data_size)
{
    assert(sound_settings->play_cursor == 0);
    assert(data_size < sound_settings->global_buffer_size_bytes);
    for (uint32_t i = 0; i < data_size; i++) {
        sound_settings->samples_buffer[
            (sound_settings->play_cursor + i) %
                sound_settings->global_samples_size] +=
                    data[i];
    }
}
