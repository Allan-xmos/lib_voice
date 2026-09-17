// Copyright 2022-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "aec_unit_tests.h"
#include <stdio.h>
#include <assert.h>
#include "aec.h"

#define TEST_NUM_Y (1)
#define TEST_NUM_X (2)
#define TEST_MAIN_PHASES (5)
#define TEST_SHADOW_PHASES (3)
#define NUM_BINS ((AEC_PROC_FRAME_LENGTH/2) + 1)
#define NUM_TAPS (AEC_FILTER_TAPS_PER_PHASE)

static double sine_lut_ifft[AEC_PROC_FRAME_LENGTH / 4 + 1];

/* Reference model of one phase of the time domain filter update.
 *
 * The AEC stores h_hat in the time domain, so the update is
 *     h += first NUM_TAPS taps of inverse_fft(T * conj(X))
 * and the gradient constraint is implicit in discarding the tail. There is no forward transform of the filter here,
 * unlike the frequency domain filter implementation which had to re-constrain H_hat after every update.
 */
void aec_filter_adapt_td_fp(
        double *h_hat,
        complex_double_t *X_fifo,
        complex_double_t *T,
        int bypass) {
    if(bypass) {
        return;
    }
    complex_double_t scratch[AEC_PROC_FRAME_LENGTH];
    int N = AEC_PROC_FRAME_LENGTH;

    //dH = T * conj(X), over the DC to nyquist bins
    complex_double_t dH[NUM_BINS];
    for(int i=0; i<N/2+1; i++) {
        dH[i].re = (T[i].re*X_fifo[i].re + T[i].im*X_fifo[i].im);
        dH[i].im = (T[i].im*X_fifo[i].re - T[i].re*X_fifo[i].im);
    }
    //Generate 2nd half of the spectrum based on symmetry
    for(int i=0; i<N/2; i++) {
        scratch[i].re = dH[i].re;
        scratch[i].im = dH[i].im;

        if(i) {
            scratch[N-i].re = scratch[i].re;
            scratch[N-i].im = -scratch[i].im;
        }
        //Copy nyquist
        scratch[N/2].re = dH[N/2].re;
        scratch[N/2].im = dH[N/2].im;
    }
    //IFFT
    bit_reverse((complex_double_t *)scratch, N);
    inverse_fft((complex_double_t *)scratch, N, sine_lut_ifft);

    //Accumulate the first NUM_TAPS taps into the filter. The rest of the update is discarded.
    for(int i=0; i<NUM_TAPS; i++) {
        h_hat[i] += scratch[i].re;
    }
}

/* The stored filter is in the low level DFT's element order (see AEC_FILTER_TD_PAIRS): stored complex element m
 * holds the sample pair (2k, 2k+1) for k = n_bitrev(m). These helpers move between that and natural tap order. */
static void scramble_taps(bfp_s16_t *h, const double *taps, exponent_t exp, headroom_t hr)
{
    h->exp = exp;
    h->hr = hr;
    for(unsigned m=0; m<AEC_FILTER_TD_PAIRS; m++) {
        const unsigned k = n_bitrev(m, AEC_FILTER_TD_PAIRS_LOG2);
        for(unsigned half=0; half<2; half++) {
            const unsigned tap = 2*k + half;
            double v = (tap < NUM_TAPS) ? ldexp(taps[tap], -exp) : 0.0;
            h->data[2*m + half] = (int16_t)((v < 0) ? (v - 0.5) : (v + 0.5));
        }
    }
}

static unsigned taps_maxdiff(const bfp_s16_t *h, const double *taps)
{
    unsigned max_diff = 0;
    for(unsigned m=0; m<AEC_FILTER_TD_PAIRS; m++) {
        const unsigned k = n_bitrev(m, AEC_FILTER_TD_PAIRS_LOG2);
        for(unsigned half=0; half<2; half++) {
            const unsigned tap = 2*k + half;
            //Taps at or beyond NUM_TAPS must be held at zero by the gradient constraint
            double r = (tap < NUM_TAPS) ? ldexp(taps[tap], -h->exp) : 0.0;
            int32_t v = (int32_t)((r < 0) ? (r - 0.5) : (r + 0.5));
            int diff = v - h->data[2*m + half];
            if(diff < 0) diff = -diff;
            if((unsigned)diff > max_diff) max_diff = (unsigned)diff;
        }
    }
    return max_diff;
}

void test_aec_filter_adapt() {
    unsigned num_y_channels = TEST_NUM_Y;
    unsigned num_x_channels = TEST_NUM_X;
    unsigned main_filter_phases = TEST_MAIN_PHASES;
    unsigned shadow_filter_phases = TEST_SHADOW_PHASES;

    aec_state_t aec_state;

    aec_init(&aec_state, num_y_channels, num_x_channels, main_filter_phases, shadow_filter_phases, &aec_tdist_chans2_threads2);

    //Declare floating point arrays
    double h_hat_fp[TEST_NUM_Y][TEST_NUM_X*TEST_MAIN_PHASES][NUM_TAPS];
    complex_double_t X_fifo_fp[TEST_NUM_X][TEST_MAIN_PHASES][NUM_BINS];
    complex_double_t T_fp[TEST_NUM_X][NUM_BINS];

    //Init FFT for reference
    make_sine_table(sine_lut_ifft, AEC_PROC_FRAME_LENGTH);
    unsigned seed=578335;
    unsigned max_diff = 0;
    for(int itt=0; itt<(100)/F; itt++) {
        int32_t new_frame[AEC_MAX_Y_CHANNELS+AEC_MAX_X_CHANNELS][AEC_FRAME_ADVANCE];
        unsigned is_main = pseudo_rand_uint32(&seed) % 2;
        aec_filter_state_t *state_ptr;
        if(is_main) {
            state_ptr = &aec_state.main_state;
        }
        else {
            state_ptr = &aec_state.shadow_state;
        }
        state_ptr->shared_state->config_params.aec_core_conf.bypass = pseudo_rand_uint32(&seed) % 2;
        unsigned test_l2_api = pseudo_rand_uint32(&seed) % 2;
        aec_frame_init(&aec_state.main_state, &aec_state.shadow_state, &new_frame[0], &new_frame[AEC_MAX_Y_CHANNELS]);
        //Generate h_hat. Generated in natural tap order and scrambled into the stored order, so the reference and
        //the DUT start from exactly the same filter.
        for(int ch=0; ch<num_y_channels; ch++) {
            for(int ph=0; ph<num_x_channels*state_ptr->num_phases; ph++) {
                const exponent_t exp = pseudo_rand_int(&seed, -31, 32);
                const headroom_t hr = pseudo_rand_uint32(&seed) % 5;
                for(int i=0; i<NUM_TAPS; i++) {
                    int16_t mant = (int16_t)(pseudo_rand_int32(&seed) >> (16 + hr));
                    h_hat_fp[ch][ph][i] = ldexp(mant, exp);
                }
                scramble_taps(&state_ptr->h_hat[ch][ph], h_hat_fp[ch][ph], exp, hr);
            }
        }
        //Generate X_fifo, (always for number of phases in aec_state.main_state)
        aec_filter_state_t *main_state_ptr = &aec_state.main_state;
        for(int ch=0; ch<num_x_channels; ch++) {
            for(int ph=0; ph<main_state_ptr->num_phases; ph++) {
                state_ptr->shared_state->X_fifo[ch][ph].exp = pseudo_rand_int(&seed, -31, 32);
                state_ptr->shared_state->X_fifo[ch][ph].hr = pseudo_rand_uint32(&seed) % 5;
                for(int i=0; i<NUM_BINS; i++) {
                    state_ptr->shared_state->X_fifo[ch][ph].data[i].re = pseudo_rand_int32(&seed) >> state_ptr->shared_state->X_fifo[ch][ph].hr;
                    state_ptr->shared_state->X_fifo[ch][ph].data[i].im = pseudo_rand_int32(&seed) >> state_ptr->shared_state->X_fifo[ch][ph].hr;

                    X_fifo_fp[ch][ph][i].re = ldexp(state_ptr->shared_state->X_fifo[ch][ph].data[i].re, state_ptr->shared_state->X_fifo[ch][ph].exp);
                    X_fifo_fp[ch][ph][i].im = ldexp(state_ptr->shared_state->X_fifo[ch][ph].data[i].im, state_ptr->shared_state->X_fifo[ch][ph].exp);
                }
                state_ptr->shared_state->X_fifo[ch][ph].data[0].im = 0;
                state_ptr->shared_state->X_fifo[ch][ph].data[NUM_BINS-1].im = 0;
                X_fifo_fp[ch][ph][0].im = 0.0;
                X_fifo_fp[ch][ph][NUM_BINS-1].im = 0.0;
            }
        }
        //Generate T
        for(int ch=0; ch<num_x_channels; ch++) {
            state_ptr->T[ch].exp = pseudo_rand_int(&seed, -31, 32);
            state_ptr->T[ch].hr = pseudo_rand_uint32(&seed) % 5;
            for(int i=0; i<NUM_BINS; i++) {
                state_ptr->T[ch].data[i].re = pseudo_rand_int32(&seed) >> state_ptr->T[ch].hr;
                state_ptr->T[ch].data[i].im = pseudo_rand_int32(&seed) >> state_ptr->T[ch].hr;

                T_fp[ch][i].re = ldexp(state_ptr->T[ch].data[i].re, state_ptr->T[ch].exp);
                T_fp[ch][i].im = ldexp(state_ptr->T[ch].data[i].im, state_ptr->T[ch].exp);
            }
            state_ptr->T[ch].data[0].im = 0;
            state_ptr->T[ch].data[NUM_BINS-1].im = 0;
            T_fp[ch][0].im = 0.0;
            T_fp[ch][NUM_BINS-1].im = 0.0;
        }
        //aec init only initialises the 2d Xfifo. Since we're using the 1d fifo for error computation, call aec_update_X_fifo_1d()
        //to update the 1d Fifo
        aec_update_X_fifo_1d(state_ptr);

        //ref
        for(int ych=0; ych<num_y_channels; ych++) {
            for(int xch=0; xch<num_x_channels; xch++) {
                for(int p=0; p<state_ptr->num_phases; p++) {
                    aec_filter_adapt_td_fp(h_hat_fp[ych][xch*state_ptr->num_phases + p], X_fifo_fp[xch][p], T_fp[xch], state_ptr->shared_state->config_params.aec_core_conf.bypass);
                }
            }
        }
        //dut
        if(!test_l2_api) {
            for(int ch=0; ch<num_y_channels; ch++) {
                aec_filter_adapt(state_ptr, ch);
            }
        }
        else {
            #define NUM_CHUNKS_PER_CH (4) //spread num_phases over 4 chunks for each y-channel
            if(!state_ptr->shared_state->config_params.aec_core_conf.bypass) {
                for(int c=0; c<num_y_channels; c++) {
                    int remaining_phases = num_x_channels * state_ptr->num_phases;
                    int start_phase=0;
                    int num_phases;
                    for(int t=0; t<NUM_CHUNKS_PER_CH; t++) {
                        int ch=c;
                        if((t == NUM_CHUNKS_PER_CH-1) || (remaining_phases <= 1))
                        {
                            num_phases = remaining_phases;
                            remaining_phases = 0;
                        }
                        else if(remaining_phases > 1) {
                            num_phases = (uint32_t)pseudo_rand_uint32(&seed) % remaining_phases;
                            remaining_phases -= num_phases;
                        }
                        for(int ph=start_phase; ph<start_phase+num_phases; ph++) {
                            aec_l2_adapt_td(&state_ptr->h_hat[ch][ph], &state_ptr->X_fifo_1d[ph], &state_ptr->T[ph/state_ptr->num_phases], &state_ptr->filter_scratch[ch]);
                        }
                        start_phase += num_phases;
                    }
                }
            }
        }
        //Compare outputs
        for(int ch=0; ch<num_y_channels; ch++) {
            for(int p=0; p<num_x_channels*state_ptr->num_phases; p++) {
                unsigned diff = taps_maxdiff(&state_ptr->h_hat[ch][p], h_hat_fp[ch][p]);
                //printf("diff %d\n",diff);
                max_diff = (diff > max_diff) ? diff : max_diff;
                /* The filter is stored with 16 bit mantissas, so the update is requantised once per frame. A handful
                 * of LSBs of disagreement with the double precision reference is expected; anything larger means the
                 * update or the gradient constraint is wrong rather than just rounded. */
                TEST_ASSERT_LESS_OR_EQUAL_UINT32_MESSAGE(1<<4, diff, "h_hat diff too large.");
            }
        }
    }
    printf("max_diff %d\n",max_diff);
}
