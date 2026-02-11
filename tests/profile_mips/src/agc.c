// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "voice.h"
#include "profile.h"

void test_agc(int32_t (*input)[AGC_FRAME_ADVANCE])
{
    static int framenum = 0;
    static agc_state_t agc_state;
    static int32_t output[AGC_FRAME_ADVANCE];
    static agc_meta_data_t agc_md;
    static agc_config_t agc_conf_asr;

    if(!framenum) {
        agc_conf_asr = AGC_PROFILE_ASR;
        agc_init(&agc_state, &agc_conf_asr);
    }

    prof(0, "start_agc_process_frame");
    agc_process_frame(&agc_state, output, input[0], &agc_md);
    prof(1, "end_agc_process_frame");

    print_prof(0, 2, framenum);
    framenum += 1;
}
