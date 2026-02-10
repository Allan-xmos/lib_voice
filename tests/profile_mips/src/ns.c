// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "voice.h"
#include "profile.h"

void test_ns(int32_t (*input)[NS_FRAME_ADVANCE])
{
    static int framenum = 0;
    static ns_state_t DWORD_ALIGNED ns_state;
    static int32_t output[NS_FRAME_ADVANCE];

    if(!framenum) {
        ns_init(&ns_state);
    }

    prof(0, "start_ns_process_frame");
    ns_process_frame(&ns_state, output, input[0]);
    prof(1, "end_ns_process_frame");
    print_prof(0, 2, framenum);
    framenum += 1;
}
