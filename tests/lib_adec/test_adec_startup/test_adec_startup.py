# Copyright 2022-2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import sys
import os
import shutil
import pytest
import numpy as np
import tempfile
import re
from pathlib import Path
from test_wav import test_wav
import soundfile as sf
import py_vs_c_utils as pvc

maximum_adec_delay_ms = 1000
maximum_adec_estimation_time_ms = 3500

xcore_binary = Path(__file__).parent / "bin" / "test_adec_startup.xe"

@pytest.fixture
def input_vectors():
    env_base = os.environ.get("hydra_audio_PATH")
    if env_base:
        hydra_audio_base_dir = Path(env_base)
    else:
        hydra_audio_base_dir = Path("~/hydra_audio").expanduser()
        print(f"Warning: hydra_audio_PATH environment variable not set. Using local path {hydra_audio_base_dir}")

    test_files = [
        hydra_audio_base_dir / "adec_regression" / "startup_test_case" / "david_b_vestel.wav"
    ]
    return test_files


def analyse_cancellation(data, de_end_frame):
    num_seconds_post = 5
    sos_pre_adapt = np.mean(np.square(data[de_end_frame-16000 : de_end_frame , 0]))
    sos_post_adapt = np.mean(np.square(data[de_end_frame+(num_seconds_post * 16000) : de_end_frame+((num_seconds_post+1) * 16000) , 0]))
    cancel_dB = 10 * np.log10(sos_post_adapt/sos_pre_adapt)
    print(f"AEC cancellation after {num_seconds_post}s ADEC is approximatedly {cancel_dB:.2f}dB", file=sys.stderr)
    assert cancel_dB > 3


def test_adec_startup(input_vectors):
    test_file = input_vectors[0]

    with tempfile.TemporaryDirectory(prefix='tmp_', dir='.') as tmp_dir:
        tmp_path = Path(tmp_dir)
        shutil.copyfile(test_file, tmp_path / "input.bin")

        # Create empty arguments file for test_wav_adec
        fp = open(tmp_path / "args.bin", "wb")
        fp.close()

        frame_advance = 240
        AEC_MAX_Y_CHANNELS = 2
        output_file = tmp_path / "output.wav"
        xcore_stdo = test_wav(xcore_binary, test_file, output_file, frame_advance, AEC_MAX_Y_CHANNELS, frame_advance, tmp_folder=tmp_dir)

        out_data, _ = sf.read(output_file)
        out_data = pvc.float_to_int32(out_data)

    transitions = []
    for line in xcore_stdo:
        match = re.search(r'!!ADEC STATE CHANGE!!\s*Frame:\s*([0-9]+)\s*old:\s([A-Z]+)\s*new:\s([A-Z]+)', line)
        if match is not None:
            transitions.append({'frame':int(match[1]), 'old_state':match[2], 'new_state':match[3]})
    print("transitions = ",transitions)
    frame_duration_ms = 15.0
    for tr in transitions:
        if tr['old_state'] == 'AEC' and tr['new_state'] == 'DE':
            transition_to_de_ms = (tr['frame'] + 1) * frame_duration_ms
            break
    for tr in transitions:
        if tr['old_state'] == 'DE' and tr['new_state'] == 'AEC':
            transition_out_of_de_ms = (tr['frame'] + 1) * frame_duration_ms
            break
    adec_estimation_time_ms = transition_out_of_de_ms - transition_to_de_ms
    print(f"first transition to DE: {transition_to_de_ms}ms, ADEC estimation time: {adec_estimation_time_ms}ms", file=sys.stderr)

    #Now calc 1s RMS of various bits to see AEC convergence
    analyse_cancellation(out_data, int(transition_out_of_de_ms / 1000 * 16000))
    assert transition_to_de_ms < maximum_adec_delay_ms, f"ADEC too late: {transition_to_de_ms} max {maximum_adec_delay_ms}"
    assert adec_estimation_time_ms < maximum_adec_estimation_time_ms, f"ADEC took too long to estimate: {adec_estimation_time_ms} max {maximum_adec_estimation_time_ms}"
