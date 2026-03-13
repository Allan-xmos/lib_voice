// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xcore/channel.h>
#include <xcore/chanend.h>
#include <xcore/channel_transaction.h>
#include <xcore/port.h>
#include <xcore/parallel.h>
#include <xcore/assert.h>
#include <xcore/hwtimer.h>
#include "xmath/xmath.h"
#include "xscope_io_device.h"
#include "fileio.h"

#include "pipeline_config.h"
#include "pipeline_state.h"

DECLARE_JOB(tx, (chanend_t, chanend_t, const char*));
DECLARE_JOB(pipeline_tile0, (chanend_t, chanend_t));
DECLARE_JOB(rx, (chanend_t, chanend_t, const char*));

DECLARE_JOB(main_tile0, (chanend_t, chanend_t, const char *, const char *));
DECLARE_JOB(main_tile1, (chanend_t, chanend_t));

extern void pipeline_tile1(chanend_t c_pcm_in_b, chanend_t c_pcm_out_a);

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
    int32_t DWORD_ALIGNED pipeline_output[AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];

    int ret = file_open(&output_file, output_file_name, "wb");
    assert((!ret) && "Failed to open file");

    unsigned frame_count = chan_in_word(c_frame_num);
    for(int frame=0; frame<frame_count; frame++)
    {
        // Receive output frame over channel
        chan_in_buf_word(c_pcm_out_b, (uint32_t*)&pipeline_output[0][0], (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE));

        file_write(&output_file, (uint8_t*)pipeline_output, (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE * sizeof(int32_t)));
    }

    shutdown_session();
    _Exit(0);
}

//**** Multi tile pipeline structure ***//
// file_read -> stage1 -> (tile0_to_tile1)-> stage2 -> stage3 -> stage4 -> (tile1_to_tile0) -> file_write
void main_tile0(chanend_t c_t0_t1, chanend_t c_t1_t0, const char *input_file_name, const char* output_file_name)
{
    channel_t c_pcm_in = chan_alloc();
    channel_t c_frame_num = chan_alloc();
    PAR_JOBS(
        PJOB(tx, (c_pcm_in.end_a, c_frame_num.end_a, input_file_name)),
        PJOB(pipeline_tile0, (c_pcm_in.end_b, c_t0_t1)),
        PJOB(rx, (c_t1_t0, c_frame_num.end_b, output_file_name))
        );
}

void main_tile1(chanend_t c_t0_t1, chanend_t c_t1_t0)
{
    pipeline_tile1(c_t0_t1, c_t1_t0);
}

int main() {
    chanend_t xscope_chan = chanend_alloc();
    channel_t c_th0_to_th1 = chan_alloc();
    channel_t c_th1_to_th0 = chan_alloc();
    xscope_io_init(xscope_chan);
    PAR_JOBS(
        PJOB(main_tile0, (c_th0_to_th1.end_a, c_th1_to_th0.end_a, "input.bin", "output.bin")),
        PJOB(main_tile1, (c_th0_to_th1.end_b, c_th1_to_th0.end_b))
    );
    chanend_free(xscope_chan);
    chan_free(c_th0_to_th1);
    chan_free(c_th1_to_th0);
}
