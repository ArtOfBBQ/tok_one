#ifndef T1_EASING_H
#define T1_EASING_H

#include <math.h>
#include <stdint.h>

#define T1_EASING_ASSERTS 1
#if T1_EASING_ASSERTS
#include <assert.h>
#endif

typedef enum : uint8_t {
    EASINGTYPE_NONE = 0,
    EASINGTYPE_ALWAYS_1,
    EASINGTYPE_INOUT_SINE,
    EASINGTYPE_OUT_QUADRATIC,
    EASINGTYPE_EASEOUT_ELASTIC_ZERO_TO_ONE,
    EASINGTYPE_SINGLE_BOUNCE_ZERO_TO_ZERO,
    EASINGTYPE_DOUBLE_BOUNCE_ZERO_TO_ZERO,
    EASINGTYPE_QUADRUPLE_BOUNCE_ZERO_TO_ZERO,
    EASINGTYPE_OCTUPLE_BOUNCE_ZERO_TO_ZERO,
    EASINGTYPE_SINGLE_PULSE_ZERO_TO_ZERO,
    EASINGTYPE_OCTUPLE_PULSE_ZERO_TO_ZERO,
    EASINGTYPE_OUTOFBOUNDS
} T1EasingType;

/*
if t.now is 0.9f and t.applied is 0.25f, that means we're currently
at the 90% point of the animation and we already previously applied the
effects of the animation up until the 25% point
*/
typedef struct {
    float now;
    float applied;
} T1TPair;

float T1_easing_t_to_eased_t(
    const float t,
    const T1EasingType easing_type);

#endif // T1_EASINGTYPE_H


