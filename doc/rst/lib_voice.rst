###################################
lib_voice: Voice processing library
###################################

|newpage|

.. rubric:: Introduction

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

.. rubric:: Usage

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

.. toctree::
   :maxdepth: 1
   :hidden:

   audio_processing/aec/index
   audio_processing/ns/index
   audio_processing/agc/index
   audio_processing/adec/index
   audio_processing/stage1/index
   audio_processing/ic/index
   audio_processing/vnr/index
   examples/index
   api_reference/index
