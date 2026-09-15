lib_voice change log
====================

1.1.0
-----

  * ADDED: Initial `vx4b` support
  * ADDED: `aec_h_hat_tap_index()`, for mapping a tap's position in an AEC filter phase's impulse
    response to its position in the stored phase
  * CHANGED: The AEC adaptive filter is stored in the time domain rather than the frequency domain,
    reducing AEC memory use by around 25%. Its taps are stored in bit-reversed index order so that
    the per-phase transforms need no index bit-reversal pass; use `aec_h_hat_tap_index()` to read
    the filter in time order
  * CHANGED: The AEC adaptive filter's taps are stored at 16 bit rather than 32 bit, saving a
    further 28800 bytes in the default 2 y channel, 2 x channel, 10 phase configuration. The
    arithmetic is unchanged: the taps are widened on the way into the per-phase forward transform,
    and the delta update is rounded back down to 16 bits on the way in. `aec_filter_state_t::h_hat`
    is therefore now a `bfp_s16_t`, as is the `h_hat` argument of `adec_estimate_delay()`
  * FIXED: `adec_estimate_delay()` compared filter phase energies against a zero mantissa carrying
    an exponent of 0, which `float_s32_gt()` turns into an absolute threshold of roughly 2^-31
    rather than a comparison against zero. A quiet filter therefore reported a peak phase index of
    0 and a peak to average ratio of 1 regardless of where its energy actually was
  * CHANGED: `ADEC_DE_MODE_MAIN_FILTER_PHASES` is 29 rather than 30, which is the longest delay
    estimation filter that fits the smaller `aec_memory_pool_t`. The delay ADEC can measure is
    bounded by `ADEC_DE_DELAY_SAMPS` and is unaffected
  * CHANGED: The lib_xcore_math FFT look-up tables are now generated at build time, sized for the
    512-point transforms lib_voice performs (`XMATH_GEN_FFT_LUT`/`XMATH_MAX_FFT_LEN_LOG2`), rather
    than using the 1024-point tables shipped with lib_xcore_math. This saves 8192 bytes on any tile
    running an FFT-based component. Building now requires Python 3 and numpy, both already needed
    to build the VNR model with ai_tools
  * CHANGED: `app_pipeline` example is now single-tile

  * Changes to dependencies:

    - ai_tools: Added dependency 1.4.3.dev40

    - lib_xcore_math: 2.4.1 -> 3.0.0

1.0.1
-----

  * CHANGED: Corrected project name in CMakeLists.txt
  * CHANGED: Added prefix to VNR model name

1.0.0
-----

  * ADDED: Top-level header file - `voice.h`
  * ADDED: Documentation updates
  * ADDED: Automated memory and MIPS profiling
  * CHANGED: Examples renamed to the form of: `app_EXAMPLE_NAME`
  * CHANGED: All examples and tests changed to build with `xcommon_cmake`
  * CHANGED: Improved AEC deconvergence during near end speech

  * Changes to dependencies:

    - lib_xcore_math: Added dependency 2.4.1

0.9.0
-----

  * ADDED: `xcommon_cmake` support for all modules
  * ADDED: `lib_stage1` module
  * ADDED: `aec_process_frame()` to `lib_aec`
  * ADDED: `vnr_process_frame()` to `lib_vnr`
  * ADDED: `ic_process_frame()` to `lib_ic`
  * ADDED: AEC memory pool structs (`aec_memory_pool_t` and
    `aec_shadow_filt_memory_pool_t`) to `lib_aec`
  * CHANGED: All bare metal examples have been moved from `examples/bare-metal`
    to `examples`
  * CHANGED: All examples have been rewritten to demonstrate the API only and
    any fileio support has been removed
  * CHANGED: Merged `aec_1_thread` and `aec_2_threads` examples into `aec`
    example with 2 build configs
  * CHNAGED: Merged `pipeline_alt_arch` and `pipeline_single_threaded` examples
    into `pipeline` with 2 configs
  * CHANGED: Merged `fwk_voice::vnr::features` and `fwk_voice::vnr::inference`
    cmake targets into `fwk_voice::vnr`
  * CHANGED: Moved the VNR model into
    `modules/lib_vnr/python/model/trained_model.tflite`
  * CHANGED: Moved the VNR MEL generation script into
    `modules/lib_vnr/python/gen_mel_filters.py`
  * CHANGED: `ic_calc_vnr_pred` now only calculates and outputs an input VNR
    prediction
  * CHANGED: Updated AGC and Loss Control algorithms
  * CHANGED: Renamed all module top-level headers from `<module>_api.h` to
    `<module>.h`
  * CHANGED: Required python version to 3.11
  * REMOVED: IC example
  * REMOVED: AGC example
  * REMOVED: Multithreaded pipeline example

0.8.1
-----

  * FIXED: Added back missing documentation
  * CHANGED: Tools version from 15.3.0 to 15.3.1

0.8.0
-----

  * CHANGED: Tools version from 15.2.1 to 15.3.0
  * CHANGED: Updated xmos-ai-tools version from 0.1.8 to 1.3.1
  * CHANGED: Updated lib_xcore_math version from 2.1.1 to 2.4.0

0.7.0
-----

  * CHANGED: Tools version from 15.1.4 to 15.2.1
  * CHANGED: Example builds and docs use Ninja instead of nmake under Windows
  * CHANGED: Update xmos_xmake_toolchain to v1.0.0 from untagged commit
    3a19f0284c66a92dbb9d5adc9d3d5016aac22646

0.6.0
-----

  * CHANGED: Improved documentation style
  * CHANGED: Replace lib_xs3_math with the lib_xcore_math v2.1.1
  * CHANGED: Integrate new version of lib_tflite_micro in VNR module

0.5.1
-----

  * ADDED: Windows documentation
  * REMOVED: VAD module
  * CHANGED: Git hash at which lib_tflite_micro is fetched during CMake
    FetchContent

0.5.0
-----

  * ADDED: Support for VNR
  * CHANGED: VNR input based IC control system (the API is not backwards
    compatible)
  * CHANGED: VNR input based AGC in pipeline examples
  * ADDED: Amazon based wake word engine testing in piplines tests

0.4.0
-----

  * CHANGED: Increased ASR AGC amplitude target
  * ADDED: -Os compile option for modules, examples and tests

0.3.0
-----

  * ADDED: Support for VAD.
  * CHANGED: xcore_sdk no longer a submodule of avona.

0.2.0
-----

  * ADDED: Support for IC, NS and ADEC.
  * CHANGED: CMake files cleanup.

0.1.0
-----

  * Initial version with support for AEC and AGC libraries.

