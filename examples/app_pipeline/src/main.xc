// Copyright 2017-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <platform.h>
#include <xs1.h>
#include <stdio.h>
#include <stdlib.h>

// producer -> stage1 -> (tile0_to_tile1) -> stage2 -> stage3 -> stage4 -> consumer
// producer and stage1 run on tile0
// stage2, stage3, stage4 and consumer run on tile1

extern "C" {
    extern void pipeline_wrapper_tile0(chanend c_pcm_out);
    extern void pipeline_wrapper_tile1(chanend c_pcm_in);
}

int main(){
    chan c_tile0_to_tile1;

    par {
        on tile[0]: 
        {
          pipeline_wrapper_tile0(c_tile0_to_tile1);
          _Exit(0);
        }
        on tile[1]:
        {
            pipeline_wrapper_tile1(c_tile0_to_tile1);
        }
    }
    return 0;
}
