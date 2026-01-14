// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "ic.h"

int main()
{
  ic_state_t state;
  ic_init(&state);
  int32_t DWORD_ALIGNED frame_y[IC_FRAME_ADVANCE];
  int32_t DWORD_ALIGNED frame_x[IC_FRAME_ADVANCE];
  int32_t DWORD_ALIGNED output[IC_FRAME_ADVANCE];

  for (unsigned i = 0; i < 10; i++)
  {
    ic_filter(&state,  frame_y, frame_x, output);
    printf("Frame %d\n", i);
  }
  printf("\n");
  return 0;
}
