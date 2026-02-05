Interference Canceller
======================

An interference canceller (IC) removes unwanted sounds — such as background noise,
appliances, or competing talkers — by exploiting differences between multiple
microphone signals. It analyses the phase and amplitude relationships across
microphones to identify components consistent with interference rather than
desired speech. Using these spatial differences, the IC constructs an adaptive
filter that suppresses the unwanted components while allowing the true speech
signal to pass. Accurate voice activity detection is required to
distinguish speech from noise.

Overview
--------

The IC component in ``lib_voice`` processes two microphone channels and attempts to cancel
one microphone signal from the other in the absence of voice.

It builds an estimate of the difference in transfer functions between the two
microphones for any present noise sources.
Since the transfer function includes spatial information about the noise
sources, applying this filter to the mic input allows any signals originating
from the noise source to be cancelled.
It uses the :ref:`vnr_module` for detecting presence or absence of voice.
When the VNR indicates absence of speech, the IC adapts its filter to remove noise from the environment.
When the VNR indicates the presence of voice, the IC suspends adaptation which allows the voice source to be passed but
maintains suppression of the interfering noise sources which have been previously adapted to.
The IC operates at a fixed 16 kHz sample rate and produces a single output
channel.

Signal representation
---------------------

Processing is performed on a frame-by-frame basis. Each frame consists of 15 ms
of new audio samples (240 samples at 16 kHz) per input channel, with a total of 2 input channels.
Input data is expected in fixed-point 32-bit, 1.31 format. The output
is the interference-cancelled primary microphone signal, in the same 32-bit, 1.31
format.

Adaptive filter
---------------

The IC uses an adaptive filter which continually adapts to the acoustic environment to
accommodate changes in the room created by events such as doors opening or closing
and people moving about. However, it will hold the current transfer
function in the presence of voice meaning it does not adapt to desired audio sources,
which can be a person speaking.
The IC filter has 10 phases, which effectively determines the tail length of the filter.

Processing flow
---------------

For each frame, the IC performs the following steps:

1. Transform microphone signals into the frequency domain.
2. Estimate the interference using the adaptive filter.
3. Subtract the estimated interference from the primary microphone signal to produce the error signal.
4. Update the filter coefficients if the VNR indicates no speech.
5. Transform the error signal back to the time domain to produce the
   interference-cancelled output.

Usage
-----

Before starting processing, the IC must be initialised by calling
:c:func:`ic_init()`, which sets up internal state of the IC.
Once initialised, interference cancellation is performed by calling :c:func:`ic_process_frame()`
for each input frame (see :ref:`pipeline_example`). :c:func:`ic_process_frame()` also outputs
the VNR estimate for the current frame.


