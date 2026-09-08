// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#if defined(__XS3A__)

/*
 * XS3 implementations of the h_hat bit-reversed gather and scatter described in aec_priv.h.
 *
 * Both move whole complex time domain elements - pairs of taps - so a single ldd/std moves each
 * one. That is the widest move available here: the VPU can only move contiguous runs, and these
 * are strided.
 *
 * The moves are blocked 6 + 6 + 3 so that every ldd/std offset stays inside the 0..11 immediate
 * range the encoding allows, with the block base advanced from a register between blocks. That is
 * what the C these replace could not express - it answered the out of range offsets by
 * recomputing addresses, spending about five instructions per double word instead of two.
 *
 *   void aec_h_hat_bitrev_scatter(int32_t *dst, const int32_t *src);
 *   void aec_h_hat_bitrev_gather (int32_t *dst, const int32_t *src);
 *
 * The layout constants below are the AEC_H_HAT_BITREV_* macros of aec_priv.h, which cannot be
 * included here. aec_l2_impl.c static asserts that they still agree.
 */

#define NGROUPS         8       // AEC_H_HAT_BITREV_DROPPED
#define NSTACKWORDS     6

// Block strides, in bytes. A group is 15 stored double words spread over 32 destination slots.
#define STEP_WIDE       96      // 12 double words: one block on the strided side
#define STEP_WIDE_LAST  64      //  8 double words: the rest of the group, past the dropped slot
#define STEP_NARROW     48      //  6 double words: one block on the packed side
#define STEP_NARROW_LAST 24     //  3 double words: the rest of the group

.text
.issue_mode dual

/* ------------------------------------------------------------------------------------------- */

#define FUNCTION_NAME aec_h_hat_bitrev_scatter_kept

.globl FUNCTION_NAME
.type FUNCTION_NAME,@function
.align 16
.cc_top FUNCTION_NAME.function,FUNCTION_NAME

// r0 = dst (AEC_PROC_FRAME_LENGTH words), r1 = src (AEC_FRAME_ADVANCE words). Both double word
// aligned. Only the kept double words are written, so the caller has to have zeroed the
// destination first - vect_s32_set() does that with the VPU, faster than this can store them.
FUNCTION_NAME:
    {   dualentsp NSTACKWORDS                                                               }
    {   std r4, r5, sp[0]                                                                   }
    {   std r6, r7, sp[1]                                                                   }
    {   std r8, r9, sp[2]                                                                   }

    {   ldc r6, STEP_WIDE                                                                   }
    {   ldc r7, STEP_WIDE_LAST                                                              }
    {   ldc r8, STEP_NARROW                                                                 }
    {   ldc r9, STEP_NARROW_LAST                                                            }
    {   ldc r3, NGROUPS                                                                     }
.align 4
.L_scatter_group:
    {   ldd r4, r5, r1[0]                                                                   }
    {   std r4, r5, r0[0]                                                                   }
    {   ldd r4, r5, r1[1]                                                                   }
    {   std r4, r5, r0[2]                                                                   }
    {   ldd r4, r5, r1[2]                                                                   }
    {   std r4, r5, r0[4]                                                                   }
    {   ldd r4, r5, r1[3]                                                                   }
    {   std r4, r5, r0[6]                                                                   }
    {   ldd r4, r5, r1[4]                                                                   }
    {   std r4, r5, r0[8]                                                                   }
    {   ldd r4, r5, r1[5]                                                                   }
    {   std r4, r5, r0[10]                                                                 }
    {   add r1, r1, r8                                                                      }

    {   add r0, r0, r6                                                                      }
    {   ldd r4, r5, r1[0]                                                                   }
    {   std r4, r5, r0[0]                                                                   }
    {   ldd r4, r5, r1[1]                                                                   }
    {   std r4, r5, r0[2]                                                                   }
    {   ldd r4, r5, r1[2]                                                                   }
    {   std r4, r5, r0[4]                                                                   }
    {   ldd r4, r5, r1[3]                                                                   }
    {   std r4, r5, r0[6]                                                                   }
    {   ldd r4, r5, r1[4]                                                                   }
    {   std r4, r5, r0[8]                                                                   }
    {   ldd r4, r5, r1[5]                                                                   }
    {   std r4, r5, r0[10]                                                                 }
    {   add r1, r1, r8                                                                      }

    {   add r0, r0, r6                                                                      }
    {   ldd r4, r5, r1[0]                                                                   }
    {   std r4, r5, r0[0]                                                                   }
    {   ldd r4, r5, r1[1]                                                                   }
    {   std r4, r5, r0[2]                                                                   }
    {   ldd r4, r5, r1[2]                                                                   }
    {   std r4, r5, r0[4]                                                                   }
    {   add r1, r1, r9                                                                      }

    {   add r0, r0, r7                          ;   sub r3, r3, 1                           }
    {   bt r3, .L_scatter_group                                                             }

    {   ldd r8, r9, sp[2]                                                                   }
    {   ldd r6, r7, sp[1]                                                                   }
    {   ldd r4, r5, sp[0]                                                                   }
    {   retsp NSTACKWORDS                                                                   }

.cc_bottom FUNCTION_NAME.function
.set FUNCTION_NAME.nstackwords,NSTACKWORDS
.globl FUNCTION_NAME.nstackwords
.set FUNCTION_NAME.maxcores,1
.globl FUNCTION_NAME.maxcores
.set FUNCTION_NAME.maxtimers,0
.globl FUNCTION_NAME.maxtimers
.set FUNCTION_NAME.maxchanends,0
.globl FUNCTION_NAME.maxchanends

#undef FUNCTION_NAME

/* ------------------------------------------------------------------------------------------- */

#define FUNCTION_NAME aec_h_hat_bitrev_gather

.globl FUNCTION_NAME
.type FUNCTION_NAME,@function
.align 16
.cc_top FUNCTION_NAME.function,FUNCTION_NAME

// r0 = dst (AEC_FRAME_ADVANCE words), r1 = src (AEC_PROC_FRAME_LENGTH words). Both double word
// aligned. src may be the buffer dst points into: the destination index of a move is never ahead
// of its source index and the moves run in increasing order, so an in place gather is safe.
FUNCTION_NAME:
    {   dualentsp NSTACKWORDS                                                               }
    {   std r4, r5, sp[0]                                                                   }
    {   std r6, r7, sp[1]                                                                   }
    {   std r8, r9, sp[2]                                                                   }

    {   ldc r6, STEP_WIDE                                                                   }
    {   ldc r7, STEP_WIDE_LAST                                                              }
    {   ldc r8, STEP_NARROW                                                                 }
    {   ldc r9, STEP_NARROW_LAST                                                            }
    {   ldc r3, NGROUPS                                                                     }
.align 4
.L_gather_group:
    {   ldd r4, r5, r1[0]                                                                   }
    {   std r4, r5, r0[0]                                                                   }
    {   ldd r4, r5, r1[2]                                                                   }
    {   std r4, r5, r0[1]                                                                   }
    {   ldd r4, r5, r1[4]                                                                   }
    {   std r4, r5, r0[2]                                                                   }
    {   ldd r4, r5, r1[6]                                                                   }
    {   std r4, r5, r0[3]                                                                   }
    {   ldd r4, r5, r1[8]                                                                   }
    {   std r4, r5, r0[4]                                                                   }
    {   ldd r4, r5, r1[10]                                                                  }
    {   std r4, r5, r0[5]                                                                   }
    {   add r1, r1, r6                                                                      }

    {   add r0, r0, r8                                                                      }
    {   ldd r4, r5, r1[0]                                                                   }
    {   std r4, r5, r0[0]                                                                   }
    {   ldd r4, r5, r1[2]                                                                   }
    {   std r4, r5, r0[1]                                                                   }
    {   ldd r4, r5, r1[4]                                                                   }
    {   std r4, r5, r0[2]                                                                   }
    {   ldd r4, r5, r1[6]                                                                   }
    {   std r4, r5, r0[3]                                                                   }
    {   ldd r4, r5, r1[8]                                                                   }
    {   std r4, r5, r0[4]                                                                   }
    {   ldd r4, r5, r1[10]                                                                  }
    {   std r4, r5, r0[5]                                                                   }
    {   add r1, r1, r6                                                                      }

    {   add r0, r0, r8                                                                      }
    {   ldd r4, r5, r1[0]                                                                   }
    {   std r4, r5, r0[0]                                                                   }
    {   ldd r4, r5, r1[2]                                                                   }
    {   std r4, r5, r0[1]                                                                   }
    {   ldd r4, r5, r1[4]                                                                   }
    {   std r4, r5, r0[2]                                                                   }
    {   add r1, r1, r7                                                                      }

    {   add r0, r0, r9                          ;   sub r3, r3, 1                           }
    {   bt r3, .L_gather_group                                                              }

    {   ldd r8, r9, sp[2]                                                                   }
    {   ldd r6, r7, sp[1]                                                                   }
    {   ldd r4, r5, sp[0]                                                                   }
    {   retsp NSTACKWORDS                                                                   }

.cc_bottom FUNCTION_NAME.function
.set FUNCTION_NAME.nstackwords,NSTACKWORDS
.globl FUNCTION_NAME.nstackwords
.set FUNCTION_NAME.maxcores,1
.globl FUNCTION_NAME.maxcores
.set FUNCTION_NAME.maxtimers,0
.globl FUNCTION_NAME.maxtimers
.set FUNCTION_NAME.maxchanends,0
.globl FUNCTION_NAME.maxchanends

#endif // defined(__XS3A__)
