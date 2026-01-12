aec
===

The AEC example demonstrates the use of API to run a frame of data through the AEC.
It also shows how to configure AEC to run on one or two threads.
For that purpose, this example is built with 2 build configs ``_1th`` and ``_2th``.

Building
********

To build the example, run the following from the root of the repository:

.. code-block:: console 

    pip install -r requirements.txt
    cmake -G "Unix Makefiles" -B build --toolchain xmos_cmake_toolchain/xs3a.cmake
    xmake -C build fwk_voice_example_bare_metal_aec_1th
    xmake -C build fwk_voice_example_bare_metal_aec_2th


Running
*******

To run the example, run the following from the root of the repository:

.. code-block:: console

    xrun --io build/examples/bare-metal/aec/bin/fwk_voice_example_bare_metal_aec_1th.xe
    xrun --io build/examples/bare-metal/aec/bin/fwk_voice_example_bare_metal_aec_2th.xe

Output
******

Upon execution, the example will print "frame done" when the AEC has processed a frame.
