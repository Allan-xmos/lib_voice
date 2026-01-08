// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef AEC_H
#define AEC_H

#include <stdio.h>
#include <string.h>
#include "xmath/xmath.h"
#include "aec_defines.h"
#include "aec_state.h"
#include "aec_schedule.h"
#include "aec_memory_pool.h"

/**
 * @page page_aec_h aec.h
 *
 * lib_aec public functions API.
 *
 * @ingroup aec_header_file
 */

/**
 * @defgroup aec_func     High Level API Functions
 * @defgroup aec_low_level_func   Low Level API Functions (STILL WIP)
 */

/**
 * @brief Initialise AEC data structures
 *
 * This function initializes AEC data structures for a given configuration.
 * The configuration parameters num_y_channels, num_x_channels, num_main_filter_phases and num_shadow_filter_phases are
 * passed in as input arguments.
 *
 * This function needs to be called at startup to first initialise the AEC and subsequently whenever the AEC configuration changes.
 *
 * @param[inout] main_state               AEC state structure for holding main filter specific state
 * @param[inout] shadow_state             AEC state structure for holding shadow filter specific state
 * @param[inout] shared_state             Shared state structure for holding state that is common to main and shadow filter
 * @param[inout] main_mem_pool            Memory pool containing main filter memory buffers
 * @param[inout] shadow_mem_pool          Memory pool containing shadow filter memory buffers
 * @param[in] num_y_channels              Number of mic input channels
 * @param[in] num_x_channels              Number of reference input channels
 * @param[in] num_main_filter_phases      Number of phases in the main filter
 * @param[in] num_shadow_filter_phases    Number of phases in the shadow filter
 *
 * `main_state`, `shadow_state` and shared_state structures must start at double word aligned addresses.
 *
 * main_mem_pool and shadow_mem_pool must point to memory buffers big enough to support main and shadow filter
 * processing.  AEC state aec_filter_state_t and shared state aec_shared_filter_state_t structures contain only the BFP data
 * structures used in the AEC. The memory these BFP structures will point to needs to be provided by the user in the
 * memory pool main and shadow filters memory pool. An example memory pool structure is present in aec_memory_pool_t and
 * aec_shadow_filt_memory_pool_t.
 *
 * main_mem_pool and shadow_mem_pool must also start at double word aligned addresses.
 *
 * @par Example
 * @code{.c}
 *      #include "aec_memory_pool.h"
        aec_filter_state_t DWORD_ALIGNED main_state;
        aec_filter_state_t DWORD_ALIGNED shadow_state;
        aec_shared_filter_state_t DWORD_ALIGNED aec_shared_state;
        uint8_t DWORD_ALIGNED aec_mem[sizeof(aec_memory_pool_t)];
        uint8_t DWORD_ALIGNED aec_shadow_mem[sizeof(aec_shadow_filt_memory_pool_t)];
        unsigned y_chans = 2, x_chans = 2;
        unsigned main_phases = 10, shadow_phases = 5;
        // There is one main and one shadow filter per x-y channel pair, so for this example there will be 4 main and 4
        // shadow filters. Each main filter will have 10 phases and each shadow filter will have 5 phases.
        aec_init(&main_state, &shadow_state, &shared_state, aec_mem, aec_shadow_mem, y_chans, x_chans, main_phases, shadow_phases);
 * @endcode
 *
 * @ingroup aec_func
 */
void aec_init(
        aec_state_t *aec_state,
        unsigned num_y_channels,
        unsigned num_x_channels,
        unsigned num_main_filter_phases,
        unsigned num_shadow_filter_phases);

/**
 * @brief Process a frame of microphone samples using the AEC
 *
 * This function performs acoustic echo cancellation on a frame of input microphone
 * samples. It uses the input reference data frame to model the room echo characteristics
 * and adapt the internal main and shadow filters.
 *
 * @param[inout] main_state    AEC state structure holding main filter state
 * @param[inout] shadow_state  AEC state structure holding shadow filter state
 * @param[inout] output_main   Output from processing the mic input through the main filter
 * @param[inout] output_shadow Output from processing the mic input through the shadow filter
 * @param[in] y_data           Input microphone data frame
 * @param[in] x_data           Input reference data frame
 * @param[in] tdist            Pointer to the task distribution array that controls
 *                             scheduling of the AEC processing across XCORE threads
 *
 * @ingroup aec_func
 */
void aec_process_frame(
        aec_state_t *aec_state,
        int32_t (*output_main)[AEC_FRAME_ADVANCE],
        int32_t (*output_shadow)[AEC_FRAME_ADVANCE],
        const int32_t (*y_data)[AEC_FRAME_ADVANCE],
        const int32_t (*x_data)[AEC_FRAME_ADVANCE],
        const aec_task_distribution_t *tdist);

/** @brief Detect activity on input channels.
 *
 * This function implements a quick check for detecting activity on the input channels. It detects signal presence by checking
 * if the maximum sample in the time domain input frame is above a given threshold.
 *
 * @param[in] input_data Pointer to input data frame. Input is assumed to be in Q1.31 fixed point format.
 * @param[in] active_threshold Threshold for detecting signal activity
 * @param[in] num_channels Number of input data channels
 * @returns 0 if no signal activity on the input channels, 1 if activity detected on the input channels
 *
 * @ingroup aec_func
 */
uint32_t aec_detect_input_activity(const int32_t (*input_data)[AEC_FRAME_ADVANCE], float_s32_t active_threshold, int32_t num_channels);

/**
 * @brief Calculate energy in the spectrum
 *
 * This function calculates the energy of frequency domain data used in the AEC. Frequency domain data in AEC is in the form of complex 32bit vectors and energy is calculated as the squared magnitude of the input vector.
 *
 * @param[out] fd_energy energy of the input spectrum
 * @param[in] input input spectrum BFP structure
 *
 * @ingroup aec_func
 */
void aec_calc_freq_domain_energy(
        float_s32_t *fd_energy,
        const bfp_complex_s32_t *input);

/** @brief Reset parts of aec state structure.
 *
 * This function resets parts of AEC state so that the echo canceller starts adapting from a zero filter.
 *
 * @param[in] pointer to AEC main filter state structure.
 * @param[in] pointer to AEC shadow filter state structure
 *
 * @ingroup aec_func
 */
void aec_reset_state(aec_state_t *aec_state);

/** @brief Calculate the energy of the input signal
 *
 * This function calculates the sum of the energy across all samples of the time domain input channel and
 * returns the maximum energy across all channels.
 *
 * @param[in] input_data Pointer to the input data buffer. The input is assumed to be in Q1.31 fixed point format.
 * @param[in] num_channels Number of input channels.
 * @returns Maximum energy in float_s32_t format.
 *
 * @ingroup aec_func
 *
 */
float_s32_t aec_calc_max_input_energy(
        const int32_t (*input_data)[AEC_FRAME_ADVANCE],
        int num_channels);

/** @brief Calculate a correlation metric between the microphone input and estimated microphone signal
 *
 * This function calculates a metric of resemblance between the mic input and the estimated mic signal. The correlation
 * metric, along with reference signal energy is used to infer presence of near and far end signals in the AEC mic
 * input.
 *
 * @param[in] state AEC state structure. `state->y` and `state->y_hat` are used to calculate the correlation metric
 * @param[in] ch mic channel index for which to calculate the metric
 * @returns correlation metric in float_s32_t format
 *
 * @ingroup aec_func
 *
 */
float_s32_t aec_calc_corr_factor(
        aec_filter_state_t *state,
        unsigned ch);

#endif
