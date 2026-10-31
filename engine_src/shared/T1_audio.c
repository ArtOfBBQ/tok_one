#include "T1_audio.h"

#include "T1_types_public.h"
#include "T1_std.h"
#include "T1_assert.h"
#include "T1_macro_settings.h"

#define T1_AUDIO_ASSERTS_ACTIVE 1
#if T1_AUDIO_ASSERTS_ACTIVE == T1_ACTIVE
#elif T1_AUDIO_ASSERTS_ACTIVE == T1_INACTIVE
#else
#error
#endif

#if T1_AUDIO_ACTIVE == T1_ACTIVE

T1AudioSettings * T1_audio_s = NULL;
T1AudioSettingsFullyPublic * T1_audio_state = NULL;

#define PERMASOUND_NAME_MAX 64
typedef struct PermaSound {
    char name[PERMASOUND_NAME_MAX];
    s32 allsamples_head_i;
    s32 allsamples_tail_i;
} PermaSound;

static PermaSound * all_permasounds = NULL;
static s32 all_permasounds_size = 0;

static s16 * all_samples = NULL;
s32 all_samples_size = 0;

void T1_audio_init(
    void * (* arg_malloc_function)(size_t size))
{
    T1_audio_state               = arg_malloc_function(sizeof(T1AudioSettingsFullyPublic));
    T1_std_memset(T1_audio_state, 0, sizeof(T1AudioSettingsFullyPublic));
    T1_audio_state->volume       = 0.08f;
    T1_audio_state->sfx_volume   = 0.08f;
    T1_audio_state->music_volume = 0.08f;
    
    T1_audio_s                 = arg_malloc_function(sizeof(T1AudioSettings));
    T1_audio_s->tone_frequency = 261.6f * 2; // 261.6 ~= Middle C frequency
    
    T1_audio_s->sample_rate    = 44100.0f;
    T1_audio_s->play_cursor    = 0;
    T1_audio_s->callback_runs  = 0;
    // sound_settings->platform_buffer_size_bytes = platform_buffer_size;
    T1_audio_s->global_samples_size = 41000 * 45;
    T1_audio_s->global_buffer_size_bytes =
        T1_audio_s->global_samples_size * (u32)sizeof(s32);
    
    T1_audio_s->samples_buffer = arg_malloc_function(
        T1_audio_s->global_buffer_size_bytes);
    
    T1_audio_stop_all();
    
    all_permasounds = (PermaSound *)
        arg_malloc_function(
            sizeof(PermaSound) *
                T1_ALL_PERMASOUNDS_CAP);
    
    for (
        u32 i = 0;
        i < T1_ALL_PERMASOUNDS_CAP;
        i++)
    {
        all_permasounds[i].name[0] = '\0';
        all_permasounds[i].allsamples_head_i = -1;
        all_permasounds[i].allsamples_tail_i = -1;
    }
    
    all_samples = arg_malloc_function(
        sizeof(s16) *
            T1_ALL_AUDIOSAMPLES_SIZE);
    #if T1_AUDIO_ASSERTS_ACTIVE == T1_ACTIVE
    T1_assert(all_samples != NULL);
    #elif T1_AUDIO_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    T1_std_memset(
        all_samples,
        0,
        sizeof(s16) *
            T1_ALL_AUDIOSAMPLES_SIZE);
}

//__attribute__((noinline))
/*
The bitcrusher (T1_audio_set_crush()), applied as samples leave for the
platform (the mix is written far ahead, so this is where a change is heard
at once): a frame held for crush_hold frames, crush_bits bits kept
*/
static volatile u32 crush_hold = 1;
static volatile u32 crush_bits = 16;
static s32 crush_held[2];

/*
The mix's sample (times the volume) as 16 bits: as it is up to 3/4 of full
scale, above that bent smoothly toward full scale instead of clipped, so
many loud sounds at once get louder without crackling
*/
static s32 T1_audio_limit(f32 x) {
    const f32 knee = 24576.0f;
    const f32 room = 32767.0f - knee;
    f32 size = x < 0.0f ? -x : x;
    if (size <= knee) { return (s32)x; }
    f32 over = size - knee;
    f32 limited = knee + room * over / (over + room);
    return (s32)(x < 0.0f ? -limited : limited);
}

// The play cursor as the audio thread has it now
static u64 T1_audio_cursor_now(void) {
    return *(volatile u64 *)&T1_audio_s->play_cursor;
}

void T1_audio_set_crush(u32 hold_frames, u32 bits) {
    crush_hold = hold_frames < 1 ? 1 : hold_frames;
    crush_bits = bits < 1 || bits > 16 ? 16 : bits;
}

void T1_audio_consume_int16_samples(
    s16 * recipient,
    u32 samples_to_copy)
{
    const u32 hold = crush_hold;
    const s32 drop_mask = ~((1 << (16 - crush_bits)) - 1);
    for (u32 _ = 0; _ < samples_to_copy; _++) {
        u32 next_i = (T1_audio_s->play_cursor) %
            T1_audio_s->global_samples_size;
        
        s32 new_val = T1_audio_limit(
            (f32)T1_audio_s->samples_buffer[next_i] *
                T1_audio_state->volume);
        
        T1_audio_s->samples_buffer[next_i] = 0;
        
        // the samples are 2 channels interleaved: a frame is 2 of them
        const u32 channel = (u32)(T1_audio_s->play_cursor & 1);
        if ((T1_audio_s->play_cursor / 2) % hold == 0) {
            crush_held[channel] = new_val & drop_mask;
        }
        new_val = crush_held[channel];
        *recipient++ = (s16)new_val;
        T1_audio_s->play_cursor += 1;
    }
    
    T1_assert(T1_audio_s->play_cursor < UINT64_MAX - 1000000);
}

#define DEFAULT_WRITING_OFFSET 12
void T1_audio_add_at_offset(
    s16 * data,
    u32 data_size,
    u64 play_cursor_offset,
    f32 volume_mult)
{
    #if T1_AUDIO_ASSERTS_ACTIVE == T1_ACTIVE
    T1_assert(data_size < T1_audio_s->global_samples_size);
    #elif T1_AUDIO_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    /*
    The audio thread moves the play cursor while this runs, by a platform
    buffer at a time: the sound goes where the cursor was when it started,
    or it would jump ahead in the middle. A sample the cursor passed
    already is dropped, written it would play a whole buffer later
    */
    const u64 start = T1_audio_s->play_cursor + play_cursor_offset;
    const s32 cap = 1 << 30; // far from overflowing, whatever is added
    for (u32 i = 0; i < data_size; i++) {
        if (start + i < T1_audio_cursor_now()) { continue; }
        u32 at = (u32)((start + i) % T1_audio_s->global_samples_size);
        s32 new_value = T1_audio_s->samples_buffer[at] +
            (s32)((f32)data[i] * volume_mult);
        new_value = new_value > cap ? cap : new_value;
        new_value = new_value < -cap ? -cap : new_value;
        T1_audio_s->samples_buffer[at] = new_value;
    }
}

void T1_audio_add(
    s16 * data,
    u32 data_size,
    f32 volume_mult)
{
    T1_audio_add_at_offset(
        /* s16 * data: */
            data,
        /* u32 data_size: */
            data_size,
        /* u64 play_cursor_offset: */
            DEFAULT_WRITING_OFFSET,
        /* f32 volume_mult: */
            volume_mult);
}

void T1_audio_copy(
    s16 * data,
    u32 data_size,
    b8 is_music)
{
    #if T1_AUDIO_ASSERTS_ACTIVE == T1_ACTIVE
    T1_assert(data_size < T1_audio_s->global_samples_size);
    #elif T1_AUDIO_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1_audio_copy_at_offset(
        /* s16 * data: */
            data,
        /* u32 data_size: */
            data_size,
        /* u64 play_cursor_offset: */
            DEFAULT_WRITING_OFFSET,
        /* bool8_t is_music: */
            is_music);
}

void T1_audio_copy_at_offset(
    s16 * samples,
    u32 samples_size,
    u64 play_cursor_offset,
    b8 is_music)
{
    #if T1_AUDIO_ASSERTS_ACTIVE == T1_ACTIVE
    T1_assert(samples_size < T1_audio_s->global_samples_size);
    #elif T1_AUDIO_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    for (u32 i = 0; i < samples_size; i++) {
        T1_audio_s->samples_buffer[
            (i + play_cursor_offset) %
                T1_audio_s->global_samples_size] = (s32)(samples[i] * (
                    is_music ?
                        T1_audio_state->music_volume :
                        T1_audio_state->sfx_volume));
    }
}

void T1_audio_add_permasound_to_global_buffer_at_offset(
    s32 permasound_id,
    u64 play_cursor_offset,
    f32 volume_mult)
{
    T1_assert(permasound_id >= 0);
    T1_assert(all_permasounds[permasound_id].allsamples_tail_i >
        all_permasounds[permasound_id].allsamples_head_i);
    T1_assert(
        all_permasounds[permasound_id].allsamples_head_i >= 0);
    T1_assert(
        all_permasounds[permasound_id].allsamples_tail_i >= 0);
    
    T1_audio_add_at_offset(
        /* s16 * data: */
            all_samples + all_permasounds[permasound_id].allsamples_head_i,
        /* u32 data_size: */
            (u32)all_permasounds[permasound_id].allsamples_tail_i -
                (u32)all_permasounds[permasound_id].allsamples_head_i,
        /* u32 play_cursor_offset: */
            play_cursor_offset,
        /* volume_mult: */
            volume_mult);
}

void T1_audio_add_offset_permasound_to_global_buffer_at_offset(
    s32 permasound_id,
    u64 permasound_offset,
    u64 play_cursor_offset,
    f32 volume_mult)
{
    T1_assert(permasound_id >= 0);
    T1_assert(all_permasounds[permasound_id].allsamples_tail_i >
        all_permasounds[permasound_id].allsamples_head_i +
            (s32)permasound_offset);
    T1_assert(
        all_permasounds[permasound_id].allsamples_head_i >= 0);
    T1_assert(
        all_permasounds[permasound_id].allsamples_tail_i >= 0);
    
    T1_audio_add_at_offset(
        /* s16 * data: */
            all_samples + all_permasounds[permasound_id].
                allsamples_head_i + permasound_offset,
        /* u32 data_size: */
            (u32)all_permasounds[permasound_id].allsamples_tail_i -
                (u32)all_permasounds[permasound_id].allsamples_head_i -
                    (u32)permasound_offset,
        /* u32 play_cursor_offset: */
            play_cursor_offset,
        /* f32 volume_mult: */
            volume_mult);
}

void T1_audio_play(
    s32 permasound_id,
    f32 volume_mult,
    u64 pause_us)
{
    // the samples are 2 channels interleaved: whole frames ahead
    const u64 frames_ahead = (u64)(
        (f64)pause_us * (f64)T1_audio_s->sample_rate / 1000000.0);
    const u64 offset = DEFAULT_WRITING_OFFSET + frames_ahead * 2;
    
    #if T1_AUDIO_ASSERTS_ACTIVE == T1_ACTIVE
    T1_assert(permasound_id >= 0);
    T1_assert(
        offset +
            (u64)(all_permasounds[permasound_id].allsamples_tail_i -
                all_permasounds[permasound_id].allsamples_head_i) <
        T1_audio_s->global_samples_size);
    #elif T1_AUDIO_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1_audio_add_permasound_to_global_buffer_at_offset(
        /* s32 permasound_id: */
            permasound_id,
        /* u64 play_cursor_offset: */
            offset,
        /* f32 volume_mult: */
            volume_mult);
}

void T1_audio_copy_permasound_to_global_buffer_at_offset(
    s32 permasound_id,
    u64 play_cursor_offset,
    b8 is_music)
{
    T1_assert(permasound_id >= 0);
    T1_assert(all_permasounds[permasound_id].allsamples_tail_i >
        all_permasounds[permasound_id].allsamples_head_i);
    T1_assert(
        all_permasounds[permasound_id].allsamples_head_i >= 0);
    T1_assert(
        all_permasounds[permasound_id].allsamples_tail_i >= 0);
    
    T1_audio_copy_at_offset(
        /* s16 * data: */
            all_samples + all_permasounds[permasound_id].allsamples_head_i,
        /* u32 data_size: */
            (u32)all_permasounds[permasound_id].allsamples_tail_i -
                (u32)all_permasounds[permasound_id].allsamples_head_i,
        /* u32 play_cursor_offset: */
            play_cursor_offset,
        /* bool32_t is_music: */
            is_music);
}

void T1_audio_stream(
    s32 permasound_id,
    u64 permasound_offset,
    u64 play_cursor_offset,
    u32 samples_to_copy_size,
    b8  is_music)
{
    T1_assert(permasound_id >= 0);
    T1_assert(all_permasounds[permasound_id].allsamples_tail_i >
        all_permasounds[permasound_id].allsamples_head_i +
            (s32)permasound_offset);
    T1_assert(
        all_permasounds[permasound_id].allsamples_head_i >= 0);
    T1_assert(
        all_permasounds[permasound_id].allsamples_tail_i >= 0);
    
    T1_audio_copy_at_offset(
        /* s16 * data: */
            all_samples +
                all_permasounds[permasound_id].allsamples_head_i +
                    permasound_offset,
        /* u32 data_size: */
            samples_to_copy_size,
        /* u32 play_cursor_offset: */
            play_cursor_offset,
        /* bool32_t is_music: */
            is_music);
}

void T1_audio_copy_permasound_to_global_buffer(
    s32 permasound_id,
    f32 volume_mult)
{
    T1_audio_add_permasound_to_global_buffer_at_offset(
        /* s32 permasound_id: */
            permasound_id,
        /* u64 play_cursor_offset: */
            DEFAULT_WRITING_OFFSET,
        /* f32 volume_mult: */
            volume_mult);
}

u64 T1_audio_play_cursor(void) {
    return T1_audio_s->play_cursor;
}

void T1_audio_peek(s16 * recip, u32 samples_size) {
    T1_assert(samples_size <= T1_audio_s->global_samples_size);

    /*
    The audio thread may be playing (and zeroing) these as we read, which
    at worst gives a mix of old and new samples: fine to draw
    */
    u64 play_cursor = T1_audio_s->play_cursor;
    for (u32 i = 0; i < samples_size; i++) {
        s32 value = T1_audio_s->samples_buffer[
            (play_cursor + i) % T1_audio_s->global_samples_size];
        value = value > INT16_MAX ? INT16_MAX : value;
        value = value < INT16_MIN ? INT16_MIN : value;
        recip[i] = (s16)value;
    }
}

void T1_audio_stop_all(void)
{
    T1_std_memset(
        T1_audio_s->samples_buffer,
        0,
        T1_audio_s->global_buffer_size_bytes);
}

s32 T1_audio_sound_id(
    const char * for_resource_name,
    char ** const fatal_error)
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
    
    if ((all_permasounds_size + 1) >= T1_ALL_PERMASOUNDS_CAP) {
        *fatal_error = "T1_audio_sound_id(): more than T1_ALL_PERMASOUNDS_CAP sounds";
        return -1;
    }
    
    T1_std_strcpy_cap(
        all_permasounds[all_permasounds_size].name,
        PERMASOUND_NAME_MAX,
        for_resource_name);
    all_permasounds_size += 1;
    return all_permasounds_size - 1;
}

void T1_audio_set_samples(
    s32 permasound_id,
    s16 * samples,
    s32 samples_size)
{
    T1_assert(samples_size + all_samples_size <= T1_ALL_AUDIOSAMPLES_SIZE);
    T1_assert(all_permasounds[permasound_id].allsamples_head_i < 0);
    T1_assert(all_permasounds[permasound_id].allsamples_tail_i < 0);
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
    T1_assert(
        (all_permasounds[permasound_id].allsamples_tail_i -
            all_permasounds[permasound_id].allsamples_head_i) + 1 ==
                samples_size);
}
#elif T1_AUDIO_ACTIVE == T1_INACTIVE
// Pass
#else
#error "T1_AUDIO_ACTIVE undefined!"
#endif // T1_AUDIO_ACTIVE
