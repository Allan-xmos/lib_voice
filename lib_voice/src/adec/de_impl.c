// Copyright 2022-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "aec.h"
#include "adec.h"

void adec_estimate_delay (
        de_output_t *de_output,
        const bfp_s16_t* h_hat,
        unsigned num_phases)
{
    //Direct manipulation of mant/exp because f64_to_float_s32(0.0) takes hundreds of cycles
    const float_s32_t zero = {0, 0};
    const float_s32_t one = {1, 0};

    float_s32_t peak_fd_power = zero;
    int32_t peak_power_phase_index = 0;
    de_output->sum_phase_powers = zero;

    /* Both accumulators are seeded from the first phase rather than from `zero`.
     *
     * A float_s32_t of {0, 0} is not a neutral zero: float_s32_add() and float_s32_sub() derive the result exponent
     * from max(x.exp - HR_S32(x.mant), y.exp - HR_S32(y.mant)), and HR_S32(0) is 31, so a {0, 0} operand pins that
     * result exponent at -30. Anything whose exponent is 32 or more below that is right shifted out of existence: it
     * accumulates as nothing, and compares as not greater than zero despite a positive mantissa.
     *
     * A phase energy is small enough to hit that. bfp_s16_energy() gives exp = 2*h_exp, and the energy of
     * AEC_FILTER_TD_LENGTH 16 bit mantissas only reaches about 2^36, so the float_s32_t exponent is roughly
     * 2*h_exp + 6 and crosses -62 once h_exp drops below about -34, which is an ordinary quiet echo path. The
     * frequency domain filter this replaced was never exposed to it, because 257 complex 32 bit bins gave an energy
     * mantissa around 2^35 larger and so an exponent that far above the cliff.
     *
     * Seeding from the first phase keeps every comparison and addition between two phase energies, which share a
     * scale. An all zero phase is safe as a seed because it carries 2*AEC_ZEROVAL_EXP, not 0. */
    for(int ph=0; ph<num_phases; ph++) { //compute delay over 1 x-y pair phases
        /* The AEC filter is stored in the time domain, so a phase's energy is the energy of its impulse response.
         * Only the relative energies of the phases matter here: the peak index gives the delay, and everything else
         * derived from these powers is a ratio. */
        float_s32_t phase_power = float_s64_to_float_s32(bfp_s16_energy(&h_hat[ph]));
        de_output->phase_power[ph] = phase_power;
        if(ph == 0) {
            de_output->sum_phase_powers = phase_power;
            peak_fd_power = phase_power;
            peak_power_phase_index = 0;
        }
        else {
            de_output->sum_phase_powers = float_s32_add(de_output->sum_phase_powers, phase_power);
            if(float_s32_gt(phase_power, peak_fd_power)) {
                peak_fd_power = phase_power;
                peak_power_phase_index = ph;
            }
        }
    }
    de_output->peak_phase_power = peak_fd_power;
    de_output->peak_power_phase_index = peak_power_phase_index;

    /* Tested on the mantissa rather than with float_s32_gt(.., zero) for the same reason: the powers are energies and
     * so never negative, which makes a zero mantissa exactly equivalent to a zero sum, and independent of exponent. */
    if(de_output->sum_phase_powers.mant != 0){
        float_s32_t num_phases_s32 = {num_phases, 0};
        de_output->peak_to_average_ratio =
                float_s32_div(float_s32_mul(peak_fd_power, num_phases_s32), de_output->sum_phase_powers);
    }else{
        de_output->peak_to_average_ratio = one;
    }
    de_output->measured_delay_samples = AEC_FRAME_ADVANCE * peak_power_phase_index;
}
