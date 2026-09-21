// Copyright 2022-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef AEC_MEMORY_POOL_H
#define AEC_MEMORY_POOL_H

#include "xmath/xmath.h"
#include "aec_defines.h"

/**
 * @defgroup aec_memory_pool AEC memory pool
 */

/**
 * @brief aec_memory_pool_t
 *
 * Memory pool for AEC main filter and shared state buffers.
 *
 * This pool provides contiguous storage for all BFP (block floating-point from ``lib_xcore_math``) structures used by the main filter (`aec_filter_state_t`)
 * and the shared filter state (`aec_shared_filter_state_t`).
 * The `aec_init()` function initializes the BFP structures in the AEC state structures to point to
 * memory buffers from this pool during AEC initialisation.
 *
 * The memory pool allocates storage based on AEC compile-time configuration parameters:
 * - @ref AEC_MAX_Y_CHANNELS
 * - @ref AEC_MAX_X_CHANNELS
 * - @ref AEC_MAIN_FILTER_PHASES
 * - @ref AEC_SHADOW_FILTER_PHASES
 *
 * The same pool can be used to initialize AEC for any runtime configuration (passed as arguments to `aec_init()`)
 * that is a subset of the compile-time configuration (See @ref aec_phase_pool_capacity).
 *
 * @note
 * This structure exists to own memory, not to describe layout.
 * The memory pool acts as a linear allocation arena used by `aec_init()` to
 * initialise BFP structures in the AEC filter state structures. Memory is assigned
 * sequentially from the pool based on the runtime configuration (number of
 * channels and filter phases), and does not have a fixed or semantic mapping to
 * the individual fields of this struct.
 * The named members of this struct exist only to reserve sufficient contiguous
 * storage at compile time. They must not be interpreted as backing specific
 * components of `aec_filter_state_t`/`aec_shared_filter_state_t` and should never be accessed directly. After
 * initialisation, all access to this memory occurs exclusively through the BFP
 * structures owned by the AEC state.
 *
 * @ingroup aec_memory_pool
 */
typedef struct {
    /** Memory pointed to by main filter aec_filter_state_t::filter_scratch*/
    int32_t filter_scratch[AEC_MAX_Y_CHANNELS][AEC_FILTER_SCRATCH_LENGTH];
    /** Memory pointed to by aec_shared_filter_state_t::y and aec_shared_filter_state_t::Y*/
    int32_t mic_input_frame[AEC_MAX_Y_CHANNELS][AEC_PROC_FRAME_LENGTH + AEC_FFT_PADDING];
    /** Memory pointed to by aec_shared_filter_state_t::x and aec_shared_filter_state_t::X. Also reused for main filter
     * aec_filter_state_t::T*/
    int32_t ref_input_frame[AEC_MAX_X_CHANNELS][AEC_PROC_FRAME_LENGTH + AEC_FFT_PADDING];
    /** Memory pointed to by aec_shared_filter_state_t::prev_y*/
    int32_t mic_prev_samples[AEC_MAX_Y_CHANNELS][AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE];
    /** Memory pointed to by aec_shared_filter_state_t::prev_x*/
    int32_t ref_prev_samples[AEC_MAX_X_CHANNELS][AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE];
    /** Memory pointed to by main filter aec_filter_state_t::h_hat. The main filter is stored in the time domain as
     * AEC_FILTER_TD_LENGTH 16bit mantissas per phase.*/
    int16_t phase_pool_h_hat[(AEC_MAX_Y_CHANNELS*AEC_MAX_X_CHANNELS*AEC_MAIN_FILTER_PHASES) * AEC_FILTER_TD_LENGTH];
    /** Memory pointed to by aec_shared_filter_state_t::X_fifo, main filter aec_filter_state_t::X_fifo_1d and shadow
     * filter aec_filter_state_t::X_fifo_1d*/
    complex_s32_t phase_pool_X_fifo[(AEC_MAX_X_CHANNELS*AEC_MAIN_FILTER_PHASES) * AEC_FD_FRAME_LENGTH];
    /** Memory pointed to by main filter aec_filter_state_t::Error and aec_filter_state_t::error*/
    complex_s32_t Error[AEC_MAX_Y_CHANNELS][AEC_FD_FRAME_LENGTH];
    /** Memory pointed to by main filter aec_filter_state_t::Y_hat and aec_filter_state_t::y_hat*/
    complex_s32_t Y_hat[AEC_MAX_Y_CHANNELS][AEC_FD_FRAME_LENGTH];
    /** Memory pointed to by main_filter aec_filter_state_t::X_energy*/
    int32_t X_energy[AEC_MAX_X_CHANNELS][AEC_FD_FRAME_LENGTH];
    /** Memory pointed to by aec_shared_filter_state_t::sigma_XX*/
    int32_t sigma_XX[AEC_MAX_X_CHANNELS][AEC_FD_FRAME_LENGTH];
    /** Memory pointed to by main filter aec_filter_state_t::inv_X_energy*/
    int32_t inv_X_energy[AEC_MAX_X_CHANNELS][AEC_FD_FRAME_LENGTH];
    /** Memory pointed to by main filter aec_filter_state_t::overlap*/
    int32_t overlap[AEC_MAX_Y_CHANNELS][AEC_UNUSED_TAPS_PER_PHASE*2];
}aec_memory_pool_t;

/**
 * @brief aec_shadow_filt_memory_pool_t
 *
 * Memory pool for AEC shadow filter.
 *
 * This pool provides contiguous storage for all BFP (block floating-point from ``lib_xcore_math``) structures used by the AEC shadow filter (`aec_filter_state_t`).
 * The `aec_init()` function initializes the BFP structures in the AEC state structures to point to
 * memory buffers from this pool during AEC initialisation.
 *
 * The memory pool allocates storage based on AEC compile-time configuration parameters:
 * - @ref AEC_MAX_Y_CHANNELS
 * - @ref AEC_MAX_X_CHANNELS
 * - @ref AEC_MAIN_FILTER_PHASES
 * - @ref AEC_SHADOW_FILTER_PHASES
 *
 * The same pool can be used to initialize AEC for any runtime configuration (passed as arguments to `aec_init()`)
 * that is a subset of the compile-time configuration (See @ref aec_phase_pool_capacity).
 *
 * @note
 * This structure exists to own memory, not to describe layout.
 * The memory pool acts as a linear allocation arena used by `aec_init()` to
 * initialise BFP structures in the AEC filter state structures. Memory is assigned
 * sequentially from the pool based on the runtime configuration (number of
 * channels and filter phases), and does not have a fixed or semantic mapping to
 * the individual fields of this struct.
 * The named members of this struct exist only to reserve sufficient contiguous
 * storage at compile time. They must not be interpreted as backing specific
 * components of `aec_filter_state_t`/`aec_shared_filter_state_t` and should never be accessed directly. After
 * initialisation, all access to this memory occurs exclusively through the BFP
 * structures owned by the AEC state.
 *
 * @ingroup aec_memory_pool
 */
typedef struct {
    /** Memory pointed to by shadow filter aec_filter_state_t::filter_scratch*/
    int32_t filter_scratch[AEC_MAX_Y_CHANNELS][AEC_FILTER_SCRATCH_LENGTH];
    /** Memory pointed to by shadow filter aec_filter_state_t::h_hat. The shadow filter is stored in the time domain as
     * AEC_FILTER_TD_LENGTH 16bit mantissas per phase.*/
    int16_t phase_pool_h_hat[AEC_MAX_Y_CHANNELS * AEC_MAX_X_CHANNELS * AEC_SHADOW_FILTER_PHASES * AEC_FILTER_TD_LENGTH];
    /** Memory pointed to by shadow filter aec_filter_state_t::Error and aec_filter_state_t::error*/
    complex_s32_t Error[AEC_MAX_Y_CHANNELS][AEC_FD_FRAME_LENGTH];
    /** Memory pointed to by shadow filter aec_filter_state_t::Y_hat and aec_filter_state_t::y_hat*/
    complex_s32_t Y_hat[AEC_MAX_Y_CHANNELS][AEC_FD_FRAME_LENGTH];
    /** Memory pointed to by shadow filter aec_filter_state_t::T*/
    complex_s32_t T[AEC_MAX_X_CHANNELS][AEC_FD_FRAME_LENGTH];
    /** Memory pointed to by shadow_filter aec_filter_state_t::X_energy*/
    int32_t X_energy[AEC_MAX_X_CHANNELS][AEC_FD_FRAME_LENGTH];
    /** Memory pointed to by shadow_filter aec_filter_state_t::inv_X_energy*/
    int32_t inv_X_energy[AEC_MAX_X_CHANNELS][AEC_FD_FRAME_LENGTH];
    /** Memory pointed to by shadow filter aec_filter_state_t::overlap*/
    int32_t overlap[AEC_MAX_Y_CHANNELS][AEC_UNUSED_TAPS_PER_PHASE*2];
}aec_shadow_filt_memory_pool_t;

/** @brief Bytes consumed from aec_memory_pool_t by a given runtime configuration.
 *
 * This mirrors the allocation sequence in `aec_priv_main_init()` exactly, and is what makes the capacity rule in
 * @ref aec_phase_pool_capacity checkable rather than merely documented. A filter phase and an X FIFO phase are
 * different sizes, so the demand cannot be expressed as a phase count.
 *
 * @ingroup aec_memory_pool
 */
#define AEC_MAIN_POOL_BYTES(Y, X, PH) ( \
      ((Y) * AEC_FILTER_SCRATCH_LENGTH * sizeof(int32_t))                   /* filter_scratch */ \
    + ((Y) * (AEC_PROC_FRAME_LENGTH + AEC_FFT_PADDING) * sizeof(int32_t))   /* y */ \
    + ((X) * (AEC_PROC_FRAME_LENGTH + AEC_FFT_PADDING) * sizeof(int32_t))   /* x */ \
    + ((Y) * (AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE) * sizeof(int32_t)) /* prev_y */ \
    + ((X) * (AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE) * sizeof(int32_t)) /* prev_x */ \
    + ((Y) * (X) * (PH) * AEC_FILTER_TD_LENGTH * sizeof(int16_t))           /* h_hat */ \
    + ((X) * (PH) * AEC_FD_FRAME_LENGTH * sizeof(complex_s32_t))            /* X_fifo */ \
    + (2 * (Y) * AEC_FD_FRAME_LENGTH * sizeof(complex_s32_t))               /* Error, Y_hat */ \
    + (3 * (X) * AEC_FD_FRAME_LENGTH * sizeof(int32_t))                     /* X_energy, sigma_XX, inv_X_energy */ \
    + ((Y) * AEC_UNUSED_TAPS_PER_PHASE * 2 * sizeof(int32_t))               /* overlap */ \
    )

/** @brief Bytes consumed from aec_shadow_filt_memory_pool_t by a given runtime configuration.
 *
 * Mirrors the allocation sequence in `aec_priv_shadow_init()`. See @ref AEC_MAIN_POOL_BYTES.
 *
 * @ingroup aec_memory_pool
 */
#define AEC_SHADOW_POOL_BYTES(Y, X, PH) ( \
      ((Y) * AEC_FILTER_SCRATCH_LENGTH * sizeof(int32_t))                   /* filter_scratch */ \
    + ((Y) * (X) * (PH) * AEC_FILTER_TD_LENGTH * sizeof(int16_t))           /* h_hat */ \
    + (2 * (Y) * AEC_FD_FRAME_LENGTH * sizeof(complex_s32_t))               /* Error, Y_hat */ \
    + ((X) * AEC_FD_FRAME_LENGTH * sizeof(complex_s32_t))                   /* T */ \
    + (2 * (X) * AEC_FD_FRAME_LENGTH * sizeof(int32_t))                     /* X_energy, inv_X_energy */ \
    + ((Y) * AEC_UNUSED_TAPS_PER_PHASE * 2 * sizeof(int32_t))               /* overlap */ \
    )

/* The pools must at least cover the compile-time configuration they are declared from. This catches the pool struct
 * and the allocators drifting apart; it cannot catch a runtime configuration that asks for more phases than the
 * compile-time maximum, which aec_init() asserts instead. */
_Static_assert(AEC_MAIN_POOL_BYTES(AEC_MAX_Y_CHANNELS, AEC_MAX_X_CHANNELS, AEC_MAIN_FILTER_PHASES)
                    <= sizeof(aec_memory_pool_t),
        "aec_memory_pool_t is too small for AEC_MAX_Y_CHANNELS x AEC_MAX_X_CHANNELS x AEC_MAIN_FILTER_PHASES.");
_Static_assert(AEC_SHADOW_POOL_BYTES(AEC_MAX_Y_CHANNELS, AEC_MAX_X_CHANNELS, AEC_SHADOW_FILTER_PHASES)
                    <= sizeof(aec_shadow_filt_memory_pool_t),
        "aec_shadow_filt_memory_pool_t is too small for AEC_MAX_Y_CHANNELS x AEC_MAX_X_CHANNELS x AEC_SHADOW_FILTER_PHASES.");

#endif
