###################################
lib_voice: Voice processing library
###################################

************
Introduction
************

``lib_voice`` is a collection of DSP components used to build a front-end voice processing pipeline.

At its core, the library provides high-performance audio processing algorithms that are combined
into a configurable pipeline. The pipeline takes input from a pair of microphones and applies
a sequence of signal processing stages to extract a clean voice signal from complex acoustic environments.
An optional reference signal from a host system can be provided to enable Acoustic Echo Cancellation (AEC),
removing echo from the microphone signal.

The pipeline produces two output streams: one optimized for Automatic Speech Recognition (ASR)
systems and another suitable for voice communications.

``lib_voice`` includes a flexible audio routing infrastructure and supports a range of
digital inputs and outputs, allowing it to be integrated into a wide variety of system configurations.
The pipeline can be configured at startup and adjusted during operation via a set configuration parameters.
All source code is provided, enabling full customization and the integration of additional audio processing algorithms.

*****
Usage
*****

``lib_voice`` is intended to be used with the `XCommon CMake <https://www.xmos.com/file/xcommon-cmake-documentation/?version=latest>`_
, the `XMOS` application build and dependency management system.

To use this library in an application include ``lib_voice`` in the application's ``APP_DEPENDENT_MODULES`` list in
`CMakeLists.txt`, for example:

.. code-block:: cmake

    set(APP_DEPENDENT_MODULES "lib_voice")

.. note:: Dependent modules should be pinned to release versions where possible, otherwise the
   latest commit on the `develop` branch will be used.  For further details on managing modules,
   pinning to a release version and other options, please see the page `xcommon-cmake Dependency Management <https://www.xmos.com/documentation/XM-015090-PC/html/doc/dependency_management.html>`_.

All ``lib_voice`` functions can be accessed via the ``voice.h`` header file, for example:

.. code-block:: C

    #include "voice.h"

***************************
Voice Processing Components
***************************

.. toctree::
   :maxdepth: 1

   audio_processing/aec/index
   audio_processing/ns/index
   audio_processing/agc/index
   audio_processing/adec/index
   audio_processing/stage1/index
   audio_processing/ic/index
   audio_processing/vnr/index


********
Examples
********

Various example applications are provided alongside the ``lib_voice`` that demonstrate basic usage.
These are located in the ``examples`` directory.

Requirements
============

To build or run any examples the user is expected to have the following:

* XTC Tools 15.3.1
* CMake 3.20 or higher
* Python 3.11

The python is required since library depends on the `xmos-ai-tools <https://pypi.org/project/xmos-ai-tools/>`_
This is required for the ``cmake`` to configure the project,
so the user must create a python environment and install ``xmos-ai-tools``, before running ``cmake``.

AEC example
===========

The AEC example demonstrates the use of API to run a frame of data through the AEC.
It also shows how to configure AEC to run on one or two threads.
``AEC_THREADS`` define toggles the AEC to use 1-threaded or 2-threaded scheduling structs:

.. literalinclude:: ../../examples/app_aec/src/main.c
    :language: c
    :start-at: #if AEC_THREADS == 1
    :end-before: // Allocate signal data

The build system will automatically create both configs called ``app_aec_1th`` and ``app_aec_2th``.

After the application has decided which thread distribution scheme to run,
it allocates memory for the AEC and initialises it:

.. literalinclude:: ../../examples/app_aec/src/main.c
    :language: c
    :start-at: // Allocate signal data
    :end-at: AEC_MAIN_FILTER_PHASES, AEC_SHADOW_FILTER_PHASES, &tdist);

After the AEC has been initialised, it is ready to process data.
In this example, ``frame_y`` and ``frame_x`` memory is reused for AEC output:

.. literalinclude:: ../../examples/app_aec/src/main.c
    :language: c
    :start-at: producer(frame_y, frame_x);
    :end-at: consumer(frame_y);

Upon execution, the example will print "frame done" when the AEC has processed a frame.

VNR example
===========

The VNR example demonstrates the use of API to run a frame of data through the VNR
and get the estimation of how much voice is present in it.

To run the VNR, the application should first allocate memory for its data and initialise it:

.. literalinclude:: ../../examples/app_vnr/src/main.c
    :language: c
    :start-at: // Allocate input and output memory
    :end-at: vnr_state_init(&vnr);

After the VNR has been initialised, it is ready to process data.
The VNR output is in ``float_s32_t`` format, so there's an extra step if the user wants it in ``float``.

.. literalinclude:: ../../examples/app_vnr/src/main.c
    :language: c
    :start-at: producer(input);
    :end-at: consumer(res);

Upon execution, the example will use the pseudo-random generator to get an input data.
This data will be run through the VNR and the score will be printed in the terminal.
It outputs a number between 0 and 1, 1 being the strongest voice with respect to noise
and 0 being the lowest voice compared to noise ratio.
The pseudo-random data is not representative of a real signal,
so the VNR scores in this example tend to be zero.

Pipeline example
================

This example demonstrates how to put together a voice processing pipeline.
The pipeline will have AEC, IC, NS and AGC stages. It also demonstrates the use of ADEC module to
do a one time estimation and correction for possible reference and loudspeaker delay offsets at start up in order to
maximise AEC performance.  ADEC processing happens on the same thread as the AEC. The VNR is introduced
to give the IC and the AGC information about the speech presence in a frame.

There are two pipelines supported in this example: Standard Architecture and Alternating Architecture.
Building one or the other config is controlled via the ``ALT_ARCH_MODE`` define.
The build system will automatically create both configs called ``app_pipeline_std_arch`` and ``app_pipeline_alt_arch``.

To create the pipeline, the application must first initialise all the individual components:

.. literalinclude:: ../../examples/app_pipeline/src/pipeline.c
    :language: c
    :start-at: // Initialise AEC, DE, ADEC stages
    :end-at: stage1_init(&state->stage_1_state, &aec_de_mode_conf, &aec_non_de_mode_conf, &adec_conf);

.. literalinclude:: ../../examples/app_pipeline/src/pipeline.c
    :language: c
    :start-at: // Initialise IC, VNR
    :end-at: agc_init(&state->agc_state, &agc_conf_asr);

After the pipeline has been initialised, the data can be run through it.
All the modules in the example can run on separate threads.
To exchange information between the pipeline stages
the metadata struct will need to be created to be populated and consumed by the different modules.

.. literalinclude:: ../../examples/app_pipeline/src/pipeline.c
    :language: c
    :start-at: /** Stage1 - AEC, DE, ADEC*/
    :end-at: &md.aec_corr_factor, &md.ref_active_flag, input_y_data, input_x_data);

.. literalinclude:: ../../examples/app_pipeline/src/pipeline.c
    :language: c
    :start-at: // Bypass IC if the reference is high in the alt arch mode
    :end-at: agc_process_frame(&state->agc_state, output_data, ns_output, &agc_md);

Upon execution, the example will print "frame done" when the AEC has processed a frame.

Standard Architecture
---------------------

In this pipeline form, all the modules are enabled and called sequentially.

The AEC is configured for 2 mic input channels, 2 reference input channels, 10 phase main filter and a 5 phase shadow
filter. The AEC gets reconfigured as a 1 mic input channel, 1 reference input channel, 30 main filter phases and no shadow
filter, when ADEC goes in delay estimation mode. This allows it to measure the room delay. During this process, the AEC
output is ignored and the mic input is directly sent to output. Once the new delay has been measured and the delay correction is
applied, the AEC gets configured back to its original configuration and starts adapting and cancellation.
The AEC stage generates the echo cancelled version of the mic input that is then sent for processing through the IC.

Alternating Architecture
------------------------

In this pipeline form, the AEC and the IC frame processing are selectively enabled and disabled based on the presence of reference input signal.
Acoustic Echo Cancellation is performed only if activity is detected on the reference input channels and disabled otherwise.
Interference Cancellation is performed only when AEC is disabled so in the absence of reference channel activity and disabled otherwise.

The AEC is configured for 1 mic input channel, 2 reference input channels, 15 phase main filter and a 5 phase shadow
filter giving an extended tail length for highly reverberant environments. The AEC gets reconfigured as a 1 mic input channel, 1 reference input channel, 30 main filter phases and no shadow
filter, when ADEC goes in delay estimation mode. This allows it to measure the room delay. During this process, the AEC
output is ignored and the mic input is directly sent to output. Once the new delay has been measured and the delay correction is
applied, the AEC gets configured back to its original configuration and starts adapting and cancellation.
In the absence of activity on the reference channels, when the AEC is disabled, the mic input is copied directly to the output of the AEC.

Common
------

If enabled, the IC only processes a two channel input. It will use the second channel as the reference to the first to output one channel of interference cancelled output.
In this manner, it tries to cancel the room noise. However, to avoid cancelling the wanted signal, it only adapts in the absence of voice.
Hence the VNR is called to calculate the voice to noise ratio estimation. The output of the VNR will allow IC to modulate the rate
at which it adapts its coefficients.

The NS will try to predict the background noise and cancel it from the frame before passing it to AGC.

The AGC is configured for ASR engine suitable gain control.
The output of the AGC stage is the pipeline output.
The AGC also takes the output of the VNR to adapt its coefficients.
This avoids noise being amplified during the absence of voice.

Building the example
====================

This section assumes that the `XMOS XTC Tools <https://www.xmos.com/software-tools/>`_ have been
downloaded and installed. The required version is specified in the accompanying ``README``.

Installation instructions can be found `here <https://xmos.com/xtc-install-guide>`_.

Special attention should be paid to the section on
`Installation of Required Third-Party Tools <https://www.xmos.com/documentation/XM-014363-PC/html/installation/install-configure/install-tools/install_prerequisites.html>`_.

The application is built using the `xcommon-cmake <https://www.xmos.com/file/xcommon-cmake-documentation/?version=latest>`_
build system, which is provided with the XTC tools and is based on `CMake <https://cmake.org/>`_.

The ``lib_voice`` software ZIP package should be downloaded and extracted to a chosen working
directory.

To configure the build, the following commands should be run from an XTC command prompt:

.. code-block:: bash

    cd lib_voice
    pip install -r requirements.txt
    cd examples/app_aec
    cmake -G "Unix Makefiles" -B build

If any dependencies are missing they will be retrieved automatically during this step.

The application binaries should then be built using ``xmake``:

.. code-block:: bash

    xmake -j -C build

Binary artifacts (.xe files) will be generated under the appropriate subdirectories of the
``app_aec/bin`` directory — one for each supported build configuration.

For subsequent builds, the ``cmake`` step may be omitted.
If ``CMakeLists.txt`` or other build files are modified, ``cmake`` will be re-run automatically
by ``xmake`` as needed.

Running the example
===================

From an XTC command prompt, the following command should be run from the ``examples/app_aec``
directory:

.. code-block:: bash

    xrun --io ./bin/2th/app_aec_2th.xe


