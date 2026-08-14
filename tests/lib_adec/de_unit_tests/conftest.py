# Copyright 2022-2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from unity_pytest_collector import add_arch_option, pytest_collect_file  # noqa: F401


def pytest_addoption(parser):
    add_arch_option(parser)

