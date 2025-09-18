#ifndef T1_WAV_H
#define T1_WAV_H

#include "T1_std.h"

#include <stdint.h>
#include <stddef.h>

// #define WAV_SILENCE
#ifndef T1_WAV_SILENCE
#include <stdio.h>
#endif

// #define WAV_IGNORE_ASSERTS
#ifndef T1_WAV_IGNORE_ASSERTS
#include <assert.h>
#endif

void T1_wav_samples_to_wav(
    unsigned char * recipient,
    uint32_t * recipient_size,
    const uint32_t recipient_cap,
    int16_t * samples,
    const uint32_t samples_size);

void T1_wav_parse(
    int16_t * recipient,
    uint32_t * recipient_size,
    const uint32_t recipient_cap,
    unsigned char * raw_file,
    const uint32_t data_size,
    uint32_t * good);

#endif // T1_WAV_H
