/**
 * @file filter.c
 * @brief 4th-order Butterworth low-pass filter implementation
 *
 * Implements: SWR-010
 *
 * Design parameters:
 *   - Filter type: Butterworth (maximally flat passband)
 *   - Order: 4 (implemented as two cascaded 2nd-order sections)
 *   - Cutoff frequency: 0.1 Hz
 *   - Sample rate: 10 Hz
 *   - Implementation: Direct Form II Transposed
 *
 * Coefficients computed for fs=10Hz, fc=0.1Hz using bilinear transform.
 * The 4th-order filter provides 80 dB/decade rolloff above cutoff,
 * effectively removing sensor noise while preserving the glucose signal
 * which changes on the timescale of minutes.
 */

#include "filter.h"
#include <string.h>

/* Number of cascaded second-order sections */
#define NUM_SECTIONS 2

/**
 * Second-order section (biquad) coefficients.
 * Transfer function: H(z) = (b0 + b1*z^-1 + b2*z^-2) / (1 + a1*z^-1 + a2*z^-2)
 */
typedef struct {
    float b0, b1, b2;
    float a1, a2;
} biquad_coeffs_t;

/**
 * Biquad filter state (Direct Form II Transposed).
 * Uses two delay elements per section.
 */
typedef struct {
    float w1, w2;
} biquad_state_t;

/*
 * Pre-computed coefficients for 4th-order Butterworth, fc=0.1Hz, fs=10Hz.
 * Designed via bilinear z-transform with frequency pre-warping.
 */
static const biquad_coeffs_t s_coeffs[NUM_SECTIONS] = {
    /* Section 1 */
    {
        .b0 =  9.446918e-07f,
        .b1 =  1.889384e-06f,
        .b2 =  9.446918e-07f,
        .a1 = -1.996860f,
        .a2 =  0.996864f
    },
    /* Section 2 */
    {
        .b0 =  1.0f,
        .b1 =  2.0f,
        .b2 =  1.0f,
        .a1 = -1.998432f,
        .a2 =  0.998436f
    }
};

static biquad_state_t s_state[NUM_SECTIONS];

void filter_init(void)
{
    memset(s_state, 0, sizeof(s_state));
}

/**
 * @brief Process one sample through the cascaded biquad filter.
 *
 * Each section uses Direct Form II Transposed:
 *   y[n] = b0 * x[n] + w1[n-1]
 *   w1[n] = b1 * x[n] - a1 * y[n] + w2[n-1]
 *   w2[n] = b2 * x[n] - a2 * y[n]
 *
 * This form has better numerical properties for fixed-point and
 * low-precision floating-point implementations.
 */
float filter_apply(float input)
{
    float x = input;

    for (int s = 0; s < NUM_SECTIONS; s++) {
        const biquad_coeffs_t *c = &s_coeffs[s];
        biquad_state_t *st = &s_state[s];

        float y = c->b0 * x + st->w1;
        st->w1 = c->b1 * x - c->a1 * y + st->w2;
        st->w2 = c->b2 * x - c->a2 * y;

        x = y; /* Output of this section feeds the next */
    }

    return x;
}

void filter_reset(void)
{
    memset(s_state, 0, sizeof(s_state));
}
