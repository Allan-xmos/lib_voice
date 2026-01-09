###########
QUICK START
###########

Requirements
------------

* XTC Tools 15.3.1
* CMake 3.20 or higher
* Python 3.10 or higher

Building
--------

The following instructions show how to build the Voice Framework and run one of the example applications. This
procedure is currently supported on MacOS, Linux and Windows.

#. Enter the clone of the Voice Framework and initialise submodules

   .. code-block:: console

      cd fwk_voice
      git submodule update --init --recursive

#. Create a python environment and install `xmos-ai-tools <https://pypi.org/project/xmos-ai-tools/>`_

   .. code-block:: console

      pip install -r requirements.txt

#. Run cmake to setup the build environment for the XMOS toolchain

   .. code-block:: console 

      cmake -G "Unix Makefiles" -B build --toolchain xmos_cmake_toolchain/xs3a.cmake

#. Running make will then build the Voice Framework libraries and example applications

   .. code-block:: console

      xmake -C build fwk_voice_example_bare_metal_aec_1th

#. Run the single-threaded AEC example

   .. code-block:: console

      xrun --io build/examples/bare-metal/aec/bin/fwk_voice_example_bare_metal_aec_1th.xe

   See ``Example Applications`` section in the User Guide for full details about the examples.
