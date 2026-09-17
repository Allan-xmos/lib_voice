// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#if defined(__XS3A__)

/*  XS3 implementations of the two hot element order conversions on the time domain filter path:

        void aec_priv_td_expand(complex_s32_t *dst, const int16_t *src, left_shift_t shl);
        void aec_priv_td_gather(complex_s32_t *dst, const complex_s32_t *src);

    Both always run over a whole filter phase, so the element count is the compile time constant
    AEC_FILTER_TD_PAIRS rather than an argument. aec_priv_impl.c static asserts that TD_PAIRS below still
    agrees with it. The reference C versions live in that file under the matching #if and remain the
    definition of what these compute.

    Scheduling notes, because they drive the shape of both loops:
      - ldd and std are long instructions and cannot be bundled at all, so every double word moved costs a
        whole issue slot. That is the floor on both loops.
      - ashr with an immediate is also long, so the high mantissa is sign extended with the short
        shr/sext pair instead, which bundles two per slot and costs the same as ashr on its own.
      - add takes an immediate of 0..11 only, so the pointer strides are held in registers.

    Neither function touches the VPU, so neither disturbs the vector unit state of the transform either side
    of it, and neither needs more than the double word alignment the C versions already required.
*/

#define TD_PAIRS        (128)

.text
.issue_mode dual


/* ---------------------------------------------------------------------------------------------------------
    void aec_priv_td_expand(complex_s32_t *dst, const int16_t *src, left_shift_t shl)

    Widens TD_PAIRS 16 bit mantissa pairs into the even complex elements of dst, holding the odd elements at
    zero, and left shifts by shl on the way through:

        for m in 0 .. TD_PAIRS-1:
            dst[2m]   = { sext16(src[2m]) << shl, sext16(src[2m+1]) << shl }
            dst[2m+1] = { 0, 0 }

    The mantissa pair for element m is loaded as one word. On this little endian target the low half is
    src[2m] and becomes the real part, the high half is src[2m+1] and becomes the imaginary part. The high
    mantissa is extracted first because sign extending the low one is done in place, destroying the word.

    The shift cannot overflow. A 16 bit mantissa with hr bits of headroom has 16+hr bits of headroom once it
    sits in a 32 bit word, and the caller picks shl as 14+hr, leaving the 2 bits fft_dit_forward wants.
--------------------------------------------------------------------------------------------------------- */

#define FUNCTION_NAME   aec_priv_td_expand
#define NSTACKWORDS     (8)

#define dst     r0
#define src     r1
#define shl_    r2
#define n       r3
#define w0      r4
#define w1      r5
#define h0      r6
#define h1      r7
#define zero    r8
#define step16  r9
#define step64  r10

.align 4
.cc_top FUNCTION_NAME.function,FUNCTION_NAME

FUNCTION_NAME:
    dualentsp NSTACKWORDS
    std r4, r5, sp[0]
    std r6, r7, sp[1]
    std r8, r9, sp[2]
    stw r10, sp[6]

  { ldc n, (TD_PAIRS/4)                     ; ldc zero, 0                               }
  //ldc takes an immediate of 0..63, so the 64 byte destination stride is doubled up from 32
  { ldc step16, 16                          ; ldc step64, 32                            }
  { add step64, step64, step64              ;                                           }

.L_expand_loop:
    ldd w1, w0, src[0]
  { shr  h0, w0, 16                         ; shr  h1, w1, 16                           }
  { sext h0, 16                             ; sext h1, 16                               }
  { sext w0, 16                             ; sext w1, 16                               }
  { shl  h0, h0, shl_                       ; shl  h1, h1, shl_                         }
  { shl  w0, w0, shl_                       ; shl  w1, w1, shl_                         }
    std h0, w0, dst[0]
    std zero, zero, dst[1]
    std h1, w1, dst[2]
    std zero, zero, dst[3]
    ldd w1, w0, src[1]
  { shr  h0, w0, 16                         ; shr  h1, w1, 16                           }
  { sext h0, 16                             ; sext h1, 16                               }
  { sext w0, 16                             ; sext w1, 16                               }
  { shl  h0, h0, shl_                       ; shl  h1, h1, shl_                         }
  { shl  w0, w0, shl_                       ; shl  w1, w1, shl_                         }
    std h0, w0, dst[4]
    std zero, zero, dst[5]
    std h1, w1, dst[6]
    std zero, zero, dst[7]
  { add  src, src, step16                   ; sub  n, n, 1                              }
  { add  dst, dst, step64                   ; bt   n, .L_expand_loop                    }

    ldd r4, r5, sp[0]
    ldd r6, r7, sp[1]
    ldd r8, r9, sp[2]
    ldw r10, sp[6]
    retsp NSTACKWORDS

.L_expand_end:
.cc_bottom FUNCTION_NAME.function

.global FUNCTION_NAME
.type FUNCTION_NAME,@function
.set FUNCTION_NAME.nstackwords,NSTACKWORDS; .global FUNCTION_NAME.nstackwords
.set FUNCTION_NAME.maxcores,1;              .global FUNCTION_NAME.maxcores
.set FUNCTION_NAME.maxtimers,0;             .global FUNCTION_NAME.maxtimers
.set FUNCTION_NAME.maxchanends,0;           .global FUNCTION_NAME.maxchanends
.size FUNCTION_NAME, .L_expand_end - FUNCTION_NAME

#undef FUNCTION_NAME
#undef NSTACKWORDS
#undef dst
#undef src
#undef shl_
#undef n
#undef w0
#undef w1
#undef h0
#undef h1
#undef zero
#undef step16
#undef step64


/* ---------------------------------------------------------------------------------------------------------
    void aec_priv_td_gather(complex_s32_t *dst, const complex_s32_t *src)

    Collects the even complex elements of src, discarding the odd ones:

        for m in 0 .. TD_PAIRS-1:
            dst[m] = src[2m]

    A complex_s32_t is exactly one double word, so an element moves with a single ldd/std pair and the real
    and imaginary order is carried through untouched.
--------------------------------------------------------------------------------------------------------- */

#define FUNCTION_NAME   aec_priv_td_gather
#define NSTACKWORDS     (6)

#define dst     r0
#define src     r1
#define n       r2
#define l0      r3
#define h0      r4
#define l1      r5
#define h1      r6
#define step64  r7
#define step32  r8

.align 4
.cc_top FUNCTION_NAME.function,FUNCTION_NAME

FUNCTION_NAME:
    dualentsp NSTACKWORDS
    std r4, r5, sp[0]
    std r6, r7, sp[1]
    stw r8, sp[4]

  { ldc n, (TD_PAIRS/4)                     ; ldc step32, 32                            }
  { add step64, step32, step32              ;                                           }

.L_gather_loop:
    ldd h0, l0, src[0]
    ldd h1, l1, src[2]
    std h0, l0, dst[0]
    std h1, l1, dst[1]
    ldd h0, l0, src[4]
    ldd h1, l1, src[6]
    std h0, l0, dst[2]
    std h1, l1, dst[3]
  { add src, src, step64                    ; sub n, n, 1                               }
  { add dst, dst, step32                    ; bt  n, .L_gather_loop                     }

    ldd r4, r5, sp[0]
    ldd r6, r7, sp[1]
    ldw r8, sp[4]
    retsp NSTACKWORDS

.L_gather_end:
.cc_bottom FUNCTION_NAME.function

.global FUNCTION_NAME
.type FUNCTION_NAME,@function
.set FUNCTION_NAME.nstackwords,NSTACKWORDS; .global FUNCTION_NAME.nstackwords
.set FUNCTION_NAME.maxcores,1;              .global FUNCTION_NAME.maxcores
.set FUNCTION_NAME.maxtimers,0;             .global FUNCTION_NAME.maxtimers
.set FUNCTION_NAME.maxchanends,0;           .global FUNCTION_NAME.maxchanends
.size FUNCTION_NAME, .L_gather_end - FUNCTION_NAME

#endif //defined(__XS3A__)
