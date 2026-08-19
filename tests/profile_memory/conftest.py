# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from pathlib import Path
import json
import re

def pytest_addoption(parser):
    """
    Register custom pytest command-line option, --update.
    When ``--update`` is provided:
    - Reference memory files lib_voice_memory.json and lib_voice_memory_table.rst are regenerated.
    - Memory deviation checks are skipped during test execution.

    This flag is intended for controlled regeneration of baseline
    profiling results after intentional performance changes.
    """
    # --arch/--sim and the `target` fixture parametrization are registered once, globally, by
    # tests/conftest.py (see arch_option.py) - a local registration here would collide with it.
    parser.addoption(
      "--update",
      action="store_true",
      help=("Regenerate lib_voice_memory.json and lib_voice_memory_table.rst. "
          "The comparison check which flags memory being out of range doesn't run in this case.")
    )

def write_rst_table(configs: dict, outfile: Path):
    """
    Generate a reStructuredText table summarizing memory usage per architecture.

    Parameters
    ----------
    configs : dict
        Nested dict mapping architecture name to a dict of app_name -> {"total": int, ...}.
        e.g. {"xs3a": {"app_memory_ns": {"total": 39424}}, "vx4b": {"app_memory_ns": {"total": 36000}}}
        Each per-arch dict must also contain an entry for ``app_memory_empty`` which serves as the
        baseline for that arch.
    outfile : pathlib.Path
        Output path for generated RST file.
    """
    archs = sorted(configs.keys())
    all_apps = sorted({app for apps in configs.values() for app in apps if "empty" not in app})
    widths = " ".join(["8"] * (1 + len(archs)))
    lines = [
        ".. _lib_voice_memory_usage:\n",
        ".. list-table:: Memory usage (in bytes)",
        "   :header-rows: 1",
        f"   :widths: {widths}",
        "",
        "   * - Component",
    ]
    for arch in archs:
        lines.append(f"     - Memory use (bytes, {arch.upper()})")
    for app in all_apps:
        m = re.search(r"app_memory_([^\s]+)", app)
        assert m, "Cannot parse app name. Should start with app_memory_"
        app_name = m.group(1).upper()
        lines.append(f"   * - {app_name}")
        for arch in archs:
            apps = configs[arch]
            if app in apps:
                assert "app_memory_empty" in apps, f"app_memory_empty not found for arch {arch}"
                value = apps[app]["total"] - apps["app_memory_empty"]["total"]
            else:
                value = "N/A"
            lines.append(f"     - {value}")
    outfile.write_text("\n".join(lines))

def pytest_sessionstart(session):
    """Clean up stale worker JSON files at the start of an --update run (master only)."""
    if hasattr(session.config, "workerinput"):
        return  # workers skip this
    try:
        update = session.config.getoption("--update")
    except ValueError:
        return
    if update:
        worker_logs = Path(__file__).parent / "worker_logs"
        for f in worker_logs.glob("*_memory_worker*.json"):
            f.unlink()

def pytest_sessionfinish(session, exitstatus):
    """
    Perform final aggregation and reference update (if run with --update).

    This hook runs once per process after all tests complete. The aggregation is done only
    on the master node (detected using, not hasattr(session.config, "workerinput")).
    It is expected to run on the master node, once all worker nodes have completed.

    When ``--update`` is specified:
    - All JSON result files updated by the workers are collected.
      Worker JSON files are expected in worker_logs/*_memory_worker*.json
    - The reference JSON and RST files is regenerated.
    """
    # master only; runs after all workers complete
    if not hasattr(session.config, "workerinput"):
        update = session.config.getoption("--update")
        if update: # update needs happen in pytest_sessionfinish after all worker nodes have run and written their corresponding <target>_memory_worker.json files
            # read all worker JSON files here — each contains {arch: {app: {...}}}
            result_files = (Path(__file__).parent / "worker_logs").glob("*_memory_worker*.json")
            data = {}
            for f in result_files:
                for arch, apps in json.loads(f.read_text()).items():
                    data.setdefault(arch, {}).update(apps)

            # Merge with existing reference JSON so other architectures are preserved
            ref_json = Path(__file__).parent / "lib_voice_memory.json"
            if ref_json.exists():
                existing = json.loads(ref_json.read_text())
                for arch, apps in existing.items():
                    if arch not in data:
                        data[arch] = apps

            print(f"Memory usage for all apps = {data}")
            # generate updated JSON
            with ref_json.open("w") as fp:
                json.dump(data, fp, indent=2)
            # generate updated RST
            rst_out = Path(__file__).parent / "lib_voice_memory_table.rst"
            write_rst_table(data, rst_out)
