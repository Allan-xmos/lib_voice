// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "stage1.h"

typedef struct {
  float_s32_t max_ref_energy;
  float_s32_t aec_corr_factor[AEC_MAX_Y_CHANNELS];
  int32_t ref_active_flag;
  float_s32_t vnr_pred_flag;
}pipeline_metadata_t;

int main()
{
  stage1_t state;
  aec_conf_t aec_de_mode_conf, aec_non_de_mode_conf;
  adec_config_t adec_conf;
  adec_conf.bypass = 1; // Bypass automatic DE correction
  adec_conf.force_de_cycle_trigger = 1;
  aec_non_de_mode_conf.num_y_channels = AEC_MAX_Y_CHANNELS;
  aec_non_de_mode_conf.num_x_channels = AEC_MAX_X_CHANNELS;
  aec_non_de_mode_conf.num_main_filt_phases = AEC_MAIN_FILTER_PHASES;
  aec_non_de_mode_conf.num_shadow_filt_phases = AEC_SHADOW_FILTER_PHASES;
  aec_non_de_mode_conf.tdist = &aec_tdist_chans2_threads1;

  aec_de_mode_conf.num_y_channels = 1;
  aec_de_mode_conf.num_x_channels = 1;
  aec_de_mode_conf.num_main_filt_phases = 30;
  aec_de_mode_conf.num_shadow_filt_phases = 0;
  aec_de_mode_conf.tdist = &aec_tdist_chans2_threads1;

  stage1_init(&state, &aec_de_mode_conf, &aec_non_de_mode_conf, &adec_conf);

  int32_t DWORD_ALIGNED frame[AEC_MAX_X_CHANNELS + AEC_MAX_Y_CHANNELS][AEC_FRAME_ADVANCE];
  int32_t DWORD_ALIGNED stage1_out[AEC_MAX_Y_CHANNELS][AEC_FRAME_ADVANCE]; // stage1 will not process the frame in-place since Mic input is needed to overwrite the output in certain cases
  pipeline_metadata_t md;

  for (unsigned i = 0; i < 10; i++)
  {
    stage1_process_frame(&state, &stage1_out[0], &md.max_ref_energy, &md.aec_corr_factor[0], &md.ref_active_flag, &frame[0], &frame[AEC_MAX_Y_CHANNELS]);
    printf("Frame %d\n", i);
  }
  printf("\n");
  return 0;
}
