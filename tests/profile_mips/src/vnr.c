// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "voice.h"
#include "profile.h"

void test_vnr(int32_t (*input)[VNR_FRAME_ADVANCE]) {
    static int framenum = 0;
    static float_s32_t vnr_out;
    static vnr_state_t vnr;
    if(!framenum) {
        vnr_state_init(&vnr);
    }

    prof(0, "start_vnr_process_frame");
    vnr_process_frame(&vnr, &vnr_out, input[0]);
    prof(1, "end_vnr_process_frame");

    print_prof(0, 2, framenum);
    framenum += 1;
}
