// Copyright 2017-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "aec.h"

#include <stdio.h>
#include <math.h>
#include "fileio.h"

void aec_dump_H_hat(aec_filter_state_t *state, file_t *file_handle){
    char strbuf[1024];
    sprintf(strbuf, "import numpy as np\n");
    file_write(file_handle, (uint8_t*)strbuf, strlen(strbuf));
    sprintf(strbuf, "frame_advance = %u\n", AEC_FRAME_ADVANCE);
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    sprintf(strbuf, "y_channel_count = %u\n", state->shared_state->num_y_channels);
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    sprintf(strbuf, "x_channel_count = %u\n", state->shared_state->num_x_channels);
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    sprintf(strbuf, "max_phase_count = %u\n", state->num_phases);
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    sprintf(strbuf, "tap_count = %u\n", AEC_FILTER_TAPS_PER_PHASE);
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    /* The AEC filter is stored in the time domain, so the impulse response is dumped directly rather than as a
     * spectrum that the reader has to inverse transform. The stored taps are in the low level DFT's element order
     * (see AEC_FILTER_TD_PAIRS), so they are unscrambled into natural tap order here. */
    sprintf(strbuf, "h_hat = np.zeros((y_channel_count, x_channel_count, max_phase_count, tap_count), dtype=np.float64)\n");
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));

    for(int ych=0; ych<state->shared_state->num_y_channels; ych++) {
        for(int xch=0; xch<state->shared_state->num_x_channels; xch++) {
            for(int ph=0; ph<state->num_phases; ph++) {
                bfp_s16_t *h_ph = &state->h_hat[ych][xch*state->num_phases + ph];
                double taps[AEC_FILTER_TAPS_PER_PHASE];

                for(unsigned m=0; m<AEC_FILTER_TD_PAIRS; m++) {
                    if(!AEC_FILTER_TD_SLOT_STORED(m)) {
                        continue; //slot the gradient constraint holds at zero, and which is therefore not stored
                    }
                    const unsigned k = n_bitrev(m, AEC_FILTER_TD_PAIRS_LOG2);
                    const unsigned s = AEC_FILTER_TD_STORED_INDEX(m);
                    taps[2*k] = ldexp(h_ph->data[2*s], h_ph->exp);
                    taps[2*k + 1] = ldexp(h_ph->data[2*s + 1], h_ph->exp);
                }

                sprintf(strbuf, "h_hat[%u][%u][%u] = ", ych, xch, ph);
                file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
                sprintf(strbuf, "np.asarray([");
                file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
                for(int i=0; i<AEC_FILTER_TAPS_PER_PHASE; i++) {
                    sprintf(strbuf, "%.12f, ", taps[i]);
                    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
                }
                sprintf(strbuf, "])\n");
                file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
            }
        }
    }
}

