# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from audio_generation import get_band_limited_noise
import time
import numpy as np
import argparse
from pathlib import Path
import soundfile as sf
from py_voice.modules.ns import ns
import shutil
from run_dut import run_dut

c_ns_xe_path = Path(__file__).parents[3] / "build" / "test" / "lib_ns" / "test_ns_profile" / "bin" / "fwk_voice_test_ns_profile"
ns_conf_path = Path(__file__).parents[4] / "py_voice" / "py_voice" / "config" / "components" / "ns_only.json"

SAMPLE_RATE = 16000
SAMPLE_COUNT = 160080


def generate_test_audio(filename, audio_dir, max_freq, db=-20, samples=SAMPLE_COUNT):
    noise = get_band_limited_noise(0, max_freq, samples=samples, db=db, sample_rate=SAMPLE_RATE)
    sf.write(audio_dir / filename, noise, SAMPLE_RATE, "PCM_32")

def process_xe(output_file, audio_dir, ns_xe, run_native = False):
    input_data, _ = sf.read(audio_dir / "input.wav", dtype=np.int32)

    assert len(input_data.shape) == 1, "Input data can be single channel only"

    local_exe = ns_xe
    if not run_native: local_exe = local_exe.with_suffix(".xe")

    output_data, _ = run_dut(input_data, local_exe)

    sf.write(audio_dir / output_file, output_data, SAMPLE_RATE)

def process_py(input_file, output_file, audio_dir):
    ns_obj = ns(ns_conf_path)
    ns_obj.process_file(audio_dir / input_file, audio_dir / output_file)

def get_attenuation(input_file, output_file, audio_dir="."):
    in_wav_data, _ = sf.read(audio_dir / input_file)
    out_wav_data, _ = sf.read(audio_dir / output_file)

    # Calculate EWM of audio power in 1s window
    in_power = np.power(in_wav_data, 2)
    out_power = np.power(out_wav_data, 2)

    attenuation = []

    for i in range(int(len(in_power)/SAMPLE_RATE)):
        window_start = i * SAMPLE_RATE
        window_end = window_start + SAMPLE_RATE
        av_in_power = np.mean(in_power[window_start:window_end])
        av_out_power = np.mean(out_power[window_start:window_end])
        new_atten = 10 * np.log10(av_in_power / av_out_power)
        attenuation.append(new_atten)

    return attenuation


def get_attenuation_c_py(test_id, noise_band, noise_db):
    input_file = "input.wav" # Required by test_wav_ns.xe

    output_file_c = "output_c.wav"
    output_file_py = "output_py.wav"

    audio_dir = Path(__file__).parent / test_id
    audio_dir.mkdir(exist_ok=True)
    generate_test_audio(input_file, audio_dir, noise_band, db=noise_db)

    process_xe(output_file_c, audio_dir, c_ns_xe_path)
    process_py(input_file, output_file_py, audio_dir)

    attenuation_c = get_attenuation(input_file, output_file_c, audio_dir)
    attenuation_py = get_attenuation(input_file, output_file_py, audio_dir)

    print("     C NS: {}".format(["%.2f"%item for item in attenuation_c]))
    print("    PY NS: {}".format(["%.2f"%item for item in attenuation_py]))

    shutil.rmtree(audio_dir)

    return attenuation_c, attenuation_py


def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("noise_band", nargs='?', default=8000, type=int, help="Noise freq bandwidth")
    parser.add_argument("noise_level", nargs='?',default=-20, type=int, help="Nominal noise level (dBFS)")

    parser.parse_args()
    args = parser.parse_args()
    return args


def main():
    start_time = time.time()
    args = parse_arguments()
    get_attenuation_c_py("test", args.noise_band, args.noise_level)
    print(("--- {0:.2f} seconds ---" .format(time.time() - start_time)))


if __name__ == "__main__":
    main()
