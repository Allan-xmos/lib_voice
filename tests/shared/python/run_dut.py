# Copyright 2025-2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import xscope_fileio
import xtagctl
import numpy as np
import subprocess
import tempfile
import re
from pathlib import Path
import platform

def get_binary_path(xe, target="xs3a"):
    xe_path = Path(xe) if not isinstance(xe, Path) else xe

    # Ensure path has at most one suffix, strip it to get the base name
    assert len(xe_path.suffixes) <= 1, f"Path has multiple suffixes: {xe_path}"
    xe_path = xe_path.with_suffix("")

    if target == "xs3a":
        return xe_path.with_suffix(".xe")
    elif target == "native":
        pltfm = platform.system()
        if pltfm == "Linux" or pltfm == "Darwin": return xe_path
        elif pltfm == "Windows": return xe_path.with_suffix(".exe")
        else: assert 0, f"{pltfm} platform is not supported"
    else:
        assert 0, f"{target} target is unsupported"

import shutil
def run_with_xscope_fileio(xe_path, cwd, timeout=600):
    target_stdout = []

    with xtagctl.acquire("XCORE-AI-EXPLORER", timeout=timeout) as adapter_id:
        print(f"Running on {adapter_id}")
        with open(Path(cwd, "stdout.txt"), "w+") as ff:
            xscope_fileio.run_on_target(adapter_id, str(xe_path), cwd=str(cwd), stdout=ff)
            ff.seek(0)
            stdout = ff.readlines()

    #ignore lines that don't contain [DEVICE]. Remove everything till and including [DEVICE] if [DEVICE] is present
    for line in stdout:
        m = re.search(r'^\s*\[DEVICE\]', line)
        if m is not None:
            target_stdout.append(re.sub(r'\[DEVICE\]\s*', '', line))
    return target_stdout

def _run_dut_inner(input_data, xe_path, tmp_path):
    """Internal helper that performs the actual DUT execution."""
    input_file = tmp_path / "input.bin"
    input_data.astype(np.int32).tofile(input_file)

    if xe_path.suffix == ".xe":
        target_stdout = run_with_xscope_fileio(xe_path, tmp_path)
    else:
        res = subprocess.run(
            [str(xe_path), "input.bin", "output.bin"],
            cwd=tmp_path,
            stdout=subprocess.PIPE,
            text=True,
        )
        target_stdout = res.stdout.splitlines()

    output_file = tmp_path / "output.bin"
    output_data = np.fromfile(output_file, dtype=np.int32)

    return output_data, target_stdout

def run_dut(input_data, xe, target="xs3a", tmp_folder=None):
    """
    Run DUT executable with binary input.

    Parameters
    ----------
    input_data : np.ndarray
        Input samples (int32). Will be written to input.bin.
    xe : str or Path
        Path to executable.
    target : str, optional
        Target architecture (default: "xs3a").
    tmp_folder : str or Path, optional
        If provided, use this directory for I/O files.
        If None, a TemporaryDirectory is created and cleaned up automatically.

    Returns
    -------
    output_data : np.ndarray
        Output samples read from output.bin (int32).
    target_stdout : list[str]
        Captured stdout lines from DUT execution.
    """
    xe_path = get_binary_path(xe, target)
    target_stdout = []
    output_data = np.empty(0, dtype=np.int32)

    if tmp_folder is not None:
        print(f"running DUT from pre-created tmp directory {tmp_folder}")
        tmp_path = Path(tmp_folder)
        tmp_path.mkdir(parents=True, exist_ok=True)

        output_data, target_stdout = _run_dut_inner(input_data, xe_path, tmp_path)
        return output_data, target_stdout

    with tempfile.TemporaryDirectory(dir=".", suffix=xe_path.stem) as auto_tmp:
        tmp_path = Path(auto_tmp)
        output_data, target_stdout = _run_dut_inner(input_data, xe_path, tmp_path)

    return output_data, target_stdout

'''
def run_dut(input_data, xe, target="xs3a", tmp_folder=None):
    target_stdout = []
    output_data = np.empty(0, dtype=np.int32)
    xe_path = get_binary_path(xe, target)

    with tempfile.TemporaryDirectory(dir=".", suffix=xe_path.stem) as tmp_folder:
        tmp_folder = Path(tmp_folder)

        input_file = tmp_folder / "input.bin"
        input_data.astype(np.int32).tofile(input_file)

        if xe_path.suffix == ".xe":  # xcore run
            target_stdout = run_with_xscope_fileio(xe_path, tmp_folder)

        else:  # native run
            res = subprocess.run([str(xe_path), "input.bin", "output.bin"], cwd=tmp_folder, stdout=subprocess.PIPE, text=True)
            target_stdout = res.stdout.splitlines()

        output_file = tmp_folder / "output.bin"
        output_data = np.fromfile(output_file, dtype=np.int32)

    return output_data, target_stdout
'''
