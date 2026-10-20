// Copyright 2022-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "aec.h"
#include "aec_priv.h"

//AEC level 2
//The adaptive filter is stored in the time domain to save memory. h_hat is transformed to the frequency domain
//on the fly, one phase at a time, during the Error and Y_hat calculation.
//
//The taps are held in bit-reversed index order so that neither of those per-phase transforms has to run an index
//bit-reversal pass.

//The mono FFT packs the real time domain signal into complex pairs to reduce the FFT size. The gather and scatter
//below move a whole complex element at a time. That needs the pair to be aligned to its own width, which holds for
//every buffer they are used on: aec_state_t declares both AEC memory pools DWORD_ALIGNED and every allocation ahead
//of h_hat in them is a whole number of double words, and the FFT scratch buffers here are declared DWORD_ALIGNED.
//
//h_hat stores 16 bit taps while the transforms work at 32 bit, so each direction changes the width as it moves. The
//scatter widens - a pair of taps is one word in h_hat and a double word in the transform buffer - putting each tap
//in the top half of its slot, which is the widening that costs no instructions. The gather narrows the other way: an
//arithmetic right shift, then rounding and saturation to the top half. The shift is an argument so that the gather
//can narrow the h_hat update straight to the exponent it is added at. It is the shift ahead of the narrowing, so it
//is 16 less than the one vect_s32_to_vect_s16() takes for the same result.
//
//On XS3 and VX4 these are hand written assembly - aec_h_hat_bitrev_scatter.S and aec_h_hat_bitrev_gather.S - because
//the load/store offsets a group needs run past the 0..11 immediate range both encodings allow and the compiler
//answers that by recomputing addresses, at about five instructions per element instead of two. The C below is the
//reference for what the assembly does, and is what other builds use.
#if defined(__XS3A__) || defined(__VX4B__)
//The assembly hard codes the layout, so fail the build rather than mis-index if the frame sizes ever change it.
_Static_assert(AEC_H_HAT_BITREV_DROPPED == 8 && AEC_H_HAT_BITREV_GROUP == 16,
        "aec_h_hat_bitrev_scatter.S and aec_h_hat_bitrev_gather.S are written for the 8 group, 16 slot h_hat layout");
#else
//Copy the taps h_hat stores out of a full bit-reversed index time domain vector, dropping the slots the gradient
//constraint zeroes and narrowing each tap to 16 bit. `src` may be the buffer `dst` points into; the copy only ever
//moves data towards the front.
//The taps in `src` are treated as complex numbers, with the real and imaginary parts stored in adjacent words.
void aec_h_hat_bitrev_gather(
        int16_t *dst,
        const int32_t *src,
        const right_shift_t shr)
{
    for(unsigned g=0; g<AEC_H_HAT_BITREV_DROPPED; g++) {
        for(unsigned i=0; i<AEC_H_HAT_BITREV_GROUP-1; i++) {
            //the odd slot beside each one holds taps AEC_PROC_FRAME_LENGTH/2 onwards
            vect_s32_to_vect_s16(&dst[2*i], &src[4*i], 2, shr + 16);
        }
        dst += 2*(AEC_H_HAT_BITREV_GROUP-1); //past the dropped even slot, which holds the taps between
        src += 4*AEC_H_HAT_BITREV_GROUP;     //AEC_FRAME_ADVANCE and AEC_PROC_FRAME_LENGTH/2, and its odd partner
    }
}

/**
 * Expand the taps h_hat stores into a full bit-reversed index time domain vector ready to be transformed in place,
 * widening each 16 bit tap into the top half of its 32 bit slot. That is a scaling by 2^16, which the caller accounts
 * for in the exponent it gives the transformed phase; it leaves the taps' headroom unchanged.
 * Every slot h_hat has no storage for is a tap the gradient constraint zeroes, so the whole vector is cleared first.
 */
void aec_h_hat_bitrev_scatter(
        int32_t *dst,
        const int16_t *src)
{
    vect_s32_set(dst, 0, AEC_PROC_FRAME_LENGTH);

    for(unsigned g=0; g<AEC_H_HAT_BITREV_DROPPED; g++) {
        for(unsigned i=0; i<AEC_H_HAT_BITREV_GROUP-1; i++) {
            dst[4*i]   = ((int32_t)src[2*i]) * (1 << 16);   //a stored pair of taps, the real and imaginary parts of one
            dst[4*i+1] = ((int32_t)src[2*i+1]) * (1 << 16); //complex element; the odd slot beside it holds taps
                                                            //AEC_PROC_FRAME_LENGTH/2 onwards, and stays zero
        }
        dst += 4*AEC_H_HAT_BITREV_GROUP;     //past the dropped even slot, which holds the taps between
        src += 2*(AEC_H_HAT_BITREV_GROUP-1); //AEC_FRAME_ADVANCE and AEC_PROC_FRAME_LENGTH/2, and its odd partner
    }
}
#endif

unsigned aec_h_hat_tap_index(unsigned n)
{
    //Tap n is the (n&1)'th half of complex time domain element n/2, which the transform expects to find in the slot
    //whose index bit-reverses to n/2. Dropping every AEC_H_HAT_BITREV_GROUP'th slot from the stored phase shifts
    //that slot down by the number of dropped slots below it.
    const unsigned slot = n_bitrev(n >> 1, u32_ceil_log2(AEC_H_HAT_BITREV_SLOTS));
    return 2*(slot - (slot / AEC_H_HAT_BITREV_GROUP)) + (n & 1);
}

/**
 * Transform one bit-reversed time domain filter phase into its AEC_FD_FRAME_LENGTH spectrum, using `scratch` as the
 * in-place transform buffer. This mirrors bfp_fft_forward_mono() with the fft_index_bit_reversal() call dropped, since
 * h_hat is already stored in the index order the decimation-in-time forward transform wants.
 */
static void h_hat_forward_fft(
        bfp_complex_s32_t *H_hat_ph,
        const bfp_s16_t *h_hat_ph,
        int32_t *scratch)
{
    //fft_dit_forward() requires 2 bits of headroom, do this on the compressed taps
    headroom_t hr = vect_s16_headroom(h_hat_ph->data, AEC_FRAME_ADVANCE);
    right_shift_t shr = 2 - (right_shift_t)hr;

    //Expand from compressed 16b to bit-reversed 32b taps
    aec_h_hat_bitrev_scatter(scratch, h_hat_ph->data);
    vect_s32_shl(scratch, scratch, AEC_PROC_FRAME_LENGTH, -shr);

    //Scatter shifts by 2^16 when going to 32b
    bfp_complex_s32_init(H_hat_ph, (complex_s32_t*)scratch, h_hat_ph->exp - 16 + shr, AEC_PROC_FRAME_LENGTH/2, 0);
    H_hat_ph->hr = hr + shr;

    // The coeffs are already bit reversed, so use DIT FFT
    fft_dit_forward(H_hat_ph->data, AEC_PROC_FRAME_LENGTH/2, &H_hat_ph->hr, &H_hat_ph->exp);
    fft_mono_adjust(H_hat_ph->data, AEC_PROC_FRAME_LENGTH, 0);
    bfp_complex_s32_headroom(H_hat_ph);
    bfp_fft_unpack_mono(H_hat_ph);
}

/**
 * Transform each filter phase in turn and accumulate X * H_hat into Y_hat.
 */
static void aec_l2_accumulate_Y_hat(
        bfp_complex_s32_t *Y_hat,
        const bfp_complex_s32_t *X_fifo,
        const bfp_s16_t *h_hat,
        unsigned phases)
{
    //Scratch to FFT the current filter phase from time domain to frequency domain
    int32_t DWORD_ALIGNED h_fft_scratch[AEC_PROC_FRAME_LENGTH + AEC_FFT_PADDING];
    for(unsigned ph=0; ph<phases; ph++) {
        bfp_complex_s32_t H_hat_ph;
        h_hat_forward_fft(&H_hat_ph, &h_hat[ph], h_fft_scratch);

        bfp_complex_s32_t X_chunk, H_hat_chunk;
        bfp_complex_s32_init(&X_chunk, X_fifo[ph].data, X_fifo[ph].exp, AEC_FD_FRAME_LENGTH, 0);
        X_chunk.hr = X_fifo[ph].hr;
        bfp_complex_s32_init(&H_hat_chunk, H_hat_ph.data, H_hat_ph.exp, AEC_FD_FRAME_LENGTH, 0);
        H_hat_chunk.hr = H_hat_ph.hr;
        bfp_complex_s32_macc(Y_hat, &X_chunk, &H_hat_chunk);
    }
}

void aec_l2_calc_Error_and_Y_hat(
        bfp_complex_s32_t *Error,
        bfp_complex_s32_t *Y_hat,
        const bfp_complex_s32_t *Y,
        const bfp_complex_s32_t *X_fifo,
        const bfp_s16_t *h_hat,
        unsigned num_x_channels,
        unsigned num_phases,
        int32_t bypass_enabled)
{
    if(bypass_enabled) { //Copy Y into Error. Set Y_hat to 0
        vpu_memcpy(Error->data, Y->data, AEC_FD_FRAME_LENGTH*sizeof(complex_s32_t));
        Error->exp = Y->exp;
        Error->hr = Y->hr;

        vect_complex_s32_set(Y_hat->data, 0, 0, AEC_FD_FRAME_LENGTH);
        Y_hat->exp = AEC_ZEROVAL_EXP;
        Y_hat->hr = AEC_ZEROVAL_HR;
    }
    else {
        aec_l2_accumulate_Y_hat(Y_hat, X_fifo, h_hat, num_x_channels * num_phases);

        bfp_complex_s32_t Y_chunk;
        bfp_complex_s32_init(&Y_chunk, Y->data, Y->exp, AEC_FD_FRAME_LENGTH, 0);
        Y_chunk.hr = Y->hr;
        bfp_complex_s32_sub(Error, &Y_chunk, Y_hat);
    }
}

/**
 * delta_h = ifft(prod), computed in place over the delta_scratch buffer. This mirrors
 * bfp_fft_inverse_mono(), but skips the fft_index_bit_reversal() and stores the bit-reversed
 * coefficients.
 */
static inline void h_hat_inverse_fft(bfp_complex_s32_t *H_hat)
{
    bfp_fft_pack_mono(H_hat);
    //fft_dif_inverse() requires 2 bits of headroom
    bfp_complex_s32_use_exponent(H_hat, H_hat->exp - H_hat->hr + 2);
    fft_mono_adjust(H_hat->data, AEC_PROC_FRAME_LENGTH, 1);
    fft_dif_inverse(H_hat->data, AEC_PROC_FRAME_LENGTH/2, &H_hat->hr, &H_hat->exp);
}

/**
 * Time domain filter adaption. The gradient constraint is applied simply by keeping only the taps
 * of the inverse FFT of T*conj(X) that h_hat has storage for (the rest would wrap in the circular
 * convolution).
 */
void aec_l2_adapt_plus_ifft(
        bfp_s16_t *h_hat_ph,
        const bfp_complex_s32_t *X_fifo_ph,
        const bfp_complex_s32_t *T_ph
        )
{
    complex_s32_t DWORD_ALIGNED delta_scratch[AEC_FD_FRAME_LENGTH];
    bfp_complex_s32_t prod;
    bfp_complex_s32_init(&prod, delta_scratch, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
    // prod = T * conj(X)
    bfp_complex_s32_conj_mul(&prod, T_ph, X_fifo_ph);

    // Perform the inverse FFT on the product to get the time-domain delta_h
    h_hat_inverse_fft(&prod);

    // Calculate (h + delta) output exponent, so the delta shift can be applied in the gather
    exponent_t sum_exp;
    right_shift_t h_hat_shr, delta_shr;
    vect_s16_add_prepare(&sum_exp, &h_hat_shr, &delta_shr, h_hat_ph->exp, prod.exp + 16, h_hat_ph->hr, prod.hr);

    // Every tap of the delta would round to zero, so leave h_hat as it is rather than truncate it to the sum's exponent
    if(delta_shr >= 16) {
        return;
    }

    // Save the non-zero taps in bit-reversed order, narrowed to 16 bit at the sum's exponent, then
    // accumulate into h_hat.
    int16_t *delta_taps = (int16_t*)delta_scratch;
    aec_h_hat_bitrev_gather(delta_taps, (const int32_t*)delta_scratch, delta_shr);
    h_hat_ph->hr = vect_s16_add(h_hat_ph->data, h_hat_ph->data, delta_taps, AEC_FRAME_ADVANCE, h_hat_shr, 0);
    h_hat_ph->exp = sum_exp;
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
