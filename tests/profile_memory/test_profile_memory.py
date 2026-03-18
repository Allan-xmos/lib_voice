# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from pathlib import Path
import subprocess
import re
import json
import pytest

def test_measure_memory(pytestconfig, target):
    """
    Measure and validate memory usage of profiling executables.

    Parameters
    ----------
    pytestconfig : pytest.Config
        Pytest configuration object used to access command-line options.
    target : str
        Architecture the executables in ``bin`` were built for (e.g. "xs3a", "vx4b"), supplied by
        the shared `--arch` fixture (see tests/conftest.py).

    Notes
    -----
    This test:

    1. Discovers all executables in the ``bin`` directory.
    2. Runs ``xobjdump --resources`` on each executable.
    3. Extracts:
           - Program size (upper bound)
           - Stack usage (upper bound)
    4. Computes total memory usage as:
           total = program + stack
    5. Writes a per-worker JSON file, aggregated in pytest_sessionfinish (if run with --update) to
       update the reference JSON and RST table, keyed by ``target``.
    6. Otherwise compares against the reference values for this ``target``, stored in
       lib_voice_memory.json / lib_voice_memory_table.rst.
    """
    update = pytestconfig.getoption("--update")
    print(f"update = {update}")
    target_xe = (Path(__file__).parent / "bin").rglob("*.xe")
    patterns = {
        "program": re.compile(r"program size,\s+upper bound:\s+(\d+)$"),
        "stack": re.compile(r"stack usage,\s+upper bound:\s+(\d+)$"),
    }
    data = {}
    for t in target_xe:
        print(t.name)
        app = t.stem
        ret = subprocess.run(["xobjdump", "--resources", t], check=True, capture_output=True, text=True)
        data[app] = {"total": 0}
        for l in ret.stdout.splitlines():
            for key, pat in patterns.items():
                m = pat.search(l)
                if m:
                    data[app][key] = int(m.group(1))
                    data[app]["total"] += data[app][key]
    print(f"data = {data}")

    # Dump {target: {app: {...}}} in a json file, to be collected in pytest_sessionfinish, if
    # reference update is required
    out_file = Path(__file__).parent / "worker_logs" / f"{target}_memory_worker.json"
    out_file.parent.mkdir(parents=True, exist_ok=True)  # create missing dirs
    out_file.write_text(json.dumps({target: data}))

    if not update:
        threshold = 500 # Allow up to 500 bytes change in overall memory before flagging
        errors = []
        fail_str = ""
        ref_json = Path(__file__).parent / "lib_voice_memory.json"
        with ref_json.open("r") as f:
            ref_data = json.load(f)
        assert target in ref_data, (f"ERROR: arch {target} not in reference json. "
                                     "Run test with pytest test_profile_memory.py --update "
                                     "to regenerate the reference json and rst")
        for app, report in data.items():
            print(f"App: {app}")
            print(report)
            assert app in ref_data[target], (f"ERROR: App {app} for arch {target} not in reference json. "
                                     "Run test with pytest test_profile_memory.py --update "
                                     "to regenerate the reference json and rst")

            if abs(report['total'] - ref_data[target][app]['total']) > threshold:
                errors.append((f"ERROR: App {app}, arch {target}, total memory: {report['total']} off by more than "
                               f"{threshold} bytes compared to the reference {ref_data[target][app]['total']}.")
                            )

        if len(errors) > 0:
            fail_str += f"Memory mismatch errors.\n"
            fail_str += "If this is expected, run the test with 'pytest test_profile_memory.py' --update to update the reference json and rst files.\n"
            fail_str += "\n".join(errors) + "\n\n"

        if len(fail_str) > 0:
            pytest.fail(fail_str)

