# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
"""Shared `--arch`/`--sim` pytest options, for both the fixture-based `target` parametrization
("Pattern B", see arch_option.py) and the Unity `.xe` collector ("Pattern A", see
unity_pytest_collector.py) - registered exactly ONCE here so suites of either kind can be
collected/run together from any directory level (top-level, a `lib_*` dir, or a single suite).

`--arch` is registered with no argparse-level default (`default=None`) so `resolve_arches` callers
(e.g. `lib_ic/conftest.py`'s native-only-dir skip) can tell "omitted entirely" apart from an
explicit `--arch` value and apply their own fallback - see `arch_option.resolve_arches`.

`pipeline` is the only suite still keeping its own local pytest.ini (own unrelated arch/topology
matrix), which keeps it out of this file's conftest chain (rootdir/confcutdir stops at the closer
pytest.ini).
"""
from arch_option import add_arch_option, generate_target_tests


def pytest_addoption(parser):
    add_arch_option(parser, choices=["xs3a", "vx4b", "native"], default=None)



def pytest_generate_tests(metafunc):
    generate_target_tests(metafunc)

