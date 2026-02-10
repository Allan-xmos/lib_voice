from pathlib import Path
import subprocess
import re
import json
import pytest

def write_rst_table(configs: dict, outfile: Path):
    """
    Generate a reStructuredText table summarizing memory usage.

    Parameters
    ----------
    configs : dict
        Dictionary mapping application name (e.g. ``app_memory_ns``)
        to a dictionary containing memory metrics. Each entry must
        include:

            "total" : int
                Total memory usage in bytes

        The dictionary must also contain an entry for
        ``app_memory_empty`` which serves as the baseline.
    outfile : pathlib.Path
        Path where the generated RST file will be written.

    Notes
    -----
    Memory usage is reported relative to the baseline
    ``app_memory_empty`` application. The value written to the table is:

        total_memory(app) - total_memory(app_memory_empty)
    This isolates module-specific memory usage from framework overhead.
    """
    lines = [
        ".. _lib_voice_memory_usage:\n",
        ".. list-table:: Memory usage (in bytes)",
        "   :header-rows: 1",
        "   :widths: 8 8 ",
        "",
        "   * - Component",
        "     - Memory use (bytes)",
    ]
    assert("app_memory_empty" in configs.keys()), "app_memory_empty not found in the list of built apps"
    empty_app_memory = configs["app_memory_empty"]["total"]
    for app, data in sorted(configs.items()):
        if not "empty" in app:
            m = re.search(r"app_memory_([^\s]+)", app)
            assert(m), "Cannot parse app name. Should start with app_memory_"
            if m:
                print(m.group(1))
                app_name = (m.group(1)).upper()

            lines.append(f"   * - {app_name}")
            lines.append(f"     - {data.get('total',0) - empty_app_memory}")
    outfile.write_text("\n".join(lines))

def test_measure_memory(pytestconfig):
    """
    Measure and validate memory usage of profiling executables.

    Parameters
    ----------
    pytestconfig : pytest.Config
        Pytest configuration object used to access command-line options.

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
    5. Either:
           - Updates reference JSON and RST table (if ``--update``), or
           - Compares against reference values. Reference values are stored in
           lib_voice_memory.json and lib_voice_memory_table.rst
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
        target = t.stem
        ret = subprocess.run(["xobjdump", "--resources", t], check=True, capture_output=True, text=True)
        data[target] = {"total": 0}
        for l in ret.stdout.splitlines():
            for key, pat in patterns.items():
                m = pat.search(l)
                if m:
                    data[target][key] = int(m.group(1))
                    data[target]["total"] += data[target][key]
    print(f"data = {data}")


    ref_json = Path(__file__).parent / "lib_voice_memory.json"
    if update:
        with ref_json.open("w") as fp:
            json.dump(data, fp, indent=2)
        rst_out = Path(__file__).parent / "lib_voice_memory_table.rst"
        write_rst_table(data, rst_out)
    else:
        threshold = 500 # Allow up to 500 bytes change in overall memory before flagging
        errors = []
        fail_str = ""
        with ref_json.open("r") as f:
            ref_data = json.load(f)
        for app, report in data.items():
            print(f"App: {app}")
            print(report)
            assert app in ref_data, (f"ERROR: App {app} not in reference json. "
                                     "Run test with pytest test_profile_memory.py --update "
                                     "to regenerate the reference json and rst")

            if abs(report['total'] - ref_data[app]['total']) > threshold:
                errors.append((f"ERROR: App {app}, total memory: {report['total']} off by more than "
                               f"{threshold} bytes compared to the reference {ref_data[app]['total']}.")
                            )

        if len(errors) > 0:
            fail_str += f"Memory mismatch errors.\n"
            fail_str += "If this is expected, run the test with 'pytest test_profile_memory.py' --update to update the reference json and rst files.\n"
            fail_str += "\n".join(errors) + "\n\n"

        if len(fail_str) > 0:
            pytest.fail(fail_str)

