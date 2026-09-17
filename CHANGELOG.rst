lib_voice change log
====================

2.0.0
-----

  * CHANGED: AEC adaptive filter is stored in the time domain as 16bit block floating point taps
    instead of as a 32bit complex spectrum, reducing AEC memory use by approximately 86kB in the
    default 2 mic, 2 reference, 10 main phase, 5 shadow phase configuration. The spectrum of a
    filter phase is recovered on demand, and the block LMS gradient constraint is applied to the
    filter update rather than to the whole filter, so the transform count per frame is unchanged.

  * ADDED: XS3 assembly implementations of the two element order conversions the time domain filter
    needs, ``aec_priv_td_expand()`` and ``aec_priv_td_gather()``. They are bit exact with the C in
    ``aec_priv_impl.c``, which remains the implementation for all other targets, and between them
    they account for most of the cost the time domain filter adds. See
    ``lib_voice/src/aec/aec_priv_td_xs3.S``.

  * Changes to the API, all of which are breaking:

    - ``aec_filter_state_t::H_hat`` (``bfp_complex_s32_t``) is replaced by
      ``aec_filter_state_t::h_hat`` (``bfp_s16_t``), holding ``AEC_FILTER_TAPS_PER_PHASE`` time
      domain taps per phase.

    - ``aec_filter_state_t::filter_scratch`` added. The memory pools grew a matching region, so
      applications that allocate the pools through ``aec_state_t`` need no change.

    - ``adec_estimate_delay()`` takes the time domain filter. Its phase powers are now impulse
      response energies; they remain meaningful only relative to each other, which is all the
      delay estimator and its convergence metrics use.

    - ``aec_l2_calc_Error_and_Y_hat_td()`` and ``aec_l2_adapt_td()`` added, and are what the AEC
      uses. The frequency domain ``aec_l2_calc_Error_and_Y_hat()`` and
      ``aec_l2_adapt_plus_fft_gc()`` are retained for the interference canceller, which continues
      to store its filter as a spectrum.

    - ``AEC_FILTER_TAPS_PER_PHASE`` and ``AEC_ZEROVAL_HR_S16`` added.

  * Note: the memory pool capacity rule is now a byte budget rather than a phase count, because
    filter phases and X FIFO phases are no longer the same size. See ``aec_init()``.

1.1.0
-----

  * ADDED: Initial `vx4b` support
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

