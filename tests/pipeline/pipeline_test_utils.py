# Copyright 2022-2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import numpy as np
import shutil, tempfile
import subprocess
from conftest import pipeline_bins, pipeline_output_base_dir, keyword_input_base_dir, xtag_aquire_timeout_s
import re
from pathlib import Path
import sys
import pytest
from test_wav import test_wav

sys.path.append(str(Path(__file__).parent / "py_pipeline"))
import wav_pipeline

def process_xcore(xe_file, input_file, output_file, arch="xs3a"):
    frame_advance = 240
    output_channels = 1
    stdout = test_wav(xe_file, input_file, output_file, frame_advance, output_channels, frame_advance, target=arch, timeout=xtag_aquire_timeout_s)
    return stdout

def process_python(input_file, output_file, pipeline_arch):
    # currently this test only runs python pipelines without ADEC, and skips those with.
    config_file = Path(__file__).parent / "py_pipeline/config/prev_arch.json"
    if pipeline_arch == 'aec_ic_ns_agc_prev_arch':
        wav_pipeline.test_file(input_file, output_file, config_file)
    elif pipeline_arch == 'alt_arch':
        # alt arch not originally supported in python pipeline, so skip for now
        pytest.skip("alt arch python not tested")
    elif pipeline_arch == 'prev_arch':
        # prev arch not originally supported in python pipeline, so skip for now
        pytest.skip("prev arch python not tested")
    else:
        raise ValueError(f"Unknown architecture for python processing: {pipeline_arch}")
        if pipeline_arch == 'aec_ic_prev_arch':
            wav_pipeline.test_file(input_file, output_file, config_file, disable_ns=True, disable_agc=True)
        elif pipeline_arch == 'aec_ic_ns_prev_arch':
            wav_pipeline.test_file(input_file, output_file, config_file, disable_agc=True)
        elif pipeline_arch == 'aec_ic_agc_prev_arch':
            wav_pipeline.test_file(input_file, output_file, config_file, disable_ns=True)
    stdo = ""
    return stdo

def process_file(input_file, pipeline_arch, arch="xs3a"):
    wav_name = input_file.name
    output_file = Path(__file__).parent / f"{pipeline_output_base_dir}_{pipeline_arch}_{arch}" / wav_name

    if arch == "python":
        stdout = process_python(input_file, output_file, pipeline_arch)
    elif arch in ("xs3a", "vx4b"):
        pipeline_bin = pipeline_bins[pipeline_arch]
        stdout = process_xcore(pipeline_bin, input_file, output_file, arch)
    else:
        assert False, f"Invalid arch {arch}"

    return output_file, stdout


def convert_keyword_wav(input_file, pipeline_arch, arch):
    keyword_file = Path(__file__).parent / f"{keyword_input_base_dir}_{pipeline_arch}_{arch}" / input_file.name
    # Strip off comms channel leaving just ASR. Sensory needs a 16b wav file
    subprocess.run(f"sox {input_file} -b 16 {keyword_file} remix 1".split())
    return keyword_file

def log_vnr(stdo, input_file, pipeline_arch, arch): # Read VNR predicitions from stdo and store in .npy files of the same name as input files
    xcore_stdo = stdo

    vnr_output_pred = np.empty(0, dtype=np.float64)
    vnr_input_pred = np.empty(0, dtype=np.float64)
    mu_log = np.empty(0, dtype=np.float64)
    for line in xcore_stdo:
        match = re.search(r'VNR INPUT PRED:\s*([-0-9]+)\s*([-0-9]+)', line)
        if match != None:
            vnr_mant = float(match.group(1))
            vnr_exp = float(match.group(2))
            vnr = vnr_mant * (2.0 ** vnr_exp)
            vnr_input_pred = np.append(vnr_input_pred, vnr)

        match = re.search(r'VNR OUTPUT PRED:\s*([-0-9]+)\s*([-0-9]+)', line)
        if match != None:
            vnr_mant = float(match.group(1))
            vnr_exp = float(match.group(2))
            vnr = vnr_mant * (2.0 ** vnr_exp)
            vnr_output_pred = np.append(vnr_output_pred, vnr)

        match = re.search(r'MU:\s*([-0-9]+)\s*([-0-9]+)', line)
        if match != None:
            mu_mant = float(match.group(1))
            mu_exp = float(match.group(2))
            mu = mu_mant * (2.0 ** mu_exp)
            mu_log = np.append(mu_log, mu)

    if(len(vnr_input_pred) > 0):
        filename = f"vnr_input_pred_{input_file.stem}.npy"
        filename = Path(__file__).parent / f"{keyword_input_base_dir}_{pipeline_arch}_{arch}" / filename
        np.save(filename, vnr_input_pred)

    if(len(vnr_output_pred) > 0):
        filename = f"vnr_output_pred_{input_file.stem}.npy"
        filename = Path(__file__).parent / f"{keyword_input_base_dir}_{pipeline_arch}_{arch}" / filename
        np.save(filename, vnr_output_pred)

    if(len(mu_log) > 0):
        filename = f"mu_{input_file.stem}.npy"
        filename = Path(__file__).parent / f"{keyword_input_base_dir}_{pipeline_arch}_{arch}" / filename
        np.save(filename, mu_log)
