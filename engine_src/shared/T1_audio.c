#include "T1_audio.h"

#if T1_AUDIO_ACTIVE == T1_ACTIVE

#include "T1_assert.h"
#include "T1_std.h"
#include "T1_os.h"

/*
THE AUDIO (see AUDIO in T1.h): T1 makes no sound itself, the client's fill
makes every sample, on the platform's audio thread. fill and user are set
once, before the device starts, so the audio thread only ever reads them
*/
static T1AudioFill T1_audio_fill = NULL;
static void * T1_audio_user = NULL;
static u8 T1_audio_started = 0;

void T1_audio_start(T1AudioFill fill, void * user, u8 * good) {
    T1_assert(fill != NULL);
    T1_assert(!T1_audio_started); // (once: see T1.h)
    *good = 0;
    if (T1_audio_started) { return; }
    T1_audio_fill = fill;
    T1_audio_user = user;
    T1_audio_started = 1;
    T1_os_audio_init(good);
    if (*good) { T1_os_audio_start_loop(); }
}

void T1_audio_fill_platform(s16 * out, const u32 frames) {
    if (T1_audio_fill == NULL) {
        T1_std_memset(out, 0, sizeof(s16) * 2 * frames);
        return;
    }
    T1_audio_fill(out, frames, T1_audio_user);
}

#elif T1_AUDIO_ACTIVE == T1_INACTIVE
// Pass
#else
#error "T1_AUDIO_ACTIVE undefined!"
#endif // T1_AUDIO_ACTIVE
