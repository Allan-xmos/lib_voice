// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "aec_unit_tests.h"
#include <stdio.h>
#include "aec.h"

//The h_hat gather and scatter are hand written assembly on XS3 and VX4 and plain C elsewhere, so the same test
//checks whichever this build has. Rather than compare against a copy of the C, it checks them against the layout
//itself: undoing the bit-reversal of a scattered vector with fft_index_bit_reversal() has to leave tap n in time
//domain position n, and aec_h_hat_tap_index() says where tap n is stored. The mono FFT packs the real taps into
//complex elements, so tap n is the real (n even) or imaginary (n odd) part of element n/2, and the bit-reversal
//permutes whole elements rather than taps.

#define ITERS (((1<<8)/F) > 0 ? ((1<<8)/F) : 1)
#define GUARD_WORDS (4)
#define GUARD_VALUE ((int32_t)0x5A5AA5A5)

//Room past the end of each output to catch an overrun.
typedef struct {
    complex_s32_t DWORD_ALIGNED data[AEC_PROC_FRAME_LENGTH/2];
    int32_t guard[GUARD_WORDS];
} full_vector_t;

typedef struct {
    complex_s32_t DWORD_ALIGNED data[AEC_FRAME_ADVANCE/2];
    int32_t guard[GUARD_WORDS];
} packed_vector_t;

static void fill_random(int32_t *x, unsigned length, unsigned *seed)
{
    for(unsigned i=0; i<length; i++) {
        x[i] = pseudo_rand_int32(seed);
    }
}

static void set_guard(int32_t *guard)
{
    for(unsigned i=0; i<GUARD_WORDS; i++) {
        guard[i] = GUARD_VALUE;
    }
}

static void check_guard(const int32_t *guard)
{
    for(unsigned i=0; i<GUARD_WORDS; i++) {
        TEST_ASSERT_EQUAL_INT32_MESSAGE(GUARD_VALUE, guard[i], "wrote past the end of dst");
    }
}

//Reorder a bit-reversed index vector of AEC_PROC_FRAME_LENGTH/2 complex elements into natural order, so that word
//n holds time domain tap n.
static void to_natural_order(int32_t *natural, const complex_s32_t *bitrev)
{
    memcpy(natural, bitrev, (AEC_PROC_FRAME_LENGTH/2) * sizeof(complex_s32_t));
    fft_index_bit_reversal((complex_s32_t*)natural, AEC_PROC_FRAME_LENGTH/2);
}

void test_aec_h_hat_bitrev_scatter()
{
    static int16_t DWORD_ALIGNED taps[AEC_FRAME_ADVANCE];
    static full_vector_t dst;
    static int32_t DWORD_ALIGNED natural[AEC_PROC_FRAME_LENGTH];

    unsigned seed = 0x1D5C8A73;
    for(int itt=0; itt<ITERS; itt++) {
        for(unsigned i=0; i<AEC_FRAME_ADVANCE; i++) {
            //Start with the extremes, which catch a sign or a half being lost in the widening.
            taps[i] = (itt == 0) ? ((i & 1) ? INT16_MIN : INT16_MAX) : pseudo_rand_int16(&seed);
        }
        //Fill dst with garbage, since the scatter has to zero every slot h_hat does not store.
        fill_random((int32_t*)dst.data, AEC_PROC_FRAME_LENGTH, &seed);
        set_guard(dst.guard);

        aec_h_hat_bitrev_scatter(dst.data, (const complex_s16_t*)taps);
        check_guard(dst.guard);

        to_natural_order(natural, dst.data);
        for(unsigned n=0; n<AEC_PROC_FRAME_LENGTH; n++) {
            int32_t expected = 0; //taps from AEC_FRAME_ADVANCE on are the ones the gradient constraint zeroes
            if(n < AEC_FRAME_ADVANCE) {
                expected = ((int32_t)taps[aec_h_hat_tap_index(n)]) * (1 << 16);
            }
            TEST_ASSERT_EQUAL_INT32(expected, natural[n]);
        }
    }
}

void test_aec_h_hat_bitrev_gather()
{
    static complex_s32_t DWORD_ALIGNED src[AEC_PROC_FRAME_LENGTH/2];
    static complex_s32_t DWORD_ALIGNED in_place[AEC_PROC_FRAME_LENGTH/2];
    static packed_vector_t dst;
    static complex_s32_t DWORD_ALIGNED src_copy[AEC_PROC_FRAME_LENGTH/2];
    static int32_t DWORD_ALIGNED natural[AEC_PROC_FRAME_LENGTH];

    unsigned seed = 0x72E1B90D;
    for(int itt=0; itt<ITERS; itt++) {
        //Every slot holds full scale data, including the ones the gather has to drop.
        fill_random((int32_t*)src, AEC_PROC_FRAME_LENGTH, &seed);
        memcpy(src_copy, src, sizeof(src_copy));
        fill_random((int32_t*)dst.data, AEC_FRAME_ADVANCE, &seed);
        set_guard(dst.guard);

        aec_h_hat_bitrev_gather(dst.data, src);
        check_guard(dst.guard);
        TEST_ASSERT_EQUAL_INT32_ARRAY((int32_t*)src_copy, (int32_t*)src, AEC_PROC_FRAME_LENGTH);

        to_natural_order(natural, src);
        const int32_t *dst_taps = (const int32_t*)dst.data;
        for(unsigned n=0; n<AEC_FRAME_ADVANCE; n++) {
            TEST_ASSERT_EQUAL_INT32(natural[n], dst_taps[aec_h_hat_tap_index(n)]);
        }

        //aec_l2_adapt_plus_ifft() gathers in place, so that has to give the same answer.
        memcpy(in_place, src, sizeof(in_place));
        aec_h_hat_bitrev_gather(in_place, in_place);
        TEST_ASSERT_EQUAL_INT32_ARRAY(dst_taps, (int32_t*)in_place, AEC_FRAME_ADVANCE);
    }
}

//The two directions together are how the AEC uses them: a phase is scattered out of h_hat, transformed, and its
//update gathered back in. Neither direction may move or lose a tap h_hat stores, and a gather then scatter is the
//gradient constraint, so it has to zero exactly the taps from AEC_FRAME_ADVANCE on.
void test_aec_h_hat_bitrev_round_trip()
{
    static int16_t DWORD_ALIGNED taps[AEC_FRAME_ADVANCE];
    static complex_s32_t DWORD_ALIGNED full[AEC_PROC_FRAME_LENGTH/2];
    static complex_s32_t DWORD_ALIGNED full_copy[AEC_PROC_FRAME_LENGTH/2];
    static packed_vector_t packed;
    static full_vector_t rescattered;
    static int32_t DWORD_ALIGNED natural_in[AEC_PROC_FRAME_LENGTH];
    static int32_t DWORD_ALIGNED natural_out[AEC_PROC_FRAME_LENGTH];

    unsigned seed = 0x3B94F26E;
    for(int itt=0; itt<ITERS; itt++) {
        //h_hat -> scatter -> gather has to give back the stored taps, in their stored order, widened by 2^16.
        for(unsigned i=0; i<AEC_FRAME_ADVANCE; i++) {
            taps[i] = (itt == 0) ? ((i & 1) ? INT16_MIN : INT16_MAX) : pseudo_rand_int16(&seed);
        }
        aec_h_hat_bitrev_scatter(full, (const complex_s16_t*)taps);
        set_guard(packed.guard);
        aec_h_hat_bitrev_gather(packed.data, full);
        check_guard(packed.guard);
        const int32_t *packed_taps = (const int32_t*)packed.data;
        for(unsigned k=0; k<AEC_FRAME_ADVANCE; k++) {
            TEST_ASSERT_EQUAL_INT32(((int32_t)taps[k]) * (1 << 16), packed_taps[k]);
        }

        //full -> gather -> narrow -> scatter has to give back full with every tap past AEC_FRAME_ADVANCE zeroed.
        //Clearing the bottom half of each word makes the narrowing exact, so any difference is down to the layout.
        fill_random((int32_t*)full, AEC_PROC_FRAME_LENGTH, &seed);
        for(unsigned i=0; i<AEC_PROC_FRAME_LENGTH; i++) {
            ((int32_t*)full)[i] &= (int32_t)0xFFFF0000;
        }
        memcpy(full_copy, full, sizeof(full_copy));
        //Gather in place, as aec_l2_adapt_plus_ifft() does.
        aec_h_hat_bitrev_gather(full, full);
        //Narrow in place to the 16 bit taps h_hat stores. Writing tap k never overwrites a word not yet read.
        int32_t *words = (int32_t*)full;
        int16_t *narrowed = (int16_t*)full;
        for(unsigned k=0; k<AEC_FRAME_ADVANCE; k++) {
            narrowed[k] = (int16_t)(words[k] >> 16);
        }
        fill_random((int32_t*)rescattered.data, AEC_PROC_FRAME_LENGTH, &seed);
        set_guard(rescattered.guard);
        aec_h_hat_bitrev_scatter(rescattered.data, (const complex_s16_t*)narrowed);
        check_guard(rescattered.guard);

        to_natural_order(natural_in, full_copy);
        to_natural_order(natural_out, rescattered.data);
        for(unsigned n=0; n<AEC_PROC_FRAME_LENGTH; n++) {
            int32_t expected = (n < AEC_FRAME_ADVANCE) ? natural_in[n] : 0;
            TEST_ASSERT_EQUAL_INT32(expected, natural_out[n]);
        }
    }
}
