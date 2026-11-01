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
static inline s32 T1_audio_limit(const f32 x) {
    const f32 knee = 24576.0f;
    const f32 room = 32767.0f - knee;
    union { f32 f; u32 u; } bits, bent;
    bits.f = x;
    const u32 sign = bits.u & 0x80000000u;
    bits.u &= 0x7FFFFFFFu;
    const f32 size = bits.f;
    const s32 is_over = size > knee;
    const f32 over = (size - knee) * (f32)is_over; // 0 at or below the knee
    bent.f = knee + room * over / (over + room);
    const u32 pick_bent = (u32)-is_over; // all 1 bits or all 0
    bits.u = ((bent.u & pick_bent) | (bits.u & ~pick_bent)) | sign;
    return (s32)bits.f;
}

// The play cursor as the audio thread has it now
static u64 T1_audio_cursor_now(void) {
    return *(volatile u64 *)&T1_audio_s->play_cursor;
}

void T1_audio_set_crush(u32 hold_frames, u32 bits) {
    crush_hold = hold_frames < 1 ? 1 : hold_frames;
    crush_bits = bits < 1 || bits > 16 ? 16 : bits;
}

/*
One contiguous run of the ring buffer to the platform: limited, bits
dropped, zeroed behind.
*/
static void T1_audio_consume_run(
    s16 * restrict recipient,
    s32 * restrict samples,
    const u32 size,
    const f32 volume,
    const s32 drop_mask)
{
    const u32 blocks8 = size / 8;
    for (u32 b = 0; b < blocks8; b++) {
        s16 * recipient_b = recipient + b * 8;
        s32 * samples_b = samples + b * 8;
        for (u32 l = 0; l < 8; l++) {
            const s32 limited = T1_audio_limit((f32)samples_b[l] * volume);
            recipient_b[l] = (s16)(limited & drop_mask);
            samples_b[l] = 0;
        }
    }
    for (u32 i = blocks8 * 8; i < size; i++) {
        const s32 limited = T1_audio_limit((f32)samples[i] * volume);
        recipient[i] = (s16)(limited & drop_mask);
        samples[i] = 0;
    }
}

void T1_audio_consume_int16_samples(
    s16 * recipient,
    u32 samples_to_copy)
{
    const u32 hold = crush_hold;
    const s32 drop_mask = ~((1 << (16 - crush_bits)) - 1);
    const f32 volume = T1_audio_state->volume;
    s32 * buffer = T1_audio_s->samples_buffer;
    const u32 buffer_size = T1_audio_s->global_samples_size;
    u64 cursor = T1_audio_s->play_cursor;
    
    u32 done = 0;
    while (done < samples_to_copy) {
        const u32 at = (u32)(cursor % buffer_size);
        const u32 left = samples_to_copy - done;
        const u32 run = buffer_size - at < left ? buffer_size - at : left;
        
        *(volatile u64 *)&T1_audio_s->play_cursor = cursor + run;
        s16 * out = recipient + done;
        T1_audio_consume_run(out, buffer + at, run, volume, drop_mask);
        
        // the samples are 2 channels interleaved: a frame is 2 of them
        if (hold > 1) {
            // a frame held for hold frames: sequential, only when crushed
            for (u32 i = 0; i < run; i++) {
                const u64 p = cursor + i;
                const u32 channel = (u32)(p & 1);
                if ((p / 2) % hold == 0) { crush_held[channel] = out[i]; }
                out[i] = (s16)crush_held[channel];
            }
        } else {
            // every frame its own: the last of each channel is held
            for (u32 i = run > 2 ? run - 2 : 0; i < run; i++) {
                crush_held[(cursor + i) & 1] = out[i];
            }
        }
        cursor += run;
        done += run;
    }
    
    T1_assert(T1_audio_s->play_cursor < UINT64_MAX - 1000000);
}

#define DEFAULT_WRITING_OFFSET 12
static void T1_audio_add_run(
    s32 * restrict samples,
    const s16 * restrict data,
    const u32 size,
    const u64 first,
    const f32 volume_mult)
{
    const s32 cap = 1 << 30; // far from overflowing, whatever is added
    const u32 blocks8 = size / 8;
    for (u32 b = 0; b < blocks8; b++) {
        s32 * samples_b = samples + b * 8;
        const s16 * data_b = data + b * 8;
        const u64 first_b = first + b * 8;
        const u64 now = T1_audio_cursor_now();
        // how many of the block the cursor passed, 0 to 8
        const u64 passed64 = now > first_b ? now - first_b : 0;
        const u32 passed = passed64 < 8 ? (u32)passed64 : 8;
        for (u32 l = 0; l < 8; l++) {
            s32 mixed = samples_b[l] + (s32)((f32)data_b[l] * volume_mult);
            mixed = mixed > cap ? cap : mixed;
            mixed = mixed < -cap ? -cap : mixed;
            const s32 write = -(s32)(l >= passed); // all 1 bits or all 0
            samples_b[l] = (mixed & write) | (samples_b[l] & ~write);
        }
    }
    for (u32 i = blocks8 * 8; i < size; i++) {
        if (first + i < T1_audio_cursor_now()) { continue; }
        s32 mixed = samples[i] + (s32)((f32)data[i] * volume_mult);
        mixed = mixed > cap ? cap : mixed;
        mixed = mixed < -cap ? -cap : mixed;
        samples[i] = mixed;
    }
}

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
    s32 * buffer = T1_audio_s->samples_buffer;
    const u32 buffer_size = T1_audio_s->global_samples_size;
    
    u32 done = 0;
    while (done < data_size) {
        const u64 first = start + done;
        const u32 at = (u32)(first % buffer_size);
        const u32 left = data_size - done;
        const u32 run = buffer_size - at < left ? buffer_size - at : left;
        T1_audio_add_run(buffer + at, data + done, run, first, volume_mult);
        done += run;
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
    
    const f32 volume = is_music ?
        T1_audio_state->music_volume :
        T1_audio_state->sfx_volume;
    s32 * buffer = T1_audio_s->samples_buffer;
    const u32 buffer_size = T1_audio_s->global_samples_size;
    
    u32 done = 0;
    while (done < samples_size) {
        const u32 at = (u32)((play_cursor_offset + done) % buffer_size);
        const u32 left = samples_size - done;
        const u32 run = buffer_size - at < left ? buffer_size - at : left;
        s32 * to = buffer + at;
        const s16 * from = samples + done;
        for (u32 i = 0; i < run; i++) { to[i] = (s32)((f32)from[i] * volume); }
        done += run;
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
    const u64 play_cursor = T1_audio_s->play_cursor;
    const s32 * buffer = T1_audio_s->samples_buffer;
    const u32 buffer_size = T1_audio_s->global_samples_size;
    
    u32 done = 0;
    while (done < samples_size) {
        const u32 at = (u32)((play_cursor + done) % buffer_size);
        const u32 left = samples_size - done;
        const u32 run = buffer_size - at < left ? buffer_size - at : left;
        const s32 * from = buffer + at;
        s16 * to = recip + done;
        for (u32 i = 0; i < run; i++) {
            s32 value = from[i];
            value = value > INT16_MAX ? INT16_MAX : value;
            value = value < INT16_MIN ? INT16_MIN : value;
            to[i] = (s16)value;
        }
        done += run;
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
