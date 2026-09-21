// Copyright 2022-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "aec.h"
#include "aec_priv.h"

//AEC level 2
void aec_l2_calc_Error_and_Y_hat(
        bfp_complex_s32_t *Error,
        bfp_complex_s32_t *Y_hat,
        const bfp_complex_s32_t *Y,
        const bfp_complex_s32_t *X_fifo,
        const bfp_complex_s32_t *H_hat,
        unsigned num_x_channels,
        unsigned num_phases,
        unsigned start_offset,
        unsigned length,
        int32_t bypass_enabled)
{
    if(!length) {
        //printf("0 length\n");
        return;
    }
    if(bypass_enabled) { //Copy Y into Error. Set Y_hat to 0
        //Both sides are interior AEC memory pool buffers, so vpu_memcpy()'s tail over-read is in bounds
        vpu_memcpy(Error->data, &Y->data[start_offset], length*sizeof(complex_s32_t));
        Error->exp = Y->exp;
        Error->hr = Y->hr;

        vect_complex_s32_set(Y_hat->data, 0, 0, length);
        Y_hat->exp = AEC_ZEROVAL_EXP;
        Y_hat->hr = AEC_ZEROVAL_HR;
    }
    else {
        uint32_t phases = num_x_channels * num_phases;
        for(unsigned ph=0; ph<phases; ph++) {
            //create input chunks
            bfp_complex_s32_t X_chunk, H_hat_chunk;
            bfp_complex_s32_init(&X_chunk, &X_fifo[ph].data[start_offset], X_fifo[ph].exp, length, 0); //Not recalculating headroom here to make sure outputs are bitexact irrespective of the length this function is called for.
            X_chunk.hr = X_fifo[ph].hr;
            bfp_complex_s32_init(&H_hat_chunk, &H_hat[ph].data[start_offset], H_hat[ph].exp, length, 0);
            H_hat_chunk.hr = H_hat[ph].hr;
            bfp_complex_s32_macc(Y_hat, &X_chunk, &H_hat_chunk);
        }

        bfp_complex_s32_t Y_chunk;
        bfp_complex_s32_init(&Y_chunk, &Y->data[start_offset], Y->exp, length, 0);
        Y_chunk.hr = Y->hr;
        bfp_complex_s32_sub(Error, &Y_chunk, Y_hat);
    }
}

void aec_l2_adapt_plus_fft_gc(
        bfp_complex_s32_t *H_hat_ph,
        const bfp_complex_s32_t *X_fifo_ph,
        const bfp_complex_s32_t *T_ph
        )
{
    bfp_complex_s32_conj_macc(H_hat_ph, T_ph, X_fifo_ph);
    bfp_fft_pack_mono(H_hat_ph);
    bfp_complex_s32_gradient_constraint_mono(H_hat_ph, AEC_FILTER_TAPS_PER_PHASE);
    bfp_fft_unpack_mono(H_hat_ph);
}

void aec_l2_filter_phase_to_spectrum(
        bfp_complex_s32_t *H_ph,
        const bfp_s16_t *h_ph,
        bfp_s32_t *scratch)
{
    complex_s32_t *buf = (complex_s32_t*)scratch->data;

    /* Expand the phase into the transform buffer. The taps are already in the element order
     * fft_dit_forward() wants, so this is a fixed stride widen with zeros in the odd elements, zeros in the slots
     * the compacted storage omits, and no index bit reversal pass.
     *
     * An int16 mantissa with `hr` bits of headroom sits in an int32 with 16 + hr bits of headroom, and
     * fft_dit_forward() wants exactly 2, so the widening shift is (14 + hr). That folds the transform's input
     * normalisation into the same pass. */
    const left_shift_t shl = 14 + (left_shift_t)h_ph->hr;
    exponent_t exp = h_ph->exp - shl;
    headroom_t hr = 2;

    aec_priv_td_expand(buf, h_ph->data, shl);

    fft_dit_forward(buf, AEC_PROC_FRAME_LENGTH/2, &hr, &exp);
    fft_mono_adjust(buf, AEC_PROC_FRAME_LENGTH, 0);

    bfp_complex_s32_init(H_ph, buf, exp, AEC_PROC_FRAME_LENGTH/2, 0);
    bfp_complex_s32_headroom(H_ph);
    bfp_fft_unpack_mono(H_ph);
}

void aec_l2_calc_Error_and_Y_hat_td(
        bfp_complex_s32_t *Error,
        bfp_complex_s32_t *Y_hat,
        const bfp_complex_s32_t *Y,
        const bfp_complex_s32_t *X_fifo,
        const bfp_s16_t *h_hat,
        unsigned num_x_channels,
        unsigned num_phases,
        unsigned start_offset,
        unsigned length,
        int32_t bypass_enabled,
        bfp_s32_t *scratch)
{
    if(!length) {
        return;
    }
    if(bypass_enabled) { //Copy Y into Error. Set Y_hat to 0
        //Both sides are interior AEC memory pool buffers, so vpu_memcpy()'s tail over-read is in bounds
        vpu_memcpy(Error->data, &Y->data[start_offset], length*sizeof(complex_s32_t));
        Error->exp = Y->exp;
        Error->hr = Y->hr;

        vect_complex_s32_set(Y_hat->data, 0, 0, length);
        Y_hat->exp = AEC_ZEROVAL_EXP;
        Y_hat->hr = AEC_ZEROVAL_HR;
    }
    else {
        uint32_t phases = num_x_channels * num_phases;
        for(unsigned ph=0; ph<phases; ph++) {
            /* Recover this phase's spectrum from its time domain taps. The transform is always done over the whole
             * phase, so the chunk taken below is bitexact irrespective of the length this function is called for. */
            bfp_complex_s32_t H_hat_ph;
            aec_l2_filter_phase_to_spectrum(&H_hat_ph, &h_hat[ph], scratch);

            //create input chunks
            bfp_complex_s32_t X_chunk, H_hat_chunk;
            bfp_complex_s32_init(&X_chunk, &X_fifo[ph].data[start_offset], X_fifo[ph].exp, length, 0); //Not recalculating headroom here to make sure outputs are bitexact irrespective of the length this function is called for.
            X_chunk.hr = X_fifo[ph].hr;
            bfp_complex_s32_init(&H_hat_chunk, &H_hat_ph.data[start_offset], H_hat_ph.exp, length, 0);
            H_hat_chunk.hr = H_hat_ph.hr;
            bfp_complex_s32_macc(Y_hat, &X_chunk, &H_hat_chunk);
        }

        bfp_complex_s32_t Y_chunk;
        bfp_complex_s32_init(&Y_chunk, &Y->data[start_offset], Y->exp, length, 0);
        Y_chunk.hr = Y->hr;
        bfp_complex_s32_sub(Error, &Y_chunk, Y_hat);
    }
}

void aec_l2_adapt_td(
        bfp_s16_t *h_hat_ph,
        const bfp_complex_s32_t *X_fifo_ph,
        const bfp_complex_s32_t *T_ph,
        bfp_s32_t *scratch)
{
    /* The frequency domain filter update is dH = T * conj(X), after which the block LMS algorithm applies the gradient
     * constraint GC() = forward_fft(zero the tail(inverse_fft())).
     *
     * GC() is linear, and the stored filter is by construction already constrained (it only has
     * AEC_FILTER_TAPS_PER_PHASE taps), so
     *
     *   GC(H + dH) = GC(H) + GC(dH) = H + GC(dH)
     *
     * which in the time domain is just h += first AEC_FILTER_TAPS_PER_PHASE taps of inverse_fft(dH). The filter itself
     * therefore never has to make the round trip; only the update does. That keeps the transform count per phase per
     * frame the same as the frequency domain filter implementation: one inverse transform here, and one forward
     * transform in aec_l2_calc_Error_and_Y_hat_td(). */
    complex_s32_t *buf = (complex_s32_t*)scratch->data;

    bfp_complex_s32_t dH;
    bfp_complex_s32_init(&dH, buf, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
    bfp_complex_s32_conj_mul(&dH, T_ph, X_fifo_ph);

    /* Inverse transform the update. fft_dif_inverse() is used rather than bfp_fft_inverse_mono() because it leaves
     * the time domain samples in the same element order that fft_dit_forward() consumes, so neither direction pays
     * for an index bit reversal pass. */
    bfp_fft_pack_mono(&dH);
    bfp_complex_s32_use_exponent(&dH, dH.exp - dH.hr + 2);

    exponent_t exp = dH.exp;
    headroom_t hr = dH.hr;
    fft_mono_adjust(buf, AEC_PROC_FRAME_LENGTH, 1);
    fft_dif_inverse(buf, AEC_PROC_FRAME_LENGTH/2, &hr, &exp);

    /* Collect the update's stored slots. This applies the whole gradient constraint: leaving the odd complex
     * elements behind discards the upper half of the impulse response, and skipping the slots that the compacted
     * storage does not keep (see AEC_FILTER_TD_STORED_PAIRS) discards the update to the taps at or beyond
     * AEC_FILTER_TAPS_PER_PHASE. Nothing has to be zeroed afterwards. */
    complex_s32_t *acc = (complex_s32_t*)&scratch->data[AEC_PROC_FRAME_LENGTH + AEC_FFT_PADDING];
    aec_priv_td_gather(acc, buf);

    /* Accumulate at 32bit precision and round to nearest once, rather than aligning the update to the filter's 16bit
     * exponent and rounding twice. The transform buffer is dead now that the update has been collected, so the low
     * part of it holds the widened filter. */
    bfp_s32_t dh;
    bfp_s32_init(&dh, (int32_t*)acc, exp, AEC_FILTER_TD_LENGTH, 1);

    bfp_s32_t h_wide;
    bfp_s32_init(&h_wide, scratch->data, 0, AEC_FILTER_TD_LENGTH, 0);
    bfp_s16_to_bfp_s32(&h_wide, h_hat_ph);

    bfp_s32_add(&dh, &dh, &h_wide);

    //vect_s32_to_vect_s16() rounds to nearest, so this is a single round-to-nearest requantisation of the new filter
    bfp_s32_to_bfp_s16(h_hat_ph, &dh);
}

void aec_l2_bfp_complex_s32_unify_exponent(
        bfp_complex_s32_t *chunks,
        int32_t *final_exp, uint32_t *final_hr,
        const uint32_t *mapping, uint32_t array_len,
        uint32_t desired_index,
        uint32_t min_headroom)
{
    *final_exp = INT_MIN;
    for(int i=0; i<array_len; i++) {
        if(((mapping == NULL) || (mapping[i] == desired_index)) && (chunks[i].length > 0)) {
            if((int32_t)(chunks[i].exp - chunks[i].hr + min_headroom) > *final_exp) {
                *final_exp = chunks[i].exp - chunks[i].hr + min_headroom;
            }
        }
    }
    *final_hr = INT_MAX; //smallest hr
    for(int i=0; i<array_len; i++) {
        if(((mapping == NULL) || (mapping[i] == desired_index)) && (chunks[i].length > 0)) {
           bfp_complex_s32_use_exponent(&chunks[i], *final_exp);
           *final_hr = (chunks[i].hr < *final_hr) ? chunks[i].hr : *final_hr;
        }
    }
}

void aec_l2_bfp_s32_unify_exponent(
        bfp_s32_t *chunks, int32_t *final_exp,
        uint32_t *final_hr,
        const uint32_t *mapping,
        uint32_t array_len,
        uint32_t desired_index,
        uint32_t min_headroom)
{
    *final_exp = INT_MIN; //find biggest exponent (fewest fraction bits)
    for(int i=0; i<array_len; i++) {
        if(((mapping == NULL) || (mapping[i] == desired_index)) && (chunks[i].length > 0)) {
            if((int32_t)(chunks[i].exp - chunks[i].hr + min_headroom) > *final_exp) {
                *final_exp = chunks[i].exp - chunks[i].hr + min_headroom;
            }
        }
    }
    *final_hr = INT_MAX; //smallest hr
    for(int i=0; i<array_len; i++) {
        if(((mapping == NULL) || (mapping[i] == desired_index)) && (chunks[i].length > 0)) {
           bfp_s32_use_exponent(&chunks[i], *final_exp);
           *final_hr = (chunks[i].hr < *final_hr) ? chunks[i].hr : *final_hr;
        }
    }
}
