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

//The mono FFT packs the real time domain signal into complex pairs to reduce the FFT size. The
//gather and scatter below move a whole complex element at a time. That needs the pair to be aligned
//to its own width, which holds for every buffer they are used on: aec_state_t declares the AEC
//memory pool DWORD_ALIGNED and aec_init() rounds every allocation from it up to a whole number of
//double words (AEC_POOL_ALIGN()), and the FFT scratch buffers here are declared DWORD_ALIGNED.
//
//h_hat stores 16 bit taps while the transforms work at 32 bit, so the two directions are not symmetric. The scatter
//widens as it goes - a pair of taps is one word in h_hat and a double word in the transform buffer - putting each
//tap in the top half of its slot, which is the widening that costs no instructions. The gather leaves the delta at
//32 bit and its caller narrows it afterwards, because narrowing a contiguous vector is a job for the VPU.

//How the gather and scatter are written is set by what the compilers do with them, measured on XS3 and VX4:
// - The pair of taps in each slot moves as a single int64_t, never as a complex_s32_t struct copy, and a pair of 16
//   bit taps as a single word, so that each is one load or store where the target has one. The may_alias types
//   make those accesses legal on objects declared as complex elements.
// - Each group of kept slots is unrolled into blocks whose word offsets fit the 0..11 immediate range of XS3's ldw
//   and stw, with the pointers advanced between blocks.
// - XS3 has no immediate or indexed ldd/std the compiler will use, so a double word is only ever addressed by a
//   pointer register. Given constant strides the compiler folds the pointer steps into large constant offsets and
//   then spends an instruction rebuilding each address. The strides are therefore passed in at run time to the
//   externally visible *_strided() functions below, which forces a single register add per step. This costs VX4
//   nothing, since its loads and stores take a register base with an offset either way.
//These give about 2.2x (gather) and 2.4x (scatter) on XS3 and 1.3x and 1.6x on VX4 over the simple loops.
_Static_assert(AEC_H_HAT_BITREV_GROUP == 16,
        "the unrolled h_hat gather and scatter below assume 15 kept slots per group");

#if defined(__XS3A__)
#define AEC_DUAL_ISSUE __attribute__((dual_issue))
#else
#define AEC_DUAL_ISSUE
#endif

//Unrolling the group loops as well would undo the blocking.
#if defined(__clang__)
#define AEC_NO_UNROLL _Pragma("clang loop unroll(disable)")
#else
#define AEC_NO_UNROLL
#endif

typedef int64_t  __attribute__((may_alias)) aec_tap_pair32_t;
typedef int32_t  __attribute__((may_alias)) aec_tap32_t;
typedef uint32_t __attribute__((may_alias)) aec_tap_pair16_t;

#define AEC_ADVANCE_BYTES(p, bytes) ((p) = (__typeof__(p))((char*)(p) + (bytes)))

__attribute__((noinline)) AEC_DUAL_ISSUE
void aec_h_hat_bitrev_gather_strided(
        complex_s32_t *dst,
        const complex_s32_t *src,
        int dst_step,
        int src_step,
        int group_skip)
{
    //In place the copy only ever moves data towards the front, and each element is read before it is overwritten.
    aec_tap_pair32_t *d = (aec_tap_pair32_t*)dst;
    const aec_tap_pair32_t *s = (const aec_tap_pair32_t*)src;
#define GATHER_ONE() do { *d = *s; AEC_ADVANCE_BYTES(d, dst_step); AEC_ADVANCE_BYTES(s, src_step); } while(0)
    AEC_NO_UNROLL
    for(unsigned g=0; g<AEC_H_HAT_BITREV_DROPPED; g++) {
        GATHER_ONE(); GATHER_ONE(); GATHER_ONE(); GATHER_ONE(); GATHER_ONE();
        GATHER_ONE(); GATHER_ONE(); GATHER_ONE(); GATHER_ONE(); GATHER_ONE();
        GATHER_ONE(); GATHER_ONE(); GATHER_ONE(); GATHER_ONE(); GATHER_ONE();
        AEC_ADVANCE_BYTES(s, group_skip);
    }
#undef GATHER_ONE
}

//Copy the taps h_hat stores out of a full bit-reversed index time domain vector, dropping the slots the gradient
//constraint zeroes. `src` may be the buffer `dst` points into.
void aec_h_hat_bitrev_gather(
        complex_s32_t *dst,
        const complex_s32_t *src)
{
    //Take every even slot; the odd slot beside each holds taps AEC_PROC_FRAME_LENGTH/2 onwards. Each group then
    //skips its dropped even slot, which holds the taps between AEC_FRAME_ADVANCE and AEC_PROC_FRAME_LENGTH/2, and
    //that slot's odd partner.
    aec_h_hat_bitrev_gather_strided(dst, src, sizeof(complex_s32_t), 2*sizeof(complex_s32_t),
                                    2*sizeof(complex_s32_t));
}

__attribute__((noinline)) AEC_DUAL_ISSUE
void aec_h_hat_bitrev_scatter_strided(
        complex_s32_t *dst,
        const complex_s16_t *src,
        int block_step,
        int group_skip)
{
    vect_complex_s32_set(dst, 0, 0, AEC_PROC_FRAME_LENGTH/2);

    //Each source word is a pair of 16 bit taps, the even one in the low half. Shifting and masking put each in the
    //top half of its own word.
    aec_tap32_t *restrict d = (aec_tap32_t*)dst;
    const aec_tap_pair16_t *restrict s = (const aec_tap_pair16_t*)src;
#define WIDEN(k, j) do { const uint32_t w = s[k];                                                   \
                         d[4*(j)]   = (int32_t)(w << 16);                                           \
                         d[4*(j)+1] = (int32_t)(w & 0xFFFF0000u); } while(0)
#define SCATTER_BLOCK(k, step) do { WIDEN(k, 0); WIDEN((k)+1, 1); WIDEN((k)+2, 2);                  \
                                    AEC_ADVANCE_BYTES(d, step); } while(0)
    AEC_NO_UNROLL
    for(unsigned g=0; g<AEC_H_HAT_BITREV_DROPPED; g++) {
        SCATTER_BLOCK(0, block_step); SCATTER_BLOCK(3, block_step);
        SCATTER_BLOCK(6, block_step); SCATTER_BLOCK(9, block_step);
        s += 12;
        SCATTER_BLOCK(0, block_step + group_skip);
        s += 3;
    }
#undef SCATTER_BLOCK
#undef WIDEN
}

/**
 * Expand the taps h_hat stores into a full bit-reversed index time domain vector ready to be transformed in place,
 * widening each 16 bit tap into the top half of its 32 bit slot. That is a scaling by 2^16, which the caller accounts
 * for in the exponent it gives the transformed phase; it leaves the taps' headroom unchanged.
 * Every slot h_hat has no storage for is a tap the gradient constraint zeroes, so the whole vector is cleared first.
 */
void aec_h_hat_bitrev_scatter(
        complex_s32_t *dst,
        const complex_s16_t *src)
{
    //A block of three stored pairs fills three even slots and steps over their odd partners, which hold taps
    //AEC_PROC_FRAME_LENGTH/2 onwards and stay zero. Each group then steps over its dropped even slot and partner.
    aec_h_hat_bitrev_scatter_strided(dst, src, 6*sizeof(complex_s32_t), 2*sizeof(complex_s32_t));
}

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
        complex_s32_t *scratch)
{
    //fft_dit_forward() requires 2 bits of headroom, do this on the compressed taps
    headroom_t hr = vect_s16_headroom(h_hat_ph->data, AEC_FRAME_ADVANCE);
    right_shift_t shr = 2 - (right_shift_t)hr;

    //Expand from compressed 16b to bit-reversed 32b taps
    aec_h_hat_bitrev_scatter(scratch, (const complex_s16_t*)h_hat_ph->data);
    if(shr) {
        vect_s32_shl((int32_t*)scratch, (const int32_t*)scratch, AEC_PROC_FRAME_LENGTH, -shr);
    }

    //Scatter shifts by 2^16 when going to 32b
    bfp_complex_s32_init(H_hat_ph, scratch, h_hat_ph->exp - 16 + shr, AEC_PROC_FRAME_LENGTH/2, 0);
    H_hat_ph->hr = hr + shr;

    // The coeffs are already bit reversed, so use DIT FFT
    fft_dit_forward(H_hat_ph->data, AEC_PROC_FRAME_LENGTH/2, &H_hat_ph->hr, &H_hat_ph->exp);
    fft_mono_adjust(H_hat_ph->data, AEC_PROC_FRAME_LENGTH, 0);
    bfp_complex_s32_headroom(H_hat_ph);
    bfp_fft_unpack_mono(H_hat_ph);
}

/**
 * Transform each filter phase in turn and accumulate X * H_hat into Y_hat over the requested chunk.
 * 
 * TODO: This avoids a prototype VX4 compiler bug related to stack frame handling when large scratch buffers are used.
 */
__attribute__((noinline))
static void aec_l2_accumulate_Y_hat(
        bfp_complex_s32_t *Y_hat,
        const bfp_complex_s32_t *X_fifo,
        const bfp_s16_t *h_hat,
        unsigned phases,
        unsigned start_offset,
        unsigned length)
{
    //Scratch to FFT the current filter phase from time domain to frequency domain
    complex_s32_t DWORD_ALIGNED h_fft_scratch[AEC_FD_FRAME_LENGTH];
    for(unsigned ph=0; ph<phases; ph++) {
        bfp_complex_s32_t H_hat_ph;
        h_hat_forward_fft(&H_hat_ph, &h_hat[ph], h_fft_scratch);

        bfp_complex_s32_t X_chunk, H_hat_chunk;
        bfp_complex_s32_init(&X_chunk, &X_fifo[ph].data[start_offset], X_fifo[ph].exp, length, 0);
        X_chunk.hr = X_fifo[ph].hr;
        bfp_complex_s32_init(&H_hat_chunk, &H_hat_ph.data[start_offset], H_hat_ph.exp, length, 0);
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
        unsigned start_offset,
        unsigned length,
        int32_t bypass_enabled)
{
    if(!length) {
        return;
    }
    if(bypass_enabled) { //Copy Y into Error. Set Y_hat to 0
        vpu_memcpy(Error->data, &Y->data[start_offset], length*sizeof(complex_s32_t));
        Error->exp = Y->exp;
        Error->hr = Y->hr;

        vect_complex_s32_set(Y_hat->data, 0, 0, length);
        Y_hat->exp = AEC_ZEROVAL_EXP;
        Y_hat->hr = AEC_ZEROVAL_HR;
    }
    else {
        aec_l2_accumulate_Y_hat(Y_hat, X_fifo, h_hat, num_x_channels * num_phases, start_offset, length);

        bfp_complex_s32_t Y_chunk;
        bfp_complex_s32_init(&Y_chunk, &Y->data[start_offset], Y->exp, length, 0);
        Y_chunk.hr = Y->hr;
        bfp_complex_s32_sub(Error, &Y_chunk, Y_hat);
    }
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
    //prod = T * conj(X)
    bfp_complex_s32_conj_mul(&prod, T_ph, X_fifo_ph);

    //delta_h = ifft(prod), computed in place over the delta_scratch buffer. This mirrors bfp_fft_inverse_mono(),
    //but skips the fft_index_bit_reversal() and stores the bit-reversed coefficients.
    bfp_fft_pack_mono(&prod);
    //fft_dif_inverse() requires 2 bits of headroom
    bfp_complex_s32_use_exponent(&prod, prod.exp - prod.hr + 2);
    fft_mono_adjust(prod.data, AEC_PROC_FRAME_LENGTH, 1);
    fft_dif_inverse(prod.data, AEC_PROC_FRAME_LENGTH/2, &prod.hr, &prod.exp);

    //Save the non-zero taps in bit-reversed order. The gradient constraint is applied as
    //the discarded taps are effectively zeroed. Although delta_scratch is complex, after the mono
    //inverse FFT it holds the real time domain delta, packed two taps to each complex element.
    aec_h_hat_bitrev_gather(delta_scratch, delta_scratch);

    // Narrow delta to 16-bit taps before adding to h_hat, this can be done inplace
    int32_t *delta_words = (int32_t*)delta_scratch;
    int16_t *delta_taps = (int16_t*)delta_scratch;

    // Calculate (h + delta) output exponent before we shift delta to 32b, so we can go directly to
    // the correct exponent
    const headroom_t delta_hr = vect_s32_headroom(delta_words, AEC_FRAME_ADVANCE);
    const exponent_t h_hat_min_exp = h_hat_ph->exp - (exponent_t)h_hat_ph->hr;
    const exponent_t delta_min_exp = prod.exp + 16 - (exponent_t)delta_hr;
    const exponent_t sum_exp = ((h_hat_min_exp > delta_min_exp) ? h_hat_min_exp : delta_min_exp) + 1;

    vect_s32_to_vect_s16(delta_taps, delta_words, AEC_FRAME_ADVANCE, sum_exp - prod.exp);

    // Update h_hat with delta
    h_hat_ph->hr = vect_s16_add(h_hat_ph->data, h_hat_ph->data, delta_taps, AEC_FRAME_ADVANCE,
                                sum_exp - h_hat_ph->exp, 0);
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
