// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xcore/channel.h>
#include <xcore/chanend.h>
#include <xcore/channel_transaction.h>
#include <xcore/port.h>
#include <xcore/parallel.h>
#include <xcore/assert.h>
#include <xcore/hwtimer.h>
#include <xcore/thread.h>
#include "xmath/xmath.h"
#include "xscope_io_device.h"
#include "fileio.h"

#include "pipeline_config.h"
#include "pipeline_state.h"

DECLARE_JOB(pipeline_stage_1, (chanend_t, chanend_t));
DECLARE_JOB(pipeline_stage_2, (chanend_t, chanend_t));
DECLARE_JOB(pipeline_stage_3, (chanend_t, chanend_t));
DECLARE_JOB(pipeline_stage_4, (chanend_t, chanend_t));
DECLARE_JOB(tx, (chanend_t, chanend_t, const char*));
DECLARE_JOB(rx, (chanend_t, chanend_t, const char*));

extern void pipeline_stage_1(chanend_t c_frame_in, chanend_t c_frame_out);
extern void pipeline_stage_2(chanend_t c_frame_in, chanend_t c_frame_out);
extern void pipeline_stage_3(chanend_t c_frame_in, chanend_t c_frame_out);
extern void pipeline_stage_4(chanend_t c_frame_in, chanend_t c_frame_out);

/// tx
void tx(chanend_t c_pcm_in_a, chanend_t c_frame_num, const char* input_file_name) {
    file_t input_file;
    // Open input wav file containing mic and ref channels of input data
    int ret = file_open(&input_file, input_file_name, "rb");
    assert((!ret) && "Failed to open file");

    const int32_t file_size = get_file_size(&input_file);
    const unsigned frame_count =
        file_size / ((AP_MAX_Y_CHANNELS + AP_MAX_X_CHANNELS) * (unsigned)sizeof(int32_t) * AP_FRAME_ADVANCE);

    chan_out_word(c_frame_num, frame_count);

    int32_t DWORD_ALIGNED frame[AP_MAX_X_CHANNELS + AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];
    for(unsigned b=0; b<frame_count; b++) {
        file_read(&input_file, (uint8_t*)&frame[0][0], (unsigned)sizeof(int32_t) * (AP_MAX_X_CHANNELS + AP_MAX_Y_CHANNELS) * AP_FRAME_ADVANCE);
        // Transmit input frame over channel
        chan_out_buf_word(c_pcm_in_a, (uint32_t*)&frame[0][0], ((AP_MAX_Y_CHANNELS+AP_MAX_X_CHANNELS) * AP_FRAME_ADVANCE));
    }
}

/// rx
void rx(chanend_t c_pcm_out_b, chanend_t c_frame_num, const char* output_file_name) {
    file_t output_file;
    int32_t DWORD_ALIGNED pipeline_output[AP_FRAME_ADVANCE];

    int ret = file_open(&output_file, output_file_name, "wb");
    assert((!ret) && "Failed to open file");

    unsigned frame_count = chan_in_word(c_frame_num);
    for(int frame=0; frame<frame_count; frame++)
    {
        // Receive output frame over channel
        chan_in_buf_word(c_pcm_out_b, (uint32_t*)&pipeline_output[0], AP_FRAME_ADVANCE);

        file_write(&output_file, (uint8_t*)pipeline_output, AP_FRAME_ADVANCE * sizeof(int32_t));
    }

    shutdown_session();
    _Exit(0);
}

int main() {
    chanend_t xscope_chan = chanend_alloc();
    channel_t tx_to_st1 = chan_alloc();
    channel_t tx_to_rx = chan_alloc();
    channel_t st1_to_st2 = chan_alloc();
    channel_t st2_to_st3 = chan_alloc();
    channel_t st3_to_st4 = chan_alloc();
    channel_t st4_to_rx = chan_alloc();
    xscope_io_init(xscope_chan);

    PAR_JOBS(
        PJOB(tx, (tx_to_st1.end_a, tx_to_rx.end_a, "input.bin")),
        PJOB(pipeline_stage_1, (tx_to_st1.end_b, st1_to_st2.end_a)),
        PJOB(pipeline_stage_2, (st1_to_st2.end_b, st2_to_st3.end_a)),
        PJOB(pipeline_stage_3, (st2_to_st3.end_b, st3_to_st4.end_a)),
        PJOB(pipeline_stage_4, (st3_to_st4.end_b, st4_to_rx.end_a)),
        PJOB(rx, (st4_to_rx.end_b, tx_to_rx.end_b, "output.bin"))
    );


    chanend_free(xscope_chan);
    chan_free(tx_to_st1);
    chan_free(tx_to_rx);
    chan_free(st1_to_st2);
    chan_free(st2_to_st3);
    chan_free(st3_to_st4);
    chan_free(st4_to_rx);
}
