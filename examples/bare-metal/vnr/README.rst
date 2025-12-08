vnr
===

The VNR example demonstrates the use of API to run a frame of data through the VNR
and get the estimation of how much voice is present in it.

Building
********

To build the example, run the following from the root of the repository:

.. code-block:: console 

    pip install -r requirements.txt
    cmake -G "Unix Makefiles" -B build --toolchain xmos_cmake_toolchain/xs3a.cmake
    xmake -C build fwk_voice_example_bare_metal_vnr

Running
*******

To run the example, run the following from the root of the repository:

.. code-block:: console

    xrun --io build/examples/bare-metal/vnr/bin/fwk_voice_example_bare_metal_vnr.xe

Output
******

Upon execution, the exapmle will use the pseudo-random generator to get an input data.
This data will be run through the VNR and the score will be printed in the terminal.
It outputs a number between 0 and 1, 1 being the strongest voice with respect to noise
and 0 being the lowest voice compared to noise ratio.
The pseudo-random data is not representative of a real signal,
so the VNR scores in this example tend to be zero.
It outputs a number between 0 and 1, 1 being the strongest voice with respect to noise and 0 being the lowest voice compared to noise ratio.
