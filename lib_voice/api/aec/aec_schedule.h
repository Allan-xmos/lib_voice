// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef AEC_SCHEDULE_H
#define AEC_SCHEDULE_H

#include "aec_defines.h"

/**
 * @defgroup aec_schedule_types AEC Scheduling Types
 */

/**
 * @brief Data structures for distributing AEC work across hardware threads.
 *
 * @details The task distribution scheme covers two scenarios:
 * -# Distribute multiple unique tasks across multiple hardware threads.
 *    For example, for 3 tasks and 2 threads, distribute
 *    [task0, task1, task2] across [Thread0, Thread1].
 * -# Distribute (task, channel) pairs across multiple hardware threads.
 *    For example, for 3 tasks, 2 channels, and 2 threads, distribute
 *    [(task0, ch0), (task0, ch1), (task1, ch0), (task1, ch1),
 *     (task2, ch0), (task2, ch1)] across [Thread0, Thread1].
 *
 * The number of channels used when defining the (task, channel) pairs is
 * fixed to @ref AEC_LIB_MAX_CHANNELS (i.e., max(@ref AEC_MAX_Y_CHANNELS,
 * @ref AEC_MAX_X_CHANNELS)).
 *
 * @ingroup aec_schedule_types
 */


/**
 * @brief Entry used when distributing tasks across hardware threads.
 *
 * @ingroup aec_schedule_types
 */
typedef struct {
    /** Task index.*/
    int task;
    /**
     * Flag indicating whether the task is active on that thread.
     * The task runs on the thread only when @c is_active is 1.
     */
    int is_active;
}aec_par_tasks_t;

/**
 * @brief Entry used when distributing (task, channel) pairs across hardware threads.
 *
 * @ingroup aec_schedule_types
 */
typedef struct {
    /** Task index.*/
    int task;
    /** Channel index.*/
    int channel;
    /**
     * Flag indicating whether the (task, channel) pair is active on that thread.
     * The pair runs on the thread only when @c is_active is 1.
     */
    int is_active;
}aec_par_tasks_and_channels_t;

#define AEC_MAX(a,b) (((a)>(b))?(a):(b))

/** @brief Maximum channel count used by scheduling tables.
 *
 * Computed as max(@ref AEC_MAX_Y_CHANNELS, @ref AEC_MAX_X_CHANNELS).
 * This defines the upper bound for the channel dimension in the
 * precomputed task–channel schedules within @ref aec_task_distribution_t.
 *
 * @ingroup aec_schedule_types
 */
#define AEC_LIB_MAX_CHANNELS AEC_MAX(AEC_MAX_Y_CHANNELS, AEC_MAX_X_CHANNELS)

/**
 * @brief Precomputed schedules for mapping AEC work to hardware threads.
 *
 * Provides lookup tables for several configurations:
 *  - Distributing (task, channel) pairs across threads. For example, distributing 3 tasks, each
 *    running 2 channels, over 2 hardware threads
 *  - Distributing unique tasks (no channel dimension) across threads. For example, distributing 3
 *    tasks over 2 hardware threads
 *
 * For each configuration, the corresponding `passes_*` value gives the number
 * of passes required to execute all work items. The 2D arrays are indexed as
 * [thread][work_item].
 * The second dimension (e.g., 3 * AEC_LIB_MAX_CHANNELS) is sized (upper bound) for the
 * worst case where a single thread performs all work items (one pass per
 * work item).
 * When multiple threads are available, the corresponding
 * `passes_*` value indicates how many passes are actually required to
 * cover all work items with the given `thread_count`.
 *
 * @ingroup aec_schedule_types
 */
typedef struct {
    /** Number of hardware threads this schedule targets (should be <= AEC_LIB_MAX_THREADS) */
    unsigned thread_count;

    /** Passes needed to cover all (3 tasks × channels) work items. */
    unsigned passes_for_3_tasks_and_channels;
    /** Schedule for 3 tasks, AEC_LIB_MAX_CHANNELS channels, scheduled across
     * AEC_LIB_MAX_THREADS threads */
    aec_par_tasks_and_channels_t par_3_tasks_and_channels[AEC_LIB_MAX_THREADS][3 * AEC_LIB_MAX_CHANNELS];

    /** Passes needed to cover all (3 tasks × channels) work items. */
    unsigned passes_for_2_tasks_and_channels;
    /** Schedule for 2 tasks, AEC_LIB_MAX_CHANNELS channels, scheduled across
     * AEC_LIB_MAX_THREADS threads */
    aec_par_tasks_and_channels_t par_2_tasks_and_channels[AEC_LIB_MAX_THREADS][2 * AEC_LIB_MAX_CHANNELS];

    /** Passes needed to cover all (1 task × channels) work items. */
    unsigned passes_for_1_tasks_and_channels;
    /** Schedule for 1 task, AEC_LIB_MAX_CHANNELS channels, scheduled across
     * AEC_LIB_MAX_THREADS threads */
    aec_par_tasks_and_channels_t par_1_tasks_and_channels[AEC_LIB_MAX_THREADS][1 * AEC_LIB_MAX_CHANNELS];

    /** Passes needed to cover 3 unique tasks (no channel dimension). */
    unsigned passes_for_3_tasks;
    /** Schedule for 3 unique tasks (no channel dimension), scheduled across AEC_LIB_MAX_THREADS threads */
    aec_par_tasks_t par_3_tasks[AEC_LIB_MAX_THREADS][3];

    /** Passes needed to cover 2 unique tasks (no channel dimension). */
    unsigned passes_for_2_tasks;
    /** Schedule for 2 unique tasks (no channel dimension), scheduled across AEC_LIB_MAX_THREADS threads */
    aec_par_tasks_t par_2_tasks[AEC_LIB_MAX_THREADS][2];
}aec_task_distribution_t;

/**
 * @brief Default schedule for running up to 2Y, 2X channels AEC on 1 hardware thread.
 *
 * Usage:
 * - Pass &aec_tdist_chans2_threads1 to @ref aec_init() via the `tdist` argument.
 *
 * @ingroup aec_schedule_types
 */
extern const aec_task_distribution_t aec_tdist_chans2_threads1;

/**
 * @brief Default schedule for running up to 2Y, 2X channels AEC on 2 hardware threads.
 *
 * Usage:
 * - Pass &aec_tdist_chans2_threads2 to @ref aec_init() via the `tdist` argument.
 *
 * @ingroup aec_schedule_types
 */
extern const aec_task_distribution_t aec_tdist_chans2_threads2;

#endif
