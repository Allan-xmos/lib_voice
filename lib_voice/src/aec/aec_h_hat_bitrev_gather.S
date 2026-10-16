// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#define FUNCTION_NAME   aec_h_hat_bitrev_gather
#define NGROUPS         8 // AEC_H_HAT_BITREV_DROPPED

#if defined(__XS3A__)

.text
.issue_mode dual

.globl FUNCTION_NAME
.type FUNCTION_NAME,@function
.align 16
.cc_top FUNCTION_NAME.function,FUNCTION_NAME

// r0 = dst (AEC_FRAME_ADVANCE int16_t), has to be word aligned
// r1 = src (AEC_PROC_FRAME_LENGTH int32_t), has to be double word aligned
// Save to use in-place

#define NSTACKWORDS     (6 + 8)

#define dst       r0
#define src       r1
#define n_loop    r2
#define _64       r3
#define _16       r4
#define _12       r5
#define msk16     r6
#define msk12     r7

// This one is different tp the next one,
// because we have to pre-increment the
// destination poiner, but the last write
// in the loop, writes 12 bytes, not 16
#define GATHER_FOUR_PAIRS_BEGIN \
    ldd r8, r9, src[0] ; \
    std r8, r9, r11[0] ; \
    ldd r8, r9, src[2] ; \
    std r8, r9, r11[1] ; \
    ldd r8, r9, src[4] ; \
    std r8, r9, r11[2] ; \
    ldd r8, r9, src[6] ; \
    std r8, r9, r11[3] ; \
    { add src, src, _64 ; vldr r11[0] } ; \
    { add dst, dst, _12 ; vdepth16 } ; \
    vstrpv dst[0], msk16

#define GATHER_FOUR_PAIRS \
    ldd r8, r9, src[0] ; \
    std r8, r9, r11[0] ; \
    ldd r8, r9, src[2] ; \
    std r8, r9, r11[1] ; \
    ldd r8, r9, src[4] ; \
    std r8, r9, r11[2] ; \
    ldd r8, r9, src[6] ; \
    std r8, r9, r11[3] ; \
    { add src, src, _64 ; vldr r11[0] } ; \
    { add dst, dst, _16 ; vdepth16 } ; \
    vstrpv dst[0], msk16

// This one will skip 2 more words
// of the source pointer
#define GATHER_THREE_PAIRS \
    ldd r8, r9, src[0] ; \
    std r8, r9, r11[0] ; \
    ldd r8, r9, src[2] ; \
    std r8, r9, r11[1] ; \
    ldd r8, r9, src[4] ; \
    std r8, r9, r11[2] ; \
    { add src, src, _64 ; vldr r11[0] } ; \
    { add dst, dst, _16 ; vdepth16 } ; \
    vstrpv dst[0], msk12

FUNCTION_NAME:
    dualentsp NSTACKWORDS
    std r4, r5, sp[0]
    std r6, r7, sp[1]
    std r8, r9, sp[2]

    // have an opportunity to add another argument
    // for the fft_size/loop_count
    ldc n_loop, NGROUPS

    { ldc r11, 0 ; ldc _16, 16 }
    { ldc _12, 12 ; vsetc r11 }
    { ldaw r11, sp[6] ; shl _64, _16, 2 }
    { mkmsk msk16, _16 ; mkmsk msk12, _12 }
    { sub dst, dst, _12 ; sub n_loop, n_loop, 1 }

.L_gather_group:
    // so in the whole loop:
    // dst += 15 * 2 * 2 = 60
    // src += 16 * 4 * 4 = 256

    // [0:7] <- [0:1, 4:5, 8:9, 12:13]
    // dst += 8 * 2
    // src += 16 * 4
    GATHER_FOUR_PAIRS_BEGIN;

    // [8:15] <- [16:17, 20:21, 24:25, 28:29]
    GATHER_FOUR_PAIRS;

    // [16:23] <- [32:33, 36:37, 40:41, 44:45]
    GATHER_FOUR_PAIRS;

    // [24:29] <- [48:49, 52:53, 56:57]
    // dst += 6 * 2
    // src += 16 * 4
    GATHER_THREE_PAIRS;

    { sub n_loop, n_loop, 1 ; bt n_loop, .L_gather_group }

    ldd r8, r9, sp[2]
    ldd r6, r7, sp[1]
    ldd r4, r5, sp[0]
    retsp NSTACKWORDS

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

#if defined(__VX4B__)

.text

// a0 = dst (AEC_FRAME_ADVANCE int16_t)
// a1 = src (AEC_PROC_FRAME_LENGTH int32_t)
// Save to use in-place

#define NSTACKWORDS     12

#define dst             a0
#define src             a1
#define count           a2
#define _28             a3
#define _32             s2
#define _256            s3

.gather_lut:
    .word 0x07060302, 0x17161312, 0x27262322, 0x37363332
    .word 0x47464342, 0x57565352, 0x67666362, 0x77767372

.p2align 2
FUNCTION_NAME:
    { li _28, 28 ; xm.entsp (NSTACKWORDS)*4 }
    xm.stdsp s3, s2, 0

    // have an opportunity to add another argument
    // for the fft_size/loop_count
    li count, NGROUPS

    { xm.ldap t3, .gather_lut ; li _32, 32 }
    { sll _256, _32, 3 ; xm.vldc src }

.L_gather_group:
    // so in the whole loop:
    // dst += 15 * 2 * 2 = 60
    // src += 16 * 4 * 4 = 256

    // [0:15] <- [0:1, 4:5, 8:9, 12:13, 16:17, 20:21, 24:25, 28:29]
    // dst += 16 * 2
    // src += 32 * 4
    { xm.ldawsp t3, 2 * 4 ; xm.vldr t3 }
    xm.vlookup src, 0, 1
    xm.vlookup src, 1, 2
    xm.vlookup src, 2, 3
    xm.vlookup src, 3, 4
    { add dst, dst, _32 ; xm.vstd dst }

    // [16:29] <- [32:33, 36:37, 40:41, 44:45, 48:49, 52:53, 56:57]
    // will actually lookup 8 pairs but only save 7
    // dst += 14 * 2
    // src += 32 * 4
    xm.vlookup src, 0, 5
    xm.vlookup src, 1, 6
    xm.vlookup src, 2, 7
    xm.vlookup src, 3, 0

    // can't store part vD, so save on stack, load to vR
    // manually incrementing + reloading src pointer into vC
    { add src, src, _256 ; xm.vstd t3 }
    { add count, count, -1 ; xm.vldc src }

    { xm.mkmsk t3, _28 ; xm.vldr t3 }
    { add dst, dst, _28 ; xm.vstrpv dst, t3 }
    
    { xm.ldap t3, .gather_lut ; xm.bt count, .L_gather_group }

    xm.lddsp s3, s2, 0
    xm.retsp (NSTACKWORDS)*4

.L_gather_end:

.globl FUNCTION_NAME
.type FUNCTION_NAME,@function
.resource_const FUNCTION_NAME, "stack_frame_bytes", (NSTACKWORDS)*4
.resource_list_empty FUNCTION_NAME, "tail_callees"
.resource_list_empty FUNCTION_NAME, "callees"
.resource_list_empty FUNCTION_NAME, "parallel_callees"
.size FUNCTION_NAME, .L_gather_end - FUNCTION_NAME

#endif // defined(__VX4B__)

