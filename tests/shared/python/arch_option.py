# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
"""Shared `--arch`/`--sim` pytest options and `target` fixture parametrization.

Reused by conftest.py files whose tests execute a DUT via `run_dut()` (tests/shared/python/run_dut.py)
for one or more of xs3a/vx4b/native, so each suite doesn't duplicate the same
`pytest_addoption`/`pytest_generate_tests` boilerplate (this is "Pattern B" - fixture-based
parametrization, as opposed to the Unity `.xe` collector in unity_pytest_collector.py, which reuses
`resolve_arches` below but registers no options of its own).

`--arch`/`--sim` and the `target` parametrization must each be registered exactly ONCE per pytest
session (via `tests/conftest.py`'s `pytest_addoption`/`pytest_generate_tests`) - pytest calls EVERY
loaded conftest's `pytest_generate_tests`, so a second suite-local `pytest_generate_tests` calling
`metafunc.parametrize("target", ...)` again raises "duplicate parametrization of 'target'" once
that suite no longer has its own isolating `pytest.ini`. A suite needing a different default than
the global `["xs3a"]` should give itself an isolating local `pytest.ini` and call `add_arch_option`/
`generate_target_tests` with its own `default=[...]`, like `pipeline` does.
"""


def add_arch_option(parser, choices=("xs3a", "vx4b"), default=("xs3a",)):
    """Register the shared `--arch`/`--sim` options.

    `default=None` registers `--arch` with no argparse-level default, so `resolve_arches` callers
    can tell "omitted entirely" apart from an explicit value and apply their own fallback - this is
    what `tests/conftest.py`'s single global registration uses.

    Call from a conftest.py's `pytest_addoption(parser)` hook.
    """
    parser.addoption(
        "--arch",
        nargs="+",
        default=list(default) if default is not None else None,
        help="One or more architectures to run on (e.g. --arch xs3a vx4b)",
        choices=list(choices),
    )
    parser.addoption(
        "--sim",
        action="store_true",
        default=False,
        help="Run the selected arch(es) under their simulator (xsim) instead of real hardware. "
             "Currently only honoured by the Unity `.xe` suites (unity_pytest_collector.py).",
    )


def resolve_arches(config, default=("xs3a",)):
    """Return the effective list of selected arches from the shared `--arch` option."""
    selected = config.getoption("arch")
    if isinstance(selected, str):
        selected = [selected]
    return selected if selected else list(default)


def generate_target_tests(metafunc, default=("xs3a",)):
    """Parametrize the `target` fixture from the selected `--arch` value(s).

    Call ONLY from the single shared `tests/conftest.py`'s `pytest_generate_tests(metafunc)` hook -
    a suite wanting a different default should call this again from its own isolated conftest.py
    instead (see module docstring).
    """
    if "target" in metafunc.fixturenames:
        metafunc.parametrize("target", resolve_arches(metafunc.config, default))
