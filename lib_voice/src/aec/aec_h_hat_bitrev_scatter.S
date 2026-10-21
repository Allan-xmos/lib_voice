// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#define FUNCTION_NAME   aec_h_hat_bitrev_scatter
#define NGROUPS         8 // AEC_H_HAT_BITREV_DROPPED

#if defined(__XS3A__)

.text
.issue_mode dual

.globl FUNCTION_NAME
.type FUNCTION_NAME,@function
.align 16
.cc_top FUNCTION_NAME.function,FUNCTION_NAME

// r0 = dst (AEC_PROC_FRAME_LENGTH int32_t, double word aligned)
// r1 = src (AEC_FRAME_ADVANCE int16_t, word aligned)

#define NSTACKWORDS     4

#define dst r0
#define src r1
#define count r2
#define msk16 r3
#define _32 r6
#define _12 r7

#define SCATTER_TWO_PAIRS \
    { add dst, dst, _32 ; ldw r4, src[0] };         \
    { shl r5, r4, 16 ; and r4, r4, msk16 };           \
    { nop ; vstd dst[0] } ; \
    std r4, r5, dst[0] ; \
    { add src, src, 8 ; ldw r4, src[1] };         \
    { shl r5, r4, 16 ; and r4, r4, msk16 };           \
    std r4, r5, dst[2]

#define SCATTER_THREE_PAIRS \
    { add dst, dst, _32 ; ldw r4, src[0] };         \
    { shl r5, r4, 16 ; and r4, r4, msk16 };           \
    { sub count, count, 1 ; vstd dst[0] } ; \
    std r4, r5, dst[0] ; \
    { nop ; ldw r4, src[1] };         \
    { shl r5, r4, 16 ; and r4, r4, msk16 };           \
    std r4, r5, dst[2] ; \
    { add dst, dst, _32 ; ldw r4, src[2] };         \
    { shl r5, r4, 16 ; and r4, r4, msk16 };           \
    { add src, src, _12 ; vstd dst[0] } ; \
    std r4, r5, dst[0]

FUNCTION_NAME:
    dualentsp NSTACKWORDS
    std r4, r5, sp[0]
    std r6, r7, sp[1]

    // have an opportunity to add another argument
    // for the fft_size/loop_count
    ldc count, NGROUPS
    { mkmsk msk16, 16 ; ldc _32, 32 }
    { shl msk16, msk16, 16 ; sub dst, dst, _32 }
    { ldc _12, 12 ; vclrdr }

.L_scatter_group:
    // so in the whole loop:
    // dst += 15 * 2 * 2 = 60
    // src += 16 * 4 * 4 = 256

    // [0:15] -> [0:1, 4:5, 8:9, 12:13, 16:17, 20:21, 24:25, 28:29]
    // dst += 32 * 4
    // src += 16 * 2
    SCATTER_TWO_PAIRS;
    SCATTER_TWO_PAIRS;
    SCATTER_TWO_PAIRS;
    SCATTER_TWO_PAIRS;

    // [16:29] -> [32:33, 36:37, 40:41, 44:45, 48:49, 52:53, 56:57]
    // dst += 32 * 4
    // src += 15 * 2
    SCATTER_TWO_PAIRS;
    SCATTER_TWO_PAIRS;
    SCATTER_THREE_PAIRS;
    bt count, .L_scatter_group

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

// a0 = dst (AEC_PROC_FRAME_LENGTH int32_t)
// a1 = src (AEC_FRAME_ADVANCE int16_t)

#define dst             a0
#define src             a1
#define count           a2
#define _32             a3

.scatter_lut0:
    .word 0x0100FFFF, 0x0302FFFF, 0xFFFFFFFF, 0xFFFFFFFF
    .word 0x0504FFFF, 0x0706FFFF, 0xFFFFFFFF, 0xFFFFFFFF

.scatter_lut1:
    .word 0x0908FFFF, 0x0b0aFFFF, 0xFFFFFFFF, 0xFFFFFFFF
    .word 0x0d0cFFFF, 0x0f0eFFFF, 0xFFFFFFFF, 0xFFFFFFFF

.scatter_lut2:
    .word 0x1110FFFF, 0x1312FFFF, 0xFFFFFFFF, 0xFFFFFFFF
    .word 0x1514FFFF, 0x1716FFFF, 0xFFFFFFFF, 0xFFFFFFFF

.scatter_lut3:
    .word 0x1918FFFF, 0x1b1aFFFF, 0xFFFFFFFF, 0xFFFFFFFF
    .word 0x1d1cFFFF, 0x1f1eFFFF, 0xFFFFFFFF, 0xFFFFFFFF

// looks up one pair instead of two
.scatter_lut4:
    .word 0x1918FFFF, 0x1b1aFFFF, 0xFFFFFFFF, 0xFFFFFFFF
    .word 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF


.p2align 2
FUNCTION_NAME:

    // have an opportunity to add another argument
    // for the fft_size/loop_count
    li count, NGROUPS
    { li _32, 32 ; xm.vclrdr }

.L_scatter_group:
    // so in the whole loop:
    // dst += 15 * 2 * 2 = 60
    // src += 16 * 4 * 4 = 256

    // load vC at the top rather than the bottom of the loop,
    // so the last group doesn't read the 32 bytes after src
    { xm.ldap t3, .scatter_lut0 ; xm.vldc src }

    // [0:15] -> [0:1, 4:5, 8:9, 12:13, 16:17, 20:21, 24:25, 28:29]
    // dst += 32 * 4
    // src += 16 * 2
    { xm.ldap t3, .scatter_lut1 ; xm.vldr t3 }
    xm.vlookup src, 0, 0
    { add dst, dst, _32 ; xm.vstd dst }
    { xm.ldap t3, .scatter_lut2 ; xm.vldr t3 }
    xm.vlookup src, 0, 0
    { add dst, dst, _32 ; xm.vstd dst }
    { xm.ldap t3, .scatter_lut3 ; xm.vldr t3 }
    xm.vlookup src, 0, 0
    { add dst, dst, _32 ; xm.vstd dst }
    { xm.ldap t3, .scatter_lut0 ; xm.vldr t3 }
    xm.vlookup src, 0, 1
    { add dst, dst, _32 ; xm.vstd dst }

    // [16:29] -> [32:33, 36:37, 40:41, 44:45, 48:49, 52:53, 56:57]
    // dst += 32 * 4
    // src += 15 * 2
    { xm.ldap t3, .scatter_lut1 ; xm.vldr t3 }
    xm.vlookup src, 0, 1
    { add dst, dst, _32 ; xm.vstd dst }
    { xm.ldap t3, .scatter_lut2 ; xm.vldr t3 }
    xm.vlookup src, 0, 1
    { add dst, dst, _32 ; xm.vstd dst }
    { xm.ldap t3, .scatter_lut4 ; xm.vldr t3 }
    xm.vlookup src, 0, 1
    { add dst, dst, _32 ; xm.vstd dst }

    // need to clear vD because we have something in vD[4:5]
    // which we want clean for this mask, as it would only do one pair
    { add count, count, -1 ; xm.vclrdr }
    { li t3, 60 ; xm.vldr t3 }
    xm.vlookup src, 0, 1
    { add src, src, t3 ; xm.vstd dst }

    { add dst, dst, _32 ; xm.bt count, .L_scatter_group }

    ret

.L_scatter_end:

.globl FUNCTION_NAME
.type FUNCTION_NAME,@function
.resource_const FUNCTION_NAME, "stack_frame_bytes", 0
.resource_list_empty FUNCTION_NAME, "tail_callees"
.resource_list_empty FUNCTION_NAME, "callees"
.resource_list_empty FUNCTION_NAME, "parallel_callees"
.size FUNCTION_NAME, .L_scatter_end - FUNCTION_NAME

#endif // defined(__VX4B__)

