#include "T1_easing.h"

#include <math.h>

#define T1_EASING_ASSERTS_ACTIVE T1_ACTIVE
#if T1_EASING_ASSERTS_ACTIVE == T1_ACTIVE
#include <assert.h>
#elif T1_EASING_ASSERTS_ACTIVE == T1_INATIVE
#else
#error
#endif

static f32 T1_easing_bounce_zero_to_zero(
    const f32 t,
    const f32 bounces)
{
    // Ensure t is clamped between 0.0f and 1.0f
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 0.0f;
    
    // Base oscillation using sine for smooth bouncing
    f32 oscillation = sinf(3.14159265359f * bounces * t); // 4 half-cycles for multiple bounces
    
    // Amplitude envelope to control bounce height and ensure 0 at endpoints
    f32 envelope = bounces * t * (1.0f - t); // Parabolic shape: peaks at t=0.5, zero at t=0 and t=1
    
    // Combine to get the bouncing effect
    f32 result = oscillation * envelope;
    
    // Scale to desired amplitude (adjust 0.5f for more/less extreme bounces)
    return result * 0.5f;
}

static f32 T1_easing_pulse_zero_to_zero(
    const f32 t,
    const f32 pulses)
{
    // Ensure t is clamped between 0.0f and 1.0f
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 0.0f;
    
    // Base oscillation using absolute sine for non-negative pulsing
    f32 oscillation = fabsf(sinf(3.14159265359f * pulses * t)); // Non-negative, bounces half-cycles
    
    // Amplitude envelope to ensure 0 at endpoints
    f32 envelope = pulses * t * (1.0f - t); // Parabolic shape: peaks at t=0.5, zero at t=0 and t=1
    
    // Combine for pulsing effect
    f32 result = oscillation * envelope;
    
    // Scale to match original amplitude feel (adjust 0.5f for intensity)
    return result * 0.5f;
}

static f32 T1_easing_in_out_sine(const f32 t) {
    return 0.5f - 0.5f * cosf(t * (f32)M_PI);
}

static f32 T1_easing_out_quadratic(const f32 t) {
    return 1.0f - (1.0f - t) * (1.0f - t);
}

static f32 T1_easing_out_elastic_zero_to_one(const f32 t) {
    const f32 c4 = (2.0f * (f32)M_PI) / 3.0f;
    
    if (t == 0.0f || t == 1.0f) { return t; }
    
    return
        powf(2, -10.0f * t) *
        sinf((t * 10.0f - 0.75f) * c4) + 1.0f;
}

f32 T1_easing_t_to_eased_t(
    const f32 t,
    const T1EasingType easing_type)
{
    f32 return_val;
    
    switch (easing_type) {
        case T1_EASINGTYPE_NONE:
            return_val = t;
            break;
        case T1_EASINGTYPE_ALWAYS_1:
            return_val = 1.0f;
            break;
        case T1_EASINGTYPE_INOUT_SINE:
            return_val = T1_easing_in_out_sine(t);
            break;
        case T1_EASINGTYPE_OUT_QUADRATIC:
            return_val = T1_easing_out_quadratic(t);
            break;
        case T1_EASINGTYPE_EASEOUT_ELASTIC_ZERO_TO_ONE:
            return_val =
                T1_easing_out_elastic_zero_to_one(
                    t);
            break;
        case T1_EASINGTYPE_SINGLE_BOUNCE_ZERO_TO_ZERO:
            return_val =
                T1_easing_bounce_zero_to_zero(
                    t, 1.0f);
            break;
        case T1_EASINGTYPE_DOUBLE_BOUNCE_ZERO_TO_ZERO:
            return_val =
                T1_easing_bounce_zero_to_zero(
                    t, 2.0f);
            break;
        case T1_EASINGTYPE_QUADRUPLE_BOUNCE_ZERO_TO_ZERO:
            return_val =
                T1_easing_bounce_zero_to_zero(
                    t, 4.0f);
            break;
        case T1_EASINGTYPE_OCTUPLE_BOUNCE_ZERO_TO_ZERO:
            return_val =
                T1_easing_bounce_zero_to_zero(
                    t, 8.0f);
            break;
        case T1_EASINGTYPE_SINGLE_PULSE_ZERO_TO_ZERO:
            return_val =
                T1_easing_pulse_zero_to_zero(
                    t, 1.0f);
            break;
        case T1_EASINGTYPE_OCTUPLE_PULSE_ZERO_TO_ZERO:
            return_val =
                T1_easing_pulse_zero_to_zero(
                    t, 8.0f);
            break;
        default:
            return_val = t;
            #if T1_EASING_ASSERTS
            assert(0);
            #endif
    }
    
    return return_val;
}
