#ifndef WAV_H
#define WAV_H

#include "common.h"

#include <stdint.h>
#include <stddef.h>

// #define WAV_SILENCE
#ifndef WAV_SILENCE
#include <stdio.h>
#endif

// #define WAV_IGNORE_ASSERTS
#ifndef WAV_IGNORE_ASSERTS
#include <assert.h>
#endif

void samples_to_wav(
    unsigned char * recipient,
    uint32_t * recipient_size,
    const uint32_t recipient_cap,
    int16_t * samples,
    const uint32_t samples_size);

void parse_wav(
    int16_t * recipient,
    uint32_t * recipient_size,
    const uint32_t recipient_cap,
    unsigned char * raw_file,
    const uint32_t data_size,
    uint32_t * good);

#endif // WAV_H

