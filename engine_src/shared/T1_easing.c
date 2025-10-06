#include "T1_easing.h"

static float T1_scheduled_animations_easing_bounce_zero_to_zero(
    const float t,
    const float bounces)
{
    // Ensure t is clamped between 0.0f and 1.0f
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 0.0f;
    
    // Base oscillation using sine for smooth bouncing
    float oscillation = sinf(3.14159265359f * bounces * t); // 4 half-cycles for multiple bounces
    
    // Amplitude envelope to control bounce height and ensure 0 at endpoints
    float envelope = bounces * t * (1.0f - t); // Parabolic shape: peaks at t=0.5, zero at t=0 and t=1
    
    // Combine to get the bouncing effect
    float result = oscillation * envelope;
    
    // Scale to desired amplitude (adjust 0.5f for more/less extreme bounces)
    return result * 0.5f;
}

static float T1_scheduled_animations_easing_pulse_zero_to_zero(
    const float t,
    const float pulses)
{
    // Ensure t is clamped between 0.0f and 1.0f
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 0.0f;
    
    // Base oscillation using absolute sine for non-negative pulsing
    float oscillation = fabsf(sinf(3.14159265359f * pulses * t)); // Non-negative, bounces half-cycles
    
    // Amplitude envelope to ensure 0 at endpoints
    float envelope = pulses * t * (1.0f - t); // Parabolic shape: peaks at t=0.5, zero at t=0 and t=1
    
    // Combine for pulsing effect
    float result = oscillation * envelope;
    
    // Scale to match original amplitude feel (adjust 0.5f for intensity)
    return result * 0.5f;
}

static float T1_scheduled_animations_easing_out_elastic_zero_to_one(const float t) {
    const float c4 = (2.0f * (float)M_PI) / 3.0f;
    
    if (t == 0.0f || t == 1.0f) { return t; }
    
    return
        powf(2, -10.0f * t) *
        sinf((t * 10.0f - 0.75f) * c4) + 1.0f;
}

T1TPair t_to_eased_t(
    const T1TPair t,
    const T1EasingType easing_type)
{
    T1TPair return_val;
    
    switch (easing_type) {
        case EASINGTYPE_NONE:
            return_val = t;
            break;
        case EASINGTYPE_EASEOUT_ELASTIC_ZERO_TO_ONE:
            return_val.now =
                T1_scheduled_animations_easing_out_elastic_zero_to_one(
                    t.now);
            return_val.applied =
                T1_scheduled_animations_easing_out_elastic_zero_to_one(
                    t.applied);
            break;
        case EASINGTYPE_SINGLE_BOUNCE_ZERO_TO_ZERO:
            return_val.now =
                T1_scheduled_animations_easing_bounce_zero_to_zero(
                    t.now, 1.0f);
            return_val.applied =
                T1_scheduled_animations_easing_bounce_zero_to_zero(
                    t.applied, 1.0f);
            break;
        case EASINGTYPE_DOUBLE_BOUNCE_ZERO_TO_ZERO:
            return_val.now =
                T1_scheduled_animations_easing_bounce_zero_to_zero(
                    t.now, 2.0f);
            return_val.applied =
                T1_scheduled_animations_easing_bounce_zero_to_zero(
                    t.applied, 2.0f);
            break;
        case EASINGTYPE_QUADRUPLE_BOUNCE_ZERO_TO_ZERO:
            return_val.now =
                T1_scheduled_animations_easing_bounce_zero_to_zero(
                    t.now, 4.0f);
            return_val.applied =
                T1_scheduled_animations_easing_bounce_zero_to_zero(
                    t.applied, 4.0f);
            break;
        case EASINGTYPE_OCTUPLE_BOUNCE_ZERO_TO_ZERO:
            return_val.now =
                T1_scheduled_animations_easing_bounce_zero_to_zero(
                    t.now, 8.0f);
            return_val.applied =
                T1_scheduled_animations_easing_bounce_zero_to_zero(
                    t.applied, 8.0f);
            break;
        case EASINGTYPE_SINGLE_PULSE_ZERO_TO_ZERO:
            return_val.now =
                T1_scheduled_animations_easing_pulse_zero_to_zero(
                    t.now, 1.0f);
            return_val.applied =
                T1_scheduled_animations_easing_pulse_zero_to_zero(
                    t.applied, 1.0f);
            break;
        case EASINGTYPE_OCTUPLE_PULSE_ZERO_TO_ZERO:
            return_val.now =
                T1_scheduled_animations_easing_pulse_zero_to_zero(
                    t.now, 8.0f);
            return_val.applied =
                T1_scheduled_animations_easing_pulse_zero_to_zero(
                    t.applied, 8.0f);
            break;
        default:
            assert(0);
    }
    
    return return_val;
}
