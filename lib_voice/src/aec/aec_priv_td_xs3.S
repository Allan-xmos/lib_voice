// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#if defined(__XS3A__)

/*  XS3 implementations of the two hot element order conversions on the time domain filter path:

        void aec_priv_td_expand(complex_s32_t *dst, const int16_t *src, left_shift_t shl);
        void aec_priv_td_gather(complex_s32_t *dst, const complex_s32_t *src);

    Both always run over a whole filter phase, so the counts are compile time constants rather than arguments.
    aec_priv_impl.c static asserts that the ones baked in below still agree with aec_defines.h, and holds the C
    reference implementations that define what these compute.

    A phase stores only the transform slots the gradient constraint does not hold at zero: of every
    TD_BLOCK slots, the last is omitted. Both loops are therefore structured as TD_BLOCKS blocks of
    (TD_BLOCK-1) stored elements, with the omitted slot supplied as zero on the way out (expand) or skipped on
    the way in (gather).

    Scheduling notes:
      - ldd, std and ldw's double word forms are long instructions that cannot be bundled, so every double word
        moved costs a whole issue slot. That is the floor on both loops.
      - ashr with an immediate is also long, so the high mantissa is sign extended with the short shr/sext pair,
        which bundles two per slot and costs the same as ashr alone.
      - every indexed addressing immediate on this core is 0..11, which is what sets the sub-block sizes below:
        std reaches 6 expand elements and ldd reaches 5 gather elements before the pointers have to move.

    Neither function touches the VPU, so neither disturbs the vector unit state of the transform either side of
    it, and neither needs more than word alignment on `src` and double word alignment on `dst`.
*/

#define TD_BLOCK        (16)    /* AEC_FILTER_TD_BLOCK */
#define TD_BLOCKS       (8)     /* AEC_FILTER_TD_PAIRS / AEC_FILTER_TD_BLOCK */

.text
.issue_mode dual


/* ---------------------------------------------------------------------------------------------------------
    void aec_priv_td_expand(complex_s32_t *dst, const int16_t *src, left_shift_t shl)

    Widens the stored 16 bit mantissa pairs into the even complex elements of dst, holding the odd elements and
    the omitted slots at zero, and left shifting by shl on the way through:

        s = 0
        for m in 0 .. (TD_BLOCK*TD_BLOCKS - 1):
            dst[2m]   = stored(m) ? { sext16(src[2s]) << shl, sext16(src[2s+1]) << shl } : { 0, 0 }
            dst[2m+1] = { 0, 0 }

    where stored(m) is (m % TD_BLOCK) != TD_BLOCK-1, and s advances only on stored slots.

    The mantissa pair for a stored slot is loaded as one word. On this little endian target the low half is the
    even tap and becomes the real part, the high half is the odd tap and becomes the imaginary part. The high
    mantissa is extracted first because sign extending the low one is done in place, destroying the word.

    The shift cannot overflow. A 16 bit mantissa with hr bits of headroom has 16+hr bits of headroom once it
    sits in a 32 bit word, and the caller picks shl as 14+hr, leaving the 2 bits fft_dit_forward wants.
--------------------------------------------------------------------------------------------------------- */

#define FUNCTION_NAME   aec_priv_td_expand
#define NSTACKWORDS     (8)

#define dst     r0
#define src     r1
#define shl_    r2
#define w       r3
#define h       r4
#define zero    r5
#define step96  r6
#define step24  r7
#define step64  r8
#define step12  r9
#define bcnt    r10

/* One stored slot: mantissa pair at src word `si`, complex element at dst double word `di`, whose odd
 * partner `dj` (= di+1) is zeroed. The assembler has no .macro, so this is a preprocessor macro using ';'
 * as the statement separator. */
#define TD_EXPAND_ELEM(di, si, dj)                                                            \
    ldw  w, src[si]                                                                         ; \
  { shr  h, w, 16                           ; sext w, 16                                 }  ; \
  { sext h, 16                               ; shl  w, w, shl_                            } ; \
  { shl  h, h, shl_                          ;                                            } ; \
    std  h, w, dst[di]                                                                      ; \
    std  zero, zero, dst[dj]

.align 4
.cc_top FUNCTION_NAME.function,FUNCTION_NAME

FUNCTION_NAME:
    dualentsp NSTACKWORDS
    std r4, r5, sp[0]
    std r6, r7, sp[1]
    std r8, r9, sp[2]
    stw r10, sp[6]

  { ldc bcnt, TD_BLOCKS                     ; ldc zero, 0                                }
  //ldc takes 0..63, so the larger strides are doubled up
  { ldc step24, 24                          ; ldc step12, 12                             }
  { ldc step96, 48                          ; ldc step64, 32                             }
  { add step96, step96, step96              ; add step64, step64, step64                 }

.L_expand_block:
    /* 6 stored slots: dst double words 0..11, src words 0..5 */
    TD_EXPAND_ELEM(0, 0, 1)
    TD_EXPAND_ELEM(2, 1, 3)
    TD_EXPAND_ELEM(4, 2, 5)
    TD_EXPAND_ELEM(6, 3, 7)
    TD_EXPAND_ELEM(8, 4, 9)
    TD_EXPAND_ELEM(10, 5, 11)
  { add dst, dst, step96                    ; add src, src, step24                       }

    /* 6 more */
    TD_EXPAND_ELEM(0, 0, 1)
    TD_EXPAND_ELEM(2, 1, 3)
    TD_EXPAND_ELEM(4, 2, 5)
    TD_EXPAND_ELEM(6, 3, 7)
    TD_EXPAND_ELEM(8, 4, 9)
    TD_EXPAND_ELEM(10, 5, 11)
  { add dst, dst, step96                    ; add src, src, step24                       }

    /* the block's last 3 stored slots, then the omitted slot, supplied as zero */
    TD_EXPAND_ELEM(0, 0, 1)
    TD_EXPAND_ELEM(2, 1, 3)
    TD_EXPAND_ELEM(4, 2, 5)
    std  zero, zero, dst[6]
    std  zero, zero, dst[7]
  { add dst, dst, step64                    ; add src, src, step12                       }
  /* The block body is far longer than bt's relative range, so the loop closes with a short forward bf and a
   * long backward bu rather than a single conditional branch.
   *
   * The decrement must not share the branch's issue bundle. Both instructions of a dual issue pair read their
   * operands at issue, which is what makes the sext/shr pair above safe, but it also means a bf bundled with the
   * sub that feeds it tests the pre-decrement count and the loop runs one block too many. */
    sub bcnt, bcnt, 1
    bf  bcnt, .L_expand_done
    bu  .L_expand_block
.L_expand_done:

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
#undef w
#undef h
#undef zero
#undef step96
#undef step24
#undef step64
#undef step12
#undef bcnt


/* ---------------------------------------------------------------------------------------------------------
    void aec_priv_td_gather(complex_s32_t *dst, const complex_s32_t *src)

    Collects the even complex elements of src that the compacted storage keeps, discarding the odd elements and
    the omitted slots:

        s = 0
        for m in 0 .. (TD_BLOCK*TD_BLOCKS - 1):
            if stored(m): dst[s++] = src[2m]

    Discarding is what applies the gradient constraint to the filter update, so there is nothing to zero
    afterwards. A complex_s32_t is exactly one double word, so an element moves with a single ldd/std pair and
    the real and imaginary order is carried through untouched.
--------------------------------------------------------------------------------------------------------- */

#define FUNCTION_NAME   aec_priv_td_gather
#define NSTACKWORDS     (6)

#define dst     r0
#define src     r1
#define lo      r2
#define hi      r3
#define step80  r4
#define step40  r5
#define step96  r6
#define bcnt    r7

/* One stored slot: src double word `si` (= 2m relative to the sub-block base) to dst double word `di`. */
#define TD_GATHER_ELEM(di, si)                                                                \
    ldd  hi, lo, src[si]                                                                    ; \
    std  hi, lo, dst[di]

.align 4
.cc_top FUNCTION_NAME.function,FUNCTION_NAME

FUNCTION_NAME:
    dualentsp NSTACKWORDS
    std r4, r5, sp[0]
    std r6, r7, sp[1]

  { ldc bcnt, TD_BLOCKS                     ; ldc step40, 40                             }
  { ldc step80, 40                          ; ldc step96, 48                             }
  { add step80, step80, step80              ; add step96, step96, step96                 }

.L_gather_block:
    /* 5 stored slots: src double words 0,2,4,6,8, dst double words 0..4. src then advances 10 complex
     * elements, because 5 stored slots span 10 of them. */
    TD_GATHER_ELEM(0, 0)
    TD_GATHER_ELEM(1, 2)
    TD_GATHER_ELEM(2, 4)
    TD_GATHER_ELEM(3, 6)
    TD_GATHER_ELEM(4, 8)
  { add src, src, step80                    ; add dst, dst, step40                        }

    /* 5 more */
    TD_GATHER_ELEM(0, 0)
    TD_GATHER_ELEM(1, 2)
    TD_GATHER_ELEM(2, 4)
    TD_GATHER_ELEM(3, 6)
    TD_GATHER_ELEM(4, 8)
  { add src, src, step80                    ; add dst, dst, step40                        }

    /* the block's last 5 stored slots. src advances 12 complex elements rather than 10, stepping over the
     * omitted slot's pair as well, which is what discards its update. */
    TD_GATHER_ELEM(0, 0)
    TD_GATHER_ELEM(1, 2)
    TD_GATHER_ELEM(2, 4)
    TD_GATHER_ELEM(3, 6)
    TD_GATHER_ELEM(4, 8)
  { add src, src, step96                    ; add dst, dst, step40                        }
  /* Not bundled with the branch, for the reason given in aec_priv_td_expand: bt would test the count before the
   * sub updated it and the loop would gather one block too many, writing past the end of the phase. */
    sub bcnt, bcnt, 1
    bt  bcnt, .L_gather_block

    ldd r4, r5, sp[0]
    ldd r6, r7, sp[1]
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
