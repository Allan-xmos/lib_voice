# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from pathlib import Path
import json
import re

def pytest_addoption(parser):
    """
    Register custom pytest command-line option, --update.
    When ``--update`` is provided:
    - Reference MIPS files lib_voice_mips.json and lib_voice_mips_table.rst are regenerated.
    - MIPS deviation checks are skipped during test execution.

    This flag is intended for controlled regeneration of baseline
    profiling results after intentional performance changes.
    """
    parser.addoption(
      "--update",
      action="store_true",
      help=("Regenerate `lib_voice_mips.json` and `lib_voice_mips_table.rst`. "
          "The comparison check which flags mips being out of range doesn't run in this case.")
    )

def write_rst_table(configs: dict, outfile: Path):
    """
    Generate a reStructuredText table summarizing MIPS usage.

    Parameters
    ----------
    configs : dict
        Dictionary mapping application name (e.g. 'app_mips_ns')
        to measured MIPS value (float).
    outfile : pathlib.Path
        Output path for generated RST file.
    """
    lines = [
        ".. _lib_voice_mips_usage:\n",
        ".. list-table:: CPU requirements (600 MHz system frequency, 120 MHz per HW thread)",
        "   :header-rows: 1",
        "   :widths: 8 8 ",
        "",
        "   * - Component",
        "     - MIPS use",
    ]
    for app, mips in sorted(configs.items()):
        m = re.search(r"app_mips_([^\s]+)", app)
        assert(m), "Cannot parse app name. Should start with app_mips_"
        if m:
            print(m.group(1))
            app_name = (m.group(1)).upper()

        lines.append(f"   * - {app_name}")
        lines.append(f"     - {mips}")
    outfile.write_text("\n".join(lines))

def pytest_sessionfinish(session, exitstatus):
    """
    Perform final aggregation and reference update (if run with --update).

    This hook runs once per process after all tests complete. The aggregation is done only
    on the master node (detected using, not hasattr(session.config, "workerinput")).
    It is expected to run on the master node, once all worker nodes have completed.

    When ``--update`` is specified:
    - All JSON result files updated by the workers are collected.
      Worker JSON files are expected in worker_logs/*_mips_worker*.json
    - The reference JSON and RST files is regenerated.
    """
     # master only; runs after all workers complete
    if not hasattr(session.config, "workerinput"):
        update = session.config.getoption("--update")
        if update: # update needs happen in pytest_sessionfinish after all worker nodes have run and written their corresponding <module>_mips_worker.json files
            # read all worker JSON files here
            result_files = (Path(__file__).parent / "worker_logs").glob("*_mips_worker*.json")
            data = {}
            for f in result_files:
                data.update(json.loads(f.read_text()))

            print(f"MIPS for all apps = {data}")
            # generate updated JSON
            ref_json = Path(__file__).parent / "lib_voice_mips.json"
            with ref_json.open("w") as fp:
                json.dump(data, fp, indent=2)
            # generate updated RST
            rst_out = Path(__file__).parent / "lib_voice_mips_table.rst"
            write_rst_table(data, rst_out)
