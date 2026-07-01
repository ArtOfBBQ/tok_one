#ifndef T1_WAV_H
#define T1_WAV_H

#include "T1_stdint.h"
#include <stddef.h>

#ifndef T1_WAV_SILENCE
#include <stdio.h>
#endif

#ifndef T1_WAV_IGNORE_ASSERTS
#include <assert.h>
#endif

void
T1_wav_samples_to_wav(
    unsigned char * recipient,
    u32 * recipient_size,
    const u32 recipient_cap,
    s16 * samples,
    const u32 samples_size);

void
T1_wav_parse(
    s16 * recipient,
    u32 * recipient_size,
    u32 recip_cap,
    unsigned char * raw_file,
    u32 data_size,
    b8 * good);

#endif // T1_WAV_H
