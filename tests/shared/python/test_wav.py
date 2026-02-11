from run_dut import run_dut
import soundfile as sf
import os
from pathlib import Path
import py_vs_c_utils as pvc

def test_wav(
    xe_path,
    input_wav_path,
    output_wav_path,
    input_frame_len,
    output_channels,
    output_frame_len,
    sample_rate=16000,
    target="xs3a",
    tmp_folder=None
):
    """
    Run a compiled DSP application on a WAV file and write the processed
    output to a new WAV file.

    Parameters
    ----------
    xe_path : str or Path
        Path to the compiled application (.xe file).
    input_wav_path : str or Path
        Path to the input WAV file.
    output_wav_path : str or Path
        Path where the processed WAV file will be written.
    input_frame_len : int
        Number of samples per frame per channel expected by the DUT input.
    output_channels : int
        Number of output channels produced by the DUT.
    output_frame_len : int
        Number of samples per frame per channel produced by the DUT.
    sample_rate : int, optional
        Sample rate used when writing the output WAV file.
        Default is 16000 Hz.
    target : str, optional
        Target architecture identifier passed to the DUT runner.
        Default is "xs3a".

    Notes
    -----
    - Input WAV files are assumed to contain floating-point samples.
    - Multi-channel input must be arranged as (channels, samples).
    - DUT input/output is expected in Q31 fixed-point format.
    """

    assert Path(input_wav_path).exists(), "Input WAV file does not exist"

    print(f"Running input wav {input_wav_path} through executable {xe_path}")

    # Read input WAV (shape: samples x channels)
    input_float, _ = sf.read(input_wav_path) # Returns float normalized to [-1.0, 1.0), even for int32 files
    # Convert to (channels, samples)
    input_float = input_float.T
    # Convert float -> Q31
    input_q31 = pvc.float_to_int32(input_float)

    # Interleave if multi-channel
    assert input_q31.ndim <= 2
    if input_q31.ndim == 2:
        print(f"Num input channels = {len(input_float)}")
        input_q31 = pvc.interleave_channel_frames(input_q31, input_frame_len)
    elif input_q31.ndim == 1:
        print(f"Num input channels = 1")

    # Run DUT
    output_interleaved_q31, xcore_stdout = run_dut(input_q31, xe_path, target=target, tmp_folder=tmp_folder)

    # Deinterleave output to (channels, frames) format
    output_q31 = pvc.deinterleave_channel_frames(
        output_interleaved_q31,
        output_frame_len,
        output_channels,
    )


    # Convert Q31 -> float
    output_float = pvc.int32_to_float(output_q31)

    print(f"Writing output (shape: {output_float.shape}) to wav file {output_wav_path}")

    # Write output WAV (samples x channels)
    sf.write(
        output_wav_path,
        output_float.T,
        samplerate=sample_rate,
        format="WAV",
        subtype="FLOAT",
    )

if __name__ == "__main__":
    hydra_audio_path = Path(os.environ.get("hydra_audio_PATH", "~/hydra_audio")).expanduser()
    wav_name = hydra_audio_path / "fwk_voice_quick_test_streams" / "aec_example_input.wav"
    xe = Path(__file__).parents[2] / "profile_mips" / "bin" / "aec_std_arch_2threads" / "app_mips_aec_std_arch_2threads"
    test_wav(xe, wav_name, "myoutput.wav", 240, 2, 240)
