# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
"""Shared `--arch` pytest option and `target` fixture parametrization.

Reused by conftest.py files whose tests execute a DUT via `run_dut()` (tests/shared/python/run_dut.py)
for one or more of xs3a/vx4b/native, so each suite doesn't duplicate the same
`pytest_addoption`/`pytest_generate_tests` boilerplate (this is "Pattern B" - fixture-based
parametrization, as opposed to the Unity `.xe` collector in unity_pytest_collector.py).
"""


def add_arch_option(parser, choices=("xs3a", "vx4b"), default=("xs3a",)):
    """Register a `--arch` option that parametrizes the `target` fixture.

    Call from a conftest.py's `pytest_addoption(parser)` hook.
    """
    parser.addoption(
        "--arch",
        nargs="+",
        default=list(default),
        help="One or more architectures to run on (e.g. --arch xs3a vx4b)",
        choices=list(choices),
    )


def generate_target_tests(metafunc):
    """Parametrize the `target` fixture from the selected `--arch` value(s).

    Call from a conftest.py's `pytest_generate_tests(metafunc)` hook.
    """
    if "target" in metafunc.fixturenames:
        selected_arches = metafunc.config.getoption("arch")
        if isinstance(selected_arches, str):
            selected_arches = [selected_arches]
        metafunc.parametrize("target", selected_arches)
