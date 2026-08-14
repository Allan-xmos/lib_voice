# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
"""Shared `--arch` pytest option for suites using the fixture-based `target` parametrization.

Applies to every suite under tests/ except the ones with their own local pytest.ini, which keeps
them out of this file's conftest chain (rootdir/confcutdir stops at the closer pytest.ini):
- the Unity `.xe` suites (their own --arch is a single-value `action="store"` option, incompatible
  with this file's `nargs="+"` option of the same name)
- lib_ic/test_calc_vnr_pred (needs a different default: `["native", "xs3a"]` instead of `["xs3a"]`)
"""
from arch_option import add_arch_option, generate_target_tests


def pytest_addoption(parser):
    add_arch_option(parser, choices=["xs3a", "vx4b", "native"])


def pytest_generate_tests(metafunc):
    generate_target_tests(metafunc)
