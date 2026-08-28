# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
"""stage_b is a native-only (x86 CFFI) comparison with no xcore build - there's nothing to run for
xs3a/vx4b, so skip collecting its test module unless --arch selects "native" (falling back to
"native" when --arch is omitted entirely, matching how it's invoked standalone with no --arch
flag)."""
from arch_option import resolve_arches

NATIVE_ONLY_FILES = {"test_stage_b_cffi.py"}


def pytest_ignore_collect(collection_path, config):
    if collection_path.name in NATIVE_ONLY_FILES:
        return "native" not in resolve_arches(config, default=("native",))
