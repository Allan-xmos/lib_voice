# Copyright 2022-2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from arch_option import add_arch_option, generate_target_tests


def pytest_addoption(parser):
    # default preserves existing behaviour of always running both native and xs3a
    add_arch_option(parser, choices=["xs3a", "vx4b", "native"], default=["native", "xs3a"])


def pytest_generate_tests(metafunc):
    generate_target_tests(metafunc)

