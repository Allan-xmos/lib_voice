
pipeline
========

This example demonstrates how to put together a voice processing pipeline.
The pipeline will have AEC, IC, NS and AGC stages. It also demonstrates the use of ADEC module to
do a one time estimation and correction for possible reference and loudspeaker delay offsets at start up in order to
maximise AEC performance.  ADEC processing happens on the same thread as the AEC. The VNR is introduced
to give the IC and the AGC information about the speech presence in a frame.

There are two pipelines supported in this example: Standard Architecture and Alternating Architecture.
For that purpose, this example is built with 2 build configs ``_std_arch`` and ``_alt_arch``.

Standard Architecture
*********************

In this pipeline form, all the modules are enabled and called sequentially.

The AEC is configured for 2 mic input channels, 2 reference input channels, 10 phase main filter and a 5 phase shadow
filter. The AEC gets reconfigured as a 1 mic input channel, 1 reference input channel, 30 main filter phases and no shadow
filter, when ADEC goes in delay estimation mode. This allows it to measure the room delay. During this process, the AEC
output is ignored and the mic input is directly sent to output. Once the new delay has been measured and the delay correction is
applied, the AEC gets configured back to its original configuration and starts adapting and cancellation.
The AEC stage generates the echo cancelled version of the mic input that is then sent for processing through the IC.

Alternating Architecture
************************

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
******

If enabled, the IC only processes a two channel input. It will use the second channel as the reference to the first to output one channel of interference cancelled output.
In this manner, it tries to cancel the room noise. However, to avoid cancelling the wanted signal, it only adapts in the absence of voice.
Hence the VNR is called to calculate the voice to noise ratio estimation. The output of the VNR will allow IC to modulate the rate
at which it adapts its coefficients. The output of the IC is copied to the second channel as well.

The NS is a single channel API, so two instances of NS should be initialised for 2 channel processing. The NS is configured the same way 
for both the channels. It will try to predict the background noise and cancel it from the frame before passing it to AGC.

The AGC is configured for ASR engine suitable gain control on both the channels. The
output of the AGC stage is the pipeline output. The AGC also takes the output
of the VNR to adapt it's coefficients. This avoids noise being amplified during the absence of voice.

Building
********

To build the example, run the following from the root of the repository:

.. code-block:: console 

    pip install -r requirements.txt
    cmake -G "Unix Makefiles" -B build --toolchain xmos_cmake_toolchain/xs3a.cmake
    xmake -C build fwk_voice_example_bare_metal_pipeline_std_arch
    xmake -C build fwk_voice_example_bare_metal_pipeline_alt_arch


Running
*******

To run the example, run the following from the root of the repository:

.. code-block:: console

    xrun --io build/examples/bare-metal/pipeline/bin/fwk_voice_example_bare_metal_pipeline_std_arch.xe
    xrun --io build/examples/bare-metal/pipeline/bin/fwk_voice_example_bare_metal_pipeline_alt_arch.xe

Output
******

Upon execution, the example will print "frame done" when the pipeline has processed a frame.
