# Copyright 2025 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import shutil
import xscope_fileio
import xtagctl
import numpy as np
import subprocess
import tempfile
import re
from pathlib import Path

def run_profiler(input_data, test_name, xe, output_file = None):
    xe_path = Path(xe) if not isinstance(xe, Path) else xe
    tmp_folder = Path(tempfile.mkdtemp(dir=".", suffix=Path(test_name).name))

    input_file = tmp_folder / "input.bin"
    input_data.astype(np.int32).tofile(input_file)

    if xe_path.suffix == ".xe":  # xcore run
        with xtagctl.acquire("XCORE-AI-EXPLORER") as adapter_id:
            with open("prof.txt", "w+") as ff:
                xscope_fileio.run_on_target(adapter_id, str(xe_path), cwd=str(tmp_folder), stdout=ff)
                ff.seek(0)
                stdout = ff.readlines()

        #ignore lines that don't contain [DEVICE]. Remove everything till and including [DEVICE] if [DEVICE] is present
        target_stdout = []
        for line in stdout:
            m = re.search(r'^\s*\[DEVICE\]', line)
            if m is not None:
                target_stdout.append(re.sub(r'\[DEVICE\]\s*', '', line))

    else:  # x86 run
        res = subprocess.run([str(xe_path), "input.bin", "output.bin"], cwd=tmp_folder, stdout=subprocess.PIPE, text=True)
        target_stdout = res.stdout.splitlines()

    if output_file: shutil.copy2(tmp_folder / "output.bin", output_file)
    shutil.rmtree(tmp_folder)
    return target_stdout
